#pragma once

#include "Intra/Container/Optional.h"
#include "Extra/Utils/FixedArray.h"
#include "Extra/Utils/Delegate.h"
#include "Extra/Unstable/Data/ValueType.h"

INTRA_BEGIN
class IAudioSource
{
protected:
	IAudioSource() {}
public:
	virtual ~IAudioSource() {}

	//! Частота дискретизации потока семплов.
	virtual int SampleRate() const = 0;

	//! Количество каналов, которое имеет поток.
	virtual int ChannelCount() const = 0;

	//! Total sample count in a stream if is known or null.
	virtual Optional<index_t> SampleCount() const {return null;}

	//! Текущая позиция в потоке в семплах.
	virtual index_t SamplePosition() const = 0;

	//! Установить текущую позицию потока в семплах.
	//! Если не удалось выполнить эту операцию или поток её не поддерживает, возвращает false.
	virtual bool SetSamplePosition(Index position) {(void)position; return false;}

	//! @return the number of remaining samples in the stream or null if stream length is unknown.
	Optional<index_t> SamplesLeft() const
	{
		const auto totalSamples = SampleCount();
		if(!totalSamples) return null;
		return totalSamples.Unwrap() - SamplePosition();
	}


	//! Загрузить следующие семплы в формате чередующихся short каналов.
	//! @param[out] outShorts Куда загружаются семплы. outShorts.Count() означает, сколько их загружать.
	//! @return Количество прочитанных семплов, то есть число прочитанных short'ов, делённое на ChannelCount.
	virtual index_t GetInterleavedSamples(Span<short> outShorts) = 0;

	//! Загрузить следующие семплы в формате чередующихся float каналов.
	//! @param[out] outFloats Куда загружаются семплы. outFloats.Count() означает, сколько их загружать.
	//! @return Количество прочитанных семплов, то есть число прочитанных float'ов, делённое на ChannelCount.
	virtual index_t GetInterleavedSamples(Span<float> outFloats) = 0;

	//! Загрузить следующие семплы в формате чередующихся float каналов.
	//! @param[out] outFloats Каналы, в которые загружаются семплы.
	//! outFloats[i].Length() означает, сколько их загружать.
	//! Для всех i outFloats[i].Length() должно быть одинаковым.
	//! outFloats.Length() должно быть равно ChannelCount.
	//! @return Количество прочитанных семплов, то есть число прочитанных float'ов, делённое на ChannelCount.
	virtual index_t GetUninterleavedSamples(CSpan<Span<float>> outFloats) = 0;

	//! Получить данные следующих семплов в том формате, в котором они получаются перед конверсией.
	//! @return Указатели, указывающие на корректные данные соответствующих каналов до тех пор, пока не будет вызыван какой-либо из Get* методов.
	//! null, если не поддерживается потоком, либо существует несколько возможных вариантов выбора типа.
	virtual FixedArray<const void*> GetRawSamplesData(Index maxSamplesToRead,
		Optional<ValueType&> oType, Optional<bool&> oInterleaved, Optional<index_t&> oSamplesRead) = 0;
};

class BasicAudioSource: public IAudioSource
{
public:
	typedef Delegate<void()> OnCloseResourceCallback;
protected:
	BasicAudioSource(OnCloseResourceCallback onClose, NonNegative<int> sampleRate = 0, NonNegative<short> numChannels = 0):
		mOnCloseResource(Move(onClose)), mSampleRate(sampleRate), mChannelCount(numChannels) {}
public:
	BasicAudioSource(BasicAudioSource&&) = default;
	BasicAudioSource(const BasicAudioSource&) = delete;
	BasicAudioSource& operator=(BasicAudioSource&&) = default;
	BasicAudioSource& operator=(const BasicAudioSource&) = delete;

	~BasicAudioSource() override {if(mOnCloseResource) mOnCloseResource();}

	FixedArray<const void*> GetRawSamplesData(Index maxSamplesToRead,
		Optional<ValueType&> oType, Optional<bool&> oInterleaved, Optional<index_t&> oSamplesRead) override
	{
		(void)maxSamplesToRead;
		if(oType) oType.Unwrap() = ValueType::Void;
		if(oInterleaved) oInterleaved.Unwrap() = false;
		if(oSamplesRead) oSamplesRead.Unwrap() = 0;
		return null;
	}
	
	int SampleRate() const final {return mSampleRate;}
	int ChannelCount() const final {return mChannelCount;}

protected:
	OnCloseResourceCallback mOnCloseResource;
	int mSampleRate;
	short mChannelCount;
};

class SeparateFloatAudioSource: public BasicAudioSource
{
protected:
	SeparateFloatAudioSource(OnCloseResourceCallback onClose, NonNegative<int> sampleRate = 0, NonNegative<short> numChannels = 0):
		BasicAudioSource(Move(onClose), sampleRate, numChannels) {}
public:
	SeparateFloatAudioSource(SeparateFloatAudioSource&&) = default;
	SeparateFloatAudioSource(const SeparateFloatAudioSource&) = delete;
	SeparateFloatAudioSource& operator=(SeparateFloatAudioSource&&) = default;
	SeparateFloatAudioSource& operator=(const SeparateFloatAudioSource&) = delete;

	index_t GetInterleavedSamples(Span<short> outShorts) override;
	index_t GetInterleavedSamples(Span<float> outFloats) override;
};
INTRA_END
