#pragma once

#include "Range/Concepts.h"
#include "Range/AsRange.h"
#include "Range/TupleOperation.h"
#include "Meta/Tuple.h"
#include "Meta/EachField.h"

namespace Intra { namespace Range {

INTRA_WARNING_PUSH
INTRA_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED

template<typename... RANGES> struct RZip
{
private:
	typedef Meta::Tuple<ReturnValueTypeOf<RANGES>...> ReturnValueType;
	typedef Meta::Tuple<RANGES...> OriginalRangeTuple;
public:
	enum: byte {RangeType = CommonRangeCategoryAnyFinite<RANGES...>::Type};
	enum: bool {RangeIsFinite = CommonRangeCategoryAnyFinite<RANGES...>::Finite};

	OriginalRangeTuple OriginalRanges;

	forceinline RZip(null_t=null) {}

	template<typename R0, typename... RANGES1> forceinline RZip(R0&& r0, RANGES1&&... ranges):
		OriginalRanges(Meta::Forward<R0>(r0), Meta::Forward<RANGES1>(ranges)...) {}
	
	forceinline RZip(OriginalRangeTuple ranges): OriginalRanges(ranges) {}

	forceinline RZip(const RZip& rhs): OriginalRanges(rhs.OriginalRanges) {}
	forceinline RZip(RZip&& rhs): OriginalRanges(Meta::Move(rhs.OriginalRanges)) {}

	forceinline RZip& operator=(const RZip& rhs)
	{
		OriginalRanges = rhs.OriginalRanges;
		return *this;
	}

	forceinline RZip& operator=(RZip&& rhs)
	{
		OriginalRanges = Meta::Move(rhs.OriginalRanges);
		return *this;
	}

	forceinline ReturnValueType First() const
	{return Meta::TransformEachField(OriginalRanges, Fronter());}
	
	forceinline void PopFirst()
	{Meta::ForEachField(OriginalRanges, PopFronter());}
	
	forceinline bool Empty() const
	{return AnyEmpty(OriginalRanges);}


	forceinline ReturnValueType Last() const {return Meta::TransformEachField(OriginalRanges, Backer());}
	
	forceinline void PopLast() {Meta::ForEachField(OriginalRanges, PopBacker());}


	forceinline size_t Length() const
	{return MinLength(OriginalRanges);}

	ReturnValueType operator[](size_t index) const
	{return Meta::TransformEachField(OriginalRanges, Indexer{index});}

	RZip operator()(size_t startIndex, size_t endIndex) const
	{return Meta::TransformEachField(OriginalRanges, Slicer{startIndex, endIndex});}

	forceinline bool operator==(const RZip& rhs) const {return OriginalRanges==rhs.OriginalRanges;}
};

INTRA_WARNING_POP


template<typename R0, typename... RANGES, typename=Meta::EnableIf<
	IsAsAccessibleRange<R0>::_>> forceinline
RZip<AsRangeResultNoCRef<R0>, AsRangeResultNoCRef<RANGES>...> Zip(R0&& range0, RANGES&&... ranges)
{return {Range::Forward<R0>(range0), Range::Forward<RANGES>(ranges)...};}



template<size_t N, typename RangeOfTuples> struct RUnzip
{
private:
	typedef decltype(Meta::Get<N>(Meta::Val<ReturnValueTypeOf<RangeOfTuples>>())) ReturnValueType;
public:
	enum: bool {RangeIsFinite = IsFiniteRange<RangeOfTuples>::_,
		RangeIsInfinite = IsInfiniteRange<RangeOfTuples>::_};
	
	RangeOfTuples OriginalRange;

	forceinline RUnzip(null_t=null) {}

	forceinline RUnzip(const RangeOfTuples& rangeOfTuples):
		OriginalRange(rangeOfTuples) {}

	forceinline ReturnValueType First() const {return Meta::Get<N>(OriginalRange.First());}
	forceinline void PopFirst() {OriginalRange.PopFirst();}
	forceinline bool Empty() const {return OriginalRange.Empty();}

	forceinline ReturnValueType operator[](size_t index) const
	{return Meta::Get<N>(OriginalRange[index]);}

	forceinline RUnzip operator()(size_t start, size_t end) const
	{return RUnzip(OriginalRange(start, end));}

	forceinline ReturnValueType Last() const
	{return Meta::Get<N>(OriginalRange.Last());}

	forceinline void PopLast() const {OriginalRange.PopLast();}

	forceinline size_t Length() const {return OriginalRange.Length();}

	forceinline bool operator==(const RUnzip& rhs) const
	{return OriginalRange==rhs.OriginalRange;}

	forceinline bool operator!=(const RUnzip& rhs) const {return !operator==(rhs);}
};

template<size_t N, typename RangeOfTuples> forceinline
RUnzip<N, RangeOfTuples> Unzip(const RangeOfTuples& range) {return range;}

template<size_t N, typename... RANGES> forceinline
Meta::TypeListAt<N, Meta::TypeList<RANGES...>> Unzip(const RZip<RANGES...>& range)
{return Meta::Get<N>(range.OriginalRanges);}

}}
