#pragma once

#include "Intra/Range/Span.h"

namespace Intra { INTRA_BEGIN
void InterleaveFloats(Span<float> dst, Span<const float> src1, Span<const float> src2);
void InterleaveFloats(Span<float> dst, Span<Span<const float>> srcChannels);

void InterleaveShorts(Span<short> dst, Span<const short> src1, Span<const short> src2);
void InterleaveShorts(Span<short> dst, Span<Span<const short>> srcChannels);

void DeinterleaveFloats(Span<const float> src, Span<float> dst1, Span<float> dst2);
void DeinterleaveFloats(Span<const float> src, Span<Span<float>> dst);

void DeinterleaveShorts(Span<const short> src, Span<short> dst1, Span<short> dst2);
void DeinterleaveShorts(Span<const short> src, Span<Span<short>> dst);

void InterleaveFloatsCastToShorts(Span<short> dst, Span<const float> src1, Span<const float> src2);
void InterleaveFloatsCastToShorts(Span<short> dst, Span<Span<const float>> srcChannels);

void DeinterleaveFloatsCastToShorts(Span<const float> src, Span<short> dst1, Span<short> dst2);
void DeinterleaveFloatsCastToShorts(Span<const float> src, Span<Span<short>> dst);

void DeinterleaveShortsCastToFloats(Span<const short> src, Span<float> dst1, Span<float> dst2);
void DeinterleaveShortsCastToFloats(Span<const short> src, Span<Span<float>> dst);
} INTRA_END
