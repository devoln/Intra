#ifndef INTRA_NO_BMP_LOADER

#include "Platform/Endianess.h"
#include "Container/Sequential/Array.h"
#include "Image/Loaders/LoaderBMP.h"
#include "Image/FormatConversion.h"
#include "Image/AnyImage.h"
#include "Math/Vector.h"

namespace Intra {

using namespace Math;

namespace Image {


ImageInfo LoaderBMP::GetInfo(InputStream stream) const
{
	byte headerBegin[14];
	stream.ReadRawTo(headerBegin, 14);
	if(!IsValidHeader(headerBegin, sizeof(headerBegin))) return ImageInfo();
	struct HeaderPart
	{
		uint infoSize;
		UVec2 size;
		ushort colorPlanes;
		ushort bitsPerPixel;
	};
	HeaderPart hdrPart = stream.ReadRaw<HeaderPart>();

	ImageFormat fmt;
	if(hdrPart.bitsPerPixel==32) fmt = ImageFormat::RGBA8;
	else if(hdrPart.bitsPerPixel==24) fmt = ImageFormat::RGB8;
	else if(hdrPart.bitsPerPixel==16) fmt = ImageFormat::RGB5A1;
	else if(hdrPart.bitsPerPixel==8 ||
		hdrPart.bitsPerPixel==4 ||
		hdrPart.bitsPerPixel==1)
			fmt = ImageFormat::RGBA8;
	else fmt = null;
	return {USVec3(hdrPart.size, 1), fmt, ImageType_2D, 0};
}

AnyImage LoaderBMP::Load(InputStream stream) const
{
	struct BitmapHeader
	{
		Vector2<intLE> sizes;
		ushortLE planes;
		ushortLE bitCount;
		uintLE Compression;
		uintLE SizeImage;
		intLE PelsPerMeterX;
		intLE PelsPerMeterY;
		uintLE clrUsed;
		uintLE clrImportant;
		Vector4<uintLE> RgbaMasks;
		uintLE CsType;
		uintLE Endpoints[9];
		Vector3<uintLE> GammaRGB;
		byte V5[16];
	} bmpHdr;

	stream.PopFirstN(2); //Предполагается, что идентификатор формата уже проверен

	uint fileSize = stream.ReadRaw<uintLE>();
	(void)fileSize;
	stream.ReadRaw<uint>();
	const uint dataPos = stream.ReadRaw<uintLE>();

	const uint hdrSize = stream.ReadRaw<uintLE>();
	stream.ReadRawTo(Span<char>(reinterpret_cast<char*>(&bmpHdr), hdrSize-sizeof(uintLE)));
	size_t bytesRead = 14+hdrSize;

	//RLE4, RLE8 и встроенный jpeg\png не поддерживаются!
	if(bmpHdr.Compression!=0 && bmpHdr.Compression!=3)
		return null;

	//const uint colorTableSize=(bmpHdr.bitCount>8 || bmpHdr.bitCount==0)? 0: (1 << bmpHdr.bitCount);

	UBVec4 colorTable[256];
	if(bmpHdr.clrUsed!=0)
	{
		stream.ReadRawTo(Range::Take(colorTable, bmpHdr.clrUsed));
		bytesRead += bmpHdr.clrUsed*sizeof(colorTable[0]);
		for(uint i=0; i<bmpHdr.clrUsed; i++)
			colorTable[i] = colorTable[i].swizzle(2, 1, 0, 3);
	}
	else if(bmpHdr.bitCount<=8)
	{
		//Если палитра отсутствует, то сделаем её сами из оттенков серого.
		//Такие случаи вроде бы не были описаны, но paint создаёт такие bmp.
		for(int i=0; i < (1 << bmpHdr.bitCount); i++)
			colorTable[i] = UBVec4(UBVec3(byte(255*i >> bmpHdr.bitCount)), 255);
	}

	AnyImage result;
	result.LineAlignment = 1;
	result.Info.Type = ImageType_2D;
	result.Info.Size = USVec3(bmpHdr.sizes, 1);
	if(bmpHdr.Compression==0)
	{
		if(bmpHdr.bitCount==1 || bmpHdr.bitCount==4 ||
			bmpHdr.bitCount==8 || bmpHdr.bitCount==32)
			result.Info.Format = ImageFormat::RGBA8;
		if(bmpHdr.bitCount==24) result.Info.Format = ImageFormat::RGB8;
		if(bmpHdr.bitCount==16) result.Info.Format = ImageFormat::RGB8;
	}
	else result.Info.Format = ImageFormat::RGBA8;
	result.Data.SetCountUninitialized(result.Info.CalculateMipmapDataSize(0, result.LineAlignment));

	if(bytesRead > dataPos) return null;
	stream.PopFirstN(dataPos-bytesRead);
	bytesRead = dataPos;

	if(bmpHdr.Compression==0)
	{
		result.SwapRB = (bmpHdr.bitCount==24 || bmpHdr.bitCount==32);
		const USVec2 size = {result.Info.Size.x, result.Info.Size.y};

		if(bmpHdr.bitCount==16)
		{
			ReadPixelDataBlock(stream, size,
				ImageFormat::RGB5A1, result.Info.Format, false, true,
				4, result.LineAlignment, result.Data);
			return result;
		}

		if(bmpHdr.bitCount==24)
		{
			ReadPixelDataBlock(stream, size,
				ImageFormat::RGB8, result.Info.Format, false, true,
				4, result.LineAlignment, result.Data);
			return result;
		}

		if(bmpHdr.bitCount==32)
		{
			ReadPixelDataBlock(stream, size,
				ImageFormat::RGBA8, result.Info.Format, false, true,
				4, result.LineAlignment, result.Data);
			return result;
		}

		CSpan<byte> paletteBytes(reinterpret_cast<byte*>(colorTable), sizeof(colorTable));
		ReadPalettedPixelDataBlock(stream, paletteBytes, bmpHdr.bitCount,
			size, result.Info.Format, true, 4, result.LineAlignment, result.Data);
		return result;
	}

	//Битовые поля
	//Предполагается, что маски цветовых компонентов могут находиться в любом порядке
	const uint lineWidth = ((uint(result.Info.Size.x)*bmpHdr.bitCount/8u)+3u)&~3u;
	Array<byte> line;
	line.SetCountUninitialized(lineWidth);
	UVec4 bitCount, bitPositions;
	for(size_t k = 0; k<4; k++)
	{
		bitCount[k] = Count1Bits(bmpHdr.RgbaMasks[k]);
		bitPositions[k] = FindBitPosition(bmpHdr.RgbaMasks[k]);
	}
	UBVec4* pixels = reinterpret_cast<UBVec4*>(result.Data.End());

	for(uint i=0; i<result.Info.Size.y; i++)
	{
		pixels -= result.Info.Size.x;
		uint index = 0;
		stream.ReadRawTo(line.AsRange());

		byte* linePtr = line.Data();

		for(uint j = 0; j<result.Info.Size.x; j++)
		{
			uint Color = 0;
			if(bmpHdr.bitCount==16) Color = *reinterpret_cast<ushortLE*>(linePtr);
			else if(bmpHdr.bitCount==32) Color = *reinterpret_cast<uintLE*>(linePtr);
			else
			{
				// Other formats are not valid
			}
			linePtr += bmpHdr.bitCount/8;
			for(size_t k = 0; k<4; k++)
			{
				uint color = (Color & bmpHdr.RgbaMasks[k]) >> bitPositions[k];
				uint pixel = ConvertColorBits(color, bitCount[k], 8);
				pixels[index][k] = byte(pixel);
			}
			index++;
		}
	}
	return result;
}

bool LoaderBMP::IsValidHeader(const void* header, size_t headerSize) const
{
	const char* headerBytes = reinterpret_cast<const char*>(header);
	return headerSize>=14 &&
		headerBytes[0]=='B' && headerBytes[1]=='M';
}


const LoaderBMP LoaderBMP::Instance;

}}

#endif
