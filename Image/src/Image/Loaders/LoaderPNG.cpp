#include "Image/Loaders/LoaderPNG.h"
#include "Image/Loaders/LoaderPlatform.h"
#include "Math/Vector.h"
#include "Algo/Comparison/Equals.h"
#include "Image/AnyImage.h"
#include "IO/Stream.h"

namespace Intra { namespace Image {

bool LoaderPNG::IsValidHeader(const void* header, size_t headerSize) const
{
	const byte* headerBytes = reinterpret_cast<const byte*>(header);
	static const byte pngSignature[] = {137, 'P', 'N', 'G', 13, 10, 26, 10};
	return headerSize>=8 &&
		Algo::Equals(ArrayRange<const byte>(headerBytes, sizeof(pngSignature)), pngSignature);
}

ImageInfo LoaderPNG::GetInfo(InputStream stream) const
{
	byte headerSignature[8];
	stream.ReadRawTo<byte>(headerSignature);
	if(!IsValidHeader(headerSignature, 8)) return ImageInfo();
	stream.PopFirstN(2*sizeof(intBE));
	struct IHeaderPart
	{
		Math::Vector2<uintBE> size;
		byte bitsPerComponent, colorType;
	};
	IHeaderPart ihdrPart = stream.ReadRaw<IHeaderPart>();

	ImageFormat fmt = null;
	if(ihdrPart.bitsPerComponent==8)
	{
		if(ihdrPart.colorType==0) fmt = ImageFormat::Luminance8;
		else if(ihdrPart.colorType==4) fmt = ImageFormat::LuminanceAlpha8;
		else if(ihdrPart.colorType==2) fmt = ImageFormat::RGB8;
		else if(ihdrPart.colorType==6) fmt = ImageFormat::RGBA8;
	}
	else if(ihdrPart.bitsPerComponent==16)
	{
		if(ihdrPart.colorType==0) fmt = ImageFormat::Luminance16;
		//else if(ihdrPart.colorType==4) fmt = ImageFormat::LuminanceAlpha16;
		else if(ihdrPart.colorType==2) fmt = ImageFormat::RGB16;
		else if(ihdrPart.colorType==6) fmt = ImageFormat::RGBA16;
	}
	return {
		Math::USVec3(ihdrPart.size.x, ihdrPart.size.y, 1),
		fmt, ImageType_2D, 0
	};
}

AnyImage LoaderPNG::Load(InputStream stream) const
{
#ifdef INTRA_USE_LIBPNG
	//TODO: сделать загрузку через libjpeg
#elif(INTRA_LIBRARY_IMAGE_LOADING!=INTRA_LIBRARY_IMAGE_LOADING_None)
	return LoadWithPlatform(Meta::Move(stream));
#else
	(void)stream;
	return null;
#endif
}

const LoaderPNG LoaderPNG::Instance;

}}
