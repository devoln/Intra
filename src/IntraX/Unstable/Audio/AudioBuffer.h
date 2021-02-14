#pragma once

#include "Intra/Range/Span.h"
#include "IntraX/Container/Sequential/Array.h"
#include "IntraX/Unstable/Audio/SoundTypes.h"

namespace Intra { INTRA_BEGIN
struct AudioBuffer
{
	AudioBuffer() = default;
	AudioBuffer(decltype(nullptr)) {}
	AudioBuffer(Index sampleCount, NonNegative<int> sampleRate = 44100, Span<const float> initData = nullptr);

	double Duration() const {return SampleRate == 0? 0: double(NumFrames())/double(SampleRate);}

	void ShiftSamples(index_t samplesToShift);

	bool operator==(decltype(nullptr)) const {return SampleRate == 0;}
	bool operator!=(decltype(nullptr)) const {return !operator==(nullptr);}

	AudioBuffer& operator=(decltype(nullptr)) {return *this = AudioBuffer{};}

	SoundInfo Info() const {return {Samples.Count(), SampleRate, NumChannels, ValueType::Float};}

	index_t NumFrames() const {return index_t(size_t(Samples.Count()) / size_t(NumChannels));}

	int SampleRate = 0;
	short NumChannels = 1;
	Array<float> Samples; //Channels are stored in the interleaved order
};
} INTRA_END
