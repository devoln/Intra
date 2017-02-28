#pragma once

#include "Range/Concepts.h"
#include "Meta/Operators.h"

namespace Intra { namespace Range {

template<typename R> struct InputIterator
{
	typedef ValueTypeOf<R> value_type;
	typedef ReturnValueTypeOf<R> return_value_type;
	typedef return_value_type& reference;
	typedef value_type* pointer;
	typedef intptr difference_type;

	forceinline InputIterator(null_t=null): mRange(null) {}
	forceinline InputIterator(R& range): mRange(&range) {}

	forceinline InputIterator& operator++() {INTRA_DEBUG_ASSERT(mRange!=null); mRange->PopFirst(); return *this;}
	forceinline return_value_type operator*() const {INTRA_DEBUG_ASSERT(mRange!=null); return mRange->First();}

	forceinline bool operator==(const InputIterator& rhs) const
	{
		return mRange==rhs.mRange ||
			(mRange!=null && rhs.mRange!=null && *mRange==*rhs.mRange);
	}

	forceinline bool operator==(null_t) const {return mRange==null || mRange->Empty();}

	InputIterator(const InputIterator&) = delete;
	InputIterator& operator=(const InputIterator&) = delete;
	InputIterator(InputIterator&& rhs): mRange(rhs.mRange) {rhs.mRange=null;}
	InputIterator operator=(InputIterator&& rhs) {mRange=rhs.mRange; rhs.mRange=null; return *this;}

protected:
	R* mRange;
};


template<typename R> struct ForwardIterator
{
	typedef ValueTypeOf<R> value_type;
	typedef ReturnValueTypeOf<R> return_value_type;
	typedef return_value_type& reference;
	typedef value_type* pointer;
	typedef intptr difference_type;

	forceinline ForwardIterator(null_t=null) {}
	forceinline ForwardIterator(const R& range): Range(range) {}
	forceinline ForwardIterator& operator++() {Range.PopFirst(); return *this;}
	forceinline ForwardIterator operator++(int) {auto copy = Range; Range.PopFirst(); return copy;}
	forceinline return_value_type operator*() const {return Range.First();}

	forceinline bool operator==(const ForwardIterator& rhs) const {return Range==rhs.Range;}
	forceinline bool operator==(null_t) const {return Range.Empty();}

	R Range;
};

template<typename R> forceinline Meta::EnableIf<
	IsInputRange<R>::_ && !IsForwardRange<R>::_ && !Meta::IsConst<R>::_,
InputIterator<Meta::RemoveReference<R>>> begin(R&& range)
{return InputIterator<R>(range);}

template<typename R> forceinline Meta::EnableIf<
	IsInputRange<R>::_ && !IsForwardRange<R>::_ && !Meta::IsConst<R>::_,
InputIterator<Meta::RemoveReference<R>>> end(R&&)
{return null;}

template<typename R> forceinline Meta::EnableIf<
	IsForwardRange<R>::_,
ForwardIterator<Meta::RemoveConstRef<R>>> begin(R&& range)
{return Meta::Forward<R>(range);}

template<typename R> forceinline Meta::EnableIf<
	IsForwardRange<R>::_,
ForwardIterator<Meta::RemoveConstRef<R>>> end(R&&)
{return null;}

}}

