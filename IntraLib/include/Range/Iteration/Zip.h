#pragma once

#include "Range/Concepts.h"
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

	forceinline ReturnValueType First() const
	{return Meta::TransformEachField(OriginalRanges, Fronter());}
	
	forceinline void PopFirst()
	{Meta::ForEachField(OriginalRanges, PopFronter());}
	
	forceinline bool Empty() const
	{return AnyEmpty(OriginalRanges);}

	template<typename U=size_t> forceinline Meta::EnableIf<
		AllHasLength<RANGES...>::_,
	U> Length() const
	{return MinLength(OriginalRanges);}

	template<typename U=RZip> Meta::EnableIf<
		U::RangeType>=TypeEnum::RandomAccess,
	ReturnValueType> operator[](size_t index) const
	{return Meta::TransformEachField(OriginalRanges, Indexer{index});}

	template<typename U=RZip> Meta::EnableIf<
		U::RangeType>=TypeEnum::RandomAccess,
	RZip> operator()(size_t startIndex, size_t endIndex) const
	{return Meta::TransformEachField(OriginalRanges, Slicer{startIndex, endIndex});}

	forceinline bool operator==(const RZip& rhs) const {return OriginalRanges==rhs.OriginalRanges;}
};

INTRA_WARNING_POP


template<typename R0, typename... RANGES> forceinline
RZip<Meta::RemoveConstRef<R0>, Meta::RemoveConstRef<RANGES>...> Zip(R0&& range0, RANGES&&... ranges)
{return {Meta::Forward<R0>(range0), Meta::Forward<RANGES>(ranges)...};}



template<size_t N, typename RangeOfTuples> struct RUnzip
{
private:
	typedef decltype(Meta::Get<N>(Meta::Val<ValueTypeOf<RangeOfTuples>>())) ReturnValueType;
public:
	enum: bool {RangeIsFinite = IsFiniteRange<RangeOfTuples>::_, RangeIsInfinite = IsInfiniteRange<RangeOfTuples>::_};
	
	RangeOfTuples OriginalRange;

	forceinline RUnzip(null_t=null) {}

	forceinline RUnzip(const RangeOfTuples& rangeOfTuples):
		OriginalRange(rangeOfTuples) {}

	forceinline ReturnValueType First() const {return Meta::Get<N>(OriginalRange.First());}
	forceinline void PopFirst() {OriginalRange.PopFirst();}
	forceinline bool Empty() const {return OriginalRange.Empty();}

	template<typename U=RangeOfTuples> forceinline Meta::EnableIf<
		HasIndex<U>::_,
	ReturnValueType> operator[](size_t index) const
	{return Meta::Get<N>(OriginalRange[index]);}

	template<typename U=RangeOfTuples> forceinline Meta::EnableIf<
		HasSlicing<U>::_,
	RUnzip> operator()(size_t start, size_t end) const
	{return RUnzip(OriginalRange(start, end));}

	template<typename U=RangeOfTuples> forceinline Meta::EnableIf<
		HasLast<U>::_,
	ReturnValueType> Last() const
	{return Meta::Get<N>(OriginalRange.Last());}

	template<typename U=RangeOfTuples> forceinline Meta::EnableIf<
		HasPopLast<U>::_
	> PopLast() const {OriginalRange.PopLast();}

	template<typename U=RangeOfTuples> forceinline Meta::EnableIf<
		HasLength<U>::_,
	size_t> Length() const {OriginalRange.Length();}

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
