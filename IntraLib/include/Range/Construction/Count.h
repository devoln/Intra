#pragma once

#include "Range/Mixins/RangeMixins.h"

namespace Intra { namespace Range {

template<typename T> struct CountRange:
	RangeMixin<CountRange<T>, T, TypeEnum::Forward, false>
{
	typedef T value_type;
	typedef const T& return_value_type;

	CountRange(size_t counter=0): Counter(counter) {}
	bool Empty() {return false;}
	const T& First() const {static const T empty; return empty;}
	void PopFirst() {Counter++;}
	void Put(const T&) {Counter++;}

	bool operator==(const CountRange& rhs) const {return Counter==rhs.Counter;}

	size_t Counter;
};

}}
