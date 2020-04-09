#pragma once

#include "Intra/Range/Span.h"
#include "Extra/Container/Sequential/Array.h"
#include "Extra/Unstable/Audio/SoundTypes.h"

INTRA_BEGIN
struct AudioBuffer
{
	AudioBuffer() = default;
	AudioBuffer(decltype(null)) {}
	AudioBuffer(Index sampleCount, NonNegative<int> sampleRate = 44100, CSpan<float> initData = null);

	double Duration() const {return SampleRate == 0? 0: double(NumFrames())/double(SampleRate);}

	void ShiftSamples(index_t samplesToShift);

	bool operator==(decltype(null)) const {return SampleRate == 0;}
	bool operator!=(decltype(null)) const {return !operator==(null);}

	AudioBuffer& operator=(decltype(null)) {return *this = AudioBuffer{};}

	SoundInfo Info() const {return {Samples.Count(), SampleRate, NumChannels, ValueType::Float};}

	index_t NumFrames() const {return index_t(size_t(Samples.Count()) / size_t(NumChannels));}

	int SampleRate = 0;
	short NumChannels = 1;
	Array<float> Samples; //Channels are stored in the interleaved order
};
INTRA_END
