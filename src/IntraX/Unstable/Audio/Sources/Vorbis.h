#pragma once

#include "Intra/Range/Span.h"
#include "IntraX/Container/Sequential/Array.h"
#include "IntraX/Unstable/Audio/AudioSource.h"

INTRA_BEGIN
namespace Sources {
/// Используемый декодер ogg vorbis
#define INTRA_LIBRARY_VORBIS_DECODER_None 0 
#define INTRA_LIBRARY_VORBIS_DECODER_STB 1
#define INTRA_LIBRARY_VORBIS_DECODER_libvorbis 2

#ifndef INTRA_LIBRARY_VORBIS_DECODER

#ifdef _WIN32
#define INTRA_LIBRARY_VORBIS_DECODER INTRA_LIBRARY_VORBIS_DECODER_None
#else
#define INTRA_LIBRARY_VORBIS_DECODER INTRA_LIBRARY_VORBIS_DECODER_None
#endif

#endif

#if(INTRA_LIBRARY_VORBIS_DECODER!=INTRA_LIBRARY_VORBIS_DECODER_None)

class Vorbis: public AAudioSource
{
	struct Decoder;
	typedef Decoder* DecoderHandle;
	CSpan<byte> data;
	DecoderHandle decoder;
public:
	Vorbis(CSpan<byte> srcFileData);
	~Vorbis();

	Vorbis& operator=(const Vorbis&) = delete;

	size_t SampleCount() const override;
	size_t CurrentSamplePosition() const override;

	size_t GetInterleavedSamples(Span<short> outShorts) override;
	size_t GetInterleavedSamples(Span<float> outFloats) override;
	size_t GetUninterleavedSamples(CSpan<Span<float>> outFloats) override;
	FixedArray<const void*> GetRawSamplesData(size_t maxSamplesToRead,
		ValueType* outType, bool* outInterleaved, size_t* outSamplesRead) override;
};

#endif
}
INTRA_END
