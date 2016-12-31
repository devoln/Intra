#pragma once

#include "Range/Concepts.h"

namespace Intra { namespace Range {

template<typename T> struct CountRange
{
	enum: bool {RangeIsInfinite = true};

	CountRange(null_t=null): Counter(0) {}
	CountRange(size_t counter): Counter(counter) {}
	bool Empty() {return false;}
	const T& First() const {static const T empty; return empty;}
	void PopFirst() {Counter++;}
	void Put(const T&) {Counter++;}

	bool operator==(const CountRange& rhs) const {return Counter==rhs.Counter;}

	size_t Counter;
};

}}
