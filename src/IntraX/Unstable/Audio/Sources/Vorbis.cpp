﻿#include "IntraX/Unstable/Audio/Sources/Vorbis.h"
#include "IntraX/Utils/Endianess.h"
#include "Intra/Range/Polymorphic/InputRange.h"

#if(INTRA_LIBRARY_VORBIS_DECODER == INTRA_LIBRARY_VORBIS_DECODER_libvorbis)

#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>
#include <stdio.h>

#elif(INTRA_LIBRARY_VORBIS_DECODER == INTRA_LIBRARY_VORBIS_DECODER_STB)

#include "stb_vorbis.h"

#endif

namespace Intra { INTRA_BEGIN

#if(INTRA_LIBRARY_VORBIS_DECODER == INTRA_LIBRARY_VORBIS_DECODER_libvorbis)

struct OggStream
{
	ForwardStream StreamStart;
	InputStream Stream;
	uint64 Pos, Size;

	static size_t ReadCallback(void* ptr, size_t size, size_t nmemb, void* datasource)
	{
		auto& os = reinterpret_cast<Decoder*>(datasource)->stream;
		size_t result = os.stream.RawRead(ptr, size*nmemb);
		Pos += result;
		return result;
	}

	static int CloseCallback(void *datasource)
	{
		StreamStart = nullptr;
		Stream = nullptr;
		Pos = 0;
		Size = 0;
		return 0;
	}

	void SeekAbs(uint64 offset)
	{
		if(offset > Size) offset = Size;

		if(Pos > offset)
		{
			os.Stream = os.StreamStart;
			Pos = 0;
		}

		while(offset > ~size_t(0))
		{
			os.Stream.PopFirstCount(~size_t(0));
			offset -= ~size_t(0);
		}
		os.Stream.PopFirstCount(size_t(offset));
		Pos = offset;
	}

	static int SeekCallback(void* datasource, ogg_int64_t offset, int whence)
	{
		auto& os = reinterpret_cast<Decoder*>(datasource)->stream;

		//Без forward range seek в общем случае реализовать нельзя.
		//Библиотека libvorbis требует всегда возвращать -1, реализовывать частные случаи смысла нет.
		if(os.StreamStart == nullptr) return -1;

		if(whence==SEEK_SET) SeekAbs(uint64(offset));
		else if(whence==SEEK_CUR) SeekAbs(uint64(int64(Pos)+offset));
		else if(whence==SEEK_END) SeekAbs(uint64(int64(Size)+offset));
		return 0;
	}

	static long TellCallback(void* datasource)
	{
		auto& os = reinterpret_cast<Decoder*>(datasource)->stream;
		return os.Pos;
	}
};

struct Vorbis::Decoder
{
	OggVorbis_File file;
	OggStream stream;
	int currentSection;
	size_t currentPosition;
};



Vorbis::Vorbis(Span<const byte> srcFileData): data(srcFileData)
{
	decoder = new Decoder;
	decoder->stream.StartStream = srcFileData.ReinterpretUnsafe<char>();
	decoder->stream.Stream = srcFileData.ReinterpretUnsafe<char>();
	decoder->stream.Pos = 0;
	decoder->stream.Size = srcFileData.Length();
	ov_callbacks c = {OggStream::ReadCallback, OggStream::SeekCallback,
		OggStream::CloseCallback, OggStream::TellCallback};
	ov_open_callbacks(&decoder, &decoder->file, nullptr, 0, c);
	decoder->currentSection = 0;

	auto info = ov_info(&decoder->file, -1);
	channelCount = uint16(info->channels);
	sampleRate = unsigned(info->rate);
}

Vorbis::~Vorbis()
{
	ov_clear(&decoder->file);
	delete decoder;
}

size_t Vorbis::SampleCount() const {return size_t(ov_pcm_total(&decoder->file, -1));}
size_t Vorbis::CurrentSamplePosition() const {return decoder->current_position;}

size_t Vorbis::GetInterleavedSamples(Span<short> outShorts)
{
	size_t totalSamplesRead=0;
	while(!outShorts.Empty())
	{
		auto ret = ov_read(&decoder->file, reinterpret_cast<char*>(outShorts.Begin), int(outShorts.Length()),
			false, sizeof(short), true, &decoder->current_section);
		if(ret==0) break;
		if(ret<0)
		{
			IO::ConsoleError.PrintLine("Error loading ogg!");
			break;
		}
		size_t samplesRead = size_t(ret)/sizeof(short);
		totalSamplesRead += samplesRead;
		decoder->current_position += samplesRead;
		outShorts.PopFirstExactly(samplesRead);
	};
	return totalSamplesRead;
}

size_t Vorbis::GetInterleavedSamples(Span<float> outFloats)
{
	size_t totalSamplesRead = 0;
	while(!outFloats.Empty())
	{
		float** pcm;
		auto bytesRead = ov_read_float(&decoder->file, &pcm,
			int(outFloats.Length()), &decoder->current_section);
		if(bytesRead<=0) return 0;
		size_t samplesRead = size_t(bytesRead)/sizeof(float);
		decoder->current_position += samplesRead;
		totalSamplesRead += samplesRead;
		Span<const float> inputChannels[8];
		for(size_t i=0; i<channelCount; i++)
			inputChannels[i] = Span<const float>(pcm[i], samplesRead);
		Algo::Interleave(outFloats.Take(samplesRead), Span<const Span<const float>>(inputChannels, channelCount));
		outFloats.PopFirstExactly(samplesRead);
	}
	return totalSamplesRead;
}

size_t Vorbis::GetUninterleavedSamples(Span<const Span<float>> outFloats)
{
	if(outFloats.Empty()) return 0;
	INTRA_DEBUG_ASSERT(outFloats.Length()<=channelCount);
	size_t totalSamplesRead = 0;
	Span<float> outFloats1[8];
	for(size_t i=0; i<outFloats.Length(); i++) outFloats1[i] = outFloats[i];
	while(!outFloats1[0].Empty())
	{
		float** pcm;
		auto bytesRead = ov_read_float(&decoder->file, &pcm, int(outFloats1[0].Length()), &decoder->current_section);
		if(bytesRead<=0) return 0;
		size_t samplesRead = size_t(bytesRead)/sizeof(float);
		decoder->current_position += samplesRead;
		totalSamplesRead += samplesRead;
		for(size_t i=0; i<outFloats.Length(); i++)
		{
			Span<const float>(pcm[i], samplesRead).WriteTo(outFloats1[i]);
		}
	}
	return totalSamplesRead;
}

FixedArray<const void*> Vorbis::GetRawSamplesData(size_t maxSamplesToRead,
	ValueType* oType, bool* oInterleaved, size_t* oSamplesRead)
{
	if(oType) *outType = ValueType::Float;
	if(oInterleaved) *oInterleaved = true;
	if(oSamplesRead) *oSamplesRead = 0;
	return nullptr;
}

#elif(INTRA_LIBRARY_VORBIS_DECODER==INTRA_LIBRARY_VORBIS_DECODER_STB)

Vorbis::Vorbis(Span<const byte> srcFileData): data(srcFileData)
{
	decoder = reinterpret_cast<DecoderHandle>(stb_vorbis_open_memory(
		reinterpret_cast<byte*>(srcFileData.Begin), unsigned(srcFileData.Count()), nullptr, nullptr));
	stb_vorbis_info info = stb_vorbis_get_info(reinterpret_cast<stb_vorbis*>(decoder));
	channelCount = uint16(info.channels);
	sampleRate = info.sample_rate;
}

Vorbis::~Vorbis()
{
	stb_vorbis_close(reinterpret_cast<stb_vorbis*>(decoder));
}

size_t Vorbis::SampleCount() const
{
	return stb_vorbis_stream_length_in_samples(reinterpret_cast<stb_vorbis*>(decoder));
}

size_t Vorbis::CurrentSamplePosition() const
{
	return stb_vorbis_get_sample_offset(reinterpret_cast<stb_vorbis*>(decoder));
}

size_t Vorbis::GetInterleavedSamples(Span<short> outShorts)
{
	const auto dec = reinterpret_cast<stb_vorbis*>(decoder);
	size_t samplesRead = stb_vorbis_get_samples_short_interleaved(dec, channelCount, outShorts.Begin, int(outShorts.Count()));
	if(channelCount*samplesRead<outShorts.Count()) stb_vorbis_seek_start(dec);
	return samplesRead;
}

size_t Vorbis::GetInterleavedSamples(Span<float> outFloats)
{
	const auto dec = reinterpret_cast<stb_vorbis*>(decoder);
	size_t shortsRead = channelCount*stb_vorbis_get_samples_float_interleaved(
		dec, channelCount, outFloats.Begin, int(outFloats.Count()));
	if(shortsRead<outFloats.Count()) stb_vorbis_seek_start(dec);
	return shortsRead/channelCount;
}

size_t Vorbis::GetUninterleavedSamples(Span<const Span<float>> outFloats)
{
	INTRA_DEBUG_ASSERT(outFloats.Length()==channelCount);
	const auto dec = reinterpret_cast<stb_vorbis*>(decoder);
	float* outFloatsPtrs[16];
	for(uint16 c=0; c<channelCount; c++)
		outFloatsPtrs[c] = outFloats[c].Begin;
	const size_t samplesRead = stb_vorbis_get_samples_float(dec,
		channelCount, outFloatsPtrs, int(outFloats.Count()));
	const size_t shortsRead = channelCount*samplesRead;
	if(shortsRead < outFloats.Length()) stb_vorbis_seek_start(dec);
	return shortsRead/channelCount;
}

FixedArray<const void*> Vorbis::GetRawSamplesData(size_t maxSamplesToRead,
	ValueType* oType, bool* oInterleaved, size_t* oSamplesRead)
{
	(void)maxSamplesToRead;
	if(oType) *oType = ValueType::Void;
	if(oInterleaved) *oInterleaved = false;
	if(oSamplesRead) *oSamplesRead = 0;
	return nullptr;
}

#else

INTRA_IGNORE_WARN_LNK4221

#endif

} INTRA_END
