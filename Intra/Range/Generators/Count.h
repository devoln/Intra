#pragma once

#include "Concepts/Range.h"
#include "Cpp/Warnings.h"

namespace Intra { namespace Range {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename T> struct CountRange
{
	enum: bool {RangeIsInfinite = true};

	forceinline CountRange(null_t=null): Counter(0) {}
	forceinline CountRange(size_t counter): Counter(counter) {}

	forceinline bool Empty() const {return false;}
	forceinline const T& First() const {static const T empty; return empty;}
	forceinline void PopFirst() {Counter++;}

	forceinline void Put(const T&) {Counter++;}

	forceinline bool operator==(const CountRange& rhs) const {return Counter==rhs.Counter;}

	size_t Counter;
};

INTRA_WARNING_POP

}}
