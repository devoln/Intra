#include "IntraX/Unstable/Image/FormatConversion.h"
#include "IntraX/Utils/Endianess.h"
#include "Intra/Range/Stream/RawRead.h"

INTRA_BEGIN
unsigned ConvertColorBits(unsigned color, unsigned fromBitCount, unsigned toBitCount)
{
	if(toBitCount < fromBitCount)
		color >>= fromBitCount-  toBitCount;
	else
	{
		color <<= toBitCount - fromBitCount;
		if(color>0) color |= BitCountToMask(toBitCount-fromBitCount);
	}
	return color;
}

template<typename T> static void SwapRedBlueTyped(size_t lineUnused,
	size_t componentCount, USVec2 sizes, T* data)
{
	for(int y = 0; y<sizes.y; y++)
	{
		for(int x = 0; x<sizes.x; x++)
		{
			Swap(data[0], data[2]);
			data += componentCount;
		}
		data += lineUnused;
	}
}

void SwapRedBlueChannels(ImageFormat format, uint16 lineAlignment, USVec2 sizes, Span<byte> data)
{
	const auto components = format.ComponentCount();
	const auto bytesPerComp = format.GetComponentType().Size();
	const uint16 lineUnusedBytes = uint16(lineAlignment-sizes.x*format.BytesPerPixel()%lineAlignment);
	INTRA_DEBUG_ASSERT(data.Length() >= sizes.y*(sizes.x*components + lineUnusedBytes));
	if(bytesPerComp == 1) SwapRedBlueTyped<byte>(lineUnusedBytes, components, sizes, data.Data());
	else if(bytesPerComp == 2) SwapRedBlueTyped<uint16>(lineUnusedBytes/2u, components, sizes, reinterpret_cast<uint16*>(data.Data()));
	else if(bytesPerComp == 4) SwapRedBlueTyped<unsigned>(lineUnusedBytes/4u, components, sizes, reinterpret_cast<unsigned*>(data.Data()));
	else INTRA_FATAL_ERROR("swap_red_blue пока не поддерживает упакованные форматы!");
}



void ReadPixelDataBlock(IInputStream& stream, USVec2 sizes,
	ImageFormat srcFormat, ImageFormat dstFormat,
	bool swapRB, bool flipVert, uint16 srcAlignment, uint16 dstAlignment, Span<byte> dstBuf)
{
	INTRA_PRECONDITION(srcFormat.ComponentCount() >= 3 || !swapRB);
	const index_t usefulSrcLineBytes = sizes.x*srcFormat.BytesPerPixel();
	const index_t usefulDstLineBytes = sizes.x*dstFormat.BytesPerPixel();
	const index_t srcLineBytes = (usefulSrcLineBytes + srcAlignment - 1) & ~index_t(srcAlignment-1);
	const index_t dstLineBytes = (usefulDstLineBytes + dstAlignment - 1) & ~index_t(dstAlignment-1);
	const index_t srcDataSize = sizes.y*srcLineBytes;
	const index_t dstDataSize = sizes.y*dstLineBytes;
	INTRA_DEBUG_ASSERT(dstBuf.Length() >= dstDataSize);
	(void)dstDataSize;

	if(srcFormat == dstFormat && srcLineBytes==dstLineBytes && !swapRB && !flipVert)
	{
		RawReadTo(stream, dstBuf.Take(srcDataSize));
		return;
	}

	auto dstPos = dstBuf;

	if(srcFormat == dstFormat)
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
			stream.PopFirstCount(srcLineBytes-usefulSrcLineBytes);
		}

	if(srcFormat == ImageFormat::A1_BGR5 && dstFormat == ImageFormat::RGB8)
		for(int y = 0; y < sizes.y; y++)
		{
			auto pixels = !flipVert?
				reinterpret_cast<UBVec3*>(dstPos.Begin):
				reinterpret_cast<UBVec3*>(dstPos.End)-sizes.x;
			for(unsigned x = 0; x < sizes.x; x++)
			{
				uint16 color = RawRead<ushortLE>(stream);
				*pixels++ = {(color >> 11) << 3, ((color >> 6) & 0x1f) << 3, ((color >> 1) & 0x1f) << 3};
			}
			if(flipVert) dstPos.PopLastExactly(dstLineBytes);
			else dstPos.PopFirstExactly(dstLineBytes);
			stream.PopFirstCount(srcLineBytes-usefulSrcLineBytes);
		}

	if(srcFormat == ImageFormat::RGB5A1 && dstFormat == ImageFormat::RGB8)
		for(int y = 0; y < sizes.y; y++)
		{
			auto pixels = !flipVert?
				reinterpret_cast<UBVec3*>(dstPos.Begin):
				reinterpret_cast<UBVec3*>(dstPos.End) - sizes.x;
			for(unsigned x = 0; x < sizes.x; x++)
			{
				uint16 color = RawRead<ushortLE>(stream);
				*pixels++ = {((color >> 10) & 0x1f) << 3, ((color >> 5) & 0x1f) << 3, (color & 0x1f) << 3};
			}
			if(flipVert) dstPos.PopLastExactly(dstLineBytes);
			else dstPos.PopFirstExactly(dstLineBytes);
			stream.PopFirstCount(srcLineBytes-usefulSrcLineBytes);
		}

	if(srcFormat == ImageFormat::RGBA8 && dstFormat == ImageFormat::RGB8)
		for(int y = 0; y < sizes.y; y++)
		{
			auto pixels = !flipVert?
				reinterpret_cast<UBVec3*>(dstPos.Begin):
				reinterpret_cast<UBVec3*>(dstPos.End)-sizes.x;
			if(swapRB) for(unsigned x = 0; x < sizes.x; x++)
				*pixels++ = RawRead<UBVec4>(stream).swizzle<2, 1, 0>();
			else for(unsigned x = 0; x < sizes.x; x++)
				*pixels++ = RawRead<UBVec4>(stream).xyz;
			if(flipVert) dstPos.PopLastExactly(dstLineBytes);
			else dstPos.PopFirstExactly(dstLineBytes);
			stream.PopFirstCount(srcLineBytes-usefulSrcLineBytes);
		}

	if(swapRB) SwapRedBlueChannels(dstFormat, dstAlignment, sizes, dstBuf);
}

void ReadPalettedPixelDataBlock(IInputStream& stream, CSpan<byte> palette,
	uint16 bpp, USVec2 sizes, ImageFormat format, bool flipVert,
	uint16 srcAlignment, uint16 dstAlignment, Span<byte> dstBuf)
{
	INTRA_PRECONDITION(palette.Length() >= 1 << bpp);
	const index_t bytesPerPixel = format.BytesPerPixel();
	const index_t usefulSrcLineBytes = index_t(unsigned(sizes.x * bpp) / 8);
	const index_t usefulDstLineBytes = sizes.x * bytesPerPixel;
	const index_t srcLineBytes = (usefulSrcLineBytes + srcAlignment - 1)& ~index_t(srcAlignment - 1);
	const index_t dstLineBytes = (usefulDstLineBytes + dstAlignment - 1)& ~index_t(dstAlignment - 1);
	const index_t dstDataSize = sizes.x*dstLineBytes;

	byte* pos = dstBuf.Begin;
	if(flipVert) pos += dstDataSize-dstLineBytes;

	for(int y = 0; y < sizes.y; y++)
	{
		byte* linePos = pos;
		if(bpp == 1) for(unsigned j = 0; j < sizes.x; j += 8)
		{
			byte colorIndices = RawRead<byte>(stream);
			for(int k = 0; k < 8; k++)
			{
				auto bytesLeftToCopy = bytesPerPixel;
				const byte* paletteSrc = palette.Data() + ((colorIndices & 0x80) >> 7)*bytesPerPixel;
				while(bytesLeftToCopy --> 0) *linePos++ = *paletteSrc++;
				colorIndices = byte(colorIndices << 1);
			}
		}
		else if(bpp == 4) for(unsigned j = 0; j < sizes.x; j += 2)
		{
			const byte colorIndices = RawRead<byte>(stream);
			const byte* paletteSrc = palette.Data() + (colorIndices >> 4)*bytesPerPixel;
			auto bytesLeftToCopy = bytesPerPixel;
			while(bytesLeftToCopy --> 0) *linePos++ = *paletteSrc++;
			paletteSrc = palette.Data() + (colorIndices & 15)*bytesPerPixel;
			while(bytesLeftToCopy --> 0) *linePos++ = *paletteSrc++;
		}
		else if(bpp == 8) for(unsigned j = 0; j < sizes.x; j++)
		{
			const byte colorIndex = RawRead<byte>(stream);
			auto bytesLeftToCopy = bytesPerPixel;
			const byte* paletteSrc = palette.Data() + colorIndex*bytesPerPixel;
			while(bytesLeftToCopy --> 0) *linePos++ = *paletteSrc++;
		}
		if(!flipVert) pos += dstLineBytes;
		else pos -= dstLineBytes;
		stream.PopFirstCount(srcLineBytes - usefulSrcLineBytes);
	}
}
INTRA_END
