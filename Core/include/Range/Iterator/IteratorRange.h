#pragma once

#include "Range/ForwardDecls.h"
#include "IteratorConcepts.h"
#include "Platform/Debug.h"

namespace Intra { namespace Range {

template<typename I1, typename I2> struct IteratorRange
{
	static_assert(IteratorConcepts::IsMinimalInputIterator<I1>::_,
		"I1 must implement at least minimal input iterator concept!");

	I1 Begin;
	I2 End;

	bool Empty() const {return Begin==End;}
	void PopFirst() {INTRA_DEBUG_ASSERT(!Empty()); ++Begin;}
	decltype(*Meta::Val<I1>()) First() const {INTRA_DEBUG_ASSERT(!Empty()); return *Begin;}

	template<typename U=I2> forceinline Meta::EnableIf<
		IteratorConcepts::IsMinimalBidirectionalIterator<U>::_
	> PopLast() {INTRA_DEBUG_ASSERT(!Empty()); --End;}

	template<typename U=I2> forceinline Meta::EnableIf<
		IteratorConcepts::IsMinimalBidirectionalIterator<U>::_,
	decltype(*Meta::Val<I2>())> Last() const {INTRA_DEBUG_ASSERT(!Empty()); return *--I2(End);}

	template<typename U=I2> forceinline Meta::EnableIf<
		IteratorConcepts::HasDifference<U, I1>::_,
	size_t> Length() const {INTRA_DEBUG_ASSERT(!Empty()); return size_t(End-Begin);}

	I1 begin() const {return Begin;}
	I2 end() const {return End;}
};

}}
