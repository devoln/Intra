#include "Audio/Sources/VorbisSource.h"
#include "Platform/Endianess.h"
#include "IO/Stream.h"

#if(INTRA_LIBRARY_VORBIS_DECODER==INTRA_LIBRARY_VORBIS_DECODER_libvorbis)

#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>
#include <stdio.h>

#elif(INTRA_LIBRARY_VORBIS_DECODER==INTRA_LIBRARY_VORBIS_DECODER_STB)

#include "stb_vorbis.h"

#endif

namespace Intra { namespace Audio { namespace Sources {

using namespace Math;
using namespace IO;


#if(INTRA_LIBRARY_VORBIS_DECODER==INTRA_LIBRARY_VORBIS_DECODER_libvorbis)

struct VorbisSource::Decoder
{
	OggVorbis_File file;
	MemoryInputStream stream;
	int current_section;
	size_t current_position;
};

static size_t stream_read_func(void* ptr, size_t size, size_t nmemb, void* datasource)
{
	auto stream = reinterpret_cast<IInputStream*>(datasource);
	stream->ReadData(ptr, size*nmemb);
	return stream->EndOfStream()? 0: size*nmemb;
}

static int stream_seek_func(void* datasource, ogg_int64_t offset, int whence)
{
	auto stream = reinterpret_cast<IInputStream>(datasource);
	if(whence==SEEK_SET) stream->SetPos(offset);
	if(whence==SEEK_CUR) stream->SetPos(ulong64(long64(stream->GetPos())+offset));
	if(whence==SEEK_END) stream->SetPos(ulong64(long64(stream->GetSize())+offset));
	return 0;
}

static long stream_tell_func(void* datasource) {return reinterpret_cast<IInputStream*>(datasource)->GetPos();}

VorbisSource::VorbisSource(ArrayRange<const byte> srcFileData): data(srcFileData)
{
	decoder = new Decoder;
	decoder->stream = srcFileData;
	ov_callbacks c = {stream_read_func, stream_seek_func, null, stream_tell_func};
	ov_open_callbacks(&decoder->stream, &decoder->file, null, 0, c);
	decoder->current_section = 0;

	auto info = ov_info(&decoder->file, -1);
	channelCount = ushort(info->channels);
	sampleRate = uint(info->rate);
}

VorbisSource::~VorbisSource()
{
	ov_clear(&decoder->file);
	delete decoder;
}

size_t VorbisSource::SampleCount() const {return size_t(ov_pcm_total(&decoder->file, -1));}
size_t VorbisSource::CurrentSamplePosition() const {return decoder->current_position;}

size_t VorbisSource::GetInterleavedSamples(ArrayRange<short> outShorts)
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

size_t VorbisSource::GetInterleavedSamples(ArrayRange<float> outFloats)
{
	size_t totalSamplesRead = 0;
	while(!outFloats.Empty())
	{
		float** pcm;
		auto bytesRead = ov_read_float(&decoder->file, &pcm, int(outFloats.Length()), &decoder->current_section);
		if(bytesRead<=0) return 0;
		size_t samplesRead = size_t(bytesRead)/sizeof(float);
		decoder->current_position += samplesRead;
		totalSamplesRead += samplesRead;
		ArrayRange<const float> inputChannels[8];
		for(size_t i=0; i<channelCount; i++)
			inputChannels[i] = ArrayRange<const float>(pcm[i], samplesRead);
		Algo::Interleave(outFloats.Take(samplesRead), ArrayRange<const ArrayRange<const float>>(inputChannels, channelCount));
		outFloats.PopFirstExactly(samplesRead);
	}
	return totalSamplesRead;
}

size_t VorbisSource::GetUninterleavedSamples(ArrayRange<const ArrayRange<float>> outFloats)
{
	if(outFloats.Empty()) return 0;
	INTRA_DEBUG_ASSERT(outFloats.Length()<=channelCount);
	size_t totalSamplesRead = 0;
	ArrayRange<float> outFloats1[8];
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
			ArrayRange<const float>(pcm[i], samplesRead).CopyToAdvance(outFloats1[i]);
		}
	}
	return totalSamplesRead;
}

Array<const void*> VorbisSource::GetRawSamplesData(size_t maxSamplesToRead,
	ValueType* outType, bool* outInterleaved, size_t* outSamplesRead)
{
	if(outType!=null) *outType = ValueType::Float;
	if(outInterleaved) *outInterleaved = true;
	if(outSamplesRead) *outSamplesRead = 0;
	return null;
}

#elif(INTRA_LIBRARY_VORBIS_DECODER==INTRA_LIBRARY_VORBIS_DECODER_STB)

VorbisSource::VorbisSource(ArrayRange<const byte> srcFileData): data(srcFileData)
{
	decoder = reinterpret_cast<DecoderHandle>(stb_vorbis_open_memory(
		reinterpret_cast<byte*>(srcFileData.Begin), uint(srcFileData.Count()), null, null));
	stb_vorbis_info info = stb_vorbis_get_info(reinterpret_cast<stb_vorbis*>(decoder));
	channelCount = ushort(info.channels);
	sampleRate = info.sample_rate;
}

VorbisSource::~VorbisSource()
{
	stb_vorbis_close(reinterpret_cast<stb_vorbis*>(decoder));
}

size_t VorbisSource::SampleCount() const
{
	return stb_vorbis_stream_length_in_samples(reinterpret_cast<stb_vorbis*>(decoder));
}

size_t VorbisSource::CurrentSamplePosition() const
{
	return stb_vorbis_get_sample_offset(reinterpret_cast<stb_vorbis*>(decoder));
}

size_t VorbisSource::GetInterleavedSamples(ArrayRange<short> outShorts)
{
	const auto dec = reinterpret_cast<stb_vorbis*>(decoder);
	size_t samplesRead = stb_vorbis_get_samples_short_interleaved(dec, channelCount, outShorts.Begin, int(outShorts.Count()));
	if(channelCount*samplesRead<outShorts.Count()) stb_vorbis_seek_start(dec);
	return samplesRead;
}

size_t VorbisSource::GetInterleavedSamples(ArrayRange<float> outFloats)
{
	const auto dec = (stb_vorbis*)decoder;
	size_t shortsRead = channelCount*stb_vorbis_get_samples_float_interleaved(
		dec, channelCount, outFloats.Begin, (int)outFloats.Count());
	if(shortsRead<outFloats.Count()) stb_vorbis_seek_start(dec);
	return shortsRead/channelCount;
}

size_t VorbisSource::GetUninterleavedSamples(ArrayRange<const ArrayRange<float>> outFloats)
{
	INTRA_DEBUG_ASSERT(outFloats.Count()==channelCount);
	const auto dec = (stb_vorbis*)decoder;
	float* outFloatsPtrs[16];
	for(ushort c=0; c<channelCount; c++)
		outFloatsPtrs[c] = outFloats[c].Begin;
	const size_t samplesRead = stb_vorbis_get_samples_float(dec, channelCount, outFloatsPtrs, (int)outFloats.Count());
	const size_t shortsRead = channelCount*samplesRead;
	if(shortsRead<outFloats.Length()) stb_vorbis_seek_start(dec);
	return shortsRead/channelCount;
}

Array<const void*> VorbisSource::GetRawSamplesData(size_t maxSamplesToRead,
	ValueType* outType, bool* outInterleaved, size_t* outSamplesRead)
{
	maxSamplesToRead;
	if(outType!=null) *outType = ValueType::Void;
	if(outInterleaved!=null) *outInterleaved = false;
	if(outSamplesRead!=null) *outSamplesRead = 0;
	return null;
}

#else

INTRA_DISABLE_LNK4221

#endif

}}}
