#pragma once

#include "Range/Mixins/RangeMixins.h"

namespace Intra { namespace Range {

template<typename R> struct TakeResult:
	RangeMixin<TakeResult<R>, typename R::value_type,
		R::RangeType==TypeEnum::Bidirectional? TypeEnum::Forward: R::RangeType, true>
{
	typedef typename R::value_type value_type;
	typedef typename R::return_value_type return_value_type;

	forceinline TakeResult(null_t=null): r(null), len(0) {}
	forceinline TakeResult(const R& range, size_t count): r(range) {set_len(count);}

	template<typename U=R> forceinline Meta::EnableIf<
		HasLength<U>::_ || IsInfiniteRange<U>::_,
	bool> Empty() const {return len==0;}

	template<typename U=R> forceinline Meta::EnableIf<
		!HasLength<U>::_ && IsFiniteRange<U>::_,
	bool> Empty() const {return len==0 || r.Empty();}


	forceinline return_value_type First() const {INTRA_ASSERT(!Empty()); return r.First();}
	forceinline void PopFirst() {r.PopFirst(); len--;}
	
	template<typename U=R> forceinline Meta::EnableIf<
		IsBidirectionalRange<U>::_,
	return_value_type> Last() const {INTRA_ASSERT(!Empty()); return r[len-1];}

	template<typename U=R> forceinline Meta::EnableIf<
		IsBidirectionalRange<U>::_
	> PopLast() {len--;}

	template<typename U=R> forceinline Meta::EnableIf<
		IsRandomAccessRange<U>::_,
	return_value_type> operator[](size_t index) const {return r[index];}


	forceinline bool operator==(const TakeResult& rhs) const
	{
		return len==rhs.len && (len==0 || r==rhs.r);
	}

	template<typename U=R> forceinline Meta::EnableIf<
		HasLength<U>::_ || IsInfiniteRange<U>::_,
	size_t> Length() const {return len;}

	TakeResult Take(size_t count) const
	{
		if(count>len) count=len;
		return TakeResult(r, count);
	}

	template<typename U=R, typename = Meta::EnableIf<
		U::RangeType==TypeEnum::RandomAccess
		>> forceinline 
	decltype(Meta::Val<U>().opSlice(1, 2)) opSlice(size_t startIndex, size_t endIndex) const
	{
		INTRA_ASSERT(startIndex <= endIndex);
		INTRA_ASSERT(endIndex <= len);
		return r.opSlice(startIndex, endIndex);
	}

private:
	R r;
	size_t len;

	template<typename U=R> forceinline Meta::EnableIf<
		HasLength<U>::_
	> set_len(size_t maxLen)
	{
		len = r.Length();
		if(len>maxLen) len=maxLen;
	}

	template<typename U=R> forceinline Meta::EnableIf<
		!HasLength<U>::_
	> set_len(size_t maxLen) {len = maxLen;}
};

template<typename R> forceinline Meta::EnableIf<
	Range::IsInputRange<R>::_ && !Range::IsFiniteRandomAccessRange<R>::_ && !Range::HasTake<R>::_,
TakeResult<Meta::RemoveConstRef<R>>> Take(R&& range, size_t count)
{return TakeResult<Meta::RemoveConstRef<R>>(core::forward<R>(range), count);}

template<typename R> forceinline Meta::EnableIf<
	Range::IsFiniteRandomAccessRange<R>::_ && !Range::HasTake<R>::_,
Meta::RemoveConstRef<R>> Take(const R& range, size_t count) {return range(0, count);}

template<typename R> forceinline Meta::EnableIf<
	Range::HasTake<R>::_,
Meta::RemoveConstRef<R>> Take(const R& range, size_t count) {return range.Take(count);}

}}
