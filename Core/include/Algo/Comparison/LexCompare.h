#pragma once

#include "Platform/PlatformInfo.h"
#include "Platform/CppWarnings.h"
#include "Range/Concepts.h"
#include "Algo/Op.h"

namespace Intra { namespace Algo {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename R1, typename R2, typename P> Meta::EnableIf<
	Range::IsForwardRange<R1>::_ &&
	Range::IsForwardRange<R2>::_ &&
	Meta::IsCallable<P, Range::ValueTypeOf<R1>, Range::ValueTypeOf<R2>>::_,
int> LexCompare(const R1& r1, const R2& r2, P pred)
{
	R1 range1 = r1;
	R2 range2 = r2;
	while(!range1.Empty() && !range2.Empty())
	{
		if(pred(range1.First(), range2.First())) return -1;
		if(pred(range2.First(), range1.First())) return 1;
	}
	if(range1.Empty())
	{
		if(range2.Empty()) return 0;
		return -1;
	}
	return 1;
}

template<typename R1, typename R2> inline Meta::EnableIf<
	(Meta::IsIntegralType<Range::ValueTypeOf<R1>>::_ ||
		Meta::IsCharType<Range::ValueTypeOf<R1>>::_) &&
	Range::ValueTypeEquals<R1, R2>::_ &&
	Range::IsArrayRange<R1>::_ &&
	Range::IsArrayRange<R2>::_ &&
	(sizeof(Range::ValueTypeOf<R1>)==1 ||
		INTRA_PLATFORM_ENDIANESS==INTRA_PLATFORM_ENDIANESS_BigEndian),
int> LexCompare(const R1& r1, const R2& r2)
{
	size_t minLen = Op::Min(r1.Length(), r2.Length());
	int result = C::memcmp(r1.Data(), r2.Data(), minLen*sizeof(r1.First()));
	if(result!=0) return result;
	if(r1.Length()<r2.Length()) return -1;
	if(r1.Length()>r2.Length()) return 1;
	return 0;
}

template<typename R1, typename R2> Meta::EnableIf<
	Range::IsForwardRange<R1>::_ &&
	Range::IsForwardRange<R2>::_ &&
	!((Meta::IsIntegralType<Range::ValueTypeOf<R1>>::_ ||
		Meta::IsCharType<Range::ValueTypeOf<R1>>::_) &&
	Range::ValueTypeEquals<R1, R2>::_ &&
	Range::IsArrayRange<R1>::_ &&
	Range::IsArrayRange<R2>::_ &&
	(sizeof(Range::ValueTypeOf<R1>)==1 ||
		INTRA_PLATFORM_ENDIANESS==INTRA_PLATFORM_ENDIANESS_BigEndian)),
int> LexCompare(const R1& r1, const R2& r2)
{return LexCompare(r1, r2, Op::Less<Range::ValueTypeOf<R1>, Range::ValueTypeOf<R2>>);}

INTRA_WARNING_POP

}}
