#pragma once

#include "Cpp/PlatformDetect.h"
#include "Utils/Span.h"
#include "Container/Sequential/Array.h"
#include "Audio/AudioSource.h"

namespace Intra { namespace Audio { namespace Sources {

//! Используемый декодер ogg vorbis
#define INTRA_LIBRARY_VORBIS_DECODER_None 0 
#define INTRA_LIBRARY_VORBIS_DECODER_STB 1
#define INTRA_LIBRARY_VORBIS_DECODER_libvorbis 2

#ifndef INTRA_LIBRARY_VORBIS_DECODER

#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)
#define INTRA_LIBRARY_VORBIS_DECODER INTRA_LIBRARY_VORBIS_DECODER_None
#else
#define INTRA_LIBRARY_VORBIS_DECODER INTRA_LIBRARY_VORBIS_DECODER_None
#endif

#endif

#if(INTRA_LIBRARY_VORBIS_DECODER!=INTRA_LIBRARY_VORBIS_DECODER_None)

class VorbisSource: public ASoundSource
{
	struct Decoder;
	typedef Decoder* DecoderHandle;
	CSpan<byte> data;
	DecoderHandle decoder;
public:
	VorbisSource(CSpan<byte> srcFileData);
	~VorbisSource();

	VorbisSource& operator=(const VorbisSource&) = delete;

	size_t SampleCount() const override;
	size_t CurrentSamplePosition() const override;

	size_t GetInterleavedSamples(Span<short> outShorts) override;
	size_t GetInterleavedSamples(Span<float> outFloats) override;
	size_t GetUninterleavedSamples(CSpan<Span<float>> outFloats) override;
	Array<const void*> GetRawSamplesData(size_t maxSamplesToRead,
		ValueType* outType, bool* outInterleaved, size_t* outSamplesRead) override;
};

#endif

}}}
