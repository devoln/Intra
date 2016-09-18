#include "Sound/SoundSource.h"
#include "Platform/Endianess.h"
#include "IO/Stream.h"

#if(INTRA_LIBRARY_VORBIS_DECODER==INTRA_LIBRARY_VORBIS_DECODER_libvorbis)

#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>
#include <stdio.h>

#endif

namespace Intra {

using namespace Math;
using namespace IO;


#if(INTRA_LIBRARY_VORBIS_DECODER==INTRA_LIBRARY_VORBIS_DECODER_libvorbis)

struct VorbisSoundSampleSource::Decoder
{
	OggVorbis_File file;
	MemoryInputStream stream;
	int current_section;
	size_t current_position;
};

static size_t stream_read_func(void* ptr, size_t size, size_t nmemb, void* datasource)
{
	auto stream = (IInputStream*)datasource;
	stream->ReadData(ptr, size*nmemb);
	return stream->EndOfStream()? 0: size*nmemb;
}

static int stream_seek_func(void* datasource, ogg_int64_t offset, int whence)
{
	auto stream = (IInputStream*)datasource;
	if(whence==SEEK_SET) stream->SetPos(offset);
	if(whence==SEEK_CUR) stream->SetPos(stream->GetPos()+offset);
	if(whence==SEEK_END) stream->SetPos(stream->GetSize()+offset);
	return 0;
}

static long stream_tell_func(void* datasource) {return ((IInputStream*)datasource)->GetPos();}

VorbisSoundSampleSource::VorbisSoundSampleSource(ArrayRange<const byte> srcFileData): data(srcFileData)
{
	decoder = new Decoder;
	decoder->stream = srcFileData;
	ov_callbacks c = {stream_read_func, stream_seek_func, null, stream_tell_func};
	ov_open_callbacks(&decoder->stream, &decoder->file, null, 0, c);
	decoder->current_section = 0;

	auto info = ov_info(&decoder->file, -1);
	channelCount = (ushort)info->channels;
	sampleRate = (uint)info->rate;
}

VorbisSoundSampleSource::~VorbisSoundSampleSource()
{
	ov_clear(&decoder->file);
	delete decoder;
}

size_t VorbisSoundSampleSource::SampleCount() const {return (size_t)ov_pcm_total(&decoder->file, -1);}
size_t VorbisSoundSampleSource::CurrentSamplePosition() const {return decoder->current_position;}

size_t VorbisSoundSampleSource::GetInterleavedSamples(ArrayRange<short> outShorts)
{
	size_t totalSamplesRead=0;
	while(!outShorts.Empty())
	{
		auto ret = ov_read(&decoder->file, (char*)outShorts.Begin, (int)outShorts.Length(),
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

size_t VorbisSoundSampleSource::GetInterleavedSamples(ArrayRange<float> outFloats)
{
	size_t totalSamplesRead = 0;
	while(!outFloats.Empty())
	{
		float** pcm;
		auto bytesRead = ov_read_float(&decoder->file, &pcm, (int)outFloats.Length(), &decoder->current_section);
		if(bytesRead<=0) return 0;
		size_t samplesRead = size_t(bytesRead)/sizeof(float);
		decoder->current_position += samplesRead;
		totalSamplesRead += samplesRead;
		ArrayRange<const float> inputChannels[8];
		for(size_t i=0; i<channelCount; i++) inputChannels[i] = ArrayRange<const float>(pcm[i], samplesRead);
		Algo::Interleave(outFloats.Take(samplesRead), ArrayRange<const ArrayRange<const float>>(inputChannels, channelCount));
		outFloats.PopFirstExactly(samplesRead);
	}
	return totalSamplesRead;
}

size_t VorbisSoundSampleSource::GetUninterleavedSamples(ArrayRange<const ArrayRange<float>> outFloats)
{
	if(outFloats.Empty()) return 0;
	INTRA_ASSERT(outFloats.Length()<=channelCount);
	size_t totalSamplesRead = 0;
	ArrayRange<float> outFloats1[8];
	for(size_t i=0; i<outFloats.Length(); i++) outFloats1[i] = outFloats[i];
	while(!outFloats1[0].Empty())
	{
		float** pcm;
		auto bytesRead = ov_read_float(&decoder->file, &pcm, (int)outFloats1[0].Length(), &decoder->current_section);
		if(bytesRead<=0) return 0;
		size_t samplesRead = size_t(bytesRead)/sizeof(float);
		decoder->current_position += samplesRead;
		totalSamplesRead += samplesRead;
		for(size_t i=0; i<outFloats.Length(); i++)
		{
			ArrayRange<const float>(pcm[i], samplesRead).CopyToAdvance(outFloats1[i]);
		}
	}
	return totalSamplesRead;
}

Array<const void*> VorbisSoundSampleSource::GetRawSamplesData(size_t maxSamplesToRead,
	ValueType* outType, bool* outInterleaved, size_t* outSamplesRead)
{
	if(outType!=null) *outType = ValueType::Float;
	if(outInterleaved) *outInterleaved = true;
	if(outSamplesRead) *outSamplesRead = 0;
	return null;
}

#else


#if(INTRA_LIBRARY_VORBIS_DECODER==INTRA_LIBRARY_VORBIS_DECODER_STB)

#include "stb_vorbis.h"

VorbisSoundSampleSource::VorbisSoundSampleSource(ArrayRange<const byte> srcFileData): data(srcFileData)
{
	decoder = (DecoderHandle)stb_vorbis_open_memory((byte*)srcFileData.Begin, (uint)srcFileData.Count(), null, null);
	stb_vorbis_info info = stb_vorbis_get_info((stb_vorbis*)decoder);
	channelCount = (ushort)info.channels;
	sampleRate = info.sample_rate;
}

VorbisSoundSampleSource::~VorbisSoundSampleSource()
{
	stb_vorbis_close((stb_vorbis*)decoder);
}

size_t VorbisSoundSampleSource::SampleCount() const
{
	return stb_vorbis_stream_length_in_samples((stb_vorbis*)decoder);
}

size_t VorbisSoundSampleSource::CurrentSamplePosition() const
{
	return stb_vorbis_get_sample_offset((stb_vorbis*)decoder);
}

size_t VorbisSoundSampleSource::GetInterleavedSamples(ArrayRange<short> outShorts)
{
	const auto dec = (stb_vorbis*)decoder;
	size_t samplesRead = stb_vorbis_get_samples_short_interleaved(dec, channelCount, outShorts.Begin, (int)outShorts.Count());
	if(channelCount*samplesRead<outShorts.Count()) stb_vorbis_seek_start(dec);
	return samplesRead;
}

size_t VorbisSoundSampleSource::GetInterleavedSamples(ArrayRange<float> outFloats)
{
	const auto dec = (stb_vorbis*)decoder;
	size_t shortsRead = channelCount*stb_vorbis_get_samples_float_interleaved(
		dec, channelCount, outFloats.Begin, (int)outFloats.Count());
	if(shortsRead<outFloats.Count()) stb_vorbis_seek_start(dec);
	return shortsRead/channelCount;
}

size_t VorbisSoundSampleSource::GetUninterleavedSamples(ArrayRange<const ArrayRange<float>> outFloats)
{
	INTRA_ASSERT(outFloats.Count()==channelCount);
	const auto dec = (stb_vorbis*)decoder;
	float* outFloatsPtrs[16];
	for(ushort c=0; c<channelCount; c++)
		outFloatsPtrs[c] = outFloats[c].Begin;
	const size_t samplesRead = stb_vorbis_get_samples_float(dec, channelCount, outFloatsPtrs, (int)outFloats.Count());
	const size_t shortsRead = channelCount*samplesRead;
	if(shortsRead<outFloats.Length()) stb_vorbis_seek_start(dec);
	return shortsRead/channelCount;
}

Array<const void*> VorbisSoundSampleSource::GetRawSamplesData(size_t maxSamplesToRead,
	ValueType* outType, bool* outInterleaved, size_t* outSamplesRead)
{
	maxSamplesToRead;
	if(outType!=null) *outType = ValueType::Void;
	if(outInterleaved!=null) *outInterleaved = false;
	if(outSamplesRead!=null) *outSamplesRead = 0;
	return null;
}

#endif

#endif



#ifndef INTRA_NO_WAVE_LOADER

struct WaveHeader
{
	char RIFF[4];
	uintLE WaveformChunkSize;
	char WAVE[4];

	char fmt[4];
	uintLE FormatChunkSize;

	ushortLE FormatTag, Channels;
	uintLE SampleRate, BytesPerSec;
	ushortLE BlockAlign, BitsPerSample;
	char data[4];
	uintLE DataSize;
};


WaveSoundSampleSource::WaveSoundSampleSource(ArrayRange<const byte> srcFileData):
	data(srcFileData), sample_count(0), current_data_pos(0)
{
	const WaveHeader& header = *reinterpret_cast<const WaveHeader*>(data.Begin);

	if(core::memcmp(header.RIFF, "RIFF", sizeof(header.RIFF))!=0 ||
		core::memcmp(header.WAVE, "WAVE", sizeof(header.WAVE))!=0 ||
		core::memcmp(header.fmt, "fmt ", sizeof(header.fmt))!=0 ||
		core::memcmp(header.data, "data", sizeof(header.data))!=0) return;
	if(data.Length()!=header.DataSize+sizeof(WaveHeader)) return;

	channel_count = ushort(header.Channels);
	sample_rate = header.SampleRate;
	sample_count = header.DataSize/sizeof(short)/channel_count;
}

size_t WaveSoundSampleSource::GetInterleavedSamples(ArrayRange<short> outShorts)
{
	INTRA_ASSERT(!outShorts.Empty());
	const auto shortsToRead = Min(outShorts.Length(), sample_count*channel_count-current_data_pos);
	const short* const streamStart = reinterpret_cast<const short*>(data.Begin+sizeof(WaveHeader));
	core::memcpy(outShorts.Begin, streamStart+current_data_pos, shortsToRead*sizeof(short));
	current_data_pos += shortsToRead;
	if(shortsToRead<outShorts.Length()) current_data_pos=0;
	return shortsToRead/channel_count;
}

size_t WaveSoundSampleSource::GetInterleavedSamples(ArrayRange<float> outFloats)
{
	INTRA_ASSERT(!outFloats.Empty());
	Array<short> outShorts;
	outShorts.SetCountUninitialized(outFloats.Length());
	auto result = GetInterleavedSamples(outShorts);
	for(size_t i=0; i<outFloats.Length(); i++)
		outFloats[i] = (outShorts[i]+0.5f)/32767.5f;
	return result;
}

size_t WaveSoundSampleSource::GetUninterleavedSamples(ArrayRange<const ArrayRange<float>> outFloats)
{
	INTRA_ASSERT(outFloats.Length()==channel_count);
	const size_t outSamplesCount = outFloats.First().Length();
	for(size_t i=1; i<channel_count; i++)
	{
		INTRA_ASSERT(outFloats[i].Length()==outSamplesCount);
	}

	Array<float> outShorts;
	outShorts.SetCountUninitialized(outSamplesCount*channel_count);
	auto result = GetInterleavedSamples(outShorts);
	for(size_t i=0, j=0; i<outShorts.Count(); i++)
	{
		for(ushort c=0; c<channel_count; c++)
			outFloats[c][i] = (outShorts[j++]+0.5f)/32767.5f;
	}
	return result;
}

Array<const void*> WaveSoundSampleSource::GetRawSamplesData(size_t maxSamplesToRead,
	ValueType* oType, bool* oInterleaved, size_t* oSamplesRead)
{
	const auto shortsToRead = Min(maxSamplesToRead, sample_count*channel_count-current_data_pos);
	if(oSamplesRead!=null) *oSamplesRead = shortsToRead/channel_count;
	if(oInterleaved!=null) *oInterleaved = true;
	if(oType!=null) *oType = ValueType::Short;
	Array<const void*> resultPtrs;
	const short* const streamStart = reinterpret_cast<const short*>(data.Begin+sizeof(WaveHeader));
	resultPtrs.AddLast(streamStart+current_data_pos);
	current_data_pos += shortsToRead;
	if(shortsToRead<maxSamplesToRead) current_data_pos = 0;
	return resultPtrs;
}

#endif

}
