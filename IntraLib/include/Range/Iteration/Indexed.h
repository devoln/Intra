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

	template<typename U=IndexRangeType> forceinline Meta::EnableIf<
		HasLast<U>::_ && HasIndex<ValueRangeType>::_,
	ReturnValueTypeOf<ValueRangeType>> Last() const
	{
		INTRA_ASSERT(!Empty());
		return ValueRange[IndexRange.Last()];
	}

	template<typename U=IndexRangeType> forceinline Meta::EnableIf<
		HasPopLast<U>::_
	> PopLast()
	{
		INTRA_ASSERT(!Empty());
		IndexRange.PopLast();
	}

	template<typename U=IndexRangeType> forceinline Meta::EnableIf<
		HasIndex<U>::_ && HasIndex<ValueRangeType>::_,
	ReturnValueTypeOf<ValueRangeType>> operator[](size_t index) const
	{return ValueRange[IndexRange[index]];}

	template<typename U=IndexRangeType> forceinline Meta::EnableIf<
		IsRandomAccessRange<U>::_,
	RIndexed> operator()(size_t start, size_t end) const
	{return RIndexed(ValueRange, IndexRange(start, end));}

	template<typename U=IndexRangeType> forceinline Meta::EnableIf<
		HasLength<U>::_,
	size_t> Length() const {return IndexRange.Length();}


	forceinline bool operator==(const RIndexed& rhs) const
	{return ValueRange==rhs.ValueRange && IndexRange==rhs.IndexRange;}

	forceinline bool operator!=(const RIndexed& rhs) const
	{return !operator==(rhs);}

	ValueRangeType ValueRange;
	IndexRangeType IndexRange;
};

INTRA_WARNING_POP


template<typename ValueRangeType, typename IndexRangeType> forceinline Meta::EnableIf<
	IsRandomAccessRange<ValueRangeType>::_ && IsInputRange<IndexRangeType>::_,
RIndexed<ValueRangeType, IndexRangeType>> Indexed(ValueRangeType&& valueRange, IndexRangeType&& indexRange)
{return {Meta::Forward<ValueRangeType>(valueRange), Meta::Forward<IndexRangeType>(indexRange)};}

template<typename VT, size_t VN, typename IndexRangeType> forceinline Meta::EnableIf<
	IsInputRange<IndexRangeType>::_,
RIndexed<AsRangeResult<VT(&)[VN]>, IndexRangeType>> Indexed(VT(&valueArr)[VN], IndexRangeType&& indexRange)
{return Indexed(AsRange(valueArr), Meta::Forward<IndexRangeType>(indexRange));}

template<typename ValueRangeType, typename IT, size_t IN> forceinline Meta::EnableIf<
	IsRandomAccessRange<ValueRangeType>::_,
RIndexed<ValueRangeType, AsRangeResult<IT(&)[IN]>>> Indexed(ValueRangeType&& valueRange, IT(&indexArr)[IN])
{return Indexed(Meta::Forward<ValueRangeType>(valueRange), AsRange(indexArr));}

template<typename VT, size_t VN, typename IT, size_t IN> forceinline
RIndexed<AsRangeResult<VT(&)[VN]>, AsRangeResult<IT(&)[IN]>> Indexed(VT(&valueArr)[VN], IT(&indexArr)[IN])
{return Indexed(AsRange(valueArr), AsRange(indexArr));}

}}
