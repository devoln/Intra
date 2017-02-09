#include "Image/Loaders/LoaderJPEG.h"
#include "Image/Loaders/LoaderPlatform.h"
#include "IO/Stream.h"
#include "Image/AnyImage.h"

namespace Intra { namespace Image {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

bool LoaderJPEG::IsValidHeader(const void* header, size_t headerSize) const
{
	const byte* headerBytes = reinterpret_cast<const byte*>(header);
	return headerSize>=2 && headerBytes[0]==0xFF && headerBytes[1]==0xD8;
}

ImageInfo LoaderJPEG::GetInfo(IO::IInputStream& stream) const
{
	ImageInfo result = {{0, 0, 1}, null, ImageType_2D, 0};
	byte SOI[2];
	stream.ReadData(SOI, sizeof(SOI));
	if(!IsValidHeader(SOI, 2)) return result;
	while(!stream.EndOfStream())
	{
		stream.Skip(1);
		byte chunkName = stream.Read<byte>();
		ushort chunkSize = ushort(stream.Read<ushortBE>()-2u);
		if(chunkName==0xC0 || chunkName==0xC2) // baseline/progressive (huffman)
		{
			stream.Skip(1); // precision
			result.Size.y = stream.Read<ushortBE>();
			result.Size.x = stream.Read<ushortBE>();
			byte bpp = stream.Read<byte>();
			if(bpp==3) result.Format = ImageFormat::RGB8;
			else if(bpp==1) result.Format = ImageFormat::Luminance8;
			break;
		}
		stream.Skip(chunkSize);
	}
	return result;
}


AnyImage LoaderJPEG::Load(IO::IInputStream& stream, size_t bytes) const
{
#ifdef INTRA_USE_LIBJPEG
	//TODO: сделать загрузку через libjpeg
#elif(INTRA_LIBRARY_IMAGE_LOADING!=INTRA_LIBRARY_IMAGE_LOADING_None)
	return LoadWithPlatform(stream, bytes);
#else
	(void)stream;
	(void)bytes;
	return null;
#endif
}

const LoaderJPEG LoaderJPEG::Instance;

INTRA_WARNING_POP

}}
