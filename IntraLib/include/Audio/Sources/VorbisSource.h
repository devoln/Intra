#pragma once

#include "Platform/PlatformInfo.h"
#include "Range/ArrayRange.h"
#include "Containers/Array.h"
#include "Audio/AudioSource.h"

namespace Intra { namespace Audio { namespace Sources {


//#define INTRA_LIBRARY_VORBIS_DECODER INTRA_LIBRARY_VORBIS_DECODER_libvorbis

#if(INTRA_LIBRARY_VORBIS_DECODER!=INTRA_LIBRARY_VORBIS_DECODER_None)

class VorbisSource: public ASoundSource
{
	struct Decoder;
	typedef Decoder* DecoderHandle;
	ArrayRange<const byte> data;
	DecoderHandle decoder;
public:
	VorbisSource(ArrayRange<const byte> srcFileData);
	~VorbisSource();

	size_t SampleCount() const override;
	size_t CurrentSamplePosition() const override;

	size_t GetInterleavedSamples(ArrayRange<short> outShorts) override;
	size_t GetInterleavedSamples(ArrayRange<float> outFloats) override;
	size_t GetUninterleavedSamples(ArrayRange<const ArrayRange<float>> outFloats) override;
	Array<const void*> GetRawSamplesData(size_t maxSamplesToRead,
		ValueType* outType, bool* outInterleaved, size_t* outSamplesRead) override;
};

#endif

}}}
