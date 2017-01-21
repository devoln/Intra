#include "Imaging/FormatConversion.h"

namespace Intra {

using namespace Math;
using namespace IO;

namespace Imaging {

uint ConvertColorBits(uint color, uint fromBitCount, uint toBitCount)
{
	if(toBitCount<fromBitCount)
		color >>= fromBitCount-toBitCount;
	else
	{
		color <<= toBitCount-fromBitCount;
		if(color>0) color |= Math::BitCountToMask(toBitCount-fromBitCount);
	}
	return color;
}

template<typename T> static void SwapRedBlueTyped(size_t lineUnused,
	size_t componentCount, Math::USVec2 sizes, T* data)
{
	for(int y = 0; y<sizes.y; y++)
	{
		for(int x = 0; x<sizes.x; x++)
		{
			Meta::Swap(data[0], data[2]);
			data += componentCount;
		}
		data += lineUnused;
	}
}

void SwapRedBlueChannels(ImageFormat format, ushort lineAlignment, USVec2 sizes, ArrayRange<byte> data)
{
	const auto components = format.ComponentCount();
	const auto bytesPerComp = format.GetComponentType().Size();
	const ushort lineUnusedBytes = ushort(lineAlignment-sizes.x*format.BytesPerPixel()%lineAlignment);
	INTRA_ASSERT(data.Length() >= size_t(sizes.y*(sizes.x*components+lineUnusedBytes)));
	if(bytesPerComp==1) SwapRedBlueTyped<byte>(lineUnusedBytes, components, sizes, data.Data());
	else if(bytesPerComp==2) SwapRedBlueTyped<ushort>(lineUnusedBytes/2u, components, sizes, reinterpret_cast<ushort*>(data.Data()));
	else if(bytesPerComp==4) SwapRedBlueTyped<uint>(lineUnusedBytes/4u, components, sizes, reinterpret_cast<uint*>(data.Data()));
	else INTRA_INTERNAL_ERROR("swap_red_blue пока не поддерживает упакованные форматы!");
}



void ReadPixelDataBlock(IInputStream& stream, USVec2 sizes,
	ImageFormat srcFormat, ImageFormat dstFormat,
	bool swapRB, bool flipVert, ushort srcAlignment, ushort dstAlignment, ArrayRange<byte> dstBuf)
{
	INTRA_ASSERT(stream.IsSeekable());
	INTRA_ASSERT(srcFormat.ComponentCount()>=3 || !swapRB);
	const size_t usefulSrcLineBytes = size_t(sizes.x*srcFormat.BytesPerPixel());
	const size_t usefulDstLineBytes = size_t(sizes.x*dstFormat.BytesPerPixel());
	const size_t srcLineBytes = (usefulSrcLineBytes+srcAlignment-1u)&~size_t(srcAlignment-1u);
	const size_t dstLineBytes = (usefulDstLineBytes+dstAlignment-1u)&~size_t(dstAlignment-1u);
	const size_t srcDataSize = sizes.y*srcLineBytes;
	const size_t dstDataSize = sizes.y*dstLineBytes;
	INTRA_ASSERT(dstBuf.Length() >= dstDataSize);

	if(srcFormat==dstFormat && srcLineBytes==dstLineBytes && !swapRB && !flipVert)
	{
		stream.ReadData(dstBuf.Data(), srcDataSize);
		return;
	}

	byte* pos = dstBuf.Begin;
	if(flipVert) pos += dstDataSize-dstLineBytes;

	if(srcFormat==dstFormat)
		for(int y = 0; y<sizes.y; y++)
		{
			stream.ReadData(pos, usefulSrcLineBytes);
			stream.Skip(srcLineBytes-usefulSrcLineBytes);
			//Algo::FillZeros(ArrayRange<byte>(pos+usefulSrcLineBytes, dstLineBytes-usefulSrcLineBytes));
			if(flipVert) pos -= dstLineBytes;
			else pos += dstLineBytes;
		}

	if(srcFormat==ImageFormat::A1_BGR5 && dstFormat==ImageFormat::RGB8)
		for(int y = 0; y<sizes.y; y++)
		{
			auto pixels = reinterpret_cast<UBVec3*>(pos);
			for(uint x = 0; x<sizes.x; x++)
			{
				ushort color = stream.Read<ushortLE>();
				*pixels++ = {(color >> 11) << 3, ((color >> 6) & 0x1f) << 3, ((color >> 1) & 0x1f) << 3};
			}
			if(flipVert) pos -= dstLineBytes;
			else pos += dstLineBytes;
			stream.Skip(srcLineBytes-usefulSrcLineBytes);
		}

	if(srcFormat==ImageFormat::RGB5A1 && dstFormat==ImageFormat::RGB8)
		for(int y = 0; y<sizes.y; y++)
		{
			auto pixels = reinterpret_cast<UBVec3*>(pos);
			for(uint x = 0; x<sizes.x; x++)
			{
				ushort color = stream.Read<ushortLE>();
				*pixels++ = {((color >> 10) & 0x1f) << 3, ((color >> 5) & 0x1f) << 3, (color & 0x1f) << 3};
			}
			if(flipVert) pos -= dstLineBytes;
			else pos += dstLineBytes;
			stream.Skip(srcLineBytes-usefulSrcLineBytes);
		}

	if(srcFormat==ImageFormat::RGBA8 && dstFormat==ImageFormat::RGB8)
		for(int y = 0; y<sizes.y; y++)
		{
			auto pixels = reinterpret_cast<UBVec3*>(pos);
			if(swapRB) for(uint x = 0; x<sizes.x; x++)
				*pixels++ = stream.Read<UBVec4>().swizzle(2, 1, 0);
			else for(uint x = 0; x<sizes.x; x++)
				*pixels++ = stream.Read<UBVec4>().xyz;
			if(flipVert) pos -= dstLineBytes;
			else pos += dstLineBytes;
			stream.Skip(srcLineBytes-usefulSrcLineBytes);
		}

	if(swapRB) SwapRedBlueChannels(dstFormat, dstAlignment, sizes, dstBuf);
}

void ReadPalettedPixelDataBlock(IInputStream& stream, ArrayRange<const byte> palette,
	ushort bpp, USVec2 sizes, ImageFormat format, bool flipVert,
	ushort srcAlignment, ushort dstAlignment, ArrayRange<byte> dstBuf)
{
	INTRA_ASSERT(stream.IsSeekable());
	INTRA_ASSERT(palette.Length() >= 1u << bpp);
	const ushort bytesPerPixel = format.BytesPerPixel();
	const uint usefulSrcLineBytes = uint(sizes.x*bpp)/8u;
	const uint usefulDstLineBytes = uint(sizes.x*bytesPerPixel);
	const uint srcLineBytes = (usefulSrcLineBytes+srcAlignment-1u)&~(srcAlignment-1u);
	const uint dstLineBytes = (usefulDstLineBytes+dstAlignment-1u)&~(dstAlignment-1u);
	const size_t dstDataSize = sizes.x*dstLineBytes;

	byte* pos = dstBuf.Begin;
	if(flipVert) pos += dstDataSize-dstLineBytes;

	for(int y = 0; y<sizes.y; y++)
	{
		byte* linePos = pos;
		if(bpp==1) for(uint j = 0; j<sizes.x; j += 8)
		{
			byte colorIndices = stream.Read<byte>();
			for(int k = 0; k<8; k++)
			{
				size_t bytesLeftToCopy = bytesPerPixel;
				const byte* paletteSrc = palette.Data() + ((colorIndices & 0x80) >> 7)*bytesPerPixel;
				while(bytesLeftToCopy--> 0) *linePos++ = *paletteSrc++;
				colorIndices = byte(colorIndices << 1);
			}
		}
		else if(bpp==4) for(uint j = 0; j<sizes.x; j += 2)
		{
			byte colorIndices = stream.Read<byte>();
			const byte* paletteSrc = palette.Data() + (colorIndices >> 4)*bytesPerPixel;
			size_t bytesLeftToCopy = bytesPerPixel;
			while(bytesLeftToCopy--> 0) *linePos++ = *paletteSrc++;
			paletteSrc = palette.Data() + (colorIndices & 15)*bytesPerPixel;
			while(bytesLeftToCopy--> 0) *linePos++ = *paletteSrc++;
		}
		else if(bpp==8) for(uint j = 0; j<sizes.x; j++)
		{
			byte colorIndex = stream.Read<byte>();
			size_t bytesLeftToCopy = bytesPerPixel;
			const byte* paletteSrc = palette.Data() + colorIndex*bytesPerPixel;
			while(bytesLeftToCopy--> 0) *linePos++ = *paletteSrc++;
		}
		if(!flipVert) pos += dstLineBytes;
		else pos -= dstLineBytes;
		stream.Skip(srcLineBytes-usefulSrcLineBytes);
	}
}


}}
