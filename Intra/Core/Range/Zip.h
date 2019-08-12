﻿#pragma once

#include "Core/Tuple.h"
#include "Core/EachField.h"
#include "Core/Range/TupleOperation.h"
#include "Core/Range/Concepts.h"

INTRA_CORE_RANGE_BEGIN
INTRA_WARNING_DISABLE_ASSIGN_IMPLICITLY_DELETED
template<typename... RANGES> struct RZip
{
	Tuple<RANGES...> OriginalRanges;

	INTRA_NODISCARD constexpr forceinline auto First() const
	{return TransformEachField(OriginalRanges, Fronter());}
	
	INTRA_CONSTEXPR2 forceinline void PopFirst()
	{ForEachField(OriginalRanges, PopFronter());}
	
	INTRA_NODISCARD constexpr forceinline bool Empty() const
	{return AnyEmpty(OriginalRanges);}


	INTRA_NODISCARD constexpr forceinline auto Last() const {return TransformEachField(OriginalRanges, Backer());}
	
	INTRA_CONSTEXPR2 forceinline void PopLast() {ForEachField(OriginalRanges, PopBacker());}


	INTRA_NODISCARD constexpr forceinline index_t Length() const
	{return VMin(OriginalRanges);}

	INTRA_NODISCARD constexpr auto operator[](size_t index) const
	{return TransformEachField(OriginalRanges, Indexer{index});}

	INTRA_NODISCARD constexpr RZip operator()(size_t startIndex, size_t endIndex) const
	{return TransformEachField(OriginalRanges, Slicer{startIndex, endIndex});}
};

template<typename... RANGES, typename = Requires<
	CAll<CAsAccessibleRangeT, RANGES...>
>> INTRA_NODISCARD constexpr forceinline RZip<TRangeOfTypeNoCRef<RANGES>...> Zip(RANGES&&... ranges)
{return {ForwardAsRange<RANGES>(ranges)...};}

template<typename... RANGES> struct RZip;
template<typename K, typename V> struct RZipKV;

template<size_t N, typename RangeOfTuples> struct RUnzip
{
	enum: bool {
		RangeIsFinite = CFiniteRange<RangeOfTuples>,
		RangeIsInfinite = CInfiniteRange<RangeOfTuples>
	};
	
	RangeOfTuples OriginalRange;

	INTRA_NODISCARD constexpr forceinline decltype(auto) First() const {return GetField<N>(OriginalRange.First());}
	INTRA_CONSTEXPR2 forceinline void PopFirst() {OriginalRange.PopFirst();}
	INTRA_NODISCARD constexpr forceinline bool Empty() const {return OriginalRange.Empty();}

	INTRA_NODISCARD constexpr forceinline decltype(auto) operator[](size_t index) const
	{return GetField<N>(OriginalRange[index]);}

	INTRA_NODISCARD constexpr forceinline RUnzip operator()(size_t start, size_t end) const
	{return RUnzip(OriginalRange(start, end));}

	INTRA_NODISCARD constexpr forceinline decltype(auto) Last() const
	{return GetField<N>(OriginalRange.Last());}

	INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline void PopLast() const {OriginalRange.PopLast();}

	INTRA_NODISCARD constexpr forceinline index_t Length() const {return OriginalRange.Length();}
};

template<size_t N, typename RangeOfTuples> INTRA_NODISCARD constexpr forceinline
RUnzip<N, RangeOfTuples> Unzip(const RangeOfTuples& range) {return {range};}

template<size_t N, typename... RANGES> INTRA_NODISCARD constexpr forceinline
TAtIndex<N, RANGES...> Unzip(const RZip<RANGES...>& range)
{return GetField<N>(range.OriginalRanges);}
INTRA_CORE_RANGE_END
