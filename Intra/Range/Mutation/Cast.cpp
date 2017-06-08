#include "Range/Mutation/Copy.h"
#include "Utils/Span.h"
#include "Simd/Simd.h"

namespace Intra { namespace Range {

void CastAdvanceToAdvance(CSpan<float>& src, Span<short>& dst)
{
	INTRA_DEBUG_ASSERT(dst.Length() >= src.Length());
#if(INTRA_SIMD_SUPPORT >= INTRA_SIMD_SSE && INTRA_PLATFORM_ARCH == INTRA_PLATFORM_X86)
	while(src.Begin<src.End-3)
	{
		*(__m64*)dst.Begin = _mm_cvtps_pi16(Simd::SetFloat4U(src.Begin));
		src.Begin+=4;
		dst.Begin+=4;
	}
	_mm_empty();
#else
	while(src.Begin + 3 < src.End)
	{
		*dst.Begin++ = short(*src.Begin++);
		*dst.Begin++ = short(*src.Begin++);
		*dst.Begin++ = short(*src.Begin++);
		*dst.Begin++ = short(*src.Begin++);
	}
#endif
	while(src.Begin < src.End)
	{
		//INTRA_DEBUG_ASSERT(*src>=-32768.0f && *src<=32767.0f);
		*dst.Begin++ = short(*src.Begin++);
	}
}

}}
