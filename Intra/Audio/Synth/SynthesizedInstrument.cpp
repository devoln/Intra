#include "Audio/Synth/SynthesizedInstrument.h"
#include "Audio/AudioBuffer.h"
#include "Audio/Synth/Types.h"
#include "Range/Mutation/Transform.h"

#ifndef INTRA_NO_AUDIO_SYNTH

namespace Intra { namespace Audio { namespace Synth {

SynthesizedInstrument::SynthesizedInstrument(Synth::SynthPass synth,
	CSpan<Synth::ModifierPass> modifiers,
	Synth::AttenuationPass attenuator,
	CSpan<Synth::PostEffectPass> postEffects,
	float minNoteDuration, float fadeOffTime):
	Synth(synth), Modifiers(modifiers), Attenuation(attenuator),
	PostEffects(postEffects), MinNoteDuration(minNoteDuration), FadeOffTime(fadeOffTime) {}

SynthesizedInstrument& SynthesizedInstrument::operator=(const SynthesizedInstrument& rhs)
{
	Synth = rhs.Synth;
	Modifiers = rhs.Modifiers;
	Attenuation = rhs.Attenuation;
	PostEffects = rhs.PostEffects;
	MinNoteDuration = rhs.MinNoteDuration;
	FadeOffTime = rhs.FadeOffTime;
	return *this;
}

SynthesizedInstrument& SynthesizedInstrument::operator=(SynthesizedInstrument&& rhs)
{
	Synth = Cpp::Move(rhs.Synth);
	Modifiers = Cpp::Move(rhs.Modifiers);
	Attenuation = Cpp::Move(rhs.Attenuation);
	PostEffects = Cpp::Move(rhs.PostEffects);
	MinNoteDuration = rhs.MinNoteDuration;
	FadeOffTime = rhs.FadeOffTime;
	return *this;
}





void SynthesizedInstrument::GetNoteSamples(Span<float> inOutSamples,
	MusicNote note, float tempo, float volume, uint sampleRate, bool add) const
{
	INTRA_DEBUG_ASSERT(Synth!=null);
	if(Synth==null) return;
	auto noteDuration=note.AbsDuration(tempo);
	if(noteDuration<0.00001 || note.IsPause() || volume==0) return;
	noteDuration = Math::Max(noteDuration, MinNoteDuration)+FadeOffTime;

	size_t noteSampleCount = inOutSamples.Length();
	if(Modifiers==null && Attenuation==null && PostEffects==null) //Если синтез однопроходный, то промежуточный буфер не нужен
	{
		Synth(note.Frequency(), volume, inOutSamples.Take(noteSampleCount), sampleRate, add);
	}
	else
	{
		Array<float> noteSampleArr;
		noteSampleArr.SetCountUninitialized(noteSampleCount);
		auto sampleBuf = noteSampleArr(0, noteSampleCount);
		Synth(note.Frequency(), volume, sampleBuf, sampleRate, false);
		for(const ModifierPass& pass: Modifiers)
		{
			if(pass==null) continue;
			pass(note.Frequency(), sampleBuf, sampleRate);
		}
		if(Attenuation!=null) Attenuation(noteDuration, sampleBuf/*result.Samples*/, sampleRate);
		for(const PostEffectPass& postEffect: PostEffects)
		{
			if(postEffect==null) continue;
			postEffect(sampleBuf, sampleRate);
		}
		if(!add) CopyTo(sampleBuf, inOutSamples);
		else Add(inOutSamples.Take(noteSampleCount), sampleBuf.AsConstRange());
	}
}

void CombinedSynthesizedInstrument::GetNoteSamples(Span<float> inOutSamples,
	MusicNote note, float tempo, float volume, uint sampleRate, bool add) const
{
	if(Combination==null || note.IsPause()) return;
	int i=0;
	for(auto& instr: Combination)
	{
		instr.GetNoteSamples(inOutSamples, note, tempo, volume, sampleRate, add || i>0);
		i++;
	}
}


}}}


#endif
