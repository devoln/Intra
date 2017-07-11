#pragma once

#include "Utils/Span.h"

namespace Intra { namespace Audio {

void InterleaveFloats(Span<float> dst, CSpan<float> src1, CSpan<float> src2);
void InterleaveFloats(Span<float> dst, CSpan<float> src1, CSpan<float> src2, CSpan<float> src3);
void InterleaveFloats(Span<float> dst, CSpan<float> src1,
	CSpan<float> src2, CSpan<float> src3, CSpan<float> src4);
void InterleaveFloats(Span<float> dst, CSpan<float> src1,
	CSpan<float> src2, CSpan<float> src3, CSpan<float> src4, CSpan<float> src5);
void InterleaveFloats(Span<float> dst, CSpan<float> src1, CSpan<float> src2,
	CSpan<float> src3, CSpan<float> src4, CSpan<float> src5, CSpan<float> src6);
void InterleaveFloats(Span<float> dst, CSpan<float> src1, CSpan<float> src2,
	CSpan<float> src3, CSpan<float> src4, CSpan<float> src5, CSpan<float> src6, CSpan<float> src7);
void InterleaveFloats(Span<float> dst, CSpan<float> src1, CSpan<float> src2, CSpan<float> src3,
	CSpan<float> src4, CSpan<float> src5, CSpan<float> src6, CSpan<float> src7, CSpan<float> src8);

void InterleaveFloats(Span<float> dst, Span<CSpan<float>> srcChannels);


void InterleaveShorts(Span<short> dst, CSpan<short> src1, CSpan<short> src2);
void InterleaveShorts(Span<short> dst, CSpan<short> src1, CSpan<short> src2, CSpan<short> src3);
void InterleaveShorts(Span<short> dst, CSpan<short> src1,
	CSpan<short> src2, CSpan<short> src3, CSpan<short> src4);
void InterleaveShorts(Span<short> dst, CSpan<short> src1,
	CSpan<short> src2, CSpan<short> src3, CSpan<short> src4, CSpan<short> src5);
void InterleaveShorts(Span<short> dst, CSpan<short> src1, CSpan<short> src2,
	CSpan<short> src3, CSpan<short> src4, CSpan<short> src5, CSpan<short> src6);
void InterleaveShorts(Span<short> dst, CSpan<short> src1, CSpan<short> src2,
	CSpan<short> src3, CSpan<short> src4, CSpan<short> src5, CSpan<short> src6, CSpan<short> src7);
void InterleaveShorts(Span<short> dst, CSpan<short> src1, CSpan<short> src2, CSpan<short> src3,
	CSpan<short> src4, CSpan<short> src5, CSpan<short> src6, CSpan<short> src7, CSpan<short> src8);

void InterleaveShorts(Span<short> dst, Span<CSpan<short>> srcChannels);


void DeinterleaveFloats(CSpan<float> src, Span<float> dst1, Span<float> dst2);
void DeinterleaveFloats(CSpan<float> src, Span<float> dst1, Span<float> dst2, Span<float> dst3);
void DeinterleaveFloats(CSpan<float> src, Span<float> dst1,
	Span<float> dst2, Span<float> dst3, Span<float> dst4);
void DeinterleaveFloats(CSpan<float> src, Span<float> dst1,
	Span<float> dst2, Span<float> dst3, Span<float> dst4, Span<float> dst5);
void DeinterleaveFloats(CSpan<float> src, Span<float> dst1,
	Span<float> dst2, Span<float> dst3, Span<float> dst4, Span<float> dst5, Span<float> dst6);
void DeinterleaveFloats(CSpan<float> src, Span<float> dst1, Span<float> dst2, Span<float> dst3,
	Span<float> dst4, Span<float> dst5, Span<float> dst6, Span<float> dst7);
void DeinterleaveFloats(CSpan<float> src, Span<float> dst1, Span<float> dst2, Span<float> dst3,
	Span<float> dst4, Span<float> dst5, Span<float> dst6, Span<float> dst7, Span<float> dst8);

void DeinterleaveFloats(CSpan<float> src, Span<Span<float>> dst);


void InterleaveFloatsCastToShorts(Span<short> dst, CSpan<float> src1, CSpan<float> src2);
void InterleaveFloatsCastToShorts(Span<short> dst, CSpan<float> src1, CSpan<float> src2, CSpan<float> src3);
void InterleaveFloatsCastToShorts(Span<short> dst, CSpan<float> src1,
	CSpan<float> src2, CSpan<float> src3, CSpan<float> src4);
void InterleaveFloatsCastToShorts(Span<short> dst, CSpan<float> src1, CSpan<float> src2,
	CSpan<float> src3, CSpan<float> src4, CSpan<float> src5);
void InterleaveFloatsCastToShorts(Span<short> dst, CSpan<float> src1, CSpan<float> src2,
	CSpan<float> src3, CSpan<float> src4, CSpan<float> src5, CSpan<float> src6);
void InterleaveFloatsCastToShorts(Span<short> dst, CSpan<float> src1, CSpan<float> src2,
	CSpan<float> src3, CSpan<float> src4, CSpan<float> src5, CSpan<float> src6, CSpan<float> src7);
void InterleaveFloatsCastToShorts(Span<short> dst, CSpan<float> src1, CSpan<float> src2, CSpan<float> src3,
	CSpan<float> src4, CSpan<float> src5, CSpan<float> src6, CSpan<float> src7, CSpan<float> src8);

void InterleaveFloatsCastToShorts(Span<short> dst, Span<CSpan<float>> srcChannels);


void DeinterleaveFloatsCastToShorts(CSpan<float> src, Span<short> dst1, Span<short> dst2);
void DeinterleaveFloatsCastToShorts(CSpan<float> src,
	Span<short> dst1, Span<short> dst2, Span<short> dst3);
void DeinterleaveFloatsCastToShorts(CSpan<float> src, Span<short> dst1,
	Span<short> dst2, Span<short> dst3, Span<short> dst4);
void DeinterleaveFloatsCastToShorts(CSpan<float> src, Span<short> dst1,
	Span<short> dst2, Span<short> dst3, Span<short> dst4, Span<short> dst5);
void DeinterleaveFloatsCastToShorts(CSpan<float> src, Span<short> dst1, Span<short> dst2,
	Span<short> dst3, Span<short> dst4, Span<short> dst5, Span<short> dst6);
void DeinterleaveFloatsCastToShorts(CSpan<float> src, Span<short> dst1, Span<short> dst2,
	Span<short> dst3, Span<short> dst4, Span<short> dst5, Span<short> dst6, Span<short> dst7);
void DeinterleaveFloatsCastToShorts(CSpan<float> src, Span<short> dst1, Span<short> dst2,
	Span<short> dst3, Span<short> dst4, Span<short> dst5, Span<short> dst6, Span<short> dst7, Span<short> dst8);

void DeinterleaveFloatsCastToShorts(CSpan<float> src, Span<Span<short>> dst);


void DeinterleaveShortsCastToFloats(CSpan<short> src, Span<float> dst1, Span<float> dst2);

void DeinterleaveShortsCastToFloats(CSpan<short> src,
	Span<float> dst1, Span<float> dst2, Span<float> dst3);

void DeinterleaveShortsCastToFloats(CSpan<short> src, Span<float> dst1,
	Span<float> dst2, Span<float> dst3, Span<float> dst4);

void DeinterleaveShortsCastToFloats(CSpan<short> src, Span<float> dst1,
	Span<float> dst2, Span<float> dst3, Span<float> dst4, Span<float> dst5);

void DeinterleaveShortsCastToFloats(CSpan<short> src, Span<float> dst1, Span<float> dst2,
	Span<float> dst3, Span<float> dst4, Span<float> dst5, Span<float> dst6);

void DeinterleaveShortsCastToFloats(CSpan<short> src, Span<float> dst1, Span<float> dst2,
	Span<float> dst3, Span<float> dst4, Span<float> dst5, Span<float> dst6, Span<float> dst7);

void DeinterleaveShortsCastToFloats(CSpan<short> src, Span<float> dst1, Span<float> dst2,
	Span<float> dst3, Span<float> dst4, Span<float> dst5, Span<float> dst6, Span<float> dst7, Span<float> dst8);

void DeinterleaveShortsCastToFloats(CSpan<short> src, Span<Span<float>> dst);

}}

