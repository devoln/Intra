#pragma once



#include "Math/Vector2.h"
#include "Math/Math.h"
#include "Math/Bit.h"

#include "Core/Range/Span.h"

#include "Core/IInput.h"

#include "Image/ImageFormat.h"

INTRA_BEGIN
namespace Image {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

inline uint ComponentByMask(uint color, uint mask)
{return (color&mask) >> Math::FindBitPosition(mask);}

uint ConvertColorBits(uint color, uint fromBitCount, uint toBitCount);
void SwapRedBlueChannels(ImageFormat format, ushort lineAlignment, Math::USVec2 sizes, Span<byte> data);

void ReadPixelDataBlock(IInputStream& stream, Math::USVec2 sizes,
	ImageFormat srcFormat, ImageFormat dstFormat,
	bool swapRB, bool flipVert, ushort srcAlignment, ushort dstAlignment, Span<byte> dstBuf);

void ReadPalettedPixelDataBlock(IInputStream& stream, CSpan<byte> palette,
	ushort bpp, Math::USVec2 sizes, ImageFormat format, bool flipVert,
	ushort srcAlignment, ushort dstAlignment, Span<byte> dstBuf);

INTRA_WARNING_POP

}}
