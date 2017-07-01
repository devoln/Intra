#include "Image/FormatConversion.h"
#include "Cpp/Fundamental.h"
#include "Cpp/Endianess.h"
#include "Range/Stream/RawRead.h"

namespace Intra {

using namespace Math;

namespace Image {

uint ConvertColorBits(uint color, uint fromBitCount, uint toBitCount)
{
	if(toBitCount < fromBitCount)
		color >>= fromBitCount-  toBitCount;
	else
	{
		color <<= toBitCount - fromBitCount;
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
			Cpp::Swap(data[0], data[2]);
			data += componentCount;
		}
		data += lineUnused;
	}
}

void SwapRedBlueChannels(ImageFormat format, ushort lineAlignment, USVec2 sizes, Span<byte> data)
{
	const auto components = format.ComponentCount();
	const auto bytesPerComp = format.GetComponentType().Size();
	const ushort lineUnusedBytes = ushort(lineAlignment-sizes.x*format.BytesPerPixel()%lineAlignment);
	INTRA_DEBUG_ASSERT(data.Length() >= size_t(sizes.y*(sizes.x*components+lineUnusedBytes)));
	if(bytesPerComp==1) SwapRedBlueTyped<byte>(lineUnusedBytes, components, sizes, data.Data());
	else if(bytesPerComp==2) SwapRedBlueTyped<ushort>(lineUnusedBytes/2u, components, sizes, reinterpret_cast<ushort*>(data.Data()));
	else if(bytesPerComp==4) SwapRedBlueTyped<uint>(lineUnusedBytes/4u, components, sizes, reinterpret_cast<uint*>(data.Data()));
	else INTRA_FATAL_ERROR("swap_red_blue пока не поддерживает упакованные форматы!");
}



void ReadPixelDataBlock(IInputStream& stream, USVec2 sizes,
	ImageFormat srcFormat, ImageFormat dstFormat,
	bool swapRB, bool flipVert, ushort srcAlignment, ushort dstAlignment, Span<byte> dstBuf)
{
	INTRA_DEBUG_ASSERT(srcFormat.ComponentCount()>=3 || !swapRB);
	const size_t usefulSrcLineBytes = size_t(sizes.x*srcFormat.BytesPerPixel());
	const size_t usefulDstLineBytes = size_t(sizes.x*dstFormat.BytesPerPixel());
	const size_t srcLineBytes = (usefulSrcLineBytes+srcAlignment-1u) & ~size_t(srcAlignment-1u);
	const size_t dstLineBytes = (usefulDstLineBytes+dstAlignment-1u) & ~size_t(dstAlignment-1u);
	const size_t srcDataSize = sizes.y*srcLineBytes;
	const size_t dstDataSize = sizes.y*dstLineBytes;
	INTRA_DEBUG_ASSERT(dstBuf.Length() >= dstDataSize);
	(void)dstDataSize;

	if(srcFormat==dstFormat && srcLineBytes==dstLineBytes && !swapRB && !flipVert)
	{
		RawReadTo(stream, dstBuf.Take(srcDataSize));
		return;
	}

	auto dstPos = dstBuf;

	if(srcFormat==dstFormat)
		for(int y = 0; y<sizes.y; y++)
		{
			if(flipVert)
			{
				RawReadTo(stream, dstPos.Tail(usefulSrcLineBytes));
				dstPos.PopLastExactly(dstLineBytes);
			}
			else
			{
				RawReadTo(stream, dstPos.Take(usefulSrcLineBytes));
				dstPos.PopFirstExactly(dstLineBytes);
				//FillZeros(dstPos.Drop(usefulSrcLineBytes));
			}
			stream.PopFirstN(srcLineBytes-usefulSrcLineBytes);
		}

	if(srcFormat==ImageFormat::A1_BGR5 && dstFormat==ImageFormat::RGB8)
		for(int y=0; y<sizes.y; y++)
		{
			auto pixels = !flipVert?
				reinterpret_cast<UBVec3*>(dstPos.Begin):
				reinterpret_cast<UBVec3*>(dstPos.End)-sizes.x;
			for(uint x=0; x<sizes.x; x++)
			{
				ushort color = Range::RawRead<ushortLE>(stream);
				*pixels++ = {(color >> 11) << 3, ((color >> 6) & 0x1f) << 3, ((color >> 1) & 0x1f) << 3};
			}
			if(flipVert) dstPos.PopLastExactly(dstLineBytes);
			else dstPos.PopFirstExactly(dstLineBytes);
			stream.PopFirstN(srcLineBytes-usefulSrcLineBytes);
		}

	if(srcFormat==ImageFormat::RGB5A1 && dstFormat==ImageFormat::RGB8)
		for(int y=0; y<sizes.y; y++)
		{
			auto pixels = !flipVert?
				reinterpret_cast<UBVec3*>(dstPos.Begin):
				reinterpret_cast<UBVec3*>(dstPos.End)-sizes.x;
			for(uint x=0; x<sizes.x; x++)
			{
				ushort color = Range::RawRead<ushortLE>(stream);
				*pixels++ = {((color >> 10) & 0x1f) << 3, ((color >> 5) & 0x1f) << 3, (color & 0x1f) << 3};
			}
			if(flipVert) dstPos.PopLastExactly(dstLineBytes);
			else dstPos.PopFirstExactly(dstLineBytes);
			stream.PopFirstN(srcLineBytes-usefulSrcLineBytes);
		}

	if(srcFormat==ImageFormat::RGBA8 && dstFormat==ImageFormat::RGB8)
		for(int y=0; y<sizes.y; y++)
		{
			auto pixels = !flipVert?
				reinterpret_cast<UBVec3*>(dstPos.Begin):
				reinterpret_cast<UBVec3*>(dstPos.End)-sizes.x;
			if(swapRB) for(uint x = 0; x<sizes.x; x++)
				*pixels++ = Range::RawRead<UBVec4>(stream).swizzle<2, 1, 0>();
			else for(uint x = 0; x<sizes.x; x++)
				*pixels++ = Range::RawRead<UBVec4>(stream).xyz;
			if(flipVert) dstPos.PopLastExactly(dstLineBytes);
			else dstPos.PopFirstExactly(dstLineBytes);
			stream.PopFirstN(srcLineBytes-usefulSrcLineBytes);
		}

	if(swapRB) SwapRedBlueChannels(dstFormat, dstAlignment, sizes, dstBuf);
}

void ReadPalettedPixelDataBlock(IInputStream& stream, CSpan<byte> palette,
	ushort bpp, USVec2 sizes, ImageFormat format, bool flipVert,
	ushort srcAlignment, ushort dstAlignment, Span<byte> dstBuf)
{
	INTRA_DEBUG_ASSERT(palette.Length() >= 1u << bpp);
	const ushort bytesPerPixel = format.BytesPerPixel();
	const uint usefulSrcLineBytes = uint(sizes.x*bpp)/8u;
	const uint usefulDstLineBytes = uint(sizes.x*bytesPerPixel);
	const uint srcLineBytes = (usefulSrcLineBytes+srcAlignment-1u)&~(srcAlignment-1u);
	const uint dstLineBytes = (usefulDstLineBytes+dstAlignment-1u)&~(dstAlignment-1u);
	const size_t dstDataSize = sizes.x*dstLineBytes;

	byte* pos = dstBuf.Begin;
	if(flipVert) pos += dstDataSize-dstLineBytes;

	for(int y=0; y<sizes.y; y++)
	{
		byte* linePos = pos;
		if(bpp==1) for(uint j=0; j<sizes.x; j += 8)
		{
			byte colorIndices = Range::RawRead<byte>(stream);
			for(int k=0; k<8; k++)
			{
				size_t bytesLeftToCopy = bytesPerPixel;
				const byte* paletteSrc = palette.Data() + ((colorIndices & 0x80) >> 7)*bytesPerPixel;
				while(bytesLeftToCopy --> 0) *linePos++ = *paletteSrc++;
				colorIndices = byte(colorIndices << 1);
			}
		}
		else if(bpp==4) for(uint j=0; j<sizes.x; j += 2)
		{
			const byte colorIndices = Range::RawRead<byte>(stream);
			const byte* paletteSrc = palette.Data() + (colorIndices >> 4)*bytesPerPixel;
			size_t bytesLeftToCopy = bytesPerPixel;
			while(bytesLeftToCopy --> 0) *linePos++ = *paletteSrc++;
			paletteSrc = palette.Data() + (colorIndices & 15)*bytesPerPixel;
			while(bytesLeftToCopy --> 0) *linePos++ = *paletteSrc++;
		}
		else if(bpp==8) for(uint j = 0; j<sizes.x; j++)
		{
			const byte colorIndex = Range::RawRead<byte>(stream);
			size_t bytesLeftToCopy = bytesPerPixel;
			const byte* paletteSrc = palette.Data() + colorIndex*bytesPerPixel;
			while(bytesLeftToCopy--> 0) *linePos++ = *paletteSrc++;
		}
		if(!flipVert) pos += dstLineBytes;
		else pos -= dstLineBytes;
		stream.PopFirstN(srcLineBytes-usefulSrcLineBytes);
	}
}


}}
