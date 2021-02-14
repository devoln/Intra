#include "IntraX/Unstable/Image/Loaders/LoaderPNG.h"
#include "IntraX/Unstable/Image/Loaders/LoaderPlatform.h"
#include "IntraX/Math/Vector2.h"
#include "IntraX/Math/Vector3.h"
#include "Intra/Range/Comparison.h"
#include "IntraX/Unstable/Image/AnyImage.h"
#include "Intra/Range/Polymorphic/InputRange.h"
#include "IntraX/Utils/Endianess.h"

namespace Intra { INTRA_BEGIN
bool LoaderPNG::IsValidHeader(const void* header, size_t headerSize) const
{
	const byte* headerBytes = reinterpret_cast<const byte*>(header);
	static const byte pngSignature[] = {137, 'P', 'N', 'G', 13, 10, 26, 10};
	return headerSize>=8 &&
		Equals(SpanOfPtr(headerBytes, sizeof(pngSignature)), SpanOf(pngSignature));
}

ImageInfo LoaderPNG::GetInfo(IInputStream& stream) const
{
	byte headerSignature[8];
	RawReadTo(stream, headerSignature, 8);
	if(!IsValidHeader(headerSignature, 8)) return ImageInfo();
	stream.PopFirstCount(2*sizeof(int32BE));

	const UVec2 ihdrSize = RawRead<Vector2<uint32BE>>(stream);
	const byte ihdrBitsPerComponent = RawRead<byte>(stream);
	const byte ihdrColorType = RawRead<byte>(stream);

	ImageFormat fmt = nullptr;
	if(ihdrBitsPerComponent == 8)
	{
		if(ihdrColorType == 0) fmt = ImageFormat::Luminance8;
		else if(ihdrColorType == 4) fmt = ImageFormat::LuminanceAlpha8;
		else if(ihdrColorType == 2) fmt = ImageFormat::RGB8;
		else if(ihdrColorType == 6) fmt = ImageFormat::RGBA8;
	}
	else if(ihdrBitsPerComponent==16)
	{
		if(ihdrColorType == 0) fmt = ImageFormat::Luminance16;
		//else if(ihdrColorType == 4) fmt = ImageFormat::LuminanceAlpha16;
		else if(ihdrColorType == 2) fmt = ImageFormat::RGB16;
		else if(ihdrColorType == 6) fmt = ImageFormat::RGBA16;
	}
	return {
		U16Vec3(ihdrSize.x, ihdrSize.y, 1),
		fmt, ImageType_2D, 0
	};
}

AnyImage LoaderPNG::Load(IInputStream& stream) const
{
#ifdef INTRA_USE_LIBPNG
	//TODO: сделать загрузку через libjpeg
#elif(INTRA_LIBRARY_IMAGE_LOADING != INTRA_LIBRARY_IMAGE_LOADING_None)
	return LoadWithPlatform(stream);
#else
	(void)stream;
	return nullptr;
#endif
}

INTRA_IGNORE_WARN_GLOBAL_CONSTRUCTION
const LoaderPNG LoaderPNG::Instance;
} INTRA_END
