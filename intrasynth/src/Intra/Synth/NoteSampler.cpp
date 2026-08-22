#include "NoteSampler.h"
#include "SamplerTask.h"

#ifdef INTRA_PROBE_NAN
#include <stdio.h>
#endif

#include <Range/Mutation/Fill.h>
#include <Range/Mutation/Transform.h>

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

/// Временное грубое ядро: рендерит весь NoteSampler в каналы контекста.
/// Позже будет расщеплено на per-channel ядра (см. архитектурную заметку).
class NoteSamplerTask: public SamplerTask
{
	NoteSampler* SamplerPtr;
public:
	NoteSamplerTask(NoteSampler* sampler, size_t offset, size_t numSamples):
		SamplerTask(offset, numSamples), SamplerPtr(sampler)
	{
		// NoteSampler генерирует все свои подсэмплеры (в т.ч. Karplus-Strong) одним
		// вызовом, поэтому его задача заметно дороже задачи наложения одной волновой
		// формы. Коэффициент подобран эвристически для балансировки нагрузки между
		// потоками (см. архитектурную заметку о цене задач).
		Cost = uint16(numSamples * 4);
	}

	void MoveConstruct(void* dst) override {new(dst) NoteSamplerTask(Move(*this));}

	void operator()(SamplerTaskContext& stc) override
	{
		const size_t n = NumSamples;
		auto left = stc.Channels[0].Drop(OffsetInSamples).Take(n);
		auto right = stc.Channels[1].Drop(OffsetInSamples).Take(n);
		auto reverb = stc.Channels[2].Drop(OffsetInSamples).Take(n);
		SamplerPtr->GenerateStereo(left, right, reverb);
		stc.UsedChannels |= LeftChannel | RightChannel | ReverbChannel;

#ifdef INTRA_PROBE_NAN
		{
			static int probeLines = 0;
			if(probeLines < 40)
			{			struct ProbeInfo { float Time; byte Channel; byte Note; };
				const auto& info = SamplerPtr->GetInfo<ProbeInfo>();
				float mx = 0;
				for(size_t pi = 0; pi < n; pi++)
				{
					float a = left[pi]; if(a < 0) a = -a; if(a > mx) mx = a;
					float b = right[pi]; if(b < 0) b = -b; if(b > mx) mx = b;
				}
				if(mx > 1e20f)
			{
				fprintf(stderr, "[VOICE] amp=%.3e ch=%u note=%d time=%.4f n=%zu\n",
					double(mx), info.Channel, info.Note, info.Time, n);
				probeLines++;
			}
			}
		}
#endif
	}
};

bool NoteSampler::Generate(SamplerTaskContainer& dstTasks, size_t offsetInSamples, size_t numSamples)
{
	if(Empty()) return false;
	dstTasks.Add<NoteSamplerTask>(this, offsetInSamples, numSamples);
	return true;
}

size_t NoteSampler::GenerateMono(Span<float> ioDst)
{
	size_t samplesProcessed = 0;
	if(!Modifiers.Empty() || ADSR)
	{
		float tempArr[1024] = {0};
		while(!ioDst.Empty() && !Empty())
		{
			// Временный буфер на стеке ограничен 1024 семплами: регион задачи
			// может быть больше, поэтому обрабатываем по кускам.
			auto tempDst = Take(tempArr, Math::Min<size_t>(1024, ioDst.Length()));
			FillZeros(tempDst);
			fill(tempDst);
			applyModifiers(tempDst);
			samplesProcessed += tempDst.Length();
			AddAdvance(ioDst, tempDst);
		}
		return samplesProcessed;
	}
	fill(ioDst);
	applyModifiers(ioDst);
	return ioDst.Length();
}

size_t NoteSampler::GenerateStereo(Span<float> dstLeft, Span<float> dstRight, Span<float> dstReverb)
{
	if(Modifiers.Empty() && !ADSR)
	{
		fillStereo(dstLeft, dstRight, dstReverb);
		return dstLeft.Length();
	}

	//TODO: добавить раздельный синтез каналов dstLeft и dstRight
	size_t sampleCount = 0;
	const float panLeft = 0.5f - Pan*0.5f;
	const float panRight = 0.5f + Pan*0.5f;
	float tempArr[1024] = {0};
	while(!dstLeft.Empty() && !Empty())
	{
		auto tempDst = Take(tempArr, Math::Min<size_t>(1024, dstLeft.Length()));
		FillZeros(tempDst);
		fill(tempDst);
		sampleCount += tempDst.Length();
		applyModifiers(tempDst);
		auto dstL = dstLeft.TakeAdvance(tempDst.Length());
		auto dstR = dstRight.TakeAdvance(tempDst.Length());
		for(size_t i = 0; i < tempDst.Length(); i++)
		{
			const float s = tempDst[i];
			dstL[i] += s*panLeft;
			dstR[i] += s*panRight;
		}
		if(ReverbCoeff != 0) AddMultiplied(dstReverb.TakeAdvance(tempDst.Length()), tempDst, ReverbCoeff);
	}
	return sampleCount;
}

void NoteSampler::fill(Span<float> ioDst)
{
	for(size_t i = 0; i < WaveFormSamplers.Length(); i++)
	{
		auto remainder = WaveFormSamplers[i].GenerateMono(ioDst);
		if(remainder != nullptr) WaveFormSamplers.RemoveUnordered(i--);
	}
	for(size_t i = 0; i < WaveTableSamplers.Length(); i++)
	{
		auto remainder = WaveTableSamplers[i].GenerateMono(ioDst);
		if(remainder != nullptr) WaveTableSamplers.RemoveUnordered(i--);
	}
	for(size_t i = 0; i < WhiteNoiseSamplers.Length(); i++)
		WhiteNoiseSamplers[i].GenerateMono(ioDst);
	for(size_t i = 0; i < GenericSamplers.Length(); i++)
	{
		const size_t samplesProcessed = GenericSamplers[i]->GenerateMono(ioDst);
		if(samplesProcessed < ioDst.Length()) GenericSamplers.RemoveUnordered(i--);
	}
}

void NoteSampler::fillStereo(Span<float> ioDstLeft, Span<float> ioDstRight, Span<float> ioDstReverb)
{
	for(size_t i = 0; i < WaveFormSamplers.Length(); i++)
	{
		size_t samplesProcessed = WaveFormSamplers[i].GenerateStereo(ioDstLeft, ioDstRight, ioDstReverb);
		if(samplesProcessed != ioDstLeft.Length()) WaveFormSamplers.RemoveUnordered(i--);
	}
	for(size_t i = 0; i < WaveTableSamplers.Length(); i++)
	{
		size_t samplesProcessed = WaveTableSamplers[i].GenerateStereo(ioDstLeft, ioDstRight, ioDstReverb);
		if(samplesProcessed != ioDstLeft.Length()) WaveTableSamplers.RemoveUnordered(i--);
	}
	for(size_t i = 0; i < WhiteNoiseSamplers.Length(); i++)
		WhiteNoiseSamplers[i].GenerateMono(ioDstLeft);
	for(size_t i = 0; i < GenericSamplers.Length(); i++)
	{
		const size_t samplesProcessed = GenericSamplers[i]->GenerateStereo(ioDstLeft, ioDstRight, ioDstReverb);
		if(samplesProcessed < ioDstLeft.Length()) GenericSamplers.RemoveUnordered(i--);
	}
}

void NoteSampler::applyModifiers(Span<float> dst)
{
	for(auto& mod: Modifiers) mod(dst);
	if(ADSR)
	{
		ADSR(dst);
		if(ADSR.SamplesLeft() == 0)
		{
			WaveFormSamplers = nullptr;
			WaveTableSamplers = nullptr;
			WhiteNoiseSamplers = nullptr;
			GenericSamplers = nullptr;
		}
	}
}

void NoteSampler::MultiplyPitch(float freqMultiplier)
{
	for(auto& wave: WaveFormSamplers) wave.MultiplyPitch(freqMultiplier);
	for(auto& wave: WaveTableSamplers) wave.MultiplyPitch(freqMultiplier);
	for(auto& sampler: GenericSamplers) sampler->MultiplyPitch(freqMultiplier);
}

void NoteSampler::NoteRelease()
{
	for(auto& sampler: WaveFormSamplers) sampler.NoteRelease();
	for(auto& sampler: WaveTableSamplers) sampler.NoteRelease();
	for(auto& sampler: GenericSamplers) sampler->NoteRelease();
	if(ADSR) ADSR.NoteRelease();
}

void NoteSampler::SetPan(float pan)
{
	for(auto& sampler: WaveFormSamplers) sampler.SetPan(pan);
	for(auto& sampler: WaveTableSamplers) sampler.SetPan(pan);
	Pan = pan;
}

void NoteSampler::MultiplyVolume(float volumeMultiplier)
{
	for(auto& sampler: WaveFormSamplers) sampler.MultiplyVolume(volumeMultiplier);
	for(auto& sampler: WaveTableSamplers) sampler.MultiplyVolume(volumeMultiplier);
}

void NoteSampler::SetReverbCoeff(float reverbCoeff)
{
	for(auto& sampler: WaveFormSamplers) sampler.SetReverbCoeff(reverbCoeff);
	for(auto& sampler: WaveTableSamplers) sampler.SetReverbCoeff(reverbCoeff);
	ReverbCoeff = reverbCoeff;
}

INTRA_WARNING_POP
