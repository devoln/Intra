#pragma once

#include "Concepts/Range.h"
#include "Cpp/Warnings.h"

namespace Intra { namespace Range {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename T> struct NullRange
{
	forceinline T* begin() const {return null;}
	forceinline T* end() const {return null;}

	forceinline NullRange(null_t=null) {}
	forceinline bool Empty() const {return true;}
	forceinline size_t Length() const {return 0;}
	forceinline T First() const {INTRA_DEBUG_ASSERT(false); return T();}
	forceinline T Last() const {INTRA_DEBUG_ASSERT(false); return T();}
	forceinline void PopFirst() {INTRA_DEBUG_ASSERT(false);}
	forceinline void PopLast() {INTRA_DEBUG_ASSERT(false);}
	forceinline T operator[](size_t) {INTRA_DEBUG_ASSERT(false); return T();}
	
	NullRange operator()(size_t startIndex, size_t endIndex) const
	{
		(void)startIndex; (void)endIndex;
		INTRA_DEBUG_ASSERT(startIndex==0 && endIndex==0);
		return NullRange();
	}

	forceinline void Put(const T&) {}
};

INTRA_WARNING_POP

}}
