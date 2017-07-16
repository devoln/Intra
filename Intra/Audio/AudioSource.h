#pragma once

#include "Cpp/Warnings.h"
#include "Cpp/Features.h"

#include "Utils/FixedArray.h"

#include "Data/ValueType.h"

#include "Funal/Delegate.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Audio {

class IAudioSource
{
protected:
	IAudioSource() {}
public:
	virtual ~IAudioSource() {}

	//! Частота дискретизации потока семплов.
	virtual uint SampleRate() const = 0;

	//! Количество каналов, которое имеет поток.
	virtual uint ChannelCount() const = 0;

	//! Полное количество семплов в потоке. Возвращает ~size_t(), если оно неизвестно.
	virtual size_t SampleCount() const {return ~size_t();}

	//! Текущая позиция в потоке в семплах.
	virtual size_t SamplePosition() const = 0;

	//! Установить текущую позицию потока в семплах.
	//! Если не удалось выполнить эту операцию или поток её не поддерживает, возвращает false.
	virtual bool SetSamplePosition(size_t position) {(void)position; return false;}

	//! Сколько непрочитанных семплов осталось в потоке.
	//! Если это значение неизвестно, возвращает ~size_t().
	forceinline size_t SamplesLeft() const
	{
		const size_t totalSamples = SampleCount();
		if(totalSamples == ~size_t()) return ~size_t();
		return totalSamples - SamplePosition();
	}


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
		Data::ValueType* oType, bool* oInterleaved, size_t* oSamplesRead) = 0;
};

class BasicAudioSource: public IAudioSource
{
public:
	typedef Funal::Delegate<void()> OnCloseResourceCallback;
protected:
	BasicAudioSource(OnCloseResourceCallback onClose, uint sampleRate=0, ushort numChannels=0):
		mOnCloseResource(Cpp::Move(onClose)), mSampleRate(sampleRate), mChannelCount(numChannels) {}
public:
	BasicAudioSource(BasicAudioSource&&) = default;
	BasicAudioSource(const BasicAudioSource&) = delete;
	BasicAudioSource& operator=(BasicAudioSource&&) = default;
	BasicAudioSource& operator=(const BasicAudioSource&) = delete;

	~BasicAudioSource() {if(mOnCloseResource) mOnCloseResource();}

	FixedArray<const void*> GetRawSamplesData(size_t maxSamplesToRead,
		Data::ValueType* oType, bool* oInterleaved, size_t* oSamplesRead) override
	{
		(void)maxSamplesToRead;
		if(oType) *oType = Data::ValueType::Void;
		if(oInterleaved) *oInterleaved = false;
		if(oSamplesRead) *oSamplesRead = 0;
		return null;
	}
	
	uint SampleRate() const final {return mSampleRate;}
	uint ChannelCount() const final {return mChannelCount;}

protected:
	OnCloseResourceCallback mOnCloseResource;
	uint mSampleRate;
	ushort mChannelCount;
};

class SeparateFloatAudioSource: public BasicAudioSource
{
protected:
	SeparateFloatAudioSource(OnCloseResourceCallback onClose, uint sampleRate=0, ushort numChannels=0):
		BasicAudioSource(Cpp::Move(onClose), sampleRate, numChannels) {}
public:
	SeparateFloatAudioSource(SeparateFloatAudioSource&&) = default;
	SeparateFloatAudioSource(const SeparateFloatAudioSource&) = delete;
	SeparateFloatAudioSource& operator=(SeparateFloatAudioSource&&) = default;
	SeparateFloatAudioSource& operator=(const SeparateFloatAudioSource&) = delete;

	size_t GetInterleavedSamples(Span<short> outShorts) override;
	size_t GetInterleavedSamples(Span<float> outFloats) override;
};

}}

INTRA_WARNING_POP
