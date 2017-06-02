#pragma once

#include "Cpp/PlatformDetect.h"
#include "Cpp/Warnings.h"

#include "Utils/Op.h"
#include "Utils/ArrayAlgo.h"
#include "Utils/StringView.h"

#include "Concepts/Range.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Range {

template<typename R1, typename R2, typename P> Meta::EnableIf<
	Concepts::IsForwardRange<R1>::_ &&
	Concepts::IsForwardRange<R2>::_ &&
	Meta::IsCallable<P, Concepts::ValueTypeOf<R1>, Concepts::ValueTypeOf<R2>>::_,
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

template<typename R1, typename R2> Meta::EnableIf<
	Concepts::IsForwardRange<R1>::_ &&
	Concepts::IsForwardRange<R2>::_ &&
	!((Meta::IsIntegralType<Concepts::ValueTypeOf<R1>>::_ ||
		Meta::IsCharType<Concepts::ValueTypeOf<R1>>::_) &&
		Meta::TypeEquals<Concepts::ElementTypeOfArray<R1>, Concepts::ElementTypeOfArray<R2>>::_ &&
		Concepts::IsArrayClass<R1>::_ &&
		Concepts::IsArrayClass<R2>::_ &&
	(sizeof(Concepts::ElementTypeOfArray<R1>)==1 ||
		INTRA_PLATFORM_ENDIANESS == INTRA_PLATFORM_ENDIANESS_BigEndian)),
int> LexCompare(const R1& r1, const R2& r2)
{return LexCompare(r1, r2, Op::Less<Concepts::ValueTypeOf<R1>, Concepts::ValueTypeOf<R2>>);}

}}

INTRA_WARNING_POP
