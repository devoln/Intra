#pragma once

#include "Range/Concepts.h"
#include "Meta/Operators.h"
#include "Platform/CppFeatures.h"
#include "Platform/Debug.h"

namespace Intra { namespace Range {

//! Реализует минимально необходимое подмножество итератора, требуемое для совместимости с range-based for.
//! Запрещено использовать где-либо кроме range-based for.
template<typename R> struct RangeForIterLike
{
	forceinline RangeForIterLike(null_t=null): mRange() {}
	forceinline RangeForIterLike(R&& range): mRange(Meta::Move(range)) {}

	forceinline RangeForIterLike& operator++() {mRange.PopFirst(); return *this;}
	forceinline ReturnValueTypeOf<R> operator*() const {return mRange.First();}

#ifndef INTRA_RANGE_FOR_DIFFERING_TYPES_SUPPORT
	forceinline bool operator!=(const RangeForIterLike& rhs) const
	{
		INTRA_DEBUG_ASSERT(rhs.mRange.Empty());
		(void)rhs;
		return !mRange.Empty();
	}
#endif

	forceinline bool operator!=(null_t) const {return !mRange.Empty();}

	RangeForIterLike(const RangeForIterLike&) = delete;
	RangeForIterLike& operator=(const RangeForIterLike&) = delete;
	forceinline RangeForIterLike(RangeForIterLike&& rhs): mRange(Meta::Move(rhs.mRange)) {}

private:
	R mRange;
};

template<typename R> forceinline Meta::EnableIf<
	IsInputRange<R>::_ &&
	(!Meta::IsCopyConstructible<Meta::RemoveConstRef<R>>::_ || !Meta::IsCopyAssignable<Meta::RemoveConstRef<R>>::_) &&
	!Meta::IsConst<R>::_,
RangeForIterLike<Meta::RemoveConstRef<R>>> begin(R&& range)
{return Meta::Move(range);}

template<typename R> forceinline Meta::EnableIf<
	IsInputRange<R>::_ &&
	Meta::IsCopyConstructible<Meta::RemoveConstRef<R>>::_ && Meta::IsCopyAssignable<Meta::RemoveConstRef<R>>::_,
RangeForIterLike<Meta::RemoveConstRef<R>>> begin(R&& range)
{
	return {Meta::RemoveConstRef<R>(Meta::Forward<R>(range))};
}

#ifndef INTRA_RANGE_FOR_DIFFERING_TYPES_SUPPORT

template<typename R> forceinline Meta::EnableIf<
	IsInputRange<R>::_ && !Meta::IsConst<R>::_,
RangeForIterLike<Meta::RemoveReference<R>>> end(R&&)
{return null;}

#else

template<typename R> forceinline Meta::EnableIf<
	IsInputRange<R>::_ && !Meta::IsConst<R>::_,
null_t> end(R&&)
{return null;}

#endif

}}

