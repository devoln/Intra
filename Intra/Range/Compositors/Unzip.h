#pragma once

#include "Concepts/Range.h"
#include "Meta/Tuple.h"
#include "Meta/GetField.h"

INTRA_WARNING_PUSH
INTRA_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED

namespace Intra { namespace Range {

template<typename... RANGES> struct RZip;
template<typename K, typename V> struct RZipKV;

template<size_t N, typename RangeOfTuples> struct RUnzip
{
private:
	typedef decltype(Meta::Get<N>(Meta::Val<Concepts::ReturnValueTypeOf<RangeOfTuples>>())) ReturnValueType;
public:
	enum: bool {RangeIsFinite = Concepts::IsFiniteRange<RangeOfTuples>::_,
		RangeIsInfinite = Concepts::IsInfiniteRange<RangeOfTuples>::_};
	
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
RUnzip<N, RangeOfTuples> Unzip(const RangeOfTuples& range) {return {range};}

template<size_t N, typename... RANGES> forceinline
Meta::TypeListAt<N, Meta::TypeList<RANGES...>> Unzip(const RZip<RANGES...>& range)
{return Meta::Get<N>(range.OriginalRanges);}

template<size_t N, typename K, typename V> forceinline
Meta::SelectType<K, V, N==0> Unzip(const RZipKV<K, V>& range)
{return Meta::Get<N>(range.OriginalRanges);}

}}

INTRA_WARNING_POP

