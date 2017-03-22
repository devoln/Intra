#pragma once

#include "Platform/CppWarnings.h"
#include "Concepts.h"
#include "ForwardDecls.h"
#include "Container/Concepts.h"
#include "Iterator/IteratorConcepts.h"
#include "Iterator/IteratorRange.h"

namespace Intra { namespace Range {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename R> forceinline Meta::EnableIf<
	IsInputRange<R>::_ || IsOutputRange<Meta::RemoveConst<R>>::_ || IsOutputCharRange<Meta::RemoveConst<R>>::_,
R&&> AsRange(R&& r) {return Meta::Forward<R>(r);}

INTRA_DEFINE_EXPRESSION_CHECKER(HasAsRangeMethod, Meta::Val<T>().AsRange());

template<typename T, typename=Meta::EnableIf<
	!IsInputRange<T>::_ && HasAsRangeMethod<T>::_
>> decltype(Meta::Val<T>().AsRange()) AsRange(T&& v) {return v.AsRange();}

template<typename T> struct ArrayRange;
template<typename Char> struct GenericStringView;

template<typename C, typename D=C, typename=Meta::EnableIf<
	!IsInputRange<C>::_ && !HasAsRangeMethod<C>::_ &&
	Container::Has_data<C>::_ && Container::Has_size<C>::_
>> forceinline ArrayRange<Meta::RemoveReference<decltype(*Meta::Val<D>().begin())>> AsRange(C&& v)
{
	typedef Meta::RemoveReference<decltype(*Meta::Val<C>().begin())> T;
	return {const_cast<T*>(v.data()), v.size()};
}

template<typename C, typename D=C, typename E=D, typename=Meta::EnableIf<
	!IsInputRange<C>::_ && !HasAsRangeMethod<C>::_ &&
	!(Container::Has_data<C>::_ && Container::Has_size<C>::_) &&
	IteratorConcepts::Has_begin_end<C>::_
>> forceinline IteratorRange<decltype(Meta::Val<D>().begin()), decltype(Meta::Val<E>().end())> AsRange(C&& v)
{return {v.begin(), v.end()};}


INTRA_DEFINE_EXPRESSION_CHECKER(HasAsRange, AsRange(Meta::Val<T>()));

namespace Concepts {

namespace RD {

template<typename T, bool=HasAsRange<T>::_> struct AsRangeResult
{typedef T _;};

template<typename T> struct AsRangeResult<T, true>
{typedef decltype(AsRange(Meta::Val<T>())) _;};

template<typename T> struct AsRangeResultNoCRef
{typedef Meta::RemoveConstRef<typename AsRangeResult<T>::_> _;};

}

template<typename T> using AsRangeResult = typename RD::AsRangeResult<T>::_;
template<typename T> using AsRangeResultNoCRef = typename RD::AsRangeResultNoCRef<T>::_;


template<typename T> using IsAsFiniteRange = IsFiniteRange<AsRangeResult<T>>;

template<typename T> using IsAsInputRange = IsInputRange<AsRangeResult<T>>;
template<typename T> using IsAsFiniteInputRange = IsFiniteInputRange<AsRangeResult<T>>;
template<typename T> using IsAsNonInfiniteInputRange = IsNonInfiniteInputRange<AsRangeResult<T>>;

template<typename T> using IsAsAssignableRange = IsAssignableRange<AsRangeResult<T>>;

template<typename T> using IsAsOutputRange = IsOutputRange<AsRangeResult<T>>;
template<typename T, typename U> using IsAsOutputRangeOf = IsOutputRangeOf<AsRangeResult<T>, U>;

template<typename T> using IsAsForwardRange = IsForwardRange<AsRangeResult<T>>;
template<typename T> using IsAsFiniteForwardRange = IsFiniteForwardRange<AsRangeResult<T>>;
template<typename T> struct IsAsNonInfiniteForwardRange: IsNonInfiniteForwardRange<AsRangeResult<T>> {};

template<typename T> using IsAsBidirectionalRange = IsBidirectionalRange<AsRangeResult<T>>;

template<typename T> using IsAsRandomAccessRange = IsRandomAccessRange<AsRangeResult<T>>;
template<typename T> using IsAsFiniteRandomAccessRange = IsFiniteRandomAccessRange<AsRangeResult<T>>;
template<typename T> using IsAsNonInfiniteRandomAccessRange = IsNonInfiniteRandomAccessRange<AsRangeResult<T>>;

template<typename T> using IsAsArrayRange = IsArrayRange<AsRangeResult<T>>;
template<typename T1, typename T2> using IsAsArrayRangeOfExactly = IsArrayRangeOfExactly<AsRangeResult<T1>, T2>;
template<typename T> using IsAsInfiniteRange = IsInfiniteRange<AsRangeResult<T>>;
template<typename T> using IsAsCharRange = IsCharRange<AsRangeResult<T>>;
template<typename T> using IsAsForwardCharRange = IsForwardCharRange<AsRangeResult<T>>;
template<typename T> using IsAsOutputCharRange = IsOutputCharRange<AsRangeResult<T>>;

template<typename T> using ReturnValueTypeOfAs = ReturnValueTypeOf<AsRangeResult<T>>;
template<typename T> using ValueTypeOfAs = ValueTypeOf<AsRangeResult<T>>;

template<typename R> struct IsAsAccessibleRange: IsAccessibleRange<AsRangeResult<R>> {};
template<typename R> struct IsAsConsumableRange: IsConsumableRange<AsRangeResult<R>> {};
template<typename R, typename T> struct IsAsConsumableRangeOf: IsConsumableRangeOf<AsRangeResult<R>, T> {};

}
using namespace Concepts;

template<typename R, typename AsR = AsRangeResult<R&&>> forceinline Meta::EnableIf<
	IsInputRange<AsR>::_ || IsOutputRange<Meta::RemoveConst<AsR>>::_ || IsOutputCharRange<Meta::RemoveConst<AsR>>::_,
AsR> Forward(Meta::RemoveReference<R>& t)
{return AsRange(static_cast<R&&>(t));}

template<typename R, typename AsR = AsRangeResult<R&&>> forceinline Meta::EnableIf<
	IsInputRange<AsR>::_ || IsOutputRange<Meta::RemoveConst<AsR>>::_ || IsOutputCharRange<Meta::RemoveConst<AsR>>::_,
AsRangeResult<R&&>> Forward(Meta::RemoveReference<R>&& t)
{
	static_assert(!Meta::IsLValueReference<R>::_, "Bad Range::Forward call!");
	return AsRange(static_cast<R&&>(t));
}


template<typename R, typename T> forceinline Meta::EnableIf<
	IsAsOutputRangeOf<Meta::RemoveConst<R>, T>::_,
AsRangeResult<R&&>> ForwardOutputOf(Meta::RemoveReference<R>& t)
{return AsRange(static_cast<R&&>(t));}

template<typename R, typename T> forceinline Meta::EnableIf<
	IsAsOutputRangeOf<Meta::RemoveConst<R>, T>::_,
AsRangeResult<R&&>> ForwardOutputOf(Meta::RemoveReference<R>&& t)
{
	static_assert(!Meta::IsLValueReference<R>::_, "Bad Range::Forward call!");
	return AsRange(static_cast<R&&>(t));
}

}

using Range::AsRange;

INTRA_WARNING_POP
}
