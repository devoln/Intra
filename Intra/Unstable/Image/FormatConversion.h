#pragma once

#include "Math/Vector2.h"
#include "Math/Math.h"
#include "Math/Bit.h"

#include "Core/Range/Span.h"

#include "Utils/IInput.h"

#include "Image/ImageFormat.h"

INTRA_BEGIN
inline uint ComponentByMask(uint color, uint mask)
{return (color&mask) >> FindBitPosition(mask);}

uint ConvertColorBits(uint color, uint fromBitCount, uint toBitCount);
void SwapRedBlueChannels(ImageFormat format, ushort lineAlignment, USVec2 sizes, Span<byte> data);

void ReadPixelDataBlock(IInputStream& stream, USVec2 sizes,
	ImageFormat srcFormat, ImageFormat dstFormat,
	bool swapRB, bool flipVert, ushort srcAlignment, ushort dstAlignment, Span<byte> dstBuf);

void ReadPalettedPixelDataBlock(IInputStream& stream, CSpan<byte> palette,
	ushort bpp, USVec2 sizes, ImageFormat format, bool flipVert,
	ushort srcAlignment, ushort dstAlignment, Span<byte> dstBuf);
INTRA_END
