#pragma once

#include "Range/Mixins/RangeMixins.h"
#include "Range/TupleOperation.h"

namespace Intra { namespace Range
{

template<typename T, typename R0, typename... RANGES> struct RoundRobinResult:
	RangeMixin<RoundRobinResult<T, R0, RANGES...>, T,
	CommonRangeCategoryAllFinite<R0, RANGES...>::Type==TypeEnum::Input? TypeEnum::Input: TypeEnum::Forward,
	CommonRangeCategoryAllFinite<R0, RANGES...>::Finite>
{
	typedef T value_type;
	typedef Meta::CommonTypeRef<typename R0::return_value_type, typename RANGES::return_value_type...> return_value_type;

	forceinline RoundRobinResult(null_t=null): counter(Meta::NumericLimits<decltype(counter)>::Max()) {}

	forceinline RoundRobinResult(R0&& r0, RANGES&&... ranges):
		range0(core::forward<R0>(r0)), next(core::forward<RANGES>(ranges)...),
		counter(r0.Empty()? Meta::NumericLimits<decltype(counter)>::Max(): 0) {}

	return_value_type First() const
	{
		return (!next.before_(counter) && !range0.Empty())?
			range0.First(): next.First();
	}

	void PopFirst()
	{
		if(!next.before_(counter))
		{
			counter++;
			range0.PopFirst();
			if(range0.Empty())
				counter = Meta::NumericLimits<decltype(counter)>::Max();
			return;
		}
		next.PopFirst();
	}

	forceinline bool Empty() const {return range0.Empty() && next.Empty();}

	forceinline bool operator==(const RoundRobinResult& rhs) const
	{
		return (range0.Empty() && rhs.range0.Empty() ||
			range0==rhs.range0 && counter==rhs.counter) && next==rhs.next;
	}

	template<typename U=R0> forceinline Meta::EnableIf<
		HasLength<U>::_ && HasLength<RoundRobinResult<T, RANGES...>>::_,
	size_t> Length() const {return range0.Length()+next.Length();}

	//template<typename U=R0> forceinline Meta::EnableIf<
		//HasSave<U>::_ && HasSave<RoundRobinResult<T, RANGES...>>::_,
	//RoundRobinResult> Save() const {return {range0.Save(), next.Save()};}

	bool before_(size_t prevCounter) const
	{return counter<prevCounter || next.before_(prevCounter);}

private:
	R0 range0;
	RoundRobinResult<T, RANGES...> next;
	size_t counter;
};

template<typename T, typename R0> struct RoundRobinResult<T, R0>:
	RangeMixin<RoundRobinResult<T, R0>, T, R0::RangeType, R0::RangeIsFinite>
{
	typedef T value_type;
	typedef typename R0::return_value_type return_value_type;

	forceinline RoundRobinResult(null_t=null):
		counter(Meta::NumericLimits<decltype(counter)>::Max()) {}

	forceinline RoundRobinResult(R0&& r0):
		range0(core::forward<R0>(r0)), counter(r0.Empty()? Meta::NumericLimits<decltype(counter)>::Max(): 0) {}


	forceinline return_value_type First() const
	{
		INTRA_ASSERT(!Empty());
		return range0.First();
	}

	forceinline void PopFirst()
	{
		INTRA_ASSERT(!range0.Empty());
		counter++;
		range0.PopFirst();
		if(range0.Empty())
			counter = Meta::NumericLimits<decltype(counter)>::Max();
	}

	forceinline bool Empty() const {return range0.Empty();}

	forceinline bool operator==(const RoundRobinResult& rhs) const
	{
		return range0.Empty() && rhs.range0.Empty() ||
			range0==rhs.range0 && counter==rhs.counter;
	}

	template<typename U=R0> forceinline Meta::EnableIf<
		HasLength<U>::_,
	size_t> Length() const {return range0.Length();}

	bool before_(size_t prevCounter) const {return counter<prevCounter;}

private:
	R0 range0;
	size_t counter;
};


template<typename R0, typename... RANGES> forceinline
RoundRobinResult<Meta::CommonType<typename R0::value_type, typename RANGES::value_type...>, R0, RANGES...>
RoundRobin(R0&& range0, RANGES&&... ranges)
{
	return {core::forward<R0>(range0), core::forward<RANGES>(ranges)...};
}


}}
