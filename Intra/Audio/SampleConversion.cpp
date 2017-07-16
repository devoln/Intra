#include "SampleConversion.h"

namespace Intra { namespace Audio {

#if(INTRA_MINEXE == 0)
void InterleaveFloats(Span<float> dst, CSpan<float> src1, CSpan<float> src2)
{
	while(dst.End != dst.Begin)
	{
		*dst.Begin++ = *src1.Begin++;
		*dst.Begin++ = *src2.Begin++;
	}
}

void InterleaveFloats(Span<float> dst, CSpan<float> src1, CSpan<float> src2, CSpan<float> src3)
{
	while(dst.End != dst.Begin)
	{
		*dst.Begin++ = *src1.Begin++;
		*dst.Begin++ = *src2.Begin++;
		*dst.Begin++ = *src3.Begin++;
	}
}

void InterleaveFloats(Span<float> dst, CSpan<float> src1,
	CSpan<float> src2, CSpan<float> src3, CSpan<float> src4)
{
	while(dst.End != dst.Begin)
	{
		*dst.Begin++ = *src1.Begin++;
		*dst.Begin++ = *src2.Begin++;
		*dst.Begin++ = *src3.Begin++;
		*dst.Begin++ = *src4.Begin++;
	}
}

void InterleaveFloats(Span<float> dst, CSpan<float> src1,
	CSpan<float> src2, CSpan<float> src3, CSpan<float> src4, CSpan<float> src5)
{
	while(dst.End != dst.Begin)
	{
		*dst.Begin++ = *src1.Begin++;
		*dst.Begin++ = *src2.Begin++;
		*dst.Begin++ = *src3.Begin++;
		*dst.Begin++ = *src4.Begin++;
		*dst.Begin++ = *src5.Begin++;
	}
}

void InterleaveFloats(Span<float> dst, CSpan<float> src1, CSpan<float> src2,
	CSpan<float> src3, CSpan<float> src4, CSpan<float> src5, CSpan<float> src6)
{
	while(dst.End != dst.Begin)
	{
		*dst.Begin++ = *src1.Begin++;
		*dst.Begin++ = *src2.Begin++;
		*dst.Begin++ = *src3.Begin++;
		*dst.Begin++ = *src4.Begin++;
		*dst.Begin++ = *src5.Begin++;
		*dst.Begin++ = *src6.Begin++;
	}
}

void InterleaveFloats(Span<float> dst, CSpan<float> src1, CSpan<float> src2,
	CSpan<float> src3, CSpan<float> src4, CSpan<float> src5, CSpan<float> src6, CSpan<float> src7)
{
	while(dst.End != dst.Begin)
	{
		*dst.Begin++ = *src1.Begin++;
		*dst.Begin++ = *src2.Begin++;
		*dst.Begin++ = *src3.Begin++;
		*dst.Begin++ = *src4.Begin++;
		*dst.Begin++ = *src5.Begin++;
		*dst.Begin++ = *src6.Begin++;
		*dst.Begin++ = *src7.Begin++;
	}
}

void InterleaveFloats(Span<float> dst, CSpan<float> src1, CSpan<float> src2, CSpan<float> src3,
	CSpan<float> src4, CSpan<float> src5, CSpan<float> src6, CSpan<float> src7, CSpan<float> src8)
{
	while(dst.End != dst.Begin)
	{
		*dst.Begin++ = *src1.Begin++;
		*dst.Begin++ = *src2.Begin++;
		*dst.Begin++ = *src3.Begin++;
		*dst.Begin++ = *src4.Begin++;
		*dst.Begin++ = *src5.Begin++;
		*dst.Begin++ = *src6.Begin++;
		*dst.Begin++ = *src7.Begin++;
		*dst.Begin++ = *src8.Begin++;
	}
}
#endif

template<typename T> void genericInterleave(Span<T> dst, Span<CSpan<T>> src)
{
	CSpan<T> srcs[16];
	auto srcsSpan = Range::Take(srcs, src.CopyTo(srcs));
	while(dst.End != dst.Begin)
		for(auto& srci: srcsSpan)
			*dst.Begin++ = *srci.Begin++;
}

void InterleaveFloats(Span<float> dst, Span<CSpan<float>> src)
{
#if(INTRA_MINEXE == 0)
	switch(src.Length())
#endif
	{
#if(INTRA_MINEXE == 0)
	case 2: InterleaveFloats(dst, src[0], src[1]); break;
	case 3: InterleaveFloats(dst, src[0], src[1], src[2]); break;
	case 4: InterleaveFloats(dst, src[0], src[1], src[2], src[3]); break;
	case 5: InterleaveFloats(dst, src[0], src[1], src[2], src[3], src[4]); break;
	case 6: InterleaveFloats(dst, src[0], src[1], src[2], src[3], src[4], src[5]); break;
	case 7: InterleaveFloats(dst, src[0], src[1], src[2], src[3], src[4], src[5], src[6]); break;
	case 8: InterleaveFloats(dst, src[0], src[1], src[2], src[3], src[4], src[5], src[6], src[7]); break;
	default:
#endif
		genericInterleave(dst, src);
	}
}


#if(INTRA_MINEXE == 0)
void InterleaveShorts(Span<short> dst, CSpan<short> src1, CSpan<short> src2)
{
	while(dst.End != dst.Begin)
	{
		*dst.Begin++ = *src1.Begin++;
		*dst.Begin++ = *src2.Begin++;
	}
}

void InterleaveShorts(Span<short> dst, CSpan<short> src1, CSpan<short> src2, CSpan<short> src3)
{
	while(dst.End != dst.Begin)
	{
		*dst.Begin++ = *src1.Begin++;
		*dst.Begin++ = *src2.Begin++;
		*dst.Begin++ = *src3.Begin++;
	}
}

void InterleaveShorts(Span<short> dst, CSpan<short> src1,
	CSpan<short> src2, CSpan<short> src3, CSpan<short> src4)
{
	while(dst.End != dst.Begin)
	{
		*dst.Begin++ = *src1.Begin++;
		*dst.Begin++ = *src2.Begin++;
		*dst.Begin++ = *src3.Begin++;
		*dst.Begin++ = *src4.Begin++;
	}
}

void InterleaveShorts(Span<short> dst, CSpan<short> src1,
	CSpan<short> src2, CSpan<short> src3, CSpan<short> src4, CSpan<short> src5)
{
	while(dst.End != dst.Begin)
	{
		*dst.Begin++ = *src1.Begin++;
		*dst.Begin++ = *src2.Begin++;
		*dst.Begin++ = *src3.Begin++;
		*dst.Begin++ = *src4.Begin++;
		*dst.Begin++ = *src5.Begin++;
	}
}

void InterleaveShorts(Span<short> dst, CSpan<short> src1, CSpan<short> src2,
	CSpan<short> src3, CSpan<short> src4, CSpan<short> src5, CSpan<short> src6)
{
	while(dst.End != dst.Begin)
	{
		*dst.Begin++ = *src1.Begin++;
		*dst.Begin++ = *src2.Begin++;
		*dst.Begin++ = *src3.Begin++;
		*dst.Begin++ = *src4.Begin++;
		*dst.Begin++ = *src5.Begin++;
		*dst.Begin++ = *src6.Begin++;
	}
}

void InterleaveShorts(Span<short> dst, CSpan<short> src1, CSpan<short> src2,
	CSpan<short> src3, CSpan<short> src4, CSpan<short> src5, CSpan<short> src6, CSpan<short> src7)
{
	while(dst.End != dst.Begin)
	{
		*dst.Begin++ = *src1.Begin++;
		*dst.Begin++ = *src2.Begin++;
		*dst.Begin++ = *src3.Begin++;
		*dst.Begin++ = *src4.Begin++;
		*dst.Begin++ = *src5.Begin++;
		*dst.Begin++ = *src6.Begin++;
		*dst.Begin++ = *src7.Begin++;
	}
}

void InterleaveShorts(Span<short> dst, CSpan<short> src1, CSpan<short> src2, CSpan<short> src3,
	CSpan<short> src4, CSpan<short> src5, CSpan<short> src6, CSpan<short> src7, CSpan<short> src8)
{
	while(dst.End != dst.Begin)
	{
		*dst.Begin++ = *src1.Begin++;
		*dst.Begin++ = *src2.Begin++;
		*dst.Begin++ = *src3.Begin++;
		*dst.Begin++ = *src4.Begin++;
		*dst.Begin++ = *src5.Begin++;
		*dst.Begin++ = *src6.Begin++;
		*dst.Begin++ = *src7.Begin++;
		*dst.Begin++ = *src8.Begin++;
	}
}
#endif

void InterleaveShorts(Span<short> dst, Span<CSpan<short>> src)
{
#if(INTRA_MINEXE == 0)
	switch(src.Length())
#endif
	{
#if(INTRA_MINEXE == 0)
	case 2: InterleaveShorts(dst, src[0], src[1]); break;
	case 3: InterleaveShorts(dst, src[0], src[1], src[2]); break;
	case 4: InterleaveShorts(dst, src[0], src[1], src[2], src[3]); break;
	case 5: InterleaveShorts(dst, src[0], src[1], src[2], src[3], src[4]); break;
	case 6: InterleaveShorts(dst, src[0], src[1], src[2], src[3], src[4], src[5]); break;
	case 7: InterleaveShorts(dst, src[0], src[1], src[2], src[3], src[4], src[5], src[6]); break;
	case 8: InterleaveShorts(dst, src[0], src[1], src[2], src[3], src[4], src[5], src[6], src[7]); break;
	default:
#endif
		genericInterleave(dst, src);
	}
}


#if(INTRA_MINEXE == 0)
void DeinterleaveFloats(CSpan<float> src, Span<float> dst1, Span<float> dst2)
{
	while(src.End != src.Begin)
	{
		*dst1.Begin++ = *src.Begin++;
		*dst2.Begin++ = *src.Begin++;
	}
}

void DeinterleaveFloats(CSpan<float> src, Span<float> dst1, Span<float> dst2, Span<float> dst3)
{
	while(src.End != src.Begin)
	{
		*dst1.Begin++ = *src.Begin++;
		*dst2.Begin++ = *src.Begin++;
		*dst3.Begin++ = *src.Begin++;
	}
}

void DeinterleaveFloats(CSpan<float> src, Span<float> dst1,
	Span<float> dst2, Span<float> dst3, Span<float> dst4)
{
	while(src.End != src.Begin)
	{
		*dst1.Begin++ = *src.Begin++;
		*dst2.Begin++ = *src.Begin++;
		*dst3.Begin++ = *src.Begin++;
		*dst4.Begin++ = *src.Begin++;
	}
}

void DeinterleaveFloats(CSpan<float> src, Span<float> dst1,
	Span<float> dst2, Span<float> dst3, Span<float> dst4, Span<float> dst5)
{
	while(src.End != src.Begin)
	{
		*dst1.Begin++ = *src.Begin++;
		*dst2.Begin++ = *src.Begin++;
		*dst3.Begin++ = *src.Begin++;
		*dst4.Begin++ = *src.Begin++;
		*dst5.Begin++ = *src.Begin++;
	}
}

void DeinterleaveFloats(CSpan<float> src, Span<float> dst1, Span<float> dst2,
	Span<float> dst3, Span<float> dst4, Span<float> dst5, Span<float> dst6)
{
	while(src.End != src.Begin)
	{
		*dst1.Begin++ = *src.Begin++;
		*dst2.Begin++ = *src.Begin++;
		*dst3.Begin++ = *src.Begin++;
		*dst4.Begin++ = *src.Begin++;
		*dst5.Begin++ = *src.Begin++;
		*dst6.Begin++ = *src.Begin++;
	}
}

void DeinterleaveFloats(CSpan<float> src, Span<float> dst1, Span<float> dst2,
	Span<float> dst3, Span<float> dst4, Span<float> dst5, Span<float> dst6, Span<float> dst7)
{
	while(src.End != src.Begin)
	{
		*dst1.Begin++ = *src.Begin++;
		*dst2.Begin++ = *src.Begin++;
		*dst3.Begin++ = *src.Begin++;
		*dst4.Begin++ = *src.Begin++;
		*dst5.Begin++ = *src.Begin++;
		*dst6.Begin++ = *src.Begin++;
		*dst7.Begin++ = *src.Begin++;
	}
}

void DeinterleaveFloats(CSpan<float> src, Span<float> dst1, Span<float> dst2, Span<float> dst3,
	Span<float> dst4, Span<float> dst5, Span<float> dst6, Span<float> dst7, Span<float> dst8)
{
	while(src.End != src.Begin)
	{
		*dst1.Begin++ = *src.Begin++;
		*dst2.Begin++ = *src.Begin++;
		*dst3.Begin++ = *src.Begin++;
		*dst4.Begin++ = *src.Begin++;
		*dst5.Begin++ = *src.Begin++;
		*dst6.Begin++ = *src.Begin++;
		*dst7.Begin++ = *src.Begin++;
		*dst8.Begin++ = *src.Begin++;
	}
}
#endif

template<typename T> void genericDeinterleave(CSpan<T> src, Span<Span<T>> dst)
{
	Span<T> dsts[16];
	auto dstsSpan = Range::Take(dsts, dst.CopyTo(dsts));
	while(src.End != src.Begin)
		for(auto& dsti: dstsSpan)
			*dsti.Begin++ = *src.Begin++;
}

void DeinterleaveFloats(CSpan<float> src, Span<Span<float>> dst)
{
#if(INTRA_MINEXE == 0)
	switch(src.Length())
#endif
	{
#if(INTRA_MINEXE == 0)
	case 2: DeinterleaveFloats(src, dst[0], dst[1]); break;
	case 3: DeinterleaveFloats(src, dst[0], dst[1], dst[2]); break;
	case 4: DeinterleaveFloats(src, dst[0], dst[1], dst[2], dst[3]); break;
	case 5: DeinterleaveFloats(src, dst[0], dst[1], dst[2], dst[3], dst[4]); break;
	case 6: DeinterleaveFloats(src, dst[0], dst[1], dst[2], dst[3], dst[4], dst[5]); break;
	case 7: DeinterleaveFloats(src, dst[0], dst[1], dst[2], dst[3], dst[4], dst[5], dst[6]); break;
	case 8: DeinterleaveFloats(src, dst[0], dst[1], dst[2], dst[3], dst[4], dst[5], dst[6], dst[7]); break;
	default:
#endif
		genericDeinterleave(src, dst);
	}
}


#if(INTRA_MINEXE == 0)
void DeinterleaveShorts(CSpan<short> src, Span<short> dst1, Span<short> dst2)
{
	while(src.End != src.Begin)
	{
		*dst1.Begin++ = *src.Begin++;
		*dst2.Begin++ = *src.Begin++;
	}
}

void DeinterleaveShorts(CSpan<short> src, Span<short> dst1, Span<short> dst2, Span<short> dst3)
{
	while(src.End != src.Begin)
	{
		*dst1.Begin++ = *src.Begin++;
		*dst2.Begin++ = *src.Begin++;
		*dst3.Begin++ = *src.Begin++;
	}
}

void DeinterleaveShorts(CSpan<short> src, Span<short> dst1,
	Span<short> dst2, Span<short> dst3, Span<short> dst4)
{
	while(src.End != src.Begin)
	{
		*dst1.Begin++ = *src.Begin++;
		*dst2.Begin++ = *src.Begin++;
		*dst3.Begin++ = *src.Begin++;
		*dst4.Begin++ = *src.Begin++;
	}
}

void DeinterleaveShorts(CSpan<short> src, Span<short> dst1,
	Span<short> dst2, Span<short> dst3, Span<short> dst4, Span<short> dst5)
{
	while(src.End != src.Begin)
	{
		*dst1.Begin++ = *src.Begin++;
		*dst2.Begin++ = *src.Begin++;
		*dst3.Begin++ = *src.Begin++;
		*dst4.Begin++ = *src.Begin++;
		*dst5.Begin++ = *src.Begin++;
	}
}

void DeinterleaveShorts(CSpan<short> src, Span<short> dst1, Span<short> dst2,
	Span<short> dst3, Span<short> dst4, Span<short> dst5, Span<short> dst6)
{
	while(src.End != src.Begin)
	{
		*dst1.Begin++ = *src.Begin++;
		*dst2.Begin++ = *src.Begin++;
		*dst3.Begin++ = *src.Begin++;
		*dst4.Begin++ = *src.Begin++;
		*dst5.Begin++ = *src.Begin++;
		*dst6.Begin++ = *src.Begin++;
	}
}

void DeinterleaveShorts(CSpan<short> src, Span<short> dst1, Span<short> dst2,
	Span<short> dst3, Span<short> dst4, Span<short> dst5, Span<short> dst6, Span<short> dst7)
{
	while(src.End != src.Begin)
	{
		*dst1.Begin++ = *src.Begin++;
		*dst2.Begin++ = *src.Begin++;
		*dst3.Begin++ = *src.Begin++;
		*dst4.Begin++ = *src.Begin++;
		*dst5.Begin++ = *src.Begin++;
		*dst6.Begin++ = *src.Begin++;
		*dst7.Begin++ = *src.Begin++;
	}
}

void DeinterleaveShorts(CSpan<short> src, Span<short> dst1, Span<short> dst2, Span<short> dst3,
	Span<short> dst4, Span<short> dst5, Span<short> dst6, Span<short> dst7, Span<short> dst8)
{
	while(src.End != src.Begin)
	{
		*dst1.Begin++ = *src.Begin++;
		*dst2.Begin++ = *src.Begin++;
		*dst3.Begin++ = *src.Begin++;
		*dst4.Begin++ = *src.Begin++;
		*dst5.Begin++ = *src.Begin++;
		*dst6.Begin++ = *src.Begin++;
		*dst7.Begin++ = *src.Begin++;
		*dst8.Begin++ = *src.Begin++;
	}
}
#endif

void DeinterleaveShorts(CSpan<short> src, Span<Span<short>> dst)
{
#if(INTRA_MINEXE == 0)
	switch(src.Length())
#endif
	{
#if(INTRA_MINEXE == 0)
	case 2: DeinterleaveShorts(src, dst[0], dst[1]); break;
	case 3: DeinterleaveShorts(src, dst[0], dst[1], dst[2]); break;
	case 4: DeinterleaveShorts(src, dst[0], dst[1], dst[2], dst[3]); break;
	case 5: DeinterleaveShorts(src, dst[0], dst[1], dst[2], dst[3], dst[4]); break;
	case 6: DeinterleaveShorts(src, dst[0], dst[1], dst[2], dst[3], dst[4], dst[5]); break;
	case 7: DeinterleaveShorts(src, dst[0], dst[1], dst[2], dst[3], dst[4], dst[5], dst[6]); break;
	case 8: DeinterleaveShorts(src, dst[0], dst[1], dst[2], dst[3], dst[4], dst[5], dst[6], dst[7]); break;
	default:
#endif
		genericDeinterleave(src, dst);
	}
}


#if(INTRA_MINEXE == 0)
void InterleaveFloatsCastToShorts(Span<short> dst, CSpan<float> src1, CSpan<float> src2)
{
	while(dst.End != dst.Begin)
	{
		*dst.Begin++ = short(*src1.Begin++ * 32767);
		*dst.Begin++ = short(*src2.Begin++ * 32767);
	}
}

void InterleaveFloatsCastToShorts(Span<short> dst, CSpan<float> src1, CSpan<float> src2, CSpan<float> src3)
{
	while(dst.End != dst.Begin)
	{
		*dst.Begin++ = short(*src1.Begin++ * 32767);
		*dst.Begin++ = short(*src2.Begin++ * 32767);
		*dst.Begin++ = short(*src3.Begin++ * 32767);
	}
}

void InterleaveFloatsCastToShorts(Span<short> dst, CSpan<float> src1,
	CSpan<float> src2, CSpan<float> src3, CSpan<float> src4)
{
	while(dst.End != dst.Begin)
	{
		*dst.Begin++ = short(*src1.Begin++ * 32767);
		*dst.Begin++ = short(*src2.Begin++ * 32767);
		*dst.Begin++ = short(*src3.Begin++ * 32767);
		*dst.Begin++ = short(*src4.Begin++ * 32767);
	}
}

void InterleaveFloatsCastToShorts(Span<short> dst, CSpan<float> src1, CSpan<float> src2,
	CSpan<float> src3, CSpan<float> src4, CSpan<float> src5)
{
	while(dst.End != dst.Begin)
	{
		*dst.Begin++ = short(*src1.Begin++ * 32767);
		*dst.Begin++ = short(*src2.Begin++ * 32767);
		*dst.Begin++ = short(*src3.Begin++ * 32767);
		*dst.Begin++ = short(*src4.Begin++ * 32767);
		*dst.Begin++ = short(*src5.Begin++ * 32767);
	}
}

void InterleaveFloatsCastToShorts(Span<short> dst, CSpan<float> src1, CSpan<float> src2,
	CSpan<float> src3, CSpan<float> src4, CSpan<float> src5, CSpan<float> src6)
{
	while(dst.End != dst.Begin)
	{
		*dst.Begin++ = short(*src1.Begin++ * 32767);
		*dst.Begin++ = short(*src2.Begin++ * 32767);
		*dst.Begin++ = short(*src3.Begin++ * 32767);
		*dst.Begin++ = short(*src4.Begin++ * 32767);
		*dst.Begin++ = short(*src5.Begin++ * 32767);
		*dst.Begin++ = short(*src6.Begin++ * 32767);
	}
}

void InterleaveFloatsCastToShorts(Span<short> dst, CSpan<float> src1, CSpan<float> src2,
	CSpan<float> src3, CSpan<float> src4, CSpan<float> src5, CSpan<float> src6, CSpan<float> src7)
{
	while(dst.End != dst.Begin)
	{
		*dst.Begin++ = short(*src1.Begin++ * 32767);
		*dst.Begin++ = short(*src2.Begin++ * 32767);
		*dst.Begin++ = short(*src3.Begin++ * 32767);
		*dst.Begin++ = short(*src4.Begin++ * 32767);
		*dst.Begin++ = short(*src5.Begin++ * 32767);
		*dst.Begin++ = short(*src6.Begin++ * 32767);
		*dst.Begin++ = short(*src7.Begin++ * 32767);
	}
}

void InterleaveFloatsCastToShorts(Span<short> dst, CSpan<float> src1, CSpan<float> src2, CSpan<float> src3,
	CSpan<float> src4, CSpan<float> src5, CSpan<float> src6, CSpan<float> src7, CSpan<float> src8)
{
	while(dst.End != dst.Begin)
	{
		*dst.Begin++ = short(*src1.Begin++ * 32767);
		*dst.Begin++ = short(*src2.Begin++ * 32767);
		*dst.Begin++ = short(*src3.Begin++ * 32767);
		*dst.Begin++ = short(*src4.Begin++ * 32767);
		*dst.Begin++ = short(*src5.Begin++ * 32767);
		*dst.Begin++ = short(*src6.Begin++ * 32767);
		*dst.Begin++ = short(*src7.Begin++ * 32767);
		*dst.Begin++ = short(*src8.Begin++ * 32767);
	}
}
#endif

static void genericInterleaveCast(Span<short> dst, Span<CSpan<float>> src)
{
	CSpan<float> srcs[16];
	auto srcsSpan = Range::Take(srcs, src.CopyTo(srcs));
	while(dst.End != dst.Begin)
		for(auto& srci: srcsSpan)
			*dst.Begin++ = short(*srci.Begin++ * 32767);
}

void InterleaveFloatsCastToShorts(Span<short> dst, Span<CSpan<float>> src)
{
#if(INTRA_MINEXE == 0)
	switch(src.Length())
#endif
	{
#if(INTRA_MINEXE == 0)
	case 2: InterleaveFloatsCastToShorts(dst, src[0], src[1]); return;
	case 3: InterleaveFloatsCastToShorts(dst, src[0], src[1], src[2]); return;
	case 4: InterleaveFloatsCastToShorts(dst, src[0], src[1], src[2], src[3]); return;
	case 5: InterleaveFloatsCastToShorts(dst, src[0], src[1], src[2], src[3], src[4]); return;
	case 6: InterleaveFloatsCastToShorts(dst, src[0], src[1], src[2], src[3], src[4], src[5]); return;
	case 7: InterleaveFloatsCastToShorts(dst, src[0], src[1], src[2], src[3], src[4], src[5], src[6]); return;
	case 8: InterleaveFloatsCastToShorts(dst, src[0], src[1], src[2], src[3], src[4], src[5], src[6], src[7]); return;
	default:
#endif
		genericInterleaveCast(dst, src);
	}
}


#if(INTRA_MINEXE == 0)
void DeinterleaveFloatsCastToShorts(CSpan<float> src, Span<short> dst1, Span<short> dst2)
{
	while(src.End != src.Begin)
	{
		*dst1.Begin++ = short(*src.Begin++ * 32767);
		*dst2.Begin++ = short(*src.Begin++ * 32767);
	}
}

void DeinterleaveFloatsCastToShorts(CSpan<float> src,
	Span<short> dst1, Span<short> dst2, Span<short> dst3)
{
	while(src.End != src.Begin)
	{
		*dst1.Begin++ = short(*src.Begin++ * 32767);
		*dst2.Begin++ = short(*src.Begin++ * 32767);
		*dst3.Begin++ = short(*src.Begin++ * 32767);
	}
}

void DeinterleaveFloatsCastToShorts(CSpan<float> src, Span<short> dst1,
	Span<short> dst2, Span<short> dst3, Span<short> dst4)
{
	while(src.End != src.Begin)
	{
		*dst1.Begin++ = short(*src.Begin++ * 32767);
		*dst2.Begin++ = short(*src.Begin++ * 32767);
		*dst3.Begin++ = short(*src.Begin++ * 32767);
		*dst4.Begin++ = short(*src.Begin++ * 32767);
	}
}

void DeinterleaveFloatsCastToShorts(CSpan<float> src, Span<short> dst1,
	Span<short> dst2, Span<short> dst3, Span<short> dst4, Span<short> dst5)
{
	while(src.End != src.Begin)
	{
		*dst1.Begin++ = short(*src.Begin++ * 32767);
		*dst2.Begin++ = short(*src.Begin++ * 32767);
		*dst3.Begin++ = short(*src.Begin++ * 32767);
		*dst4.Begin++ = short(*src.Begin++ * 32767);
		*dst5.Begin++ = short(*src.Begin++ * 32767);
	}
}

void DeinterleaveFloatsCastToShorts(CSpan<float> src, Span<short> dst1, Span<short> dst2,
	Span<short> dst3, Span<short> dst4, Span<short> dst5, Span<short> dst6)
{
	while(src.End != src.Begin)
	{
		*dst1.Begin++ = short(*src.Begin++ * 32767);
		*dst2.Begin++ = short(*src.Begin++ * 32767);
		*dst3.Begin++ = short(*src.Begin++ * 32767);
		*dst4.Begin++ = short(*src.Begin++ * 32767);
		*dst5.Begin++ = short(*src.Begin++ * 32767);
		*dst6.Begin++ = short(*src.Begin++ * 32767);
	}
}

void DeinterleaveFloatsCastToShorts(CSpan<float> src, Span<short> dst1, Span<short> dst2,
	Span<short> dst3, Span<short> dst4, Span<short> dst5, Span<short> dst6, Span<short> dst7)
{
	while(src.End != src.Begin)
	{
		*dst1.Begin++ = short(*src.Begin++ * 32767);
		*dst2.Begin++ = short(*src.Begin++ * 32767);
		*dst3.Begin++ = short(*src.Begin++ * 32767);
		*dst4.Begin++ = short(*src.Begin++ * 32767);
		*dst5.Begin++ = short(*src.Begin++ * 32767);
		*dst6.Begin++ = short(*src.Begin++ * 32767);
		*dst7.Begin++ = short(*src.Begin++ * 32767);
	}
}

void DeinterleaveFloatsCastToShorts(CSpan<float> src, Span<short> dst1, Span<short> dst2,
	Span<short> dst3, Span<short> dst4, Span<short> dst5, Span<short> dst6, Span<short> dst7, Span<short> dst8)
{
	while(src.End != src.Begin)
	{
		*dst1.Begin++ = short(*src.Begin++ * 32767);
		*dst2.Begin++ = short(*src.Begin++ * 32767);
		*dst3.Begin++ = short(*src.Begin++ * 32767);
		*dst4.Begin++ = short(*src.Begin++ * 32767);
		*dst5.Begin++ = short(*src.Begin++ * 32767);
		*dst6.Begin++ = short(*src.Begin++ * 32767);
		*dst7.Begin++ = short(*src.Begin++ * 32767);
		*dst8.Begin++ = short(*src.Begin++ * 32767);
	}
}
#endif

static void genericDeinterleaveCastToShorts(CSpan<float> src, Span<Span<short>> dst)
{
	Span<short> dsts[16];
	auto dstsSpan = Range::Take(dsts, dst.CopyTo(dsts));
	while(src.End != src.Begin)
		for(auto& dsti: dstsSpan)
			*dsti.Begin++ = short(*src.Begin++ * 32767);
}

void DeinterleaveFloatsCastToShorts(CSpan<float> src, Span<Span<short>> dst)
{
#if(INTRA_MINEXE == 0)
	switch(src.Length())
#endif
	{
#if(INTRA_MINEXE == 0)
	case 2: DeinterleaveFloatsCastToShorts(src, dst[0], dst[1]); break;
	case 3: DeinterleaveFloatsCastToShorts(src, dst[0], dst[1], dst[2]); break;
	case 4: DeinterleaveFloatsCastToShorts(src, dst[0], dst[1], dst[2], dst[3]); break;
	case 5: DeinterleaveFloatsCastToShorts(src, dst[0], dst[1], dst[2], dst[3], dst[4]); break;
	case 6: DeinterleaveFloatsCastToShorts(src, dst[0], dst[1], dst[2], dst[3], dst[4], dst[5]); break;
	case 7: DeinterleaveFloatsCastToShorts(src, dst[0], dst[1], dst[2], dst[3], dst[4], dst[5], dst[6]); break;
	case 8: DeinterleaveFloatsCastToShorts(src, dst[0], dst[1], dst[2], dst[3], dst[4], dst[5], dst[6], dst[7]); break;
	default:
#endif
		genericDeinterleaveCastToShorts(src, dst);
	}
}


#if(INTRA_MINEXE == 0)
void DeinterleaveShortsCastToFloats(CSpan<short> src, Span<float> dst1, Span<float> dst2)
{
	while(src.End != src.Begin)
	{
		*dst1.Begin++ = *src.Begin++ / 32768.0f;
		*dst2.Begin++ = *src.Begin++ / 32768.0f;
	}
}

void DeinterleaveShortsCastToFloats(CSpan<short> src,
	Span<float> dst1, Span<float> dst2, Span<float> dst3)
{
	while(src.End != src.Begin)
	{
		*dst1.Begin++ = *src.Begin++ / 32768.0f;
		*dst2.Begin++ = *src.Begin++ / 32768.0f;
		*dst3.Begin++ = *src.Begin++ / 32768.0f;
	}
}

void DeinterleaveShortsCastToFloats(CSpan<short> src,
	Span<float> dst1, Span<float> dst2, Span<float> dst3, Span<float> dst4)
{
	while(src.End != src.Begin)
	{
		*dst1.Begin++ = *src.Begin++ / 32768.0f;
		*dst2.Begin++ = *src.Begin++ / 32768.0f;
		*dst3.Begin++ = *src.Begin++ / 32768.0f;
		*dst4.Begin++ = *src.Begin++ / 32768.0f;
	}
}

void DeinterleaveShortsCastToFloats(CSpan<short> src, Span<float> dst1,
	Span<float> dst2, Span<float> dst3, Span<float> dst4, Span<float> dst5)
{
	while(src.End != src.Begin)
	{
		*dst1.Begin++ = *src.Begin++ / 32768.0f;
		*dst2.Begin++ = *src.Begin++ / 32768.0f;
		*dst3.Begin++ = *src.Begin++ / 32768.0f;
		*dst4.Begin++ = *src.Begin++ / 32768.0f;
		*dst5.Begin++ = *src.Begin++ / 32768.0f;
	}
}

void DeinterleaveShortsCastToFloats(CSpan<short> src, Span<float> dst1,
	Span<float> dst2, Span<float> dst3, Span<float> dst4, Span<float> dst5, Span<float> dst6)
{
	while(src.End != src.Begin)
	{
		*dst1.Begin++ = *src.Begin++ / 32768.0f;
		*dst2.Begin++ = *src.Begin++ / 32768.0f;
		*dst3.Begin++ = *src.Begin++ / 32768.0f;
		*dst4.Begin++ = *src.Begin++ / 32768.0f;
		*dst5.Begin++ = *src.Begin++ / 32768.0f;
		*dst6.Begin++ = *src.Begin++ / 32768.0f;
	}
}

void DeinterleaveShortsCastToFloats(CSpan<short> src, Span<float> dst1, Span<float> dst2,
	Span<float> dst3, Span<float> dst4, Span<float> dst5, Span<float> dst6, Span<float> dst7)
{
	while(src.End != src.Begin)
	{
		*dst1.Begin++ = *src.Begin++ / 32768.0f;
		*dst2.Begin++ = *src.Begin++ / 32768.0f;
		*dst3.Begin++ = *src.Begin++ / 32768.0f;
		*dst4.Begin++ = *src.Begin++ / 32768.0f;
		*dst5.Begin++ = *src.Begin++ / 32768.0f;
		*dst6.Begin++ = *src.Begin++ / 32768.0f;
		*dst7.Begin++ = *src.Begin++ / 32768.0f;
	}
}

void DeinterleaveShortsCastToFloats(CSpan<short> src, Span<float> dst1, Span<float> dst2,
	Span<float> dst3, Span<float> dst4, Span<float> dst5, Span<float> dst6, Span<float> dst7, Span<float> dst8)
{
	while(src.End != src.Begin)
	{
		*dst1.Begin++ = *src.Begin++ / 32768.0f;
		*dst2.Begin++ = *src.Begin++ / 32768.0f;
		*dst3.Begin++ = *src.Begin++ / 32768.0f;
		*dst4.Begin++ = *src.Begin++ / 32768.0f;
		*dst5.Begin++ = *src.Begin++ / 32768.0f;
		*dst6.Begin++ = *src.Begin++ / 32768.0f;
		*dst7.Begin++ = *src.Begin++ / 32768.0f;
		*dst8.Begin++ = *src.Begin++ / 32768.0f;
	}
}
#endif

static void genericDeinterleaveCastToFloats(CSpan<short> src, Span<Span<float>> dst)
{
	Span<float> dsts[16];
	auto dstsSpan = Range::Take(dsts, dst.CopyTo(dsts));
	while(src.End != src.Begin)
		for(auto& dsti: dstsSpan)
			*dsti.Begin++ = *src.Begin++ / 32768.0f;
}

void DeinterleaveShortsCastToFloats(CSpan<short> src, Span<Span<float>> dst)
{
#if(INTRA_MINEXE == 0)
	switch(src.Length())
#endif
	{
#if(INTRA_MINEXE == 0)
	case 2: DeinterleaveShortsCastToFloats(src, dst[0], dst[1]); return;
	case 3: DeinterleaveShortsCastToFloats(src, dst[0], dst[1], dst[2]); return;
	case 4: DeinterleaveShortsCastToFloats(src, dst[0], dst[1], dst[2], dst[3]); return;
	case 5: DeinterleaveShortsCastToFloats(src, dst[0], dst[1], dst[2], dst[3], dst[4]); return;
	case 6: DeinterleaveShortsCastToFloats(src, dst[0], dst[1], dst[2], dst[3], dst[4], dst[5]); return;
	case 7: DeinterleaveShortsCastToFloats(src, dst[0], dst[1], dst[2], dst[3], dst[4], dst[5], dst[6]); return;
	case 8: DeinterleaveShortsCastToFloats(src, dst[0], dst[1], dst[2], dst[3], dst[4], dst[5], dst[6], dst[7]); return;
	default:
#endif
		genericDeinterleaveCastToFloats(src, dst);
	}
}

}}
