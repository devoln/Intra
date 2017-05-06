#include "Image/Loaders/LoaderJPEG.h"
#include "Image/Loaders/LoaderPlatform.h"
#include "Range/Polymorphic/InputRange.h"
#include "Image/AnyImage.h"
#include "Platform/Endianess.h"

namespace Intra { namespace Image {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

bool LoaderJPEG::IsValidHeader(const void* header, size_t headerSize) const
{
	const byte* headerBytes = reinterpret_cast<const byte*>(header);
	return headerSize>=2 && headerBytes[0]==0xFF && headerBytes[1]==0xD8;
}

ImageInfo LoaderJPEG::GetInfo(InputStream stream) const
{
	ImageInfo result = {{0, 0, 1}, null, ImageType_2D, 0};
	byte SOI[2];
	stream.ReadRawTo(SOI, 2);
	if(!IsValidHeader(SOI, 2)) return result;
	while(!stream.Empty())
	{
		stream.PopFirst();
		byte chunkName = stream.ReadRaw<byte>();
		ushort chunkSize = ushort(stream.ReadRaw<ushortBE>()-2u);
		if(chunkName==0xC0 || chunkName==0xC2) // baseline/progressive (huffman)
		{
			stream.PopFirst(); // precision
			result.Size.y = stream.ReadRaw<ushortBE>();
			result.Size.x = stream.ReadRaw<ushortBE>();
			byte bpp = stream.ReadRaw<byte>();
			if(bpp==3) result.Format = ImageFormat::RGB8;
			else if(bpp==1) result.Format = ImageFormat::Luminance8;
			break;
		}
		stream.PopFirstN(chunkSize);
	}
	return result;
}


AnyImage LoaderJPEG::Load(InputStream stream) const
{
#ifdef INTRA_USE_LIBJPEG
	//TODO: сделать загрузку через libjpeg
#elif(INTRA_LIBRARY_IMAGE_LOADING!=INTRA_LIBRARY_IMAGE_LOADING_None)
	return LoadWithPlatform(Cpp::Move(stream));
#else
	(void)stream;
	return null;
#endif
}

const LoaderJPEG LoaderJPEG::Instance;

INTRA_WARNING_POP

}}
