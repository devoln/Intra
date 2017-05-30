#ifndef INTRA_NO_TGA_LOADER

#include "Image/Loaders/LoaderTGA.h"
#include "Image/AnyImage.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Image/FormatConversion.h"

namespace Intra {

using namespace Math;

namespace Image {

static ImageInfo GetInfoFromHeader(const byte* header)
{
	if(!LoaderTGA::Instance.IsValidHeader(header, 18))
		return ImageInfo();

	USVec2 size = {
		(header[13] << 8)+header[12],
		(header[15] << 8)+header[14]
	};
	static const ImageFormat formatsFromComponents[] = {
		ImageFormat::Luminance8, ImageFormat::RG8,
		ImageFormat::RGB8, ImageFormat::RGBA8
	};
	ImageFormat format = formatsFromComponents[header[16]/8-1];
	return {
		USVec3(size.x, size.y, 1),
		format, ImageType_2D, 0
	};
}

ImageInfo LoaderTGA::GetInfo(InputStream stream) const
{
	byte header[18];
	stream.ReadRawTo(header, 18);
	return GetInfoFromHeader(header);
}

AnyImage LoaderTGA::Load(InputStream stream) const
{
	byte header[18];
	stream.ReadRawTo(header, 18);

	const bool compressedRLE = (header[2]==10);

	AnyImage result;
	auto fileInfo = GetInfoFromHeader(header);
	if(fileInfo==null) return result;

	result.Info = fileInfo;
	result.SwapRB = true;
	result.LineAlignment = compressedRLE? byte(1): byte(4);

	const ushort bytesPerPixel = result.Info.Format.BytesPerPixel();
	const size_t newSize = result.Info.CalculateMipmapDataSize(0, result.LineAlignment);
	result.Data.SetCountUninitialized(newSize);

	if(!compressedRLE)
	{
		ReadPixelDataBlock(stream, {result.Info.Size.x, result.Info.Size.y},
			result.Info.Format, result.Info.Format, false, true, 4, result.LineAlignment, result.Data);
		return result;
	}

	const uint pixelcount = uint(result.Info.Size.x*result.Info.Size.y);
	uint index = 0;
	byte* pos = result.Data.End() - result.Info.Size.x*bytesPerPixel;
	for(uint currentPixel = 0; currentPixel<pixelcount;)
	{
		int chunkheader = stream.ReadRaw<byte>();
		if(chunkheader<128)
		{
			chunkheader++;
			for(; chunkheader--!=0; currentPixel++)
			{
				stream.ReadRawTo(pos+index*3, bytesPerPixel);
				index++;
				if(index==result.Info.Size.x)
				{
					index = 0;
					pos -= result.Info.Size.x*bytesPerPixel;
				}
			}
			continue;
		}
		chunkheader -= 127;
		byte colorBuffer[4];
		stream.ReadRawTo(colorBuffer, bytesPerPixel);
		for(; chunkheader--!=0; currentPixel++)
		{
			C::memcpy(pos+index*3, colorBuffer, bytesPerPixel);
			index++;
			if(index==result.Info.Size.x)
			{
				index = 0;
				pos -= result.Info.Size.x*bytesPerPixel;
			}
		}
	}
	return result;
}

bool LoaderTGA::IsValidHeader(const void* header, size_t headerSize) const
{
	if(headerSize<12) return false;
	const byte* headerBytes = reinterpret_cast<const byte*>(header);
	
	for(int i: {0, 1, 3, 4, 5, 6, 8, 9, 10, 11})
		if(headerBytes[i]!=0) return false;

	//Поддерживаются пока только форматы 2 (несжатый) и 10 (RLE)
	if(headerBytes[2]!=10 && headerBytes[2]!=2) return false;

	return true;
}

const LoaderTGA LoaderTGA::Instance;

}}

#endif
