#include "Sound/SoundSource.h"
#include "Platform/Endianess.h"

namespace Intra {

using namespace Math;


#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Linux)

#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>

#ifdef EOF
#undef EOF
#endif

static size_t read_func(void* ptr, size_t size, size_t nmemb, void* datasource)
{
	auto stream=(IInputStream*)datasource;
	stream->ReadData(ptr, size*nmemb);
	return stream->EOF()? 0: size*nmemb;
}

static int seek_func(void* datasource, ogg_int64_t offset, int whence)
{
	auto stream=(IInputStream*)datasource;
	if(whence==SEEK_SET) stream->SetPos(offset);
	if(whence==SEEK_CUR) stream->SetPos(stream->GetPos()+offset);
	if(whence==SEEK_END) stream->SetPos(stream->GetSize()+offset);
	return 0;
}

static long tell_func(void* datasource) {return ((IInputStream*)datasource)->GetPos();}

VorbisSoundSampleSource::VorbisSoundSampleSource(ArrayRange<const byte> srcFileData): data(srcFileData)
{

}

uint VorbisSoundSampleSource::SamplesCount() const
{

}

uint VorbisSoundSampleSource::CurrentSamplePosition() const
{

}

void VorbisSoundSampleSource::GetChannelSamples(ArrayRange<short> outSamples, ushort channel)
{

}

void VorbisSoundSampleSource::GetInterleavedSamples(ArrayRange<short> outShorts)
{

}

Sound Sound::LoadOgg(IInputStream* stream)
{
	INTRA_ASSERT(stream->IsSeekable());
	if(!stream->IsSeekable()) return;
	OggVorbis_File vf;
	ov_callbacks c = {read_func, seek_func, null, tell_func};
	ov_open_callbacks(stream, &vf, null, 0, c);
	vorbis_info* vi=ov_info(&vf,-1);
	int current_section=0;
	Sound result=Sound(vi->channels, (uint)ov_pcm_total(&vf,-1), (uint)vi->rate, null);
	short* data = result.Lock();
	bool eof=false;
	uint pos=0;
	while(!eof && result.GetBufferSize()!=pos)
	{
		auto ret = ov_read(&vf, (char*)data+pos, result.GetBufferSize()-pos, false, sizeof(short), true, &current_section);
		if(ret==0) eof=true;
		else if(ret<0) {Console << "Error loading ogg!\n"; eof=true;}
		else pos+=ret;
	};
	ov_clear(&vf);
	result.Unlock();
	return result;
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



#ifndef NO_WAVE_LOADER

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
	data(srcFileData), current_data_pos(0)
{
	WaveHeader& header = *(WaveHeader*)data.Begin;

	if(core::memcmp(header.RIFF, "RIFF", sizeof(header.RIFF))!=0 ||
		core::memcmp(header.WAVE, "WAVE", sizeof(header.WAVE))!=0 ||
		core::memcmp(header.fmt, "fmt ", sizeof(header.fmt))!=0 ||
		core::memcmp(header.data, "data", sizeof(header.data))!=0) return;
	if(data.Length()!=header.DataSize+sizeof(WaveHeader)) return;

	channelCount = (ushort)header.Channels;
	sampleRate = header.SampleRate;
	sample_count = header.DataSize/sizeof(short)/channelCount;
}

size_t WaveSoundSampleSource::GetInterleavedSamples(ArrayRange<short> outShorts)
{
	INTRA_ASSERT(!outShorts.Empty());
	const auto shortsToRead = Min(outShorts.Length(), sample_count*channelCount-current_data_pos);
	core::memcpy(outShorts.Begin, (short*)(data.Begin+sizeof(WaveHeader))+current_data_pos, shortsToRead*sizeof(short));
	current_data_pos+=shortsToRead;
	if(shortsToRead<outShorts.Length()) current_data_pos=0;
	return shortsToRead/channelCount;
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
	INTRA_ASSERT(outFloats.Length()==channelCount);
	const size_t outSamplesCount = outFloats.First().Length();
	for(size_t i=1; i<channelCount; i++)
	{
		INTRA_ASSERT(outFloats[i].Length()==outSamplesCount);
	}

	Array<float> outShorts;
	outShorts.SetCountUninitialized(outSamplesCount*channelCount);
	auto result = GetInterleavedSamples(outShorts);
	for(size_t i=0, j=0; i<outShorts.Count(); i++)
	{
		for(ushort c=0; c<channelCount; c++)
			outFloats[c][i] = (outShorts[j++]+0.5f)/32767.5f;
	}
	return result;
}

Array<const void*> WaveSoundSampleSource::GetRawSamplesData(size_t maxSamplesToRead,
	ValueType* outType, bool* outInterleaved, size_t* outSamplesRead)
{
	const auto shortsToRead = Min(maxSamplesToRead, sample_count*channelCount-current_data_pos);
	if(outSamplesRead!=null) *outSamplesRead = shortsToRead/channelCount;
	if(outInterleaved!=null) *outInterleaved = true;
	if(outType!=null) *outType = ValueType::Short;
	Array<const void*> resultPtrs;
	resultPtrs.AddLast((short*)(data.Begin+sizeof(WaveHeader))+current_data_pos);
	current_data_pos+=shortsToRead;
	if(shortsToRead<maxSamplesToRead) current_data_pos=0;
	return resultPtrs;
}

#endif

}
