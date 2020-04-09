#pragma once

#include "Intra/Concepts.h"
#include "Intra/Assert.h"
#include "Intra/TypeSafe.h"

/**
  This header file defines concepts for different range categories and method availability checkers.
  It makes range-based for available for all range types defined in namespace Intra.
  To make range-based for available for ranges defined outside Intra use:
  using Intra::RangeFor::begin;
  using Intra::RangeFor::end;
*/

INTRA_BEGIN

INTRA_DEFINE_SAFE_DECLTYPE(TReturnValueTypeOf, Val<T>().First());
template<typename R> using TValueTypeOf = TRemoveConstRef<TReturnValueTypeOf<R>>;

template<class R> concept CHasFirst = !CSame<TReturnValueTypeOf<R>, void>;
INTRA_DEFINE_CONCEPT_REQUIRES(CHasPopFirst, Val<T>().PopFirst());
INTRA_DEFINE_CONCEPT_REQUIRES(CHasEmpty, Val<bool&>() = Val<T>().Empty());
INTRA_DEFINE_CONCEPT_REQUIRES(CHasLast, Val<TReturnValueTypeOf<T>&>() = Val<T>().Last()); //implies that CHasFirst is true
INTRA_DEFINE_CONCEPT_REQUIRES(CHasPopLast, Val<T>().PopLast());
INTRA_DEFINE_CONCEPT_REQUIRES(CHasIndex, Val<T>()[size_t()]);

INTRA_DEFINE_CONCEPT_REQUIRES(CHasPopFirstCount, Val<index_t&>() = Val<T>().PopFirstCount(ClampedSize()));
INTRA_DEFINE_CONCEPT_REQUIRES(CHasPopLastCount, Val<index_t&>() = Val<T>().PopLastCount(ClampedSize()));
INTRA_DEFINE_CONCEPT_REQUIRES(CHasNext, Val<TReturnValueTypeOf<T>&>() = Val<T>().Next());
INTRA_DEFINE_CONCEPT_REQUIRES2(CHasReadWriteMethod, Val<T1>().ReadWrite(Val<T2>()),,);
INTRA_DEFINE_CONCEPT_REQUIRES2(CHasPutAllAdvanceMethod, Val<T1>().PutAllAdvance(Val<T2>()),,);


INTRA_DEFINE_CONCEPT_REQUIRES(CHasFull, Val<bool&>() = Val<T>().Full());
INTRA_DEFINE_CONCEPT_REQUIRES2(CHasPut, Val<T1>().Put(Val<T2>()),,);
INTRA_DEFINE_CONCEPT_REQUIRES(CHasReset, Val<T>().Reset());
INTRA_DEFINE_SAFE_DECLTYPE(TWrittenRangeType, Val<T>().WrittenRange());

INTRA_DEFINE_SAFE_DECLTYPE(TRawUnicodeUnit, Val<T>().RawUnicodeUnits().First());

template<typename R> concept CInputRange =
	CHasFirst<R> &&
	CHasPopFirst<TRemoveConst<R>> &&
	CHasEmpty<R>;

template<typename R, typename T> concept COutputRangeOf = CHasPut<R, T>;
template<class R> concept CHasWrittenRange = CInputRange<TWrittenRangeType<R>>;
template<typename R, typename T> concept COutputBufferOf =
	COutputRangeOf<R, T> &&
	CHasReset<R> &&
	CHasWrittenRange<R>;

template<typename R> concept CForwardRange =
	CInputRange<R> &&
	CCopyConstructible<TRemoveReference<R>>;

template<typename R> concept CBidirectionalRange =
	CForwardRange<R> &&
	CHasLast<R> &&
	CHasPopLast<TRemoveConst<R>>;

template<typename R> concept CRandomAccessRange =
	CForwardRange<R> &&
	CHasIndex<R>;

template<typename R> concept CArrayRange =
	CRandomAccessRange<R> &&
	CHasData<R> &&
	CHasLength<R>;

template<typename T1, typename T2> concept CArrayRangeOfExactly =
	CArrayRange<T1> &&
	CSameUnqual<TValueTypeOf<T1>, T2>;

template<typename T> concept CCharRange =
	CInputRange<T> &&
	CChar<TValueTypeOf<T>>;

template<typename T> concept CForwardCharRange =
	CForwardRange<T> &&
	CChar<TValueTypeOf<T>>;

template<typename T> concept COutputCharRange = CHasPut<T, char>;

template<typename R> concept COutputRange =
	CHasPut<R, TValueTypeOf<R>> ||
	COutputCharRange<R>;

template<typename T> concept CAssignableRange =
	CInputRange<T> &&
	CNCLValueReference<TReturnValueTypeOf<T>>;


namespace z_D {
/** A range type must define the following
		static constexpr bool IsAnyInstanceFinite = true;
	only if it is statically known that all its instances are finite.
	This concept returns the value of this field or false if it is not defined.
*/
INTRA_DEFINE_CONCEPT_EXPR(CAnyInstanceFinite, TRemoveReference<T>::IsAnyInstanceFinite, false);

/** A range type must define the following
		static constexpr bool IsAnyInstanceInfinite = true;
	only if it is statically known that all its instances are infinite.
	This concept returns the value of this field or false if it is not defined.
*/
INTRA_DEFINE_CONCEPT_EXPR(CAnyInstanceInfinite, TRemoveReference<T>::IsAnyInstanceInfinite, false);
}

template<typename R> concept CFiniteInputRange =
	CInputRange<R> &&
	(z_D::CAnyInstanceFinite<R> || CHasLength<R> || CBidirectionalRange<R>);

template<typename R> concept CInfiniteInputRange =
	CInputRange<R> &&
	z_D::CAnyInstanceInfinite<R>;

//! CInputRange which is not statically known to be infinite
template<typename R> concept CNonInfiniteInputRange =
	!CInfiniteInputRange<R> &&
	CInputRange<R>;

template<typename R> concept CFiniteForwardRange =
	CFiniteInputRange<R> &&
	CForwardRange<R>;

template<typename R> concept CNonInfiniteForwardRange =
	CNonInfiniteInputRange<R> &&
	CForwardRange<R>;

template<typename R> concept CFiniteRandomAccessRange =
	CRandomAccessRange<R> &&
	CHasLength<R>;

template<typename R> concept CInfiniteRandomAccessRange =
	CRandomAccessRange<R> &&
	!CHasLength<R>;

template<typename R1, typename R2> concept CSameValueType =
	CSame<TValueTypeOf<R1>, TValueTypeOf<R2>>;

template<typename R, typename T> concept CFiniteInputRangeOfExactly =
	CFiniteInputRange<R> &&
	CSame<TValueTypeOf<R>, T>;

template<typename R, typename T> concept CFiniteForwardRangeOfExactly =
	CFiniteForwardRange<R> &&
	CSame<TValueTypeOf<R>, T>;

template<typename R> concept CAccessibleRange =
	CInputRange<R> &&
	(CNCRValueReference<R> ||
		CCopyConstructible<TRemoveReference<R>>);

template<typename R> concept CConsumableRange =
	CAccessibleRange<R> &&
	!CInfiniteInputRange<R>;

template<typename R, typename T> concept CConsumableRangeOf =
	CConsumableRange<R> &&
	CConvertibleTo<TValueTypeOf<R>, T>;

template<typename P, typename... R> concept CElementPredicate =
	CSame<bool, TResultOfOrVoid<P, TValueTypeOf<R>...>>;

template<typename P, typename... R> concept CElementAsPredicate =
	CSame<bool, TResultOfOrVoid<TFunctorOf<P>, TValueTypeOf<R>...>>;


INTRA_DEFINE_CONCEPT_REQUIRES2(CHasIntegralDifference, Val<index_t&>() = Val<T1>() - Val<T2>(), , = U1);
namespace z_D {
template<typename R> struct RangeForIterLike
{
	constexpr RangeForIterLike& operator++() {Range.PopFirst(); return *this;}
	constexpr TReturnValueTypeOf<R> operator*() const {return Range.First();}
	constexpr bool operator!=(decltype(null)) const {return !Range.Empty();}
	R Range;
};

INTRA_DEFINE_CONCEPT_REQUIRES(CRangeForIterableClass, (
    Val<bool&>() = Val<T>().begin() != Val<T>().end(),
    *++Val<decltype(Val<T>().begin())&>()
));
INTRA_DEFINE_CONCEPT_REQUIRES(CRangeForIterableEx, (
    Val<bool&>() = begin(Val<T>()) != end(Val<T>()),
    *++Val<decltype(begin(Val<T>()))&>()
));
}
template<typename T> concept CRangeForIterable = CArrayType<TRemoveReference<T>> ||
	z_D::CRangeForIterableClass<T> || z_D::CRangeForIterableEx<T>;

#if INTRA_CONSTEXPR_TEST
static_assert(CRangeForIterable<int(&)[5]>);
#endif

INTRA_DEFINE_SAFE_DECLTYPE(TIteratorReturnValueTypeOf, *Val<TRemoveConstRef<T>>());
template<typename R> using TIteratorValueTypeOf = TRemoveConstRef<TIteratorReturnValueTypeOf<R>>;

template<typename T> concept CMinimalInputIterator =
	CHasPreIncrement<T> &&
	CHasDereference<T> &&
	CNonEqualityComparable<T, T> &&
	CMoveConstructible<T> &&
	CMoveAssignable<T> &&
	CDestructible<T>;

template<typename T> concept CInputIterator =
	CMinimalInputIterator<T> &&
	CHasPostIncrement<T> &&
	CEqualityComparable<T, T> &&
	CCopyConstructible<T> &&
	CCopyAssignable<T>;

template<typename T> concept CMinimalBidirectionalIterator =
	CMinimalInputIterator<T> &&
	CHasPreDecrement<T> &&
	CCopyConstructible<T> &&
	CCopyAssignable<T>;

template<typename I1, typename I2> struct IteratorRange
{
	static_assert(CMinimalInputIterator<I1>);

	I1 Begin;
	I2 End;

	[[nodiscard]] constexpr bool Empty() const {return Begin == End;}

	constexpr void PopFirst()
	{
	    INTRA_PRECONDITION(!Empty());
	    ++Begin;
	}

	[[nodiscard]] constexpr decltype(auto) First() const
	{
	    INTRA_PRECONDITION(!Empty());
	    return *Begin;
	}

	template<typename U = I2, typename = Requires<CMinimalBidirectionalIterator<U>>>
	constexpr void PopLast()
	{
	    INTRA_PRECONDITION(!Empty());
	    --End;
	}

	template<typename U = I2, typename = Requires<CMinimalBidirectionalIterator<U>>>
	constexpr decltype(auto) Last() const
	{
	    INTRA_PRECONDITION(!Empty());
	    return *--I2(End);
	}

	template<typename U = I2, typename = Requires<CHasIntegralDifference<U, I1>>>
	[[nodiscard]] constexpr index_t Length() const
	{
	    INTRA_PRECONDITION(!Empty());
	    return index_t(End - Begin);
	}
};

template<typename T> struct FListNode;
template<typename T> struct BListNode;
template<typename T, typename Node = FListNode<T>> struct FListRange;
template<typename T, typename Node = BListNode<T>> struct BListRange;

INTRA_DEFINE_CONCEPT_REQUIRES(CHasNextListNodeMethod, Val<T>().NextListNode());
INTRA_DEFINE_CONCEPT_REQUIRES(CHasPrevListNodeMethod, Val<T>().PrevListNode());

template<typename T> struct Span;
template<typename R, template<typename> class ArrayClass = Span, template<typename, typename> class ListRange = FListRange>
[[nodiscard]] constexpr decltype(auto) RangeOf(R&& r)
{
	if constexpr(CInputRange<R> || COutputRange<R>) return Forward<R>(r);
	else if constexpr(COutputRange<TRemoveConstRef<R>> && CCopyConstructible<R>) return r;
	else if constexpr(CArrayClass<R>) return ArrayClass(r);
	else if constexpr(CHasNextListNodeMethod<R>) return ListRange(r);
	else if constexpr(z_D::CRangeForIterableEx<R>) return IteratorRange{begin(r), end(r)};
	else if constexpr(z_D::CRangeForIterableClass<R>) return IteratorRange{r.begin(), r.end()};
}

template<typename T> using TRangeOfRef = decltype(RangeOf(Val<T>()));
template<typename T> using TRangeOf = TRemoveConstRef<TRangeOfRef<T>>;
template<typename T> concept CHasRangeOf = !CVoid<TRangeOfRef<T>>;


template<typename T> concept CAsInputRange = CInputRange<TRangeOfRef<T>>;
template<typename T> concept CAsFiniteInputRange = CFiniteInputRange<TRangeOfRef<T>>;
template<typename T> concept CAsNonInfiniteInputRange = CNonInfiniteInputRange<TRangeOfRef<T>>;
template<typename T> concept CAsAssignableRange = CAssignableRange<TRangeOfRef<T>>;
template<typename T> concept CAsOutputRange = COutputRange<TRangeOfRef<T>>;
template<typename T, typename U> concept CAsOutputRangeOf = COutputRangeOf<TRangeOfRef<T>, U>;

template<typename T> concept CAsForwardRange = CForwardRange<TRangeOfRef<T>>;
template<typename T> concept CAsFiniteForwardRange = CFiniteForwardRange<TRangeOfRef<T>>;
template<typename T> concept CAsNonInfiniteForwardRange = CNonInfiniteForwardRange<TRangeOfRef<T>>;

template<typename T> concept CAsBidirectionalRange = CBidirectionalRange<TRangeOfRef<T>>;

template<typename T> concept CAsRandomAccessRange = CRandomAccessRange<TRangeOfRef<T>>;
template<typename T> concept CAsFiniteRandomAccessRange = CFiniteRandomAccessRange<TRangeOfRef<T>>;

template<typename T> concept CAsArrayRange = CArrayRange<TRangeOfRef<T>>;
template<typename T1, typename T2> concept CAsArrayRangeOfExactly = CArrayRangeOfExactly<TRangeOfRef<T1>, T2>;
template<typename T> concept CAsInfiniteInputRange = CInfiniteInputRange<TRangeOfRef<T>>;
template<typename T> concept CAsCharRange = CCharRange<TRangeOfRef<T>>;
template<typename T> concept CAsForwardCharRange = CForwardCharRange<TRangeOfRef<T>>;
template<typename T> concept CAsOutputCharRange = COutputCharRange<TRangeOfRef<T>>;

template<typename T> using TReturnValueTypeOfAs = TReturnValueTypeOf<TRangeOfRef<T>>;
template<typename T> using TValueTypeOfAs = TValueTypeOf<TRangeOfRef<T>>;

template<typename R> concept CAsAccessibleRange = CAccessibleRange<TRangeOfRef<R>>;
template<typename R> concept CAsConsumableRange = CConsumableRange<TRangeOfRef<R>>;
template<typename R, typename T> concept CAsConsumableRangeOf = CConsumableRangeOf<TRangeOfRef<R>, T>;

template<typename R> using CAsAccessibleRangeT = TBool<CAsAccessibleRange<R>>;

template<typename P, typename... Rs> concept CAsElementPredicate = CElementPredicate<P, TRangeOfRef<Rs>...>;
template<typename P, typename... Rs> concept CAsElementAsPredicate = CElementAsPredicate<P, TRangeOfRef<Rs>...>;


template<typename R> constexpr Requires<
	CInputRange<TRangeOf<R&&>> ||
	COutputRange<TRemoveConst<TRangeOfRef<R&&>>> ||
	COutputCharRange<TRemoveConst<TRangeOfRef<R&&>>>,
TRangeOfRef<R&&>> ForwardAsRange(TRemoveReference<R>& t)
{return RangeOf(static_cast<R&&>(t));}

template<typename R> constexpr Requires<
	CInputRange<TRangeOf<R&&>> ||
	COutputRange<TRemoveConst<TRangeOfRef<R&&>>> ||
	COutputCharRange<TRemoveConst<TRangeOfRef<R&&>>>,
TRangeOfRef<R&&>> ForwardAsRange(TRemoveReference<R>&& t)
{
	static_assert(!CLValueReference<R>, "Bad ForwardAsRange call!");
	return RangeOf(static_cast<R&&>(t));
}


template<typename R, typename T, typename = Requires<CAsOutputRangeOf<TRemoveConst<R>, T>>>
[[nodiscard]] constexpr decltype(auto) ForwardAsOutputRangeOf(TRemoveReference<R>& t)
{return RangeOf(static_cast<R&&>(t));}

template<typename R, typename T, typename = Requires<CAsOutputRangeOf<TRemoveConst<R>, T>>>
[[nodiscard]] constexpr decltype(auto) ForwardAsOutputRangeOf(TRemoveReference<R>&& t)
{
	static_assert(!CLValueReference<R>, "Bad ForwardAsOutputRangeOf call!");
	return RangeOf(static_cast<R&&>(t));
}

template<typename R, typename F, typename = Requires<CCallable<F, R> && (CInputRange<R> || COutputRange<R>)>>
constexpr INTRA_FORCEINLINE decltype(auto) operator|(R&& range, F&& func) {return Forward<F>(func)(Forward<R>(range));}

namespace Tags {
enum TKeepTerminator: bool {KeepTerminator = true};
}

inline namespace RangeFor {
template<typename R, typename = Requires<CAsConsumableRange<R>>>
[[nodiscard]] constexpr auto begin(R&& range) {return z_D::RangeForIterLike{ForwardAsRange<R>(range)};}

template<typename R, typename = Requires<CAsConsumableRange<R>>>
[[nodiscard]] constexpr auto end(R&&) {return null;}
}

INTRA_END
