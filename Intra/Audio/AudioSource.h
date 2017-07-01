#pragma once

#include "Cpp/Warnings.h"
#include "Cpp/Features.h"

#include "AudioBuffer.h"
#include "Utils/FixedArray.h"
#include "Data/ValueType.h"

#include "Funal/Delegate.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Audio {

class AAudioSource
{
protected:
	AAudioSource(uint sampleRate=0, ushort channelCount=0):
		mSampleRate(sampleRate), mChannelCount(channelCount) {}


public:
	AAudioSource(AAudioSource&&) = default;
	AAudioSource(const AAudioSource&) = delete;
	AAudioSource& operator=(AAudioSource&&) = default;
	AAudioSource& operator=(const AAudioSource&) = delete;

	typedef Funal::Delegate<void()> OnCloseResourceCallback;
	AAudioSource(OnCloseResourceCallback onCloseResource): OnCloseResource(Cpp::Move(onCloseResource)) {}

	virtual ~AAudioSource() {if(OnCloseResource) OnCloseResource();}
	virtual size_t SampleCount() const = 0;
	virtual size_t CurrentSamplePosition() const = 0;

	//! Загрузить следующие семплы в формате чередующихся short каналов.
	//! @param[out] outShorts Куда загружаются семплы. outShorts.Count() означает, сколько их загружать.
	//! @return Количество прочитанных семплов, то есть число прочитанных short'ов, делённое на ChannelCount.
	virtual size_t GetInterleavedSamples(Span<short> outShorts) = 0;

	//! Загрузить следующие семплы в формате чередующихся float каналов.
	//! @param[out] outFloats Куда загружаются семплы. outFloats.Count() означает, сколько их загружать.
	//! @return Количество прочитанных семплов, то есть число прочитанных float'ов, делённое на ChannelCount.
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
		Data::ValueType* oType, bool* oInterleaved, size_t* oSamplesRead)
	{
		(void)maxSamplesToRead;
		if(oType) *oType = Data::ValueType::Void;
		if(oInterleaved) *oInterleaved = false;
		if(oSamplesRead) *oSamplesRead = 0;
		return null;
	}

	//! Сколько непрочитанных семплов осталось в потоке
	forceinline size_t SamplesLeft() const {return SampleCount() - CurrentSamplePosition();}
	forceinline uint SampleRate() const {return mSampleRate;}
	forceinline uint ChannelCount() const {return mChannelCount;}

	OnCloseResourceCallback OnCloseResource;
protected:
	uint mSampleRate;
	ushort mChannelCount;
};

}}

INTRA_WARNING_POP
