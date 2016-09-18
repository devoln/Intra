#pragma once

#include "SoundBuilder.h"
#include "Containers/Array.h"
#include "Data/ValueType.h"

namespace Intra {

class ASoundSampleSource
{
	ASoundSampleSource& operator=(const ASoundSampleSource& rhs) = delete;
protected:
	ASoundSampleSource(uint sampleRate=0, ushort channelCount=0):
		sample_rate(sampleRate), channel_count(channelCount) {}

	ASoundSampleSource(const ASoundSampleSource& rhs) = default;

public:
	virtual ~ASoundSampleSource() {}
	virtual size_t SampleCount() const=0;
	virtual size_t CurrentSamplePosition() const=0;

	//! Загрузить следующие семплы в формате чередующихся short каналов.
	//! \param[out] outShorts Куда загружаются семплы. outShorts.Count() означает, сколько их загружать.
	//! \returns Количество прочитанных семплов, то есть число прочитанных short'ов, делённое на ChannelCount.
	virtual size_t GetInterleavedSamples(ArrayRange<short> outShorts)=0;

	//! Загрузить следующие семплы в формате чередующихся float каналов.
	//! \param[out] outFloats Куда загружаются семплы. outFloats.Count() означает, сколько их загружать.
	//! \returns Количество прочитанных семплов, то есть число прочитанных float'ов, делённое на ChannelCount.
	virtual size_t GetInterleavedSamples(ArrayRange<float> outFloats)=0;

	//! Загрузить следующие семплы в формате чередующихся float каналов.
	//! \param[out] outFloats Каналы, в которые загружаются семплы.
	//! outFloats[i].Count() означает, сколько их загружать.
	//! Для всех i outFloats[i].Count() должно быть одинаковым.
	//! outFloats.Count() должно быть равно ChannelCount.
	//! \returns Количество прочитанных семплов, то есть число прочитанных float'ов, делённое на ChannelCount.
	virtual size_t GetUninterleavedSamples(ArrayRange<const ArrayRange<float>> outFloats)=0;

	//! Получить данные следующих семплов в том формате, в котором они получаются перед конверсией.
	//! \returns Указатели, указывающие на корректные данные соответствующих каналов до тех пор, пока не будет вызыван какой-либо из Get* методов.
	//! null, если не поддерживается потоком, либо существует несколько возможных вариантов выбора типа.
	virtual Array<const void*> GetRawSamplesData(size_t maxSamplesToRead,
		ValueType* outType, bool* outInterleaved, size_t* outSamplesRead)
	{
		(void)maxSamplesToRead;
		if(outType!=null) *outType = ValueType::Void;
		if(outInterleaved!=null) *outInterleaved=false;
		if(outSamplesRead!=null) *outSamplesRead=0;
		return null;
	}

	//! Сколько непрочитанных семплов осталось в потоке
	size_t SamplesLeft() const {return SampleCount()-CurrentSamplePosition();}
	uint SampleRate() const {return sample_rate;}
	uint ChannelCount() const {return channel_count;}

protected:
	uint sample_rate;
	ushort channel_count;
};


//#define INTRA_LIBRARY_VORBIS_DECODER INTRA_LIBRARY_VORBIS_DECODER_libvorbis

#if(INTRA_LIBRARY_VORBIS_DECODER!=INTRA_LIBRARY_VORBIS_DECODER_None)

class VorbisSoundSampleSource: public ASoundSampleSource
{
	struct Decoder;
	typedef Decoder* DecoderHandle;
	ArrayRange<const byte> data;
	DecoderHandle decoder;
public:
	VorbisSoundSampleSource(ArrayRange<const byte> srcFileData);
	~VorbisSoundSampleSource();

	size_t SampleCount() const override;
	size_t CurrentSamplePosition() const override;

	size_t GetInterleavedSamples(ArrayRange<short> outShorts) override;
	size_t GetInterleavedSamples(ArrayRange<float> outFloats) override;
	size_t GetUninterleavedSamples(ArrayRange<const ArrayRange<float>> outFloats) override;
	Array<const void*> GetRawSamplesData(size_t maxSamplesToRead,
		ValueType* outType, bool* outInterleaved, size_t* outSamplesRead) override;
};

#endif



#ifndef INTRA_NO_WAVE_LOADER

class WaveSoundSampleSource: public ASoundSampleSource
{
	ArrayRange<const byte> data;
	size_t sample_count, current_data_pos;
public:
	WaveSoundSampleSource(ArrayRange<const byte> srcFileData);

	size_t SampleCount() const override {return sample_count;}
	size_t CurrentSamplePosition() const override {return current_data_pos/channel_count;}

	size_t GetInterleavedSamples(ArrayRange<short> outShorts) override;
	size_t GetInterleavedSamples(ArrayRange<float> outFloats) override;
	size_t GetUninterleavedSamples(ArrayRange<const ArrayRange<float>> outFloats) override;
	Array<const void*> GetRawSamplesData(size_t maxSamplesToRead,
		ValueType* oType, bool* oInterleaved, size_t* oSamplesRead) override;

	WaveSoundSampleSource& operator=(const WaveSoundSampleSource& rhs) = delete;
};

#endif

}
