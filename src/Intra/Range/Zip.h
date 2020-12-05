#pragma once

#include "Intra/Functional.h"
#include "Intra/Numeric.h"
#include "Intra/Range/Concepts.h"
#include "Intra/Range/Operations.h"
#include "Intra/Container/Tuple.h"

INTRA_BEGIN
INTRA_IGNORE_WARN_ASSIGN_IMPLICITLY_DELETED
template<typename... Ranges> struct Zip: CopyableIf<!(CReference<Ranges> || ...)>
{
	Tuple<Ranges...> OriginalRanges;

	template<typename... Rs> requires(CAccessibleList<Rs> && ...)
	[[nodiscard]] constexpr Zip(Rs&&... ranges):
		OriginalRanges(ForwardAsRange<Rs>(ranges)...)
	{
		static_assert(!(CFiniteRange<Ranges> || ...) || !(CInfiniteRange<Ranges> || ...),
			"All ranges are required to have the same length!");
		if constexpr(DebugCheckLevel >= 1 && (CFiniteRange<Ranges> || ...))
		{
			bool allRangeLengthsAreEqual = true;
			if constexpr(CHasLength<Ranges> && ...)
			{
				index_t length = -1;
				Apply([&](auto& range) {
					auto len = Count(range);
					if(length == -1) length = len;
					if(len != length) allRangeLengthsAreEqual = false;
				}, OriginalRanges);
			}
			else if constexpr(DebugCheckLevel >= 2 && CCopyConstructible<RZip>)
			{
				auto copy = *this;
				for(;;)
				{
					auto anyEmpty = AccumAny(IsEmpty);
					auto allEmpty = AccumAll(IsEmpty);
					copy.OriginalRanges|ForEachField(anyEmpty);
					copy.OriginalRanges|ForEachField(allEmpty);
					if(!anyEmpty.Result) continue;
					if(anyEmpty.Result != allEmpty.Result) allRangeLengthsAreEqual = false;
					break;
				}
			}
			INTRA_PRECONDITION(allRangesAreOfSameLength);
		}
	}

	[[nodiscard]] constexpr auto First() const {return TransformEachField(OriginalRanges, Intra::First);}
	constexpr void PopFirst() {OriginalRanges|ForEachField(Intra::PopFirst);}
	[[nodiscard]] constexpr bool Empty() const {return get<0>(OriginalRanges).Empty();}

	[[nodiscard]] constexpr auto Last() const requires(CHasLast<Ranges> && ...)
	{return TransformEachField(OriginalRanges, Intra::Last);}
	constexpr void PopLast() {OriginalRanges|ForEachField(Intra::PopLast);}


	[[nodiscard]] constexpr index_t Length() const requires CHasLength<TPackFirst<Ranges>>
	{return get<0>(OriginalRanges).Length();}

	[[nodiscard]] constexpr auto operator[](Index index) const
	{return TransformEachField(OriginalRanges, AtIndex(index));}

	[[nodiscard]] constexpr index_t PopFirstCount(ClampedSize maxElementsToPop)
	{return get<0>(TransformEachField(OriginalRanges, Intra::PopFirstCount(maxElementsToPop)));}
};

template<typename... RANGES> struct RZip;

template<size_t N, typename RangeOfTuples> struct RUnzip
{
	static constexpr bool IsAnyInstanceFinite = CFiniteRange<RangeOfTuples>,
		IsAnyInstanceInfinite = CInfiniteRange<RangeOfTuples>;
	
	RangeOfTuples OriginalRange;

	[[nodiscard]] constexpr decltype(auto) First() const {return get<N>(OriginalRange.First());}
	constexpr void PopFirst() {OriginalRange.PopFirst();}
	[[nodiscard]] constexpr bool Empty() const {return OriginalRange.Empty();}

	[[nodiscard]] constexpr decltype(auto) operator[](size_t index) const
	{return get<N>(OriginalRange[index]);}

	
	[[nodiscard]] constexpr index_t PopFirstCount(ClampedSize maxElementsToPop)
	{return OriginalRange.PopFirstCount(maxElementsToPop);}

	[[nodiscard]] constexpr decltype(auto) Last() const
	{return get<N>(OriginalRange.Last());}

	constexpr void PopLast() const {OriginalRange.PopLast();}

	[[nodiscard]] constexpr index_t Length() const {return OriginalRange.Length();}
};

template<size_t N, typename RangeOfTuples>
[[nodiscard]] constexpr RUnzip<N, RangeOfTuples> Unzip(const RangeOfTuples& range) {return {range};}

template<size_t N, typename... RANGES>
[[nodiscard]] constexpr TPackAt<N, RANGES...> Unzip(const RZip<RANGES...>& range)
{return get<N>(range.OriginalRanges);}
INTRA_END
