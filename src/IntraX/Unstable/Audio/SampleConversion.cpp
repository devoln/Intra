#include "SampleConversion.h"

namespace Intra { INTRA_BEGIN
void InterleaveFloats(Span<float> dst, Span<const float> src1, Span<const float> src2)
{
	while(dst.End != dst.Begin)
	{
		*dst.Begin++ = *src1.Begin++;
		*dst.Begin++ = *src2.Begin++;
	}
}

template<typename T> void genericInterleave(Span<T> dst, Span<Span<const T>> src)
{
	Span<const T> srcs[16];
	auto srcsSpan = Take(srcs, src.CopyTo(srcs));
	while(dst.End != dst.Begin)
		for(auto& srci: srcsSpan)
			*dst.Begin++ = *srci.Begin++;
}

void InterleaveFloats(Span<float> dst, Span<Span<const float>> src)
{
	switch(src.Length())
	{
	case 2:
#if(INTRA_MINEXE == 0)
		InterleaveFloats(dst, src[0], src[1]); break;
#endif
	default:
		genericInterleave(dst, src);
	}
}


void InterleaveShorts(Span<short> dst, Span<const short> src1, Span<const short> src2)
{
	while(dst.End != dst.Begin)
	{
		*dst.Begin++ = *src1.Begin++;
		*dst.Begin++ = *src2.Begin++;
	}
}

void InterleaveShorts(Span<short> dst, Span<Span<const short>> src)
{
	switch(src.Length())
	{
	case 2:
#if(INTRA_MINEXE == 0)
		InterleaveShorts(dst, src[0], src[1]); break;
#endif
	default:
		genericInterleave(dst, src);
	}
}


void DeinterleaveFloats(Span<const float> src, Span<float> dst1, Span<float> dst2)
{
	while(src.End != src.Begin)
	{
		*dst1.Begin++ = *src.Begin++;
		*dst2.Begin++ = *src.Begin++;
	}
}

template<typename T> void genericDeinterleave(Span<const T> src, Span<Span<T>> dst)
{
	Span<T> dsts[16];
	auto dstsSpan = Take(dsts, dst.CopyTo(dsts));
	while(src.End != src.Begin)
		for(auto& dsti: dstsSpan)
			*dsti.Begin++ = *src.Begin++;
}

void DeinterleaveFloats(Span<const float> src, Span<Span<float>> dst)
{
	switch(src.Length())
	{
	case 2:
#if(INTRA_MINEXE == 0)
		DeinterleaveFloats(src, dst[0], dst[1]); break;
#endif
	default:
		genericDeinterleave(src, dst);
	}
}


void DeinterleaveShorts(Span<const short> src, Span<short> dst1, Span<short> dst2)
{
	while(src.End != src.Begin)
	{
		*dst1.Begin++ = *src.Begin++;
		*dst2.Begin++ = *src.Begin++;
	}
}

void DeinterleaveShorts(Span<const short> src, Span<Span<short>> dst)
{
	switch(src.Length())
	{
	case 2:
#if(INTRA_MINEXE == 0)
		DeinterleaveShorts(src, dst[0], dst[1]); break;
#endif
	default:
		genericDeinterleave(src, dst);
	}
}


void InterleaveFloatsCastToShorts(Span<short> dst, Span<const float> src1, Span<const float> src2)
{
	while(dst.End > dst.Begin + 1)
	{
		*dst.Begin++ = short(*src1.Begin++ * 32767);
		*dst.Begin++ = short(*src2.Begin++ * 32767);
	}
}

static void genericInterleaveCast(Span<short> dst, Span<Span<const float>> src)
{
	Span<const float> srcs[16];
	auto srcsSpan = Take(srcs, src.CopyTo(srcs));
	while(dst.End != dst.Begin)
		for(auto& srci: srcsSpan)
			*dst.Begin++ = short(*srci.Begin++ * 32767);
}

void InterleaveFloatsCastToShorts(Span<short> dst, Span<Span<const float>> src)
{
	switch(src.Length())
	{
	case 2:
#if(INTRA_MINEXE == 0)
		InterleaveFloatsCastToShorts(dst, src[0], src[1]); return;
#endif
	default:
		genericInterleaveCast(dst, src);
	}
}


void DeinterleaveFloatsCastToShorts(Span<const float> src, Span<short> dst1, Span<short> dst2)
{
	while(src.End != src.Begin)
	{
		*dst1.Begin++ = short(*src.Begin++ * 32767);
		*dst2.Begin++ = short(*src.Begin++ * 32767);
	}
}

static void genericDeinterleaveCastToShorts(Span<const float> src, Span<Span<short>> dst)
{
	Span<short> dsts[16];
	auto dstsSpan = Take(dsts, dst.CopyTo(dsts));
	while(src.End != src.Begin)
		for(auto& dsti: dstsSpan)
			*dsti.Begin++ = short(*src.Begin++ * 32767);
}

void DeinterleaveFloatsCastToShorts(Span<const float> src, Span<Span<short>> dst)
{
	switch(src.Length())
	{
	case 2:
#if(INTRA_MINEXE == 0)
		DeinterleaveFloatsCastToShorts(src, dst[0], dst[1]); break;
#endif
	default:
		genericDeinterleaveCastToShorts(src, dst);
	}
}


void DeinterleaveShortsCastToFloats(Span<const short> src, Span<float> dst1, Span<float> dst2)
{
	while(src.End != src.Begin)
	{
		*dst1.Begin++ = *src.Begin++ / 32768.0f;
		*dst2.Begin++ = *src.Begin++ / 32768.0f;
	}
}

static void genericDeinterleaveCastToFloats(Span<const short> src, Span<Span<float>> dst)
{
	Span<float> dsts[16];
	auto dstsSpan = Take(dsts, dst.CopyTo(dsts));
	while(src.End != src.Begin)
		for(auto& dsti: dstsSpan)
			*dsti.Begin++ = *src.Begin++ / 32768.0f;
}

void DeinterleaveShortsCastToFloats(Span<const short> src, Span<Span<float>> dst)
{
	switch(src.Length())
	{
	case 2:
#if(INTRA_MINEXE == 0)
		DeinterleaveShortsCastToFloats(src, dst[0], dst[1]); return;
#endif
	default:
		genericDeinterleaveCastToFloats(src, dst);
	}
}
} INTRA_END
