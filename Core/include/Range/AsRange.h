#pragma once

#include "Platform/CppWarnings.h"
#include "Concepts.h"
#include "ForwardDecls.h"

namespace Intra {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Range {

template<typename R> forceinline Meta::EnableIf<
	IsInputRange<R>::_ || IsOutputRange<R>::_ || IsOutputCharRange<R>::_,
R&&> AsRange(R&& r) {return Meta::Forward<R>(r);}

INTRA_DEFINE_EXPRESSION_CHECKER(HasAsRangeMethod, Meta::Val<T>().AsRange());

template<typename T, typename=Meta::EnableIf<
	!IsInputRange<T>::_ && HasAsRangeMethod<T>::_
>> decltype(Meta::Val<T>().AsRange()) AsRange(T&& v) {return v.AsRange();}

INTRA_DEFINE_EXPRESSION_CHECKER(HasAsRange, AsRange(Meta::Val<T>()));

template<typename T> struct ArrayRange;
template<typename Char> struct GenericStringView;

namespace RD {

template<typename T, int=HasAsRange<T>::_? 1:
	(Meta::IsArrayType<Meta::RemoveReference<T>>::_? 2: 0)
> struct AsRangeResult;
template<typename T> struct AsRangeResult<T, 1>
{typedef decltype(AsRange(Meta::Val<T>())) _;};

template<typename T> struct AsRangeResult<T, 0>
{typedef T _;};

template<typename T, size_t N> struct AsRangeResult<T(&)[N], 2>
{typedef Meta::SelectType<GenericStringView<Meta::RemoveConst<T>>, ArrayRange<T>, Meta::IsCharType<T>::_> _;};

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

template<typename R> forceinline Meta::EnableIf<
	IsAsInputRange<R>::_ || IsAsOutputRange<R>::_,
AsRangeResult<R&&>> Forward(Meta::RemoveReference<R>& t)
{return AsRange(static_cast<R&&>(t));}

template<typename R> forceinline Meta::EnableIf<
	IsAsInputRange<R>::_ || IsAsOutputRange<R>::_,
AsRangeResult<R&&>> Forward(Meta::RemoveReference<R>&& t)
{
	static_assert(!Meta::IsLValueReference<R>::_, "Bad Range::Forward call!");
	return AsRange(static_cast<R&&>(t));
}

}

using Range::AsRange;

INTRA_WARNING_POP
}
