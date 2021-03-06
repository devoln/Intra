﻿#pragma once

#include "Concepts/Range.h"
#include "Concepts/RangeOf.h"

#include "Meta/Type.h"

#include "Utils/Debug.h"

#include "Range/Operations.h"

namespace Intra { namespace Range {

INTRA_WARNING_PUSH
INTRA_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_COPY_MOVE_IMPLICITLY_DELETED

INTRA_DEFINE_EXPRESSION_CHECKER(HasTake, Meta::Val<T>().Take(size_t()));

template<typename R> struct RTake: Meta::CopyableIf<!Meta::IsReference<R>::_>
{
	enum: bool {RangeIsFinite=true};

	template<typename U=R, typename = Meta::EnableIf<
		!Meta::IsReference<U>::_>
	> forceinline RTake(null_t=null):
		mOriginalRange(null), mLen(0) {}

	template<typename R2> forceinline RTake(R2&& range, size_t count):
		mOriginalRange(Cpp::Forward<R2>(range)) {set_len(count);}

	RTake(RTake&&) = default;
	RTake(const RTake&) = default;
	RTake& operator=(RTake&&) = default;
	RTake& operator=(const RTake&) = default;

	template<typename U = Meta::RemoveConstRef<R>> forceinline Meta::EnableIf<
		Concepts::HasLength<U>::_ ||
		Concepts::IsInfiniteRange<U>::_,
	bool> Empty() const {return mLen == 0;}

	template<typename U = Meta::RemoveConstRef<R>> forceinline Meta::EnableIf<
		!Concepts::HasLength<U>::_ &&
		!Concepts::IsInfiniteRange<U>::_,
	bool> Empty() const {return mLen == 0 || mOriginalRange.Empty();}


	forceinline Concepts::ReturnValueTypeOf<R> First() const
	{INTRA_DEBUG_ASSERT(!Empty()); return mOriginalRange.First();}

	forceinline void PopFirst()
	{mOriginalRange.PopFirst(); mLen--;}
	
	forceinline Concepts::ReturnValueTypeOf<R> Last() const
	{INTRA_DEBUG_ASSERT(!Empty()); return mOriginalRange[mLen-1];}

	forceinline void PopLast() {mLen--;}

	template<typename U=R> forceinline Meta::EnableIf<
		Concepts::HasIndex<U>::_,
	Concepts::ReturnValueTypeOf<U>> operator[](size_t index) const {return mOriginalRange[index];}

	template<typename U=R> forceinline Meta::EnableIf<
		Concepts::HasLength<U>::_ ||
		Concepts::IsInfiniteRange<U>::_,
	size_t> Length() const noexcept {return mLen;}

	forceinline size_t LengthLimit() const noexcept {return mLen;}

	template<typename U=R> forceinline Meta::EnableIf<
		Concepts::HasSlicing<U>::_,
	Concepts::SliceTypeOf<U>> operator()(size_t startIndex, size_t endIndex) const
	{
		INTRA_DEBUG_ASSERT(startIndex <= endIndex);
		INTRA_DEBUG_ASSERT(endIndex <= mLen);
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
		Concepts::HasLength<U>::_
	> set_len(size_t maxLen)
	{
		mLen = mOriginalRange.Length();
		if(mLen > maxLen) mLen = maxLen;
	}

	template<typename U=R> forceinline Meta::EnableIf<
		!Concepts::HasLength<U>::_
	> set_len(size_t maxLen) {mLen = maxLen;}
};


template<typename R> forceinline Meta::EnableIf<
	Concepts::IsInputRange<R>::_ &&
	!(Concepts::HasSlicing<R>::_ &&
		Concepts::HasLength<R>::_) &&
	!HasTake<R>::_,
RTake<Meta::RemoveConstRef<R>>> Take(R&& range, size_t count)
{return RTake<Meta::RemoveConstRef<R>>(Cpp::Forward<R>(range), count);}

template<typename R> forceinline Meta::EnableIf<
	Concepts::IsInputRange<R>::_ &&
	Concepts::HasSlicing<R>::_ &&
	Concepts::HasLength<R>::_ &&
	!HasTake<R>::_,
Concepts::SliceTypeOf<R>> Take(R&& range, size_t count)
{return Cpp::Forward<R>(range)(0, range.Length()>count? count: range.Length());}

template<typename R, typename = Meta::EnableIf<
	Concepts::IsInputRange<R>::_ && HasTake<R>::_
>> forceinline decltype(Meta::Val<R>().Take(size_t())) Take(R&& range, size_t count)
{return range.Take(count);}


namespace D {

INTRA_DEFINE_EXPRESSION_CHECKER(TakeCompiles, Range::Take(Meta::Val<T>(), size_t()));

template<typename R, bool=TakeCompiles<R>::_> struct TakeResult
{typedef decltype(Take(Meta::Val<R>(), size_t())) _;};

template<typename R> struct TakeResult<R, false>
{typedef void _;};

}

template<typename R> using TakeResult = typename D::TakeResult<R>::_;

template<typename R> forceinline Meta::EnableIf<
	!Concepts::IsInputRange<R>::_ && Concepts::IsAsInputRange<R>::_,
TakeResult<Concepts::RangeOfType<R>>> Take(R&& range, size_t count)
{return Take(Range::Forward<R>(range), count);}

INTRA_WARNING_POP

}}
