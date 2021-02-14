﻿#include "IntraX/Unstable/Image/Loaders/LoaderJPEG.h"
#include "IntraX/Unstable/Image/Loaders/LoaderPlatform.h"
#include "Intra/Range/Polymorphic/InputRange.h"
#include "IntraX/Unstable/Image/AnyImage.h"
#include "IntraX/Utils/Endianess.h"

namespace Intra { INTRA_BEGIN
bool LoaderJPEG::IsValidHeader(const void* header, size_t headerSize) const
{
	const byte* headerBytes = reinterpret_cast<const byte*>(header);
	return headerSize >= 2 && headerBytes[0] == 0xFF && headerBytes[1] == 0xD8;
}

ImageInfo LoaderJPEG::GetInfo(IInputStream& stream) const
{
	ImageInfo result = {{0, 0, 1}, nullptr, ImageType_2D, 0};
	byte SOI[2];
	RawReadTo(stream, SpanOfBuffer(SOI));
	if(!IsValidHeader(SOI, 2)) return result;
	while(!stream.Empty())
	{
		stream.PopFirst();
		const byte chunkName = RawRead<byte>(stream);
		uint16 chunkSize = uint16(uint16(RawRead<ushortBE>(stream)) - 2u);
		if(chunkName == 0xC0 || chunkName == 0xC2) // baseline/progressive (huffman)
		{
			stream.PopFirst(); // precision
			result.Size.y = RawRead<ushortBE>(stream);
			result.Size.x = RawRead<ushortBE>(stream);
			const byte bpp = RawRead<byte>(stream);
			if(bpp == 3) result.Format = ImageFormat::RGB8;
			else if(bpp == 1) result.Format = ImageFormat::Luminance8;
			break;
		}
		stream.PopFirstCount(chunkSize);
	}
	return result;
}


AnyImage LoaderJPEG::Load(IInputStream& stream) const
{
#ifdef INTRA_USE_LIBJPEG
	//TODO: сделать загрузку через libjpeg
#error Not implemented!
#elif(INTRA_LIBRARY_IMAGE_LOADING != INTRA_LIBRARY_IMAGE_LOADING_None)
	return LoadWithPlatform(stream);
#else
	(void)stream;
	return nullptr;
#endif
}

INTRA_IGNORE_WARN_GLOBAL_CONSTRUCTION
const LoaderJPEG LoaderJPEG::Instance;

} INTRA_END