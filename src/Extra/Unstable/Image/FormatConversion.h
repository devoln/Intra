#pragma once

#include "Intra/Math/Math.h"
#include "Intra/Math/Bit.h"
#include "Intra/Range/Span.h"
#include "Intra/Range/Polymorphic/IInput.h"

#include "Extra/Math/Vector2.h"

#include "Extra/Unstable/Image/ImageFormat.h"

INTRA_BEGIN
inline unsigned ComponentByMask(unsigned color, unsigned mask)
{return (color&mask) >> FindBitPosition(mask);}

unsigned ConvertColorBits(unsigned color, unsigned fromBitCount, unsigned toBitCount);
void SwapRedBlueChannels(ImageFormat format, uint16 lineAlignment, USVec2 sizes, Span<byte> data);

void ReadPixelDataBlock(IInputStream& stream, USVec2 sizes,
	ImageFormat srcFormat, ImageFormat dstFormat,
	bool swapRB, bool flipVert, uint16 srcAlignment, uint16 dstAlignment, Span<byte> dstBuf);

void ReadPalettedPixelDataBlock(IInputStream& stream, CSpan<byte> palette,
	uint16 bpp, USVec2 sizes, ImageFormat format, bool flipVert,
	uint16 srcAlignment, uint16 dstAlignment, Span<byte> dstBuf);
INTRA_END
