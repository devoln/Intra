#pragma once

#include "Core/Range/Span.h"
#include "Container/Sequential/Array.h"
#include "Audio/SoundTypes.h"

INTRA_BEGIN
struct AudioBuffer
{
	AudioBuffer() = default;
	AudioBuffer(null_t) {}
	AudioBuffer(size_t sampleCount, uint sampleRate=44100, CSpan<float> initData=null);

	double Duration() const {return SampleRate == 0? 0: double(NumFrames())/double(SampleRate);}

	void ShiftSamples(intptr samplesToShift);

	bool operator==(null_t) const {return SampleRate == 0;}
	bool operator!=(null_t) const {return !operator==(null);}

	AudioBuffer& operator=(null_t) {return *this = AudioBuffer{};}

	SoundInfo Info() const {return {Samples.Count(), SampleRate, NumChannels, ValueType::Float};}

	size_t NumFrames() const {return Samples.Count() / NumChannels;}

	uint SampleRate = 0;
	ushort NumChannels = 1;
	Array<float> Samples; //Channeks are stored in the interleaved order
};
INTRA_END
