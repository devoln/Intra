#pragma once

#include "Platform/CppFeatures.h"
#include "Platform/CppWarnings.h"
#include "Core/FundamentalTypes.h"
#include "Meta/Type.h"
#include "Meta/Tuple.h"
#include "Meta/Preprocessor.h"

namespace Intra { namespace Range {

INTRA_PUSH_DISABLE_ALL_WARNINGS

INTRA_DEFINE_EXPRESSION_CHECKER(HasEmpty, static_cast<bool>(Meta::Val<T>().Empty()));
INTRA_DEFINE_EXPRESSION_CHECKER(HasLength, static_cast<size_t>(Meta::Val<T>().Length()));
INTRA_DEFINE_EXPRESSION_CHECKER(HasPopFirst, Meta::Val<T>().PopFirst());
INTRA_DEFINE_EXPRESSION_CHECKER(HasFirst, Meta::Val<T>().First());

template<typename R> struct HasSlicing: Meta::IsCallable<R, size_t, size_t> {};

namespace RD {

INTRA_DEFINE_EXPRESSION_CHECKER(HasValueType, Meta::Val<typename T::value_type>());

template<typename R, bool=HasFirst<Meta::RemoveConstRef<R>>::_> struct ReturnValueTypeOf2
{
	typedef Meta::RemoveConstRef<R> RMut;
	typedef decltype(Meta::Val<RMut>().First()) _;
};

template<typename R> struct ReturnValueTypeOf2<R, false>
{typedef void _;};

template<typename R> struct ReturnValueTypeOf: ReturnValueTypeOf2<R> {};

template<typename R, bool=HasSlicing<Meta::RemoveConstRef<R>>::_> struct SliceTypeOf
{
	typedef Meta::RemoveConstRef<R> RMut;
	typedef Meta::ResultOf<RMut, size_t, size_t> _;
};

template<typename R> struct SliceTypeOf<R, false>
{typedef void _;};

template<typename R, bool=HasFirst<Meta::RemoveConstRef<R>>::_, bool=HasValueType<Meta::RemoveConstRef<R>>::_> struct ValueTypeOf
{typedef Meta::RemoveConstRef<typename ReturnValueTypeOf<R>::_> _;};

template<typename R> struct ValueTypeOf<R, false, true>
{
	typedef Meta::RemoveConstRef<R> Range;
	typedef typename Range::value_type _;
};

template<typename R> struct ValueTypeOf<R, false, false>
{typedef void _;};

}

template<typename R> using ReturnValueTypeOf = typename RD::ReturnValueTypeOf<R>::_;

template<typename R> using ValueTypeOf = typename RD::ValueTypeOf<R>::_;
template<typename R> using SliceTypeOf = typename RD::SliceTypeOf<R>::_;

template<typename R> struct ValueTypeIsChar: Meta::IsCharType<ValueTypeOf<R>> {};
template<typename R> struct ValueTypeIsTuple: Meta::IsTuple<ValueTypeOf<R>> {};
template<typename R, typename T> struct ValueTypeIsConvertible: Meta::IsConvertible<ValueTypeOf<R>, T> {};

INTRA_DEFINE_EXPRESSION_CHECKER(HasLast, Meta::Val<T>().Last());
INTRA_DEFINE_EXPRESSION_CHECKER(HasPopLast, Meta::Val<T>().PopLast());
INTRA_DEFINE_EXPRESSION_CHECKER(HasIndex, Meta::Val<T>()[size_t()]);
INTRA_DEFINE_EXPRESSION_CHECKER(HasData, Meta::Val<T>().Data()==static_cast<ValueTypeOf<T>*>(null));

INTRA_DEFINE_EXPRESSION_CHECKER(HasPut, Meta::Val<T>().Put(Meta::Val<ValueTypeOf<T>>()));
INTRA_DEFINE_EXPRESSION_CHECKER2(HasTypedPut, Meta::Val<T1>().Put(Meta::Val<T2>()),,);

namespace TypeEnum
{
	typedef byte Type;
	enum: Type {NotRange, Input, Forward, Bidirectional, RandomAccess, Array};
}

template<typename R>
struct IsOutputRange: Meta::TypeFromValue<bool,
	HasPut<Meta::RemoveReference<R>>::_
> {};

template<typename R>
struct IsInputRange: Meta::TypeFromValue<bool,
	HasFirst<R>::_ && HasPopFirst<Meta::RemoveConstRef<R>>::_ && HasEmpty<R>::_
> {};

INTRA_DEFINE_EXPRESSION_CHECKER_WITH_CONDITION(IsForwardRange, Meta::Val<ValueTypeOf<T>>(),
	IsInputRange<T>::_ && Meta::IsCopyConstructible<Meta::RemoveReference<T>>::_);

template<typename R>
struct IsBidirectionalRange: Meta::TypeFromValue<bool,
	IsForwardRange<R>::_ &&
	HasLast<R>::_ && HasPopLast<Meta::RemoveConstRef<R>>::_
> {};

template<typename R>
struct IsRandomAccessRange: Meta::TypeFromValue<bool,
	IsForwardRange<R>::_ &&
	HasIndex<R>::_
> {};


template<typename R>
struct IsFiniteRandomAccessRange: Meta::TypeFromValue<bool,
	IsRandomAccessRange<R>::_ &&
	HasLength<R>::_
> {};

template<typename R>
struct IsArrayRange: Meta::TypeFromValue<bool,
	IsFiniteRandomAccessRange<R>::_ &&
	HasData<R>::_
> {};

template<typename T1, typename T2> struct IsArrayRangeOfExactly: Meta::TypeFromValue<bool,
	IsArrayRange<T1>::_ && Meta::TypeEqualsIgnoreCV<ValueTypeOf<T1>, T2>::_
> {};


template<class R> struct GetRangeType: Meta::TypeFromValue<byte,
	!IsInputRange<R>::_?
		TypeEnum::NotRange:
	!Meta::IsCopyConstructible<R>::_?
		TypeEnum::Input:
		(HasIndex<R>::_ && HasSlicing<R>::_)?
			(HasData<R>::_?
				TypeEnum::Array:
				TypeEnum::RandomAccess):
			(HasLast<R>::_ && HasPopLast<R>::_)?
				TypeEnum::Bidirectional:
				TypeEnum::Forward
> {};


template<class T> struct HasRangeIsFinite
{
    template<bool> struct Helper {};
    template<typename U> static char HelpMe(...);
    template<typename U> static short HelpMe(Helper<U::RangeIsFinite>*);
    enum: bool {_=(sizeof(HelpMe<Meta::RemoveConstRef<T>>(0))==sizeof(short))};
};

template<class T> struct HasRangeIsInfinite
{
    template<bool> struct Helper {};
    template<typename U> static char HelpMe(...);
    template<typename U> static short HelpMe(Helper<U::RangeIsInfinite>*);
    enum: bool {_=(sizeof(HelpMe<Meta::RemoveConstRef<T>>(0))==sizeof(short))};
};

template<typename R, bool=HasRangeIsFinite<R>::_, typename Range=Meta::RemoveConstRef<R>> struct IsFiniteRange:
	Meta::TypeFromValue<bool, Range::RangeIsFinite> {};
template<typename R> struct IsFiniteRange<R, false>:
	Meta::TypeFromValue<bool, HasLength<R>::_ || HasLast<R>::_ || HasPopLast<R>::_> {};

template<typename R, bool=HasRangeIsInfinite<R>::_, typename Range=Meta::RemoveConstRef<R>> struct IsInfiniteRange:
	Meta::TypeFromValue<bool, !IsFiniteRange<R>::_ && Range::RangeIsInfinite> {};
template<typename R> struct IsInfiniteRange<R, false>:
	Meta::TypeFromValue<bool, false> {};


template<typename T> struct IsCharRange: Meta::TypeFromValue<bool,
	IsInputRange<T>::_ && Meta::IsCharType<ValueTypeOf<T>>::_
> {};

template<typename T> struct IsOutputCharRange: Meta::TypeFromValue<bool,
	IsOutputRange<T>::_ && Meta::IsCharType<ValueTypeOf<T>>::_
> {};

template<typename T> struct IsAssignableInputRange: Meta::TypeFromValue<bool,
	IsInputRange<T>::_ && Meta::IsLValueReference<ReturnValueTypeOf<T>>::_
> {};


template<typename R>
struct IsFiniteInputRange: Meta::TypeFromValue<bool,
	IsInputRange<R>::_ && IsFiniteRange<R>::_
> {};

template<typename R>
struct IsFiniteForwardRange: Meta::TypeFromValue<bool,
	IsForwardRange<R>::_ && IsFiniteRange<R>::_
> {};

template<typename R>
struct IsInputRangeOfPod: Meta::TypeFromValue<bool,
	IsInputRange<R>::_ && Meta::IsPod<ValueTypeOf<R>>::_
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

template<typename... RANGES> struct CommonRangeCategoryAnyFinite;
template<typename R0> struct CommonRangeCategoryAnyFinite<R0>
{
	enum: byte {Type = GetRangeType<R0>::_};
	enum: bool {Finite = IsFiniteRange<R0>::_};
};

template<typename R0, typename R1, typename... RANGES> struct CommonRangeCategoryAnyFinite<R0, R1, RANGES...>
{
	typedef CommonRangeCategoryAnyFinite<R1, RANGES...> Rest;
	enum: byte {Type = byte(Rest::Type)>byte(GetRangeType<R0>::_)? byte(GetRangeType<R0>::_): byte(Rest::Type)};
	enum: byte {Finite = Rest::Finite || IsFiniteRange<R0>::_};
};



template<typename... RANGES> struct CommonRangeCategoryAllFinite;
template<typename R0> struct CommonRangeCategoryAllFinite<R0>
{
	enum: byte {Type = byte(GetRangeType<R0>::_)==TypeEnum::Array? byte(TypeEnum::RandomAccess): byte(GetRangeType<R0>::_)};
	enum: bool {Finite = IsFiniteRange<R0>::_};
};

template<typename R0, typename R1, typename... RANGES> struct CommonRangeCategoryAllFinite<R0, R1, RANGES...>
{
	typedef CommonRangeCategoryAllFinite<R1, RANGES...> Rest;
	enum: byte {Type = byte(Rest::Type)? byte(GetRangeType<R0>::_): byte(Rest::Type)};
	enum: bool {Finite = Rest::Finite && IsFiniteRange<R0>::_};
};



template<typename R> forceinline Meta::EnableIf<
	IsInputRange<R>::_ || IsOutputRange<R>::_,
R&&> AsRange(R&& r) {return Meta::Forward<R>(r);}


template<typename R> Meta::EnableIf<
	IsInputRange<R>::_,
R&> AsConstRange(R& r) {return r;}

template<typename R> Meta::EnableIf<
	IsInputRange<R>::_,
const R&> AsConstRange(const R& r) {return r;}

INTRA_DEFINE_EXPRESSION_CHECKER(HasAsRangeMethod, Meta::Val<T>().AsRange());
INTRA_DEFINE_EXPRESSION_CHECKER(HasAsConstRangeMethod, Meta::Val<T>().AsConstRange());

template<typename T, typename=Meta::EnableIf<
	!IsInputRange<T>::_ && HasAsRangeMethod<T>::_
>> decltype(Meta::Val<T>().AsRange()) AsRange(T&& v) {return v.AsRange();}
template<typename T, typename=Meta::EnableIf<
	!IsInputRange<T>::_ && HasAsConstRangeMethod<T>::_
>> decltype(Meta::Val<T>().AsConstRange()) AsConstRange(T&& v) {return v.AsConstRange();}

INTRA_DEFINE_EXPRESSION_CHECKER(HasAsRange, AsRange(Meta::Val<T>()));
INTRA_DEFINE_EXPRESSION_CHECKER(HasAsConstRange, AsConstRange(Meta::Val<T>()));

template<typename T> struct ArrayRange;
template<typename Char> struct GenericStringView;

namespace RD {

template<typename T, int=HasAsRange<T>::_? 1:
	(Meta::IsArrayType<Meta::RemoveReference<T>>::_? 2: 0)
> struct AsRangeResult;
template<typename T> struct AsRangeResult<T, 1>
{typedef decltype(AsRange(Meta::Val<T>())) _;};

template<typename T> struct AsRangeResult<T, 0>
{typedef void _;};

template<typename T, size_t N> struct AsRangeResult<T(&)[N], 2>
{typedef Meta::SelectType<GenericStringView<Meta::RemoveConst<T>>, ArrayRange<T>, Meta::IsCharType<T>::_> _;};


template<typename T, bool=HasAsConstRange<T>::_> struct AsConstRangeResult
{typedef decltype(AsConstRange(Meta::Val<T>())) _;};

template<typename T> struct AsConstRangeResult<T, false>
{typedef void _;};

}

template<typename T> using AsRangeResult = typename RD::AsRangeResult<T>::_;
template<typename T> using AsConstRangeResult = typename RD::AsConstRangeResult<T>::_;

template<typename R>
struct IsInputRangeOfContainers: Meta::TypeFromValue<bool,
	IsInputRange<R>::_ && IsInputRange<AsRangeResult<ValueTypeOf<R> >>::_
> {};

}

using Range::AsRange;
using Range::AsConstRange;

//! Оператор == для сравнения с null для диапазонов эквивалентен вызову Empty()
template<typename R> forceinline Meta::EnableIf<
	Range::IsInputRange<R>::_,
bool> operator==(const R& range, null_t) {return range.Empty();}

INTRA_WARNING_POP

}
