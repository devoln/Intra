#pragma once

#include "Audio/Music.h"
#include "Audio/AudioBuffer.h"
#include "Cpp/Warnings.h"

namespace Intra { namespace Audio { namespace Sources {

#ifndef INTRA_NO_MUSIC_LOADER

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

class MusicSynthSource: public ASoundSource
{
	Music mData;
	AudioBuffer mBuffer;
	size_t mCurrentSamplePos, mSampleCount;

	struct Position { uint samplePos; uint noteId; };

	Array<Position> mCurrentPositions;
	float mMaxVolume;
	size_t mProcessedSamplesToFlush;

public:
	MusicSynthSource(const Music& mydata, uint sampleRate=48000);
	~MusicSynthSource() {}

	size_t SampleCount() const override {return mSampleCount;}
	size_t CurrentSamplePosition() const override {return mCurrentSamplePos; }

	//! Загрузить следующие maxFloatsToGet/ChannelCount семплов в текущий буфер
	//! Если в семплов осталось меньше, то загрузится столько семплов, сколько осталось.
	//! \returns Количество прочитанных float'ов, то есть прочитанное количество семплов, умноженное на ChannelCount
	size_t LoadNextNonNormalizedSamples(uint maxFloatsToGet);


	size_t LoadNextNormalizedSamples(uint maxFloatsToGet);

	//! Удалить уже обработанные семплы из буфера
	void FlushProcessedSamples();


	size_t GetInterleavedSamples(Span<short> outShorts) override;
	size_t GetInterleavedSamples(Span<float> outFloats) override;
	size_t GetUninterleavedSamples(CSpan<Span<float>> outFloats) override;

	Array<const void*> GetRawSamplesData(size_t maxSamplesToRead,
		Data::ValueType* outType, bool* outInterleaved, size_t* outSamplesRead) override;


	MusicSynthSource& operator=(const MusicSynthSource&) = delete;
};

INTRA_WARNING_POP

#endif

}}}
