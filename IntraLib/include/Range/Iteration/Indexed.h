#pragma once

#include "Range/Mixins/RangeMixins.h"

namespace Intra { namespace Range {

template<typename ValueRangeType, typename IndexRangeType> struct IndexedResult:
	RangeMixin<
		IndexedResult<ValueRangeType, IndexRangeType>,
		typename ValueRangeType::value_type,
		IndexRangeType::RangeType, IndexRangeType::RangeIsFinite>
{
	typedef typename ValueRangeType::value_type value_type;
	typedef typename ValueRangeType::return_value_type return_value_type;

	IndexedResult(null_t=null): ValueRange(null), IndexRange(null) {}
	IndexedResult(const ValueRangeType& valueRange, const IndexRangeType& indexRange):
		ValueRange(valueRange), IndexRange(indexRange) {}
	IndexedResult(ValueRangeType&& valueRange, IndexRangeType&& indexRange):
		ValueRange(core::move(valueRange)), IndexRange(core::move(indexRange)) {}

	forceinline bool Empty() const {return IndexRange.Empty();}

	forceinline return_value_type First() const
	{
		INTRA_ASSERT(!Empty());
		return ValueRange[IndexRange.First()];
	}

	forceinline void PopFirst()
	{
		INTRA_ASSERT(!Empty());
		IndexRange.PopFirst();
	}

	template<typename U=IndexRangeType> forceinline Meta::EnableIf<
		IsBidirectionalRange<U>::_,
	return_value_type> Last() const
	{
		INTRA_ASSERT(!Empty());
		return ValueRange[IndexRange.Last()];
	}

	template<typename U=IndexRangeType> forceinline Meta::EnableIf<
		IsBidirectionalRange<U>::_
	> PopLast()
	{
		INTRA_ASSERT(!Empty());
		IndexRange.PopLast();
	}

	template<typename U=IndexRangeType> forceinline Meta::EnableIf<
		IsRandomAccessRange<U>::_,
	return_value_type> operator[](size_t index) const {return ValueRange[IndexRange[index]];}

	template<typename U=IndexRangeType> forceinline Meta::EnableIf<
		IsRandomAccessRange<U>::_,
	IndexedResult> opSlice(size_t start, size_t end) const
	{
		return IndexedResult(ValueRange, IndexRange.opSlice(start, end));
	}

	template<typename U=IndexRangeType> forceinline Meta::EnableIf<
		HasLength<U>::_,
	size_t> Length() const {return IndexRange.Length();}


	forceinline bool operator==(const IndexedResult& rhs) const
	{
		return ValueRange==rhs.ValueRange && IndexRange==rhs.IndexRange;
	}

	ValueRangeType ValueRange;
	IndexRangeType IndexRange;
};


template<typename ValueRangeType, typename IndexRangeType> forceinline Meta::EnableIf<
	IsRandomAccessRange<ValueRangeType>::_ && IsInputRange<IndexRangeType>::_,
IndexedResult<ValueRangeType, IndexRangeType>> Indexed(ValueRangeType&& valueRange, IndexRangeType&& indexRange)
{
	return IndexedResult<ValueRangeType, IndexRangeType>(
		core::forward<ValueRangeType>(valueRange),
		core::forward<IndexRangeType>(indexRange));
}

}}
