#pragma once

#include "Cpp/Features.h"
#include "Debug.h"

#ifndef INTRA_UTILS_NO_CONCEPTS
#include "Concepts/Iterator.h"
#endif

namespace Intra {

namespace Range {

template<typename I1, typename I2> struct IteratorRange
{
#ifndef INTRA_UTILS_NO_CONCEPTS
	static_assert(Concepts::IsMinimalInputIterator<I1>::_,
		"I1 must implement at least minimal input iterator concept!");
#endif

	I1 Begin;
	I2 End;

	forceinline bool Empty() const {return Begin == End;}
	forceinline void PopFirst() {INTRA_DEBUG_ASSERT(!Empty()); ++Begin;}
	forceinline decltype(*Meta::Val<I1>()) First() const {INTRA_DEBUG_ASSERT(!Empty()); return *Begin;}

#ifndef INTRA_UTILS_NO_CONCEPTS
	template<typename U=I2> forceinline Meta::EnableIf<
		Concepts::IsMinimalBidirectionalIterator<U>::_
	> PopLast() {INTRA_DEBUG_ASSERT(!Empty()); --End;}

	template<typename U=I2> forceinline Meta::EnableIf<
		Concepts::IsMinimalBidirectionalIterator<U>::_,
	decltype(*Meta::Val<I2>())> Last() const {INTRA_DEBUG_ASSERT(!Empty()); return *--I2(End);}

	template<typename U=I2> forceinline Meta::EnableIf<
		Concepts::HasDifference<U, I1>::_,
	size_t> Length() const {INTRA_DEBUG_ASSERT(!Empty()); return size_t(End-Begin);}
#endif

	forceinline I1 begin() const {return Begin;}
	forceinline I2 end() const {return End;}
};

}

namespace Utils {
using Range::IteratorRange;
}
using Range::IteratorRange;

}
