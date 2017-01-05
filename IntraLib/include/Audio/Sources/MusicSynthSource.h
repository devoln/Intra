#pragma once

#include "Audio/Music.h"
#include "Audio/AudioBuffer.h"
#include "Platform/CppWarnings.h"

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

	//! ��������� ��������� maxFloatsToGet/ChannelCount ������� � ������� �����
	//! ���� � ������� �������� ������, �� ���������� ������� �������, ������� ��������.
	//! \returns ���������� ����������� float'��, �� ���� ����������� ���������� �������, ���������� �� ChannelCount
	size_t LoadNextNonNormalizedSamples(uint maxFloatsToGet);


	size_t LoadNextNormalizedSamples(uint maxFloatsToGet);

	//! ������� ��� ������������ ������ �� ������
	void FlushProcessedSamples();


	size_t GetInterleavedSamples(ArrayRange<short> outShorts) override;
	size_t GetInterleavedSamples(ArrayRange<float> outFloats) override;
	size_t GetUninterleavedSamples(ArrayRange<const ArrayRange<float>> outFloats) override;

	Array<const void*> GetRawSamplesData(size_t maxSamplesToRead,
		ValueType* outType, bool* outInterleaved, size_t* outSamplesRead) override;


	MusicSynthSource& operator=(const MusicSynthSource&) = delete;
};

INTRA_WARNING_POP

#endif

}}}
