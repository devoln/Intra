#pragma once

#include "Range/Concepts.h"
#include "Platform/CppWarnings.h"

namespace Intra { namespace Range {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename T> struct NullRange
{
	T* begin() const {return null;}
	T* end() const {return null;}

	NullRange(null_t=null) {}
	bool Empty() const {return true;}
	size_t Length() const {return 0;}
	T First() const {INTRA_ASSERT(false); return T();}
	T Last() const {INTRA_ASSERT(false); return T();}
	void PopFirst() {INTRA_ASSERT(false);}
	void PopLast() {INTRA_ASSERT(false);}
	T operator[](size_t) {INTRA_ASSERT(false); return T();}
	
	NullRange operator()(size_t startIndex, size_t endIndex) const
	{
		(void)startIndex; (void)endIndex;
		INTRA_ASSERT(startIndex==0 && endIndex==0);
		return NullRange();
	}

	void Put(const T&) {}
};

INTRA_WARNING_POP

}}
