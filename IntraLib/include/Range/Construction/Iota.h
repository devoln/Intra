#pragma once

#include "Range/Mixins/RangeMixins.h"
#include "Take.h"

namespace Intra { namespace Range {

template<typename T, typename S> struct IotaResult:
	RangeMixin<IotaResult<T, S>, T, TypeEnum::RandomAccess, false>
{
	typedef T value_type;
	typedef value_type return_value_type;

	T Begin;
	S Step;

	forceinline IotaResult(null_t=null): Begin(0), Step(0) {}
	forceinline IotaResult(T begin, S step): Begin(begin), Step(step) {}

	forceinline T First() const {return Begin;}
	forceinline void PopFirst() {Begin+=Step;}
	forceinline bool Empty() const {return Step==0;}
	forceinline T operator[](size_t index) const {return Begin+Step*index;}

	forceinline bool operator==(const IotaResult<T, S>& rhs) const
	{
		return (Begin==rhs.Begin || Step==0) && Step==rhs.Step;
	}

	forceinline TakeResult<IotaResult> opSlice(size_t start, size_t end) const
	{
		INTRA_ASSERT(start<=end);
		return TakeResult<IotaResult>(IotaResult(Begin+Step*start, Step), end-start);
	}
};

template<typename T, typename S> forceinline
TakeResult<IotaResult<T, S>> Iota(T begin, T end, S step)
{
	return IotaResult<T, S>(begin, step).Take((end-begin+step-1)/step);
}


static_assert(IsInputRange<IotaResult<int, int>>::_, "Not input range???");
static_assert(IsForwardRange<decltype(Iota(1, 2, 3))>::_, "Not forward range???");
static_assert(!IsBidirectionalRange<IotaResult<int, int>>::_, "Is bidirectional range???");
static_assert(IsRandomAccessRange<IotaResult<int, int>>::_, "Not random access range???");
static_assert(!IsFiniteRandomAccessRange<IotaResult<int, int>>::_, "Is finite random access range???");

}}
