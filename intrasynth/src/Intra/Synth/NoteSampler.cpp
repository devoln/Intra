#include "NoteSampler.h"
#include "SamplerTask.h"

#ifdef INTRA_PROBE_NAN
#include <stdio.h>
#endif

#include <Range/Mutation/Fill.h>
#include <Range/Mutation/Transform.h>
#include <Range/Mutation/Copy.h>

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
		// Громкость канала (MIDI CC7/мьют дорожки) — живой слой поверх
		// «запечённой» в тело ноты стартовой громкости. 1 — как родилась
		// (обычный путь, без лишних проходов); иначе нота рендерится в отдельный
		// буфер, потому что буферы кадра общие для всех нот. При нуле сигнал в
		// микс не попадает, но состояния (огибающие, затухания) идут своим
		// ходом — голос завершается сам и оживает при снятии мьюта или подъёме
		// громкости канала.
		const float channelGain = SamplerPtr->ChannelGain;
		if(channelGain == 1.0f)
			SamplerPtr->GenerateStereo(left, right);
		else
		{
			auto scratchL = stc.ScratchL.Take(n);
			auto scratchR = stc.ScratchR.Take(n);
			FillZeros(scratchL);
			FillZeros(scratchR);
			SamplerPtr->GenerateStereo(scratchL, scratchR);
			if(channelGain != 0.0f)
				for(size_t i = 0; i < n; i++)
				{
					left[i] += scratchL[i]*channelGain;
					right[i] += scratchR[i]*channelGain;
				}
		}
		stc.UsedChannels |= LeftChannel | RightChannel;

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

bool NoteSampler::CanRenderAdsrDirect() const
{
	if(Modifiers.Length() != 0 || !WaveFormSamplers.Empty() ||
		!WaveTableSamplers.Empty() || !WhiteNoiseSamplers.Empty() ||
		GenericSamplers.Empty()) return false;
	for(size_t i = 0; i < GenericSamplers.Length(); i++)
		if(!GenericSamplers[i]->SupportsEnvelopeRender()) return false;
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

#ifdef INTRA_UI_METERS
float NoteSampler::GetLevel() const
{
	const float adsr = ADSR.Active ? ADSR.Env.CurrentSegment.Volume : 1.0f;
	// Тело ноты: самая громкая из огибающих слоёв (волновые таблицы и формы).
	float body = -1.0f;
	for(const auto& s: WaveTableSamplers) body = Max(body, s.GetLevel());
	for(const auto& s: WaveFormSamplers) body = Max(body, s.GetLevel());
	// Generic-семплеры (пиано-партиалы, струны, физические модели): уровень
	// отдают только те, у кого он есть (пиано — по партиалам, струны — по
	// своему затуханию); остальные возвращают -1 «не измеряю» — тогда body
	// остаётся прежним и поведение индикатора не меняется.
	for(size_t i = 0; i < GenericSamplers.Length(); i++)
		body = Max(body, GenericSamplers[i]->GetLevel());
	// Огибающая дорaботала — ноты больше нет: 0, а не «огибающей нет → 1.0»
	// (иначе после релиза индикатор вспыхивал бы обратно на полную яркость).
	if(ADSR.Done) return 0.0f;
	if(body < 0)
	{
		// Измеримых слоёв нет. Если они были раньше (mBodySeen) или слоёв не
		// осталось вовсе (Empty) — тело ноты отзвучало, индикатор обязан
		// погаснуть: раньше здесь возвращалось 1.0 «огибающая неизвестна», и
		// прямоугольник вспыхивал на полную яркость уже после релиза.
		if(Empty() || mBodySeen) return 0.0f;
		// Слои есть, но все без огибающей (ударные/шум/generic-семплеры):
		// измерить нечего — считаем уровень ровным, как раньше.
		return adsr;
	}
	mBodySeen = true;
	return adsr * body;
}
#endif

size_t NoteSampler::GenerateStereo(Span<float> dstLeft, Span<float> dstRight)
{
		// Честный стерео-рендер (Update 18): все ноты идут через один путь —
		// источники пишут в оба канала напрямую (wavetable-тело — истинно стерео
		// с задержкой правого канала, шумовые слои — 0.5/0.5 на канал),
		// модификаторы и ADSR применяются к каждому каналу отдельно. Их
		// расписание зависит только от времени, поэтому одинаковое расписание
		// фильтров на L и R корректно. Раньше ноты с модификаторами/ADSR
		// сводились в моно-буфер (0.5/0.5 панорамы): тело теряло истинную
		// стерео (звучало одним центрированным голосом), а шум на канал был на
		// +6 дБ громче, чем у нот без модификаторов — отсюда «два тела» у
		// FluteHybrid против FluteClean.
		if(Modifiers.Empty() && !ADSR)
		{
			fillStereo(dstLeft, dstRight);
			return dstLeft.Length();
		}

		// Generic ADSR-only voices can pass the envelope into their own render
		// loop. The shared frame mix is then touched only by +=, so no snapshot
		// or clear/add restoration is needed.
		if(ADSR && CanRenderAdsrDirect())
		{
			const size_t n = Math::Min(dstLeft.Length(), dstRight.Length());
			size_t processed = 0;
			while(processed < n && ADSR)
			{
				const size_t chunkLen = Math::Min<size_t>(1024,
					Math::Min<size_t>(n - processed, ADSR.Env.CurrentSegment.SamplesLeft));
				auto chunkL = dstLeft.Drop(processed).Take(chunkLen);
				auto chunkR = dstRight.Drop(processed).Take(chunkLen);
				EnvelopeSegment segment(ADSR.Env.CurrentSegment);
				for(size_t i = 0; i < GenericSamplers.Length();)
				{
					const size_t rendered = GenericSamplers[i]->GenerateStereoWithEnvelope(chunkL, chunkR, segment);
					if(rendered < chunkLen) GenericSamplers.RemoveUnordered(i);
					else i++;
				}
				ADSR.Advance(chunkLen);
				processed += chunkLen;
			}
			if(!ADSR)
			{
				GenericSamplers = nullptr;
			}
			return n;
		}

		// Модификаторы и ADSR голоса можно применять ТОЛЬКО к его собственному
		// сигналу. Буферы кадра (stc.Channels) общие: в них уже накоплен микс
		// остальных голосов, поэтому обработка спана на месте умножает чужие
		// сэмплы своей огибающей (линейный релиз 1→0 «глушит» соседние голоса
		// на всю свою длину) и гонит чужой сигнал через свои фильтры — это и
		// была причина «piano ломает звук соседних инструментов» (Update 18):
		// голос с ADSR/фильтрами, чья задача выполнялась после соседней,
		// портил уже сведённый микс. Рендерим голос в обнулённый кусок кадра
		// (стековый скретч 1024, как в прежнем mono-пути), применяем его
		// модификаторы/ADSR к сигналу одного голоса и возвращаем микс обратно.
		float savedL[1024] = {0}, savedR[1024] = {0};
		size_t processed = 0;
		while(processed < dstLeft.Length())
		{
			const size_t chunkLen = Math::Min<size_t>(1024, dstLeft.Length() - processed);
			auto chunkL = dstLeft.Drop(processed).Take(chunkLen);
			auto chunkR = dstRight.Drop(processed).Take(chunkLen);

			// Чужой микс в регионе сохраняем, регион обнуляем.
			CopyTo(chunkL, Take(savedL, chunkLen));
			CopyTo(chunkR, Take(savedR, chunkLen));
			FillZeros(chunkL);
			FillZeros(chunkR);

			// В обнулённый регион — только этот голос, затем его обработка.
			fillStereo(chunkL, chunkR);
			applyModifiersStereo(chunkL, chunkR);

			// Микс обратно: итог = чужой микс + обработанный голос.
			Add(chunkL, Take(savedL, chunkLen));
			Add(chunkR, Take(savedR, chunkLen));

			processed += chunkLen;
		}
		return dstLeft.Length();
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

void NoteSampler::fillStereo(Span<float> ioDstLeft, Span<float> ioDstRight)
{
	for(size_t i = 0; i < WaveFormSamplers.Length(); i++)
	{
		size_t samplesProcessed = WaveFormSamplers[i].GenerateStereo(ioDstLeft, ioDstRight);
		if(samplesProcessed != ioDstLeft.Length()) WaveFormSamplers.RemoveUnordered(i--);
	}
	for(size_t i = 0; i < WaveTableSamplers.Length(); i++)
	{
		size_t samplesProcessed = WaveTableSamplers[i].GenerateStereo(ioDstLeft, ioDstRight);
		if(samplesProcessed != ioDstLeft.Length()) WaveTableSamplers.RemoveUnordered(i--);
	}
	for(size_t i = 0; i < WhiteNoiseSamplers.Length(); i++)
		WhiteNoiseSamplers[i].GenerateStereo(ioDstLeft, ioDstRight);
	for(size_t i = 0; i < GenericSamplers.Length(); i++)
	{
		const size_t samplesProcessed = GenericSamplers[i]->GenerateStereo(ioDstLeft, ioDstRight);
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

void NoteSampler::applyModifiersStereo(Span<float> dstL, Span<float> dstR)
{
	// Модификаторы (фильтры со своим расписанием и IIR-памятью) хранятся парой:
	// Modifiers[i] ведёт левый канал, ModifiersRight[i] — правый. Обе копии
	// созданы одной фабрикой и обрабатывают одинаковые по длине куски, поэтому
	// расписание во времени у них совпадает (атака-свип CutoffFactory и т.п. не
	// сдвигается на длину буфера для R), а память фильтров у каналов своя.
	// Прежний вариант копировал модификатор перед проходом левого канала и
	// обрабатывал правый этой копией: на КАЖДОЙ границе куска (1024 семпла)
	// память правого канала подменялась состоянием левого, и правый канал щёлкал
	// (у FluteClean/Titanic — слышимый треск на B5, где фильтр атаки ещё открыт).
	INTRA_DEBUG_ASSERT(ModifiersRight.Length() == Modifiers.Length());
	for(size_t i = 0; i < Modifiers.Length(); i++)
	{
		Modifiers[i](dstL);
		ModifiersRight[i](dstR);
	}
	if(!ADSR) return;

	// Огибающая зависит только от времени: один и тот же фактор на оба канала.
	// Снапшот состояния (включая Active!) до левого канала восстанавливаем
	// перед правым — иначе, если огибающая завершилась ровно на левом,
	// правый остался бы без финального сегмента (и без обнуления хвоста).
	// Весь спан обрабатывается ОДНИМ вызовом ADSR на канал: AdsrAttenuator сам
	// проходит все сегменты внутри вызова (и сразу переходит на следующий
	// сегмент на границе буфера — см. ADSR.h). Прежний вариант дробил спан по
	// границам сегментов и выходил из цикла на каждой такой границе: нота
	// обнулялась и снималась в конце первого же сегмента (атаки) — «ноты
	// прерываются, в целом тише, рендер быстрее».
	Envelope envState = ADSR.Env;
	const bool activeState = ADSR.Active;
	ADSR(dstL);
	ADSR.Env = Move(envState);
	ADSR.Active = activeState;
	ADSR(dstR);

	// Active == false ⇔ огибающая полностью отработала (хвост обоих каналов
	// уже обнулён внутри AdsrAttenuator). SamplesLeft() == 0 сюда не годится:
	// при точном попадании конца сегмента на конец буфера это значение не
	// отличает «завершилась» от «на границе» без eager-перехода выше.
	if(!ADSR.Active)
	{
		WaveFormSamplers = nullptr;
		WaveTableSamplers = nullptr;
		WhiteNoiseSamplers = nullptr;
		GenericSamplers = nullptr;
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
	for(auto& sampler: GenericSamplers) sampler->SetPan(pan);
	Pan = pan;
}

void NoteSampler::SetVelocity(float velocity01)
{
	for(auto& sampler: GenericSamplers) sampler->SetVelocity(velocity01);
}

void NoteSampler::MultiplyVolume(float volumeMultiplier)
{
	for(auto& sampler: WaveFormSamplers) sampler.MultiplyVolume(volumeMultiplier);
	for(auto& sampler: WaveTableSamplers) sampler.MultiplyVolume(volumeMultiplier);
}

void NoteSampler::SetRenderParams(const RenderParams& params)
{
	for(auto& sampler: GenericSamplers) sampler->SetRenderParams(params);
}

INTRA_WARNING_POP
