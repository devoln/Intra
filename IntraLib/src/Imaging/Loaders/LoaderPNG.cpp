#include "Imaging/Loaders/LoaderPNG.h"
#include "Imaging/Loaders/LoaderPlatform.h"
#include "Math/Vector.h"
#include "Algo/Comparison.h"
#include "Imaging/Image.h"
#include "IO/Stream.h"

namespace Intra { namespace Imaging {

bool LoaderPNG::IsValidHeader(const void* header, size_t headerSize) const
{
	const byte* headerBytes = reinterpret_cast<const byte*>(header);
	static const byte pngSignature[] = {137, 'P', 'N', 'G', 13, 10, 26, 10};
	return headerSize>=8 &&
		Algo::Equals(ArrayRange<const byte>(headerBytes, sizeof(pngSignature)), pngSignature);
}

ImageInfo LoaderPNG::GetInfo(IO::IInputStream& stream) const
{
	byte headerSignature[8];
	stream.ReadData(headerSignature, 8);
	if(!IsValidHeader(headerSignature, 8)) return ImageInfo();
	stream.Skip(2*sizeof(intBE));
	struct IHeaderPart
	{
		Math::Vector2<uintBE> size;
		byte bitsPerComponent, colorType;
	};
	IHeaderPart ihdrPart = stream.Read<IHeaderPart>();

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

Image LoaderPNG::Load(IO::IInputStream& stream, size_t bytes) const
{
#ifdef INTRA_USE_LIBPNG
	//TODO: сделать загрузку через libjpeg
#elif(INTRA_LIBRARY_IMAGE_LOADING!=INTRA_LIBRARY_IMAGE_LOADING_None)
	return LoadWithPlatform(stream, bytes);
#else
	(void)stream;
	(void)bytes;
	return null;
#endif
}

const LoaderPNG LoaderPNG::Instance;

}}
