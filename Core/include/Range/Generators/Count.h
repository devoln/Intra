#pragma once

#include "Range/Concepts.h"
#include "Platform/CppWarnings.h"

namespace Intra { namespace Range {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename T> struct CountRange
{
	enum: bool {RangeIsInfinite = true};

	CountRange(null_t=null): Counter(0) {}
	CountRange(size_t counter): Counter(counter) {}

	bool Empty() const {return false;}
	const T& First() const {static const T empty; return empty;}
	void PopFirst() {Counter++;}

	void Put(const T&) {Counter++;}

	bool operator==(const CountRange& rhs) const {return Counter==rhs.Counter;}

	size_t Counter;
};

INTRA_WARNING_POP

}}
