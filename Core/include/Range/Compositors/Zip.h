#pragma once

#include "Range/Concepts.h"
#include "Range/AsRange.h"
#include "Range/TupleOperation.h"
#include "Meta/Tuple.h"
#include "Meta/EachField.h"

INTRA_WARNING_PUSH
INTRA_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED

namespace Intra { namespace Range {

template<typename... RANGES> struct RZip
{
private:
	typedef Meta::Tuple<ReturnValueTypeOf<RANGES>...> ReturnValueType;
	typedef Meta::Tuple<RANGES...> OriginalRangeTuple;
public:
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


	forceinline ReturnValueType Last() const {return Meta::TransformEachField(OriginalRanges, Backer());}
	
	forceinline void PopLast() {Meta::ForEachField(OriginalRanges, PopBacker());}


	forceinline size_t Length() const
	{return MinLength(OriginalRanges);}

	ReturnValueType operator[](size_t index) const
	{return Meta::TransformEachField(OriginalRanges, Indexer{index});}

	RZip operator()(size_t startIndex, size_t endIndex) const
	{return Meta::TransformEachField(OriginalRanges, Slicer{startIndex, endIndex});}
};



template<typename R0, typename... RANGES, typename=Meta::EnableIf<
	IsAsAccessibleRange<R0>::_>> forceinline
RZip<AsRangeResultNoCRef<R0>, AsRangeResultNoCRef<RANGES>...> Zip(R0&& range0, RANGES&&... ranges)
{return {Range::Forward<R0>(range0), Range::Forward<RANGES>(ranges)...};}

}}

INTRA_WARNING_POP
