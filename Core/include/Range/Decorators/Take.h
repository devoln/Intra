#pragma once

#include "Range/Concepts.h"
#include "Platform/Debug.h"
#include "Range/AsRange.h"

namespace Intra { namespace Range {

INTRA_WARNING_PUSH
INTRA_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED

INTRA_DEFINE_EXPRESSION_CHECKER(HasTake, Meta::Val<T>().Take(size_t()));

template<typename R> struct RTake
{
	enum: bool {RangeIsFinite=true};

	forceinline RTake(null_t=null):
		mOriginalRange(null), mLen(0) {}

	forceinline RTake(const R& range, size_t count):
		mOriginalRange(range) {set_len(count);}

	forceinline RTake(R&& range, size_t count):
		mOriginalRange(Meta::Move(range)) {set_len(count);}


	//Для совместимости с Visual Studio 2013:
	RTake(const RTake&) = default;
	RTake& operator=(const RTake&) = default;

	forceinline RTake(RTake&& rhs):
		mOriginalRange(Meta::Move(rhs.mOriginalRange)), mLen(rhs.mLen) {}

	forceinline RTake& operator=(RTake&& rhs)
	{
		mOriginalRange = Meta::Move(rhs.mOriginalRange);
		mLen = rhs.mLen;
		return *this;
	}

	template<typename U=R> forceinline Meta::EnableIf<
		HasLength<U>::_ || IsInfiniteRange<U>::_,
	bool> Empty() const {return mLen==0;}

	template<typename U=R> forceinline Meta::EnableIf<
		!HasLength<U>::_ && IsFiniteRange<U>::_,
	bool> Empty() const {return mLen==0 || mOriginalRange.Empty();}


	forceinline ReturnValueTypeOf<R> First() const
	{INTRA_ASSERT(!Empty()); return mOriginalRange.First();}

	forceinline void PopFirst()
	{mOriginalRange.PopFirst(); mLen--;}
	
	template<typename U=R> forceinline Meta::EnableIf<
		HasLast<U>::_,
	ReturnValueTypeOf<R>> Last() const
	{INTRA_ASSERT(!Empty()); return mOriginalRange[mLen-1];}

	template<typename U=R> forceinline Meta::EnableIf<
		HasPopLast<U>::_
	> PopLast() {mLen--;}

	template<typename U=R> forceinline Meta::EnableIf<
		HasIndex<U>::_,
	ReturnValueTypeOf<R>> operator[](size_t index) const {return mOriginalRange[index];}


	forceinline bool operator==(const RTake& rhs) const
	{return mLen==rhs.mLen && (mLen==0 || mOriginalRange==rhs.mOriginalRange);}

	template<typename U=R> forceinline Meta::EnableIf<
		HasLength<U>::_ || IsInfiniteRange<U>::_,
	size_t> Length() const {return mLen;}

	template<typename U=R> forceinline Meta::EnableIf<
		HasSlicing<U>::_,
	SliceTypeOf<U>> operator()(size_t startIndex, size_t endIndex) const
	{
		INTRA_ASSERT(startIndex <= endIndex);
		INTRA_ASSERT(endIndex <= mLen);
		return mOriginalRange(startIndex, endIndex);
	}

	
	RTake Take(size_t count) const
	{
		if(count>mLen) count = mLen;
		return Take(mOriginalRange, count);
	}

private:
	R mOriginalRange;
	size_t mLen;

	template<typename U=R> forceinline Meta::EnableIf<
		HasLength<U>::_
	> set_len(size_t maxLen)
	{
		mLen = mOriginalRange.Length();
		if(mLen>maxLen) mLen = maxLen;
	}

	template<typename U=R> forceinline Meta::EnableIf<
		!HasLength<U>::_
	> set_len(size_t maxLen) {mLen = maxLen;}
};


template<typename R> forceinline Meta::EnableIf<
	IsInputRange<R>::_ && !(HasSlicing<R>::_ && HasLength<R>::_) && !HasTake<R>::_,
RTake<Meta::RemoveConstRef<R>>> Take(R&& range, size_t count)
{return RTake<Meta::RemoveConstRef<R>>(Meta::Forward<R>(range), count);}

template<typename R> forceinline Meta::EnableIf<
	IsInputRange<R>::_ && HasSlicing<R>::_ && HasLength<R>::_ && !HasTake<R>::_,
SliceTypeOf<R>> Take(R&& range, size_t count)
{return Meta::Forward<R>(range)(0, range.Length()>count? count: range.Length());}

template<typename R, typename = Meta::EnableIf<
	IsInputRange<R>::_ && HasTake<R>::_
>> forceinline decltype(Meta::Val<R>().Take(size_t())) Take(R&& range, size_t count)
{return range.Take(count);}

namespace D {

INTRA_DEFINE_EXPRESSION_CHECKER(TakeCompiles, Range::Take(Meta::Val<T>(), size_t()));

template<typename R, bool=TakeCompiles<R>::_> struct ResultOfTake
{typedef decltype(Take(Meta::Val<R>(), size_t())) _;};

template<typename R> struct ResultOfTake<R, false>
{typedef void _;};

}

template<typename R> using ResultOfTake = typename D::ResultOfTake<R>::_;

template<typename R> forceinline Meta::EnableIf<
	!IsInputRange<R>::_ && IsAsInputRange<R>::_,
ResultOfTake<AsRangeResult<R>>> Take(R&& range, size_t count)
{return Take(Range::Forward<R>(range), count);}

INTRA_WARNING_POP

}}
