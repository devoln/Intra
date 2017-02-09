#pragma once

#include "Range/ForwardDecls.h"
#include "IteratorConcepts.h"

namespace Intra { namespace Range {

template<typename I1, typename I2> struct IteratorRange
{
	I1 Begin;
	I2 End;

	bool Empty() const {return Begin==End;}
	void PopFirst() {INTRA_ASSERT(!Empty()); ++Begin;}
	decltype(*Meta::Val<I1>()) First() {INTRA_ASSERT(!Empty()); return *Begin;}

	I1 begin() const {return Begin;}
	I1 end() const {return End;}
};

}}
