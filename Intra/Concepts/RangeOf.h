#pragma once

#include "Cpp/Warnings.h"

#include "Meta/Type.h"
#include "Meta/Pair.h"

#include "Range.h"
#include "Container.h"
#include "Iterator.h"

namespace Intra {

namespace Range {
template<typename T> struct Span;
template<typename I1, typename I2 = I1> struct IteratorRange;

template<typename T> struct FListNode;
template<typename T> struct BListNode;
template<typename T, typename Node = FListNode<T>> struct FListRange;
template<typename T, typename Node = BListNode<T>> struct BListRange;
}

namespace Concepts {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

INTRA_DEFINE_EXPRESSION_CHECKER(HasNextListNodeMethod, ::Intra::Meta::Val<T>().NextListNode());
INTRA_DEFINE_EXPRESSION_CHECKER(HasPrevListNodeMethod, ::Intra::Meta::Val<T>().PrevListNode());

}

namespace Range {

template<typename T> Meta::EnableIf<
	!Concepts::IsInputRange<T>::_ &&
	Concepts::HasNextListNodeMethod<T>::_/* &&
	  !HasPrevListNodeMethod<T>::_*/,
FListRange<T, T>> RangeOf(T& objectWithIntrusiveList);

}

namespace Concepts {


template<typename R> forceinline Meta::EnableIf<
	IsInputRange<R>::_ ||
	IsOutputRange<Meta::RemoveConst<R>>::_ ||
	IsOutputCharRange<Meta::RemoveConst<R>>::_,
R&&> RangeOf(R&& r) {return Cpp::Forward<R>(r);}

INTRA_DEFINE_EXPRESSION_CHECKER(HasAsRangeMethod, Meta::Val<T>().AsRange());

template<typename T, typename = Meta::EnableIf<
	!IsInputRange<T>::_ &&
	HasAsRangeMethod<T>::_
>> forceinline decltype(Meta::Val<T>().AsRange()) RangeOf(T&& v) {return v.AsRange();}

template<typename C, typename D = C, typename = Meta::EnableIf<
	!IsInputRange<C>::_ &&
	!IsArrayClass<C>::_ &&
	!HasAsRangeMethod<C>::_ &&
	Has_begin_end<C>::_
>> forceinline Range::IteratorRange<decltype(begin(Meta::Val<D>())), decltype(end(Meta::Val<D>()))> RangeOf(C&& v)
{return {begin(v), end(v)};}

template<typename R> forceinline Meta::EnableIf<
	!IsInputRange<R>::_ &&
	!HasAsRangeMethod<R>::_,
Range::Span<Meta::RemovePointer<PtrElementTypeOfArrayOrDisable<R>>>> RangeOf(R&& r) noexcept;

template<typename T, size_t N> forceinline constexpr Range::Span<T> RangeOf(T(&arr)[N]) noexcept;


INTRA_DEFINE_EXPRESSION_CHECKER(HasRangeOf, RangeOf(Meta::Val<T>()));

namespace RD {

template<typename T, bool = HasRangeOf<T>::_> struct TRangeOfType
{typedef T _;};

template<typename T, size_t N> struct TRangeOfType<T[N], false>
{typedef Range::Span<T> _;};

template<typename T, size_t N> struct TRangeOfType<T(&)[N], false>
{typedef Range::Span<T> _;};

template<typename T> struct TRangeOfType<T, true>
{typedef decltype(RangeOf(Meta::Val<T>())) _;};

template<typename T> struct TRangeOfTypeNoCRef
{typedef Meta::RemoveConstRef<typename TRangeOfType<T>::_> _;};

}

template<typename T> using RangeOfType = typename RD::TRangeOfType<T>::_;
template<typename T> using RangeOfTypeNoRef = Meta::RemoveReference<typename RD::TRangeOfType<T>::_>;
template<typename T> using RangeOfTypeNoCRef = typename RD::TRangeOfTypeNoCRef<T>::_;


template<typename T> using IsAsFiniteRange = IsFiniteRange<RangeOfType<T>>;

template<typename T> using IsAsInputRange = IsInputRange<RangeOfType<T>>;
template<typename T> using IsAsFiniteInputRange = IsFiniteInputRange<RangeOfType<T>>;
template<typename T> using IsAsNonInfiniteInputRange = IsNonInfiniteInputRange<RangeOfType<T>>;

template<typename T> using IsAsAssignableRange = IsAssignableRange<RangeOfType<T>>;

template<typename T> using IsAsOutputRange = IsOutputRange<RangeOfType<T>>;
template<typename T, typename U> using IsAsOutputRangeOf = IsOutputRangeOf<RangeOfType<T>, U>;

template<typename T> using IsAsForwardRange = IsForwardRange<RangeOfType<T>>;
template<typename T> using IsAsFiniteForwardRange = IsFiniteForwardRange<RangeOfType<T>>;
template<typename T> struct IsAsNonInfiniteForwardRange: IsNonInfiniteForwardRange<RangeOfType<T>> {};

template<typename T> using IsAsBidirectionalRange = IsBidirectionalRange<RangeOfType<T>>;

template<typename T> using IsAsRandomAccessRange = IsRandomAccessRange<RangeOfType<T>>;
template<typename T> using IsAsFiniteRandomAccessRange = IsFiniteRandomAccessRange<RangeOfType<T>>;
template<typename T> using IsAsNonInfiniteRandomAccessRange = IsNonInfiniteRandomAccessRange<RangeOfType<T>>;

template<typename T> using IsAsArrayRange = IsArrayRange<RangeOfType<T>>;
template<typename T1, typename T2> using IsAsArrayRangeOfExactly = IsArrayRangeOfExactly<RangeOfType<T1>, T2>;
template<typename T> using IsAsInfiniteRange = IsInfiniteRange<RangeOfType<T>>;
template<typename T> using IsAsCharRange = IsCharRange<RangeOfType<T>>;
template<typename T> using IsAsForwardCharRange = IsForwardCharRange<RangeOfType<T>>;
template<typename T> using IsAsOutputCharRange = IsOutputCharRange<RangeOfType<T>>;

template<typename T> using ReturnValueTypeOfAs = ReturnValueTypeOf<RangeOfType<T>>;
template<typename T> using ValueTypeOfAs = ValueTypeOf<RangeOfType<T>>;

template<typename R> struct IsAsAccessibleRange: IsAccessibleRange<RangeOfType<R>> {};
template<typename R> struct IsAsConsumableRange: IsConsumableRange<RangeOfType<R>> {};
template<typename R, typename T> struct IsAsConsumableRangeOf: IsConsumableRangeOf<RangeOfType<R>, T> {};

}

namespace Range {

using Concepts::RangeOf;

template<typename R, typename AsR = Concepts::RangeOfType<R&&>> forceinline Meta::EnableIf<
	Concepts::IsInputRange<AsR>::_ ||
	Concepts::IsOutputRange<Meta::RemoveConst<AsR>>::_ ||
	Concepts::IsOutputCharRange<Meta::RemoveConst<AsR>>::_,
AsR> Forward(Meta::RemoveReference<R>& t)
{return RangeOf(static_cast<R&&>(t));}

template<typename R, typename AsR = Concepts::RangeOfType<R&&>> forceinline Meta::EnableIf<
	Concepts::IsInputRange<AsR>::_ ||
	Concepts::IsOutputRange<Meta::RemoveConst<AsR>>::_ ||
	Concepts::IsOutputCharRange<Meta::RemoveConst<AsR>>::_,
Concepts::RangeOfType<R&&>> Forward(Meta::RemoveReference<R>&& t)
{
	static_assert(!Meta::IsLValueReference<R>::_, "Bad Range::Forward call!");
	return RangeOf(static_cast<R&&>(t));
}


template<typename R, typename T> forceinline Meta::EnableIf<
	Concepts::IsAsOutputRangeOf<Meta::RemoveConst<R>, T>::_,
Concepts::RangeOfType<R&&>> ForwardOutputOf(Meta::RemoveReference<R>& t)
{return RangeOf(static_cast<R&&>(t));}

template<typename R, typename T> forceinline Meta::EnableIf<
	Concepts::IsAsOutputRangeOf<Meta::RemoveConst<R>, T>::_,
Concepts::RangeOfType<R&&>> ForwardOutputOf(Meta::RemoveReference<R>&& t)
{
	static_assert(!Meta::IsLValueReference<R>::_, "Bad Range::Forward call!");
	return RangeOf(static_cast<R&&>(t));
}

}

using Concepts::RangeOf;

INTRA_WARNING_POP
}
