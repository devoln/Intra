#pragma once

#include "Math/Vector.h"
#include "Imaging/ImageFormat.h"
#include "IO/Stream.h"
#include "Range/ForwardDecls.h"
#include "Math/MathEx.h"
#include "Math/Bit.h"
#include "Platform/CppWarnings.h"

namespace Intra { namespace Imaging {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

inline uint ComponentByMask(uint color, uint mask)
{return (color&mask) >> Math::FindBitPosition(mask);}

uint ConvertColorBits(uint color, uint fromBitCount, uint toBitCount);
void SwapRedBlueChannels(ImageFormat format, ushort lineAlignment, Math::USVec2 sizes, ArrayRange<byte> data);

void ReadPixelDataBlock(IO::IInputStream& stream, Math::USVec2 sizes,
	ImageFormat srcFormat, ImageFormat dstFormat,
	bool swapRB, bool flipVert, ushort srcAlignment, ushort dstAlignment, ArrayRange<byte> dstBuf);

void ReadPalettedPixelDataBlock(IO::IInputStream& stream, ArrayRange<const byte> palette,
	ushort bpp, Math::USVec2 sizes, ImageFormat format, bool flipVert,
	ushort srcAlignment, ushort dstAlignment, ArrayRange<byte> dstBuf);

INTRA_WARNING_POP

}}
