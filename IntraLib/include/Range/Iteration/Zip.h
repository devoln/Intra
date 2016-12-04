#pragma once
#include "Range/Mixins/RangeMixins.h"

namespace Intra { namespace Range {

template<typename... RANGES> struct ZipResult:
	RangeMixin<ZipResult<RANGES...>, TupleOfElement<RANGES...>,
		CommonRangeCategoryAnyFinite<RANGES...>::Type, CommonRangeCategoryAnyFinite<RANGES...>::Finite>
{
	enum {RangeType = CommonRangeCategoryAnyFinite<RANGES...>::Type};
	typedef TupleOfElement<RANGES...> value_type;
	typedef TupleOfReturnElement<RANGES...> return_value_type;
	typedef Meta::Tuple<RANGES...> OriginalRangeTuple;
	OriginalRangeTuple OriginalRanges;

	forceinline ZipResult(null_t=null) {}
	forceinline ZipResult(const RANGES&... ranges): OriginalRanges(ranges...) {}
	forceinline ZipResult(OriginalRangeTuple ranges): OriginalRanges(ranges) {}

	forceinline return_value_type First() const {return OriginalRanges.template TransformEach<Fronter>(Fronter());}
	forceinline void PopFirst() {OriginalRanges.ForEachField(PopFronter());}
	forceinline bool Empty() const {return AnyEmpty(OriginalRanges);}

	template<typename U=OriginalRangeTuple> forceinline
		Meta::EnableIfCompiles<size_t, decltype(MinLength(U()))>
		Length() const {return MinLength(OriginalRanges);}

	template<typename U=ZipResult> Meta::EnableIf<
		U::RangeType>=TypeEnum::RandomAccess,
	return_value_type> operator[](size_t index) const {return OriginalRanges.TransformEach(Indexer{index});}

	template<typename U=ZipResult> Meta::EnableIf<
		U::RangeType>=TypeEnum::RandomAccess,
	ZipResult> opSlice(size_t startIndex, size_t endIndex) const {return OriginalRanges.TransformEach(Slicer{startIndex, endIndex});}

	forceinline bool operator==(const ZipResult& rhs) const {return OriginalRanges==rhs.OriginalRanges;}
};


template<typename R0, typename... RANGES> forceinline
ZipResult<R0, RANGES...> Zip(const R0& range0, const RANGES&... ranges)
{
	return ZipResult<R0, RANGES...>(range0, ranges...);
}



template<size_t N, typename RangeOfTuples> struct UnzipResult:
	RangeMixin<UnzipResult<N, RangeOfTuples>, Meta::TypeListAt<N, typename RangeOfTuples::value_type::TL>,
		RangeOfTuples::RangeType, RangeOfTuples::RangeIsFinite>
{
	typedef Meta::TypeListAt<N, typename RangeOfTuples::value_type::TL> value_type;
	typedef decltype(Meta::Get<N>(Meta::Val<typename RangeOfTuples::value_type>())) return_value_type;
	
	RangeOfTuples OriginalRange;

	forceinline UnzipResult(null_t=null) {}

	forceinline UnzipResult(const RangeOfTuples& rangeOfTuples):
		OriginalRange(rangeOfTuples) {}

	forceinline return_value_type First() const
	{
		return Meta::Get<N>(OriginalRange.First());
	}

	forceinline void PopFirst()
	{
		OriginalRange.PopFirst();
	}

	forceinline bool Empty() const
	{
		return OriginalRange.Empty();
	}

	template<typename U=RangeOfTuples> forceinline Meta::EnableIf<
		HasIndex<U>::_,
	return_value_type> operator[](size_t index) const
	{
		return Meta::Get<N>(OriginalRange[index]);
	}

	template<typename U=RangeOfTuples> forceinline Meta::EnableIf<
		HasSlicing<U>::_,
	UnzipResult> opSlice(size_t start, size_t end) const
	{
		return UnzipResult(OriginalRange.opSlice(start, end));
	}

	template<typename U=RangeOfTuples> forceinline Meta::EnableIf<
		HasLast<U>::_,
	return_value_type> Last() const
	{
		return Meta::Get<N>(OriginalRange.Last());
	}

	template<typename U=RangeOfTuples> forceinline Meta::EnableIf<
		HasPopLast<U>::_> PopLast() const
	{
		OriginalRange.PopLast();
	}

	forceinline bool operator==(const UnzipResult& rhs) const {return OriginalRange==rhs.OriginalRange;}
};

template<size_t N, typename RangeOfTuples> forceinline
UnzipResult<N, RangeOfTuples> Unzip(const RangeOfTuples& range)
{
	return UnzipResult<N, RangeOfTuples>(range);
}

template<size_t N, typename... RANGES> forceinline
Meta::TypeListAt<N, Meta::TypeList<RANGES...>> Unzip(const ZipResult<RANGES...>& range)
{
	return Meta::Get<N>(range.OriginalRanges);
}

}}
