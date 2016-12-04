#pragma once

#include "Range/Mixins/RangeMixins.h"
#include "Range/ArrayRange.h"

namespace Intra { namespace Range {

template<typename R> struct CycleResult:
	RangeMixin<CycleResult<R>, typename R::value_type,
		R::RangeType==TypeEnum::Bidirectional? TypeEnum::Forward: R::RangeType, false>
{
	typedef typename R::value_type value_type;
	typedef typename R::return_value_type return_value_type;

	forceinline CycleResult(null_t=null): counter(0) {}
	forceinline CycleResult(const R& range):
		original_range(range), offset_range(range), counter(0) {}

	forceinline bool Empty() const {return original_range.Empty();}
	forceinline return_value_type First() const {return offset_range.First();}

	forceinline void PopFirst()
	{
		offset_range.PopFirst();
		counter++;
		if(!offset_range.Empty()) return;
		offset_range=original_range;
		counter=0;
	}

	template<typename U=R> forceinline Meta::EnableIf<
		IsRandomAccessRange<U>::_,
	return_value_type> operator[](size_t index) const
	{
		return original_range[(index+counter) % original_range.Length()];
	}

	forceinline bool operator==(const CycleResult& rhs) const
	{
		return offset_range==rhs.offset_range && original_range==rhs.original_range;
	}

private:
	R original_range, offset_range;
	size_t counter;
};

template<typename R> struct CycleRandomResult:
	RangeMixin<CycleRandomResult<R>, typename R::value_type,
		R::RangeType>=TypeEnum::RandomAccess && HasLength<R>::_? TypeEnum::RandomAccess: TypeEnum::Error, false>
{
	typedef typename R::value_type value_type;
	typedef typename R::return_value_type return_value_type;

	forceinline CycleRandomResult(null_t=null): counter(0) {}
	forceinline CycleRandomResult(const R& range): original_range(range), counter(0) {}

	forceinline bool Empty() const {return original_range.Empty();}

	forceinline return_value_type First() const {return original_range[counter];}

	forceinline void PopFirst()
	{
		counter++;
		if(counter==original_range.Length()) counter=0;
	}

	forceinline return_value_type operator[](size_t index) const
	{
		return original_range[(index+counter) % original_range.Length()];
	}

	forceinline bool operator==(const CycleRandomResult& rhs) const
	{
		return original_range==rhs.original_range && counter==rhs.counter;
	}

	forceinline TakeResult<CycleRandomResult<R>> opSlice(size_t startIndex, size_t endIndex) const
	{
		INTRA_ASSERT(startIndex <= endIndex);
		CycleRandomResult<R> result(original_range);
		result.counter = (counter+startIndex) % original_range.Length();
		return result.Take(endIndex-startIndex);
	}

private:
	R original_range;
	size_t counter;
};

template<typename T, size_t N> forceinline
CycleRandomResult<ArrayRange<const T>> Cycle(const T(&arr)[N])
{
	return ArrayRange<const T>(arr).Cycle();
}

template<typename R> forceinline Meta::EnableIf<
	IsInfiniteRange<R>::_,
R&&> Cycle(R&& range) {return core::forward<R>(range);}

}}
