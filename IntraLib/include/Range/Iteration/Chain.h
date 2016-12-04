#pragma once

#include "Range/Mixins/RangeMixins.h"
#include "Range/TupleOperation.h"

namespace Intra { namespace Range
{

template<typename T, typename... RANGES> struct ChainResult;

template<typename T, typename R0, typename R1, typename... RANGES> struct ChainResult<T, R0, R1, RANGES...>:
	CommonTypedAllFiniteRangeMixin<ChainResult<T, R0, R1, RANGES...>, T, R0, R1, RANGES...>
{
	enum {RangeType = CommonRangeCategoryAllFinite<R0, R1, RANGES...>::Type};
	typedef T value_type;
	typedef Meta::CommonTypeRef<typename R0::return_value_type, typename R1::return_value_type, typename RANGES::return_value_type...> return_value_type;

	forceinline ChainResult(null_t=null) {}

	forceinline ChainResult(R0&& r0, R1&& r1, RANGES&&... ranges):
		range0(core::forward<R0>(r0)), next(core::forward<R1>(r1), core::forward<RANGES>(ranges)...) {}

	forceinline return_value_type First() const
	{
		return range0.Empty()? next.First(): range0.First();
	}

	forceinline void PopFirst()
	{
		if(!range0.Empty()) range0.PopFirst();
		else next.PopFirst();
	}

	forceinline bool Empty() const {return range0.Empty() && next.Empty();}

	template<typename U=R0> forceinline Meta::EnableIf<
		RangeType>=TypeEnum::Bidirectional,
	return_value_type> Last() const {return next.Empty()? range0.Last(): next.Last();}

	template<typename U=R0> forceinline Meta::EnableIf<
		RangeType>=TypeEnum::Bidirectional
	> PopLast()
	{
		if(next.Empty()) range0.PopLast();
		else next.PopLast();
	}


	template<typename U=R0> forceinline Meta::EnableIf<
		HasLength<U>::_ && HasLength<ChainResult<T, R1, RANGES...>>::_,
	size_t> Length() const {return range0.Length()+next.Length();}


	template<typename U=R0> forceinline Meta::EnableIf<
		RangeType>=TypeEnum::RandomAccess,
	return_value_type> operator[](size_t index) const
	{
		size_t len = range0.Length();
		if(index<len) return range0[index];
		return next[index-len];
	}

	template<typename U=R0> forceinline Meta::EnableIf<
		U::RangeType>=TypeEnum::RandomAccess,
	ChainResult> opSlice(size_t startIndex, size_t endIndex) const
	{
		const size_t len = range0.Length();
		return ChainResult(
			range0.opSlice(startIndex>len? len: startIndex, len>endIndex? endIndex: len),
			next.opSlice(startIndex>len? startIndex-len: 0, endIndex>len? endIndex-len: 0)
		);
	}

	forceinline bool operator==(const ChainResult& rhs) const {return range0==rhs.range0 && next==rhs.next;}

private:
	R0 range0;
	ChainResult<T, R1, RANGES...> next;

	forceinline ChainResult(R0&& r0, ChainResult<T, R1, RANGES...>&& ranges):
		range0(core::forward<R0>(r0)), next(core::move(ranges)) {}
};

template<typename T, typename R0> struct ChainResult<T, R0>:
	RangeMixin<ChainResult<T, R0>, T, R0::RangeType, R0::RangeIsFinite>
{
	typedef T value_type;
	typedef typename R0::return_value_type return_value_type;

	forceinline ChainResult(null_t=null) {}

	forceinline ChainResult(R0&& r0): range0(core::forward<R0>(r0)) {}

	forceinline return_value_type First() const
	{
		INTRA_ASSERT(!range0.Empty());
		return range0.First();
	}

	forceinline void PopFirst()
	{
		INTRA_ASSERT(!range0.Empty());
		range0.PopFirst();
	}

	forceinline bool Empty() const {return range0.Empty();}

	template<typename U=R0> forceinline Meta::EnableIf<
		U::RangeType>=TypeEnum::Bidirectional,
	return_value_type> Last() const {return range0.Last();}

	template<typename U=R0> forceinline Meta::EnableIf<
		U::RangeType>=TypeEnum::Bidirectional
	> PopLast()
	{
		range0.PopLast();
	}

	template<typename U=R0> forceinline Meta::EnableIf<
		HasLength<U>::_,
	size_t> Length() const {return range0.Length();}

	template<typename U=R0> forceinline Meta::EnableIf<
		IsRandomAccessRange<U>::_,
	return_value_type> operator[](size_t index) const {return range0[index];}

	template<typename U=R0> forceinline Meta::EnableIf<
		U::RangeType>=TypeEnum::RandomAccess,
	ChainResult> opSlice(size_t startIndex, size_t endIndex) const {return ChainResult(range0.opSlice(startIndex, endIndex));}

	forceinline bool operator==(const ChainResult& rhs) const {return range0==rhs.range0;}


private:
	R0 range0;
};


template<typename R0, typename... RANGES> forceinline
ChainResult<Meta::CommonType<typename R0::value_type, typename RANGES::value_type...>, R0, RANGES...>
Chain(R0&& range0, RANGES&&... ranges)
{
	return {core::forward<R0>(range0), core::forward<RANGES>(ranges)...};
}


}}
