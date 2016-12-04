#pragma once

#include "Range/Mixins/RangeMixins.h"
#include "Utils/Optional.h"

namespace Intra { namespace Range {

template<typename T> struct RepeatResult:
	RangeMixin<RepeatResult<T>, T, TypeEnum::RandomAccess, false>
{
	typedef T value_type;
	typedef const T& return_value_type;

	RepeatResult(null_t=null): value(null) {}
	RepeatResult(T&& val): value(core::move(val)) {}
	RepeatResult(const T& val): value(val) {}

	forceinline bool Empty() const {return value==null;}
	forceinline const T& First() const {return value();}

	forceinline void PopFirst() {}
	const T& operator[](size_t) const {return value();}

	forceinline bool operator==(const RepeatResult& rhs) const
	{
		if(value==null && rhs.value==null) return true;
		if(value==null && rhs.value!=null || value!=null && rhs.value==null) return false;
		return value()==rhs.value();
	}
	
private:
	Utils::Optional<T> value;
};

template<typename T> RepeatResult<T> Repeat(T&& val)
{
	return RepeatResult<T>(core::forward<T>(val));
}

template<typename T> TakeResult<RepeatResult<T>> Repeat(T&& val, size_t n)
{
	return RepeatResult<T>(core::forward<T>(val)).Take(n);
}

}}
