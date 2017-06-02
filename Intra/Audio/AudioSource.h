#pragma once

#include "Cpp/Warnings.h"
#include "Cpp/Features.h"

#include "AudioBuffer.h"
#include "Utils/FixedArray.h"
#include "Data/ValueType.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Audio {

class ASoundSource
{
	ASoundSource& operator=(const ASoundSource& rhs) = delete;
protected:
	ASoundSource(uint sampleRate=0, ushort channelCount=0):
		mSampleRate(sampleRate), mChannelCount(channelCount) {}

public:
	virtual ~ASoundSource() {}
	virtual size_t SampleCount() const = 0;
	virtual size_t CurrentSamplePosition() const = 0;

	//! Загрузить следующие семплы в формате чередующихся short каналов.
	//! \param[out] outShorts Куда загружаются семплы. outShorts.Count() означает, сколько их загружать.
	//! \returns Количество прочитанных семплов, то есть число прочитанных short'ов, делённое на ChannelCount.
	virtual size_t GetInterleavedSamples(Span<short> outShorts) = 0;

	//! Загрузить следующие семплы в формате чередующихся float каналов.
	//! \param[out] outFloats Куда загружаются семплы. outFloats.Count() означает, сколько их загружать.
	//! \returns Количество прочитанных семплов, то есть число прочитанных float'ов, делённое на ChannelCount.
	virtual size_t GetInterleavedSamples(Span<float> outFloats) = 0;

	//! Загрузить следующие семплы в формате чередующихся float каналов.
	//! @param[out] outFloats Каналы, в которые загружаются семплы.
	//! outFloats[i].Length() означает, сколько их загружать.
	//! Для всех i outFloats[i].Length() должно быть одинаковым.
	//! outFloats.Length() должно быть равно ChannelCount.
	//! @return Количество прочитанных семплов, то есть число прочитанных float'ов, делённое на ChannelCount.
	virtual size_t GetUninterleavedSamples(CSpan<Span<float>> outFloats) = 0;

	//! Получить данные следующих семплов в том формате, в котором они получаются перед конверсией.
	//! @return Указатели, указывающие на корректные данные соответствующих каналов до тех пор, пока не будет вызыван какой-либо из Get* методов.
	//! null, если не поддерживается потоком, либо существует несколько возможных вариантов выбора типа.
	virtual FixedArray<const void*> GetRawSamplesData(size_t maxSamplesToRead,
		Data::ValueType* outType, bool* outInterleaved, size_t* outSamplesRead)
	{
		(void)maxSamplesToRead;
		if(outType) *outType = Data::ValueType::Void;
		if(outInterleaved) *outInterleaved = false;
		if(outSamplesRead) *outSamplesRead = 0;
		return null;
	}

	//! Сколько непрочитанных семплов осталось в потоке
	forceinline size_t SamplesLeft() const {return SampleCount()-CurrentSamplePosition();}
	forceinline uint SampleRate() const {return mSampleRate;}
	forceinline uint ChannelCount() const {return mChannelCount;}

protected:
	uint mSampleRate;
	ushort mChannelCount;
};

}}

INTRA_WARNING_POP
