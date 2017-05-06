#pragma once

#include "Concepts/Range.h"
#include "Range/ForwardDecls.h"
#include "Utils/Optional.h"
#include "Range/Decorators/Take.h"
#include "Cpp/Warnings.h"
#include "Utils/Span.h"

namespace Intra { namespace Range {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED

template<typename T> struct RRepeat
{
	enum: bool {RangeIsInfinite = true};

	forceinline RRepeat(null_t=null): mValue(null) {}
	forceinline RRepeat(T&& val): mValue(Cpp::Move(val)) {}
	forceinline RRepeat(const T& val): mValue(val) {}

	forceinline bool Empty() const {return mValue==null;}
	forceinline const T& First() const {return mValue();}

	forceinline void PopFirst() {}
	const T& operator[](size_t) const {return mValue();}

	forceinline bool operator==(const RRepeat& rhs) const
	{
		if(mValue==null && rhs.mValue==null) return true;
		if((mValue==null && rhs.mValue!=null) ||
			(mValue!=null && rhs.mValue==null)) return false;
		return mValue()==rhs.mValue();
	}
	
private:
	Utils::Optional<T> mValue;
};


template<typename T> forceinline RRepeat<T> Repeat(T&& val) noexcept {return {Cpp::Forward<T>(val)};}

template<typename T> forceinline RTake<RRepeat<T>> Repeat(T&& val, size_t n) noexcept {return Take(Repeat(Cpp::Forward<T>(val)), n);}

template<typename T, size_t N> forceinline RRepeat<Span<T>> Repeat(T(&arr)[N]) noexcept {return Repeat(SpanOf(arr));}

template<typename T, size_t N> forceinline RTake<RRepeat<Span<T>>> Repeat(T(&arr)[N], size_t n) noexcept {return Take(Repeat(SpanOf(arr)), n);}

INTRA_WARNING_POP

}}
