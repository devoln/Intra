#pragma once

#include "Cpp/Features.h"
#include "Cpp/Warnings.h"
#include "Cpp/Fundamental.h"
#include "Meta/Type.h"
#include "Array.h"

//! Данный заголовочный файл содержит type traits для проверки наличия методов у типа, относящихся к понятию диапазона,
//! а также проверки соответствия типа определённой категории диапазонов.
//! Делает range-based for доступным для всех диапазонов в модуле Range.
//! Чтобы сделать range-based for доступным для диапазонов из другого пространства имён, используйте
//! using Range::begin;
//! using Range::end;
//! в их пространстве имён или в глобальном пространстве имён.

INTRA_PUSH_DISABLE_ALL_WARNINGS

namespace Intra { namespace Concepts {

INTRA_DEFINE_EXPRESSION_CHECKER(HasPopFirst, Meta::Val<T>().PopFirst());
INTRA_DEFINE_EXPRESSION_CHECKER(HasFirst, Meta::Val<T>().First());
INTRA_DEFINE_EXPRESSION_CHECKER(HasEmpty, static_cast<bool>(Meta::Val<T>().Empty()));
INTRA_DEFINE_EXPRESSION_CHECKER(HasFull, static_cast<bool>(Meta::Val<T>().Full()));
INTRA_DEFINE_EXPRESSION_CHECKER(HasPopFirstN, static_cast<size_t>(Meta::Val<T>().PopFirstN(size_t())));
INTRA_DEFINE_EXPRESSION_CHECKER(HasPopLastN, static_cast<size_t>(Meta::Val<T>().PopLastN(size_t())));
INTRA_DEFINE_EXPRESSION_CHECKER(Has_value_type, Meta::Val<typename Meta::RemoveReference<T>::value_type>());

INTRA_DEFINE_EXPRESSION_CHECKER2(HasReadToAdvanceMethod, Meta::Val<T1>().ReadToAdvance(Meta::Val<T2&>()),,);
INTRA_DEFINE_EXPRESSION_CHECKER2(HasPutAllAdvanceMethod, Meta::Val<T1>().PutAllAdvance(Meta::Val<T2&>()),,);

template<typename R> struct HasSlicing: Meta::IsCallable<R, size_t, size_t> {};

namespace RD {

template<typename R, bool = HasFirst<R>::_> struct ReturnValueTypeOf2
{typedef void _;};

template<typename R> struct ReturnValueTypeOf2<R, true>
{
	typedef Meta::RemoveReference<R> RMut;
	typedef decltype(Meta::Val<RMut>().First()) _;
};

template<typename R> struct ReturnValueTypeOf {typedef typename ReturnValueTypeOf2<R>::_ _;};

template<typename R, bool=HasFirst<R>::_, bool = Has_value_type<R>::_> struct ValueTypeOf
{typedef Meta::RemoveConstRef<decltype(Meta::Val<R>().First())> _;};

template<typename R> struct ValueTypeOf<R, false, false>
{typedef void _;};

template<typename R> struct ValueTypeOf<R, false, true>
{
	typedef Meta::RemoveReference<R> R2;
	typedef typename R2::value_type _;
};

}

template<typename R> using ReturnValueTypeOf = typename RD::ReturnValueTypeOf<R>::_;

template<typename R> using ValueTypeOf = typename RD::ValueTypeOf<R>::_;
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
	HasData<R>::_ && HasLength<R>::_
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


namespace Range {

//! Реализует минимально необходимое подмножество итератора, требуемое для совместимости с range-based for.
//! Запрещено использовать где-либо кроме range-based for.
template<typename R> struct RangeForIterLike
{
	forceinline RangeForIterLike(null_t=null): mRange() {}
	forceinline RangeForIterLike(R&& range): mRange(Cpp::Move(range)) {}

	forceinline RangeForIterLike& operator++() {mRange.PopFirst(); return *this;}
	forceinline Concepts::ReturnValueTypeOf<R> operator*() const {return mRange.First();}

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
	forceinline RangeForIterLike(RangeForIterLike&& rhs): mRange(Cpp::Move(rhs.mRange)) {}

private:
	R mRange;
};

template<typename R> forceinline Meta::EnableIf<
	Concepts::IsInputRange<R>::_ &&
	(!Meta::IsCopyConstructible<Meta::RemoveConstRef<R>>::_ ||
		!Meta::IsCopyAssignable<Meta::RemoveConstRef<R>>::_) &&
	!Meta::IsAbstractClass<R>::_ &&
	!Meta::IsConst<R>::_,
RangeForIterLike<Meta::RemoveConstRef<R>>> begin(R&& range)
{return Cpp::Move(range);}

template<typename R> forceinline Meta::EnableIf<
	Concepts::IsInputRange<R>::_ &&
	Meta::IsCopyConstructible<Meta::RemoveConstRef<R>>::_ &&
	Meta::IsCopyAssignable<Meta::RemoveConstRef<R>>::_ &&
	!Meta::IsAbstractClass<R>::_,
RangeForIterLike<Meta::RemoveConstRef<R>>> begin(R&& range)
{return {Meta::RemoveConstRef<R>(Cpp::Forward<R>(range))};}

#ifndef INTRA_RANGE_FOR_DIFFERING_TYPES_SUPPORT

template<typename R> forceinline Meta::EnableIf<
	Concepts::IsInputRange<R>::_ &&
	!Meta::IsAbstractClass<R>::_ &&
	!Meta::IsConst<R>::_,
RangeForIterLike<Meta::RemoveReference<R>>> end(R&&)
{return null;}

#else

template<typename R> forceinline Meta::EnableIf<
	Concepts::IsInputRange<R>::_ &&
	!Meta::IsConst<R>::_,
null_t> end(R&&)
{return null;}

#endif

}

namespace Concepts {
using Range::begin;
using Range::end;
}




}

INTRA_WARNING_POP
