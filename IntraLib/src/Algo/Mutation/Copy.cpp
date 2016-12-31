#include "Algo/Mutation/Copy.h"
#include "Range/ArrayRange.h"
#include "Math/Simd.h"

namespace Intra { namespace Algo {

void CastAdvanceToAdvance(ArrayRange<const float>& src, ArrayRange<short>& dst)
{
	INTRA_ASSERT(dst.Length()>=src.Length());
#if(INTRA_MIN_SIMD_SUPPORT>=INTRA_SIMD_SSE && INTRA_PLATFORM_ARCH==INTRA_PLATFORM_X86)
	while(src.Begin<src.End-3)
	{
		*(__m64*)dst.Begin = _mm_cvtps_pi16(Simd::SetFloat4U(src.Begin));
		src.Begin+=4;
		dst.Begin+=4;
	}
	_mm_empty();
#else
	while(src.Begin<src.End-3)
	{
		*dst.Begin++ = short(*src.Begin++);
		*dst.Begin++ = short(*src.Begin++);
		*dst.Begin++ = short(*src.Begin++);
		*dst.Begin++ = short(*src.Begin++);
	}
#endif
	while(src.Begin<src.End)
	{
		//INTRA_ASSERT(*src>=-32768.0f && *src<=32767.0f);
		*dst.Begin++ = short(*src.Begin++);
	}
}

}}