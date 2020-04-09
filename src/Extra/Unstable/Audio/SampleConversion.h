#pragma once

#include "Intra/Range/Span.h"

INTRA_BEGIN
void InterleaveFloats(Span<float> dst, CSpan<float> src1, CSpan<float> src2);
void InterleaveFloats(Span<float> dst, Span<CSpan<float>> srcChannels);

void InterleaveShorts(Span<short> dst, CSpan<short> src1, CSpan<short> src2);
void InterleaveShorts(Span<short> dst, Span<CSpan<short>> srcChannels);

void DeinterleaveFloats(CSpan<float> src, Span<float> dst1, Span<float> dst2);
void DeinterleaveFloats(CSpan<float> src, Span<Span<float>> dst);

void DeinterleaveShorts(CSpan<short> src, Span<short> dst1, Span<short> dst2);
void DeinterleaveShorts(CSpan<short> src, Span<Span<short>> dst);

void InterleaveFloatsCastToShorts(Span<short> dst, CSpan<float> src1, CSpan<float> src2);
void InterleaveFloatsCastToShorts(Span<short> dst, Span<CSpan<float>> srcChannels);

void DeinterleaveFloatsCastToShorts(CSpan<float> src, Span<short> dst1, Span<short> dst2);
void DeinterleaveFloatsCastToShorts(CSpan<float> src, Span<Span<short>> dst);

void DeinterleaveShortsCastToFloats(CSpan<short> src, Span<float> dst1, Span<float> dst2);
void DeinterleaveShortsCastToFloats(CSpan<short> src, Span<Span<float>> dst);
INTRA_END
