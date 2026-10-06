#include <Simd/SimdVector.h>

#include "Range/Mutation/Copy.h"
#include "Utils/Span.h"

namespace Intra { namespace Range {

void CastAdvanceToAdvance(CSpan<float>& src, Span<short>& dst)
{
	INTRA_DEBUG_ASSERT(dst.Length() >= src.Length());
	while(src.End - src.Begin >= 4)
	{
		const auto converted = SimdCastTo<SimdVector<int16, 4>>(SimdLoad<4>(src.Begin));
		SimdStore(dst.Begin, converted);
		src.Begin += 4;
		dst.Begin += 4;
	}
	while(src.Begin < src.End)
	{
		//INTRA_DEBUG_ASSERT(*src>=-32768.0f && *src<=32767.0f);
		*dst.Begin++ = short(*src.Begin++);
	}
}

}}
