#pragma once

#include "Platform/CppFeatures.h"
#include "Platform/CppWarnings.h"
#include "Platform/FundamentalTypes.h"
#include "Meta/Type.h"
#include "Meta/Tuple.h"
#include "Meta/Preprocessor.h"

namespace Intra { namespace Range {

INTRA_PUSH_DISABLE_ALL_WARNINGS

namespace Concepts {

INTRA_DEFINE_EXPRESSION_CHECKER(HasEmpty, static_cast<bool>(Meta::Val<T>().Empty()));
INTRA_DEFINE_EXPRESSION_CHECKER(HasLength, static_cast<size_t>(Meta::Val<T>().Length()));
INTRA_DEFINE_EXPRESSION_CHECKER(HasPopFirst, Meta::Val<T>().PopFirst());
INTRA_DEFINE_EXPRESSION_CHECKER(HasFirst, Meta::Val<T>().First());
INTRA_DEFINE_EXPRESSION_CHECKER(HasData, Meta::Val<T>().Data()==null);

template<typename R>
struct IsArrayClass: Meta::TypeFromValue<bool,
	HasLength<R>::_ && HasData<R>::_
> {};

template<typename R> struct HasSlicing: Meta::IsCallable<R, size_t, size_t> {};

namespace RD {

INTRA_DEFINE_EXPRESSION_CHECKER(Has_value_type, Meta::Val<typename Meta::RemoveReference<T>::value_type>());

template<typename R, bool = HasFirst<R>::_> struct ReturnValueTypeOf2
{typedef void _;};

template<typename R> struct ReturnValueTypeOf2<R, true>
{
	typedef Meta::RemoveReference<R> RMut;
	typedef decltype(Meta::Val<RMut>().First()) _;
};

template<typename R> struct ReturnValueTypeOf {typedef typename ReturnValueTypeOf2<R>::_ _;};

template<typename R, bool=HasFirst<R>::_, bool=Has_value_type<R>::_> struct ValueTypeOf
{typedef Meta::RemoveConstRef<decltype(Meta::Val<R>().First())> _;};

template<typename R> struct ValueTypeOf<R, false, false>
{typedef void _;};

template<typename R> struct ValueTypeOf<R, false, true>
{
	typedef Meta::RemoveReference<R> R2;
	typedef typename R2::value_type _;
};



template<typename R, bool=IsArrayClass<R>::_> struct ValueTypeOfArray
{
	typedef decltype(*Meta::Val<R>().Data()) TMut;
	typedef Meta::RemoveConstRef<TMut> _;
};

template<typename R> struct ValueTypeOfArray<R, false>
{typedef void _;};

template<typename R, bool=IsArrayClass<R>::_> struct ReturnValueTypeOfArray
{typedef decltype(*Meta::Val<R>().Data()) _;};

template<typename R> struct ReturnValueTypeOfArray<R, false>
{typedef void _;};

}

template<typename R> using ReturnValueTypeOf = typename RD::ReturnValueTypeOf<R>::_;

template<typename R> using ValueTypeOf = typename RD::ValueTypeOf<R>::_;
template<typename R> using ValueTypeOfArray = typename RD::ValueTypeOfArray<R>::_;
template<typename R> using ReturnValueTypeOfArray = typename RD::ReturnValueTypeOfArray<R>::_;
template<typename R> using SliceTypeOf = Meta::ResultOfOrVoid<R, size_t, size_t>;

INTRA_DEFINE_EXPRESSION_CHECKER(HasLast, Meta::Val<T>().Last());
INTRA_DEFINE_EXPRESSION_CHECKER(HasPopLast, Meta::Val<T>().PopLast());
INTRA_DEFINE_EXPRESSION_CHECKER(HasIndex, Meta::Val<T>()[size_t()]);

INTRA_DEFINE_EXPRESSION_CHECKER2(HasPut, Meta::Val<T1>().Put(Meta::Val<T2>()),,);

namespace TypeEnum
{
	typedef byte Type;
	enum: Type {NotRange, Input, Forward, Bidirectional, RandomAccess, Array};
}

template<typename R>
struct IsOutputRange: Meta::TypeFromValue<bool,
	HasPut<R, ValueTypeOf<R>>::_
> {};

template<typename R, typename T>
struct IsOutputRangeOf: Meta::TypeFromValue<bool,
	HasPut<R, T>::_
> {};

template<typename R>
struct IsInputRange: Meta::TypeFromValue<bool,
	HasFirst<R>::_ && HasPopFirst<Meta::RemoveConst<R>>::_ && HasEmpty<R>::_
> {};

template<typename R>
struct IsInputRangeWithLength: Meta::TypeFromValue<bool,
	IsInputRange<R>::_ && HasLength<R>::_
> {};

template<typename T> struct IsForwardRange: Meta::TypeFromValue<bool,
	IsInputRange<T>::_ && Meta::IsCopyConstructible<Meta::RemoveReference<T>>::_
> {};

template<typename R>
struct IsForwardRangeWithLength: Meta::TypeFromValue<bool,
	IsForwardRange<R>::_ && HasLength<R>::_
> {};

template<typename R>
struct IsBidirectionalRange: Meta::TypeFromValue<bool,
	IsForwardRange<R>::_ &&
	HasLast<R>::_ && HasPopLast<Meta::RemoveConst<R>>::_
> {};

template<typename R>
struct IsBidirectionalRangeWithLength: Meta::TypeFromValue<bool,
	IsBidirectionalRange<R>::_ && HasLength<R>::_
> {};

template<typename R>
struct IsRandomAccessRange: Meta::TypeFromValue<bool,
	IsForwardRange<R>::_ &&
	HasIndex<R>::_
> {};

template<typename R>
struct IsRandomAccessRangeWithLength: Meta::TypeFromValue<bool,
	IsRandomAccessRange<R>::_ && HasLength<R>::_
> {};

template<typename R>
struct IsArrayRange: Meta::TypeFromValue<bool,
	IsRandomAccessRange<R>::_ &&
	IsArrayClass<R>::_
> {};

template<typename R>
struct IsTrivCopyableArray: Meta::TypeFromValue<bool,
	IsArrayClass<R>::_ &&
	Meta::IsTriviallyCopyable<ValueTypeOf<R>>::_
> {};

template<typename R1, typename R2>
struct IsTrivCopyCompatibleArrayWith: Meta::TypeFromValue<bool,
	IsTrivCopyableArray<R1>::_ &&
	IsArrayClass<R2>::_ &&
	Meta::TypeEqualsIgnoreCV<ValueTypeOf<R1>, ValueTypeOf<R2>>::_
> {};

template<typename T1, typename T2> struct IsArrayRangeOfExactly: Meta::TypeFromValue<bool,
	IsArrayRange<T1>::_ &&
	Meta::TypeEqualsIgnoreCV<ValueTypeOf<T1>, T2>::_
> {};


template<class T> struct HasRangeIsFinite
{
    template<bool> struct dummy {};
    template<typename U> static char test(...);
    template<typename U> static short test(dummy<U::RangeIsFinite>*);
    enum: bool {_=(sizeof(test<Meta::RemoveConstRef<T>>(0))==sizeof(short))};
};

template<class T> struct HasRangeIsInfinite
{
    template<bool> struct dummy {};
    template<typename U> static char test(...);
    template<typename U> static short test(dummy<U::RangeIsInfinite>*);
    enum: bool {_=(sizeof(test<Meta::RemoveConstRef<T>>(0))==sizeof(short))};
};

template<typename R, bool=HasRangeIsFinite<R>::_, typename Range=Meta::RemoveConstRef<R>> struct IsFiniteRange:
	Meta::TypeFromValue<bool, Range::RangeIsFinite> {};
template<typename R> struct IsFiniteRange<R, false>:
	Meta::TypeFromValue<bool, HasLength<R>::_ || HasLast<R>::_ || HasPopLast<R>::_> {};

template<typename R, bool=HasRangeIsInfinite<R>::_, typename Range=Meta::RemoveConstRef<R>> struct IsInfiniteRange:
	Meta::TypeFromValue<bool, Range::RangeIsInfinite> {};
template<typename R> struct IsInfiniteRange<R, false>:
	Meta::TypeFromValue<bool, false> {};


template<typename T> struct IsCharRange: Meta::TypeFromValue<bool,
	IsInputRange<T>::_ && Meta::IsCharType<ValueTypeOf<T>>::_
> {};

template<typename T> struct IsForwardCharRange: Meta::TypeFromValue<bool,
	IsForwardRange<T>::_ && Meta::IsCharType<ValueTypeOf<T>>::_
> {};

template<typename T> struct IsOutputCharRange: Meta::TypeFromValue<bool,
	HasPut<T, char>::_// || HasPut<T, wchar>::_ || HasPut<T, dchar>::_ || HasPut<T, wchar_t>::_
> {};

template<typename T> struct IsAssignableRange: Meta::TypeFromValue<bool,
	IsInputRange<T>::_ && Meta::IsNCLValueReference<ReturnValueTypeOf<T>>::_
> {};


template<typename R>
struct IsFiniteInputRange: Meta::TypeFromValue<bool,
	IsInputRange<R>::_ && IsFiniteRange<R>::_
> {};

template<typename R>
struct IsNonInfiniteInputRange: Meta::TypeFromValue<bool,
	IsInputRange<R>::_ && !IsInfiniteRange<R>::_
> {};

template<typename R>
struct IsFiniteForwardRange: Meta::TypeFromValue<bool,
	IsForwardRange<R>::_ && IsFiniteRange<R>::_
> {};

template<typename R>
struct IsNonInfiniteForwardRange: Meta::TypeFromValue<bool,
	IsForwardRange<R>::_ && !IsInfiniteRange<R>::_
> {};

template<typename R>
struct IsFiniteRandomAccessRange: Meta::TypeFromValue<bool,
	IsRandomAccessRange<R>::_ && IsFiniteRange<R>::_
> {};

template<typename R>
struct IsNonInfiniteRandomAccessRange: Meta::TypeFromValue<bool,
	IsRandomAccessRange<R>::_ && !IsInfiniteRange<R>::_
> {};


template<typename R1, typename R2>
struct ValueTypeEquals: Meta::TypeFromValue<bool,
	Meta::TypeEquals<ValueTypeOf<R1>, ValueTypeOf<R2>>::_
> {};

template<typename R, typename T> struct IsFiniteInputRangeOfExactly: Meta::TypeFromValue<bool, 
	IsFiniteInputRange<R>::_ && Meta::TypeEquals<ValueTypeOf<R>, T>::_
> {};

template<typename R, typename T> struct IsFiniteForwardRangeOfExactly: Meta::TypeFromValue<bool, 
	IsFiniteForwardRange<R>::_ && Meta::TypeEquals<ValueTypeOf<R>, T>::_
> {};

template<typename R> struct IsAccessibleRange: Meta::TypeFromValue<bool,
	IsInputRange<R>::_ &&
	(Meta::IsNCRValueReference<R>::_ ||
		Meta::IsCopyConstructible<Meta::RemoveReference<R>>::_)
> {};

template<typename R> struct IsConsumableRange: Meta::TypeFromValue<bool,
	IsAccessibleRange<R>::_ && !IsInfiniteRange<R>::_
> {};

template<typename R> struct IsAccessibleRangeWithLength: Meta::TypeFromValue<bool,
	IsAccessibleRange<R>::_ && HasLength<R>::_
> {};

template<typename R, typename T> struct IsConsumableRangeOf: Meta::TypeFromValue<bool,
	IsConsumableRange<R>::_ &&
	Meta::IsConvertible<ValueTypeOf<R>, T>::_
> {};


template<typename P, typename... R> struct IsElementPredicate:
	Meta::TypeEquals<bool, Meta::ResultOfOrVoid<P, ValueTypeOf<R>...>> {};



template<class R> struct RangeCategory: Meta::TypeFromValue<byte,
	!IsInputRange<R>::_?
		TypeEnum::NotRange:
	!Meta::IsCopyConstructible<R>::_?
		TypeEnum::Input:
		HasIndex<R>::_?
			(HasData<R>::_?
				TypeEnum::Array:
				TypeEnum::RandomAccess):
			(HasLast<R>::_ && HasPopLast<R>::_)?
				TypeEnum::Bidirectional:
				TypeEnum::Forward
> {};


template<typename... RANGES> struct CommonRangeCategoryAnyFinite;
template<typename R0> struct CommonRangeCategoryAnyFinite<R0>
{
	enum: byte {Type = RangeCategory<R0>::_};
	enum: bool {Finite = IsFiniteRange<R0>::_};
};

template<typename R0, typename R1, typename... RANGES> struct CommonRangeCategoryAnyFinite<R0, R1, RANGES...>
{
	typedef CommonRangeCategoryAnyFinite<R1, RANGES...> Rest;
	enum: byte {Type = byte(Rest::Type)>byte(RangeCategory<R0>::_)? byte(RangeCategory<R0>::_): byte(Rest::Type)};
	enum: byte {Finite = Rest::Finite || IsFiniteRange<R0>::_};
};



template<typename... RANGES> struct CommonRangeCategoryAllFinite;
template<typename R0> struct CommonRangeCategoryAllFinite<R0>
{
	enum: byte {Type = byte(RangeCategory<R0>::_)==TypeEnum::Array?
		byte(TypeEnum::RandomAccess):
		byte(RangeCategory<R0>::_)};
	enum: bool {Finite = IsFiniteRange<R0>::_};
};

template<typename R0, typename R1, typename... RANGES> struct CommonRangeCategoryAllFinite<R0, R1, RANGES...>
{
	typedef CommonRangeCategoryAllFinite<R1, RANGES...> Rest;
	enum: byte {Type = byte(Rest::Type)? byte(RangeCategory<R0>::_): byte(Rest::Type)};
	enum: bool {Finite = Rest::Finite && IsFiniteRange<R0>::_};
};

}

using namespace Concepts;

}

INTRA_WARNING_POP

}
