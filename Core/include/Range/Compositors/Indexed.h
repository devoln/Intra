#pragma once

#include "Range/ForwardDecls.h"
#include "Range/Concepts.h"

namespace Intra { namespace Range {

INTRA_WARNING_PUSH
INTRA_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED

template<typename ValueRangeType, typename IndexRangeType> struct RIndexed
{
	enum: bool {
		RangeIsFinite = IsFiniteRange<IndexRangeType>::_,
		RangeIsInfinite = IsInfiniteRange<IndexRangeType>::_
	};

	RIndexed(null_t=null): ValueRange(null), IndexRange(null) {}

	RIndexed(const ValueRangeType& valueRange, const IndexRangeType& indexRange):
		ValueRange(valueRange), IndexRange(indexRange) {}

	RIndexed(const ValueRangeType& valueRange, IndexRangeType&& indexRange):
		ValueRange(valueRange), IndexRange(Meta::Move(indexRange)) {}

	RIndexed(ValueRangeType&& valueRange, const IndexRangeType& indexRange):
		ValueRange(Meta::Move(valueRange)), IndexRange(indexRange) {}

	RIndexed(ValueRangeType&& valueRange, IndexRangeType&& indexRange):
		ValueRange(Meta::Move(valueRange)), IndexRange(Meta::Move(indexRange)) {}

	forceinline bool Empty() const {return IndexRange.Empty();}

	forceinline ReturnValueTypeOf<ValueRangeType> First() const
	{
		INTRA_ASSERT(!Empty());
		return ValueRange[IndexRange.First()];
	}

	forceinline void PopFirst()
	{
		INTRA_ASSERT(!Empty());
		IndexRange.PopFirst();
	}

	forceinline ReturnValueTypeOf<ValueRangeType> Last() const
	{
		INTRA_ASSERT(!Empty());
		return ValueRange[IndexRange.Last()];
	}

	forceinline void PopLast()
	{
		INTRA_ASSERT(!Empty());
		IndexRange.PopLast();
	}

	forceinline ReturnValueTypeOf<ValueRangeType> operator[](size_t index) const
	{return ValueRange[IndexRange[index]];}

	forceinline RIndexed operator()(size_t start, size_t end) const
	{return RIndexed(ValueRange, IndexRange(start, end));}

	forceinline size_t Length() const {return IndexRange.Length();}


	forceinline bool operator==(const RIndexed& rhs) const
	{return ValueRange==rhs.ValueRange && IndexRange==rhs.IndexRange;}

	forceinline bool operator!=(const RIndexed& rhs) const
	{return !operator==(rhs);}

	ValueRangeType ValueRange;
	IndexRangeType IndexRange;
};

INTRA_WARNING_POP


template<typename VR, typename IR> forceinline Meta::EnableIf<
	IsAsRandomAccessRange<VR>::_ &&
	IsAsInputRange<IR>::_,
RIndexed<AsRangeResultNoCRef<VR>, AsRangeResultNoCRef<IR>>> Indexed(VR&& valueRange, IR&& indexRange)
{return {Range::Forward<VR>(valueRange), Range::Forward<IR>(indexRange)};}

}}
