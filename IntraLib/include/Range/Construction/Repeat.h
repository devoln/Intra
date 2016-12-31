#pragma once

#include "Range/Concepts.h"
#include "Range/ForwardDecls.h"
#include "Utils/Optional.h"
#include "Take.h"

namespace Intra { namespace Range {

INTRA_WARNING_PUSH_DISABLE_COPY_MOVE_IMPLICITLY_DELETED

template<typename T> struct RRepeat
{
	enum: bool {RangeIsInfinite = true};

	forceinline RRepeat(null_t=null): mValue(null) {}
	forceinline RRepeat(T&& val): mValue(Meta::Move(val)) {}
	forceinline RRepeat(const T& val): mValue(val) {}

	forceinline bool Empty() const {return mValue==null;}
	forceinline const T& First() const {return mValue();}

	forceinline void PopFirst() {}
	const T& operator[](size_t) const {return mValue();}

	forceinline bool operator==(const RRepeat& rhs) const
	{
		if(mValue==null && rhs.mValue==null) return true;
		if(mValue==null && rhs.mValue!=null || mValue!=null && rhs.mValue==null) return false;
		return mValue()==rhs.mValue();
	}
	
private:
	Utils::Optional<T> mValue;
};

INTRA_WARNING_POP

template<typename T> RRepeat<T> Repeat(T&& val)
{return {Meta::Forward<T>(val)};}

template<typename T> RTake<RRepeat<T>> Repeat(T&& val, size_t n)
{return Take(Repeat(Meta::Forward<T>(val)), n);}

template<typename T, size_t N> RRepeat<AsRangeResult<T(&)[N]>> Repeat(T(&arr)[N])
{return Repeat(AsRange(arr));}

template<typename T, size_t N> RTake<RRepeat<AsRangeResult<T(&)[N]>>> Repeat(T(&arr)[N], size_t n)
{return Take(Repeat(AsRange(arr)), n);}

}}
