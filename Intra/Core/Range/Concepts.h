#pragma once

#include "Core/Type.h"
#include "Core/Functional.h"
#include "Core/CArray.h"
#include "Core/CIterator.h"
#include "Core/Assert.h"

/**
  This header file defines concepts for different range categories and method availability checkers.
  It makes range-based for available for all range types defined in namespace Range.
  To make range-based for available for ranges defined in different namespace use:
  using begin;
  using end;
  in their namespace or in the global namespace.
*/

INTRA_BEGIN

INTRA_DEFINE_CONCEPT_REQUIRES(CHasPopFirst, Val<T>().PopFirst());
INTRA_DEFINE_CONCEPT_REQUIRES(CHasFirst, Val<T>().First());
INTRA_DEFINE_CONCEPT_REQUIRES(CHasEmpty, static_cast<bool>(Val<T>().Empty()));
INTRA_DEFINE_CONCEPT_REQUIRES(CHasFull, static_cast<bool>(Val<T>().Full()));
INTRA_DEFINE_CONCEPT_REQUIRES(CHasPopFirstN, static_cast<size_t>(Val<T>().PopFirstN(size_t())));
INTRA_DEFINE_CONCEPT_REQUIRES(CHasPopLastN, static_cast<size_t>(Val<T>().PopLastN(size_t())));
INTRA_DEFINE_CONCEPT_REQUIRES(CHas_value_type, Val<typename TRemoveReference<T>::value_type>());

INTRA_DEFINE_CONCEPT_REQUIRES2(CHasReadWriteMethod, Val<T1>().ReadWrite(Val<T2&>()),,);
INTRA_DEFINE_CONCEPT_REQUIRES2(CHasPutAllAdvanceMethod, Val<T1>().PutAllAdvance(Val<T2&>()),,);

// TODO: check that return type is a range
template<typename R> concept CSliceable = CCallable<R, size_t, size_t>;

namespace z__R {

template<typename R, bool = CHasFirst<R>> struct TReturnValueTypeOf2
{typedef void _;};

template<typename R> struct TReturnValueTypeOf2<R, true>
{
	typedef TRemoveReference<R> RMut;
	typedef decltype(Val<RMut>().First()) _;
};

template<typename R> struct TReturnValueTypeOf_
{typedef typename TReturnValueTypeOf2<R>::_ _;};

template<typename R,
	bool = CHasFirst<R>,
	bool = CHas_value_type<R>
> struct TValueTypeOf_
{typedef TRemoveConstRef<decltype(Val<R>().First())> _;};

template<typename R> struct TValueTypeOf_<R, false, false>
{typedef void _;};

template<typename R> struct TValueTypeOf_<R, false, true>
{
	typedef TRemoveReference<R> R2;
	typedef typename R2::value_type _;
};

}

template<typename R> using TReturnValueTypeOf = typename z__R::TReturnValueTypeOf_<R>::_;

template<typename R> using TValueTypeOf = typename z__R::TValueTypeOf_<R>::_;
template<typename R> using TSliceTypeOf = TResultOf<R, size_t, size_t>;
template<typename R> using TSliceTypeOfOrVoid = TResultOfOrVoid<R, size_t, size_t>;

INTRA_DEFINE_CONCEPT_REQUIRES(CHasLast, Val<T>().Last());
INTRA_DEFINE_CONCEPT_REQUIRES(CHasPopLast, Val<T>().PopLast());
INTRA_DEFINE_CONCEPT_REQUIRES(CHasIndex, Val<T>()[size_t()]);

INTRA_DEFINE_CONCEPT_REQUIRES2(CHasPut, Val<T1>().Put(Val<T2>()),,);

template<typename R, typename T> concept COutputRangeOf = CHasPut<R, T>;

INTRA_DEFINE_CONCEPT_REQUIRES(CHasReset, Val<T>().Reset());
INTRA_DEFINE_CONCEPT_REQUIRES(CHasWrittenRange, Val<T>().WrittenRange());

template<typename R, typename T> concept COutputBufferOf =
	COutputRangeOf<R, T> &&
	CHasReset<R> &&
	CHasWrittenRange<R>; //TODO: check that return value of WrittenRange is CInputRange

template<typename R> concept CInputRange =
	CHasFirst<R> &&
	CHasPopFirst<TRemoveConst<R>> &&
	CHasEmpty<R>;

template<typename R> concept CInputRangeWithLength =
	CInputRange<R> &&
	CHasLength<R>;

template<typename R> concept CForwardRange =
	CInputRange<R> &&
	CCopyConstructible<TRemoveReference<R>>;

template<typename R> concept CForwardRangeWithLength =
	CForwardRange<R> &&
	CHasLength<R>;

template<typename R> concept CBidirectionalRange =
	CForwardRange<R> &&
	CHasLast<R> &&
	CHasPopLast<TRemoveConst<R>>; // TODO: && CSame<decltype(.First(), .Last())>

template<typename R> concept CBidirectionalRangeWithLength =
	CBidirectionalRange<R> &&
	CHasLength<R>;

template<typename R> concept CRandomAccessRange =
	CForwardRange<R> &&
	CHasIndex<R>;

template<typename R> concept CRandomAccessRangeWithLength =
	CRandomAccessRange<R> &&
	CHasLength<R>;

template<typename R> concept CArrayRange =
	CRandomAccessRange<R> &&
	CHasData<R> &&
	CHasLength<R>;

template<typename T1, typename T2> concept CArrayRangeOfExactly =
	CArrayRange<T1> &&
	CSameUnqual<TValueTypeOf<T1>, T2>;


namespace z__R {
template<class T> struct HasRangeIsFinite
{
	template<bool> struct dummy {};
	template<typename U> static char test(...);
	template<typename U> static short test(dummy<U::RangeIsFinite>*);
	enum: bool { _ = (sizeof(test<TRemoveConstRef<T>>(0)) == sizeof(short)) };
};
template<typename R, bool = HasRangeIsFinite<R>::_, typename Range = TRemoveConstRef<R>>
constexpr bool CFiniteRange_ = Range::RangeIsFinite;
template<typename R> constexpr bool CFiniteRange_<R, false> =
	CHasLength<R> ||
	CHasLast<R> ||
	CHasPopLast<R>;
}
template<typename R> concept CFiniteRange = z__R::CFiniteRange_<R>;
template<typename R> using CFiniteRangeT = TBool<z__R::CFiniteRange_<R>>;

//TODO: replace with INTRA_DEFINE_CONCEPT_REQUIRES
namespace z__R {
template<class T> struct HasRangeIsInfinite
{
	template<bool> struct dummy {};
	template<typename U> static char test(...);
	template<typename U> static short test(dummy<U::RangeIsInfinite>*);
	enum: bool { _ = (sizeof(test<TRemoveConstRef<T>>(0))==sizeof(short)) };
};
template<typename R, bool = HasRangeIsInfinite<R>::_, typename Range = TRemoveConstRef<R>>
constexpr bool CInfiniteRange_ = Range::RangeIsInfinite;
template<typename R> constexpr bool CInfiniteRange_<R, false> = false;
}
template<typename R> concept CInfiniteRange = z__R::CInfiniteRange_<R>;
template<typename R> using CInfiniteRangeT = TBool<z__R::CInfiniteRange_<R>>;

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

template<typename R> concept CFiniteInputRange =
	CInputRange<R> &&
	CFiniteRange<R>;

template<typename R> concept CNonInfiniteInputRange =
	CInputRange<R> &&
	!CInfiniteRange<R>;

template<typename R> concept CFiniteForwardRange =
	CForwardRange<R> &&
	CFiniteRange<R>;

template<typename R> concept CNonInfiniteForwardRange =
	CForwardRange<R> &&
	!CInfiniteRange<R>;

template<typename R> concept CFiniteRandomAccessRange =
	CRandomAccessRange<R> &&
	CFiniteRange<R>;
template<typename R> concept CNonInfiniteRandomAccessRange =
	CRandomAccessRange<R> &&
	!CInfiniteRange<R>;

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
	!CInfiniteRange<R>;

template<typename R> concept CAccessibleRangeWithLength =
	CAccessibleRange<R> &&
	CHasLength<R>;

template<typename R, typename T> concept CConsumableRangeOf =
	CConsumableRange<R> &&
	CConvertible<TValueTypeOf<R>, T>;

template<typename P, typename... R> concept CElementPredicate =
	CSame<bool, TResultOfOrVoid<P, TValueTypeOf<R>...>>;

template<typename P, typename... R> concept CElementAsPredicate =
	CSame<bool, TResultOfOrVoid<TFunctorOf<P>, TValueTypeOf<R>...>>;


//! Implements minimal iterator subset compatible with range-based for.
//! It must not be used anywhere except range-based for.
template<typename R> struct RangeForIterLike
{
	constexpr forceinline RangeForIterLike(null_t=null): mRange() {}
	constexpr forceinline RangeForIterLike(R&& range): mRange(Move(range)) {}

	constexpr forceinline RangeForIterLike& operator++() {mRange.PopFirst(); return *this;}
	constexpr forceinline TReturnValueTypeOf<R> operator*() const {return mRange.First();}

#if !defined(__cpp_range_based_for) || __cpp_range_based_for < 201603
	constexpr forceinline bool operator!=(const RangeForIterLike& rhs) const
	{
		INTRA_DEBUG_ASSERT(rhs.mRange.Empty());
		(void)rhs;
		return !mRange.Empty();
	}
#endif

	constexpr forceinline bool operator!=(null_t) const {return !mRange.Empty();}

	RangeForIterLike(const RangeForIterLike&) = delete;
	RangeForIterLike& operator=(const RangeForIterLike&) = delete;
	RangeForIterLike& operator=(RangeForIterLike&&) = delete;
	constexpr forceinline RangeForIterLike(RangeForIterLike&& rhs) = default;

private:
	R mRange;
};

template<typename R> constexpr forceinline Requires<
	CInputRange<R> &&
	(!CCopyConstructible<TRemoveConstRef<R>> ||
		!CCopyAssignable<TRemoveConstRef<R>>) &&
	!CAbstractClass<R> &&
	!CConst<R>,
RangeForIterLike<TRemoveConstRef<R>>> begin(R&& range)
{return Move(range);}

template<typename R> constexpr forceinline Requires<
	CInputRange<R> &&
	CCopyConstructible<TRemoveConstRef<R>> &&
	CCopyAssignable<TRemoveConstRef<R>> &&
	!CAbstractClass<R>,
RangeForIterLike<TRemoveConstRef<R>>> begin(R&& range)
{return {TRemoveConstRef<R>(Forward<R>(range))};}

#if !defined(__cpp_range_based_for) || __cpp_range_based_for < 201603

template<typename R> constexpr forceinline Requires<
	CInputRange<R> &&
	!CAbstractClass<R> &&
	!CConst<R>,
RangeForIterLike<TRemoveReference<R>>> end(R&&)
{return null;}

#else

template<typename R> constexpr forceinline Requires<
	CInputRange<R> &&
	!CConst<R>,
null_t> end(R&&)
{return null;}

#endif


template<typename T> struct Span;

template<typename I1, typename I2> struct IteratorRange
{
	static_assert(CMinimalInputIterator<I1>, "I1 must implement at least minimal input iterator concept!");

	I1 Begin;
	I2 End;

	INTRA_NODISCARD constexpr forceinline bool Empty() const {return Begin == End;}
	constexpr forceinline void PopFirst() {INTRA_DEBUG_ASSERT(!Empty()); ++Begin;}
	INTRA_NODISCARD constexpr forceinline decltype(auto) First() const {return INTRA_DEBUG_ASSERT(!Empty()), *Begin;}

	template<typename U=I2> forceinline Requires<
		CMinimalBidirectionalIterator<U>
	> PopLast() {INTRA_DEBUG_ASSERT(!Empty()); --End;}

	template<typename U=I2> forceinline Requires<
		CMinimalBidirectionalIterator<U>,
	decltype(*Val<I2>())> Last() const {INTRA_DEBUG_ASSERT(!Empty()); return *--I2(End);}

	template<typename U=I2> INTRA_NODISCARD constexpr forceinline Requires<
		CHasDifference<U, I1>,
	index_t> Length() const {return INTRA_DEBUG_ASSERT(!Empty()), index_t(End - Begin);}

	INTRA_NODISCARD constexpr forceinline I1 begin() const {return Begin;}
	INTRA_NODISCARD constexpr forceinline I2 end() const {return End;}
};

template<typename T> struct FListNode;
template<typename T> struct BListNode;
template<typename T, typename Node = FListNode<T>> struct FListRange;
template<typename T, typename Node = BListNode<T>> struct BListRange;

INTRA_DEFINE_CONCEPT_REQUIRES(CHasNextListNodeMethod, Val<T>().NextListNode());
INTRA_DEFINE_CONCEPT_REQUIRES(CHasPrevListNodeMethod, Val<T>().PrevListNode());

template<typename T> Requires<
	!CInputRange<T> &&
	CHasNextListNodeMethod<T>/* &&
	  !CHasPrevListNodeMethod<T>*/,
FListRange<T, T>> RangeOf(T& objectWithIntrusiveList);

template<typename R> forceinline Requires<
	CInputRange<R> ||
	COutputRange<R>,
R&&> RangeOf(R&& r) {return Forward<R>(r);}

template<typename R, typename NCRR = TRemoveConstRef<R>> forceinline Requires<
	!(CInputRange<R> || COutputRange<R>) &&
	(CInputRange<NCRR> || COutputRange<NCRR>) &&
	CCopyConstructible<NCRR>,
NCRR> RangeOf(R&& r) {return r;}

INTRA_DEFINE_CONCEPT_REQUIRES(CHasAsRangeMethod, Val<T>().AsRange());

template<typename T, typename = Requires<
	!CInputRange<T> &&
	CHasAsRangeMethod<T>
>> forceinline decltype(Val<T>().AsRange()) RangeOf(T&& v) {return v.AsRange();}

template<typename C, typename D = C, typename = Requires<
	!CInputRange<C> &&
	!CArrayClass<C> &&
	!CHasAsRangeMethod<C> &&
	CHas_begin_end<C>
>> forceinline IteratorRange<decltype(begin(Val<D>())), decltype(end(Val<D>()))> RangeOf(C&& v)
{return {begin(v), end(v)};}

template<typename R> constexpr forceinline Requires<
	!CInputRange<TRemoveConstRef<R>> &&
	!CHasAsRangeMethod<TRemoveConstRef<R>>,
Span<TRemovePointer<TArrayElementPtrRequired<R>>>> RangeOf(R&& r) noexcept; //defined in Span.h

template<typename T, size_t N> INTRA_NODISCARD constexpr forceinline Span<T> RangeOf(T(&arr)[N]) noexcept;


INTRA_DEFINE_CONCEPT_REQUIRES(CHasRangeOf, RangeOf(Val<T>()));

namespace z__R {

template<typename T,
	bool = CHasRangeOf<T>
> struct TRangeOfType
{typedef T _;};

template<typename T, size_t N> struct TRangeOfType<T[N], false>
{typedef Span<T> _;};

template<typename T, size_t N> struct TRangeOfType<T(&)[N], false>
{typedef Span<T> _;};

template<typename T> struct TRangeOfType<T, true>
{typedef decltype(RangeOf(Val<T>())) _;};

template<typename T> struct TRangeOfTypeNoCRef
{typedef TRemoveConstRef<typename TRangeOfType<T>::_> _;};

}

template<typename T> using TRangeOfType = typename z__R::TRangeOfType<T>::_;
template<typename T> using TRangeOfTypeNoRef = TRemoveReference<typename z__R::TRangeOfType<T>::_>;
template<typename T> using TRangeOfTypeNoCRef = typename z__R::TRangeOfTypeNoCRef<T>::_;


template<typename T> concept CAsFiniteRange = CFiniteRange<TRangeOfType<T>>;
template<typename T> concept CAsInputRange = CInputRange<TRangeOfType<T>>;
template<typename T> concept CAsFiniteInputRange = CFiniteInputRange<TRangeOfType<T>>;
template<typename T> concept CAsNonInfiniteInputRange = CNonInfiniteInputRange<TRangeOfType<T>>;
template<typename T> concept CAsAssignableRange = CAssignableRange<TRangeOfType<T>>;
template<typename T> concept CAsOutputRange = COutputRange<TRangeOfType<T>>;
template<typename T, typename U> concept CAsOutputRangeOf = COutputRangeOf<TRangeOfType<T>, U>;

template<typename T> concept CAsForwardRange = CForwardRange<TRangeOfType<T>>;
template<typename T> concept CAsForwardRangeWithLength = CForwardRangeWithLength<TRangeOfType<T>>;
template<typename T> concept CAsFiniteForwardRange = CFiniteForwardRange<TRangeOfType<T>>;
template<typename T> concept CAsNonInfiniteForwardRange = CNonInfiniteForwardRange<TRangeOfType<T>>;

template<typename T> concept CAsBidirectionalRange = CBidirectionalRange<TRangeOfType<T>>;

template<typename T> concept CAsRandomAccessRange = CRandomAccessRange<TRangeOfType<T>>;
template<typename T> concept CAsFiniteRandomAccessRange = CFiniteRandomAccessRange<TRangeOfType<T>>;
template<typename T> concept CAsNonInfiniteRandomAccessRange = CNonInfiniteRandomAccessRange<TRangeOfType<T>>;

template<typename T> concept CAsArrayRange = CArrayRange<TRangeOfType<T>>;
template<typename T1, typename T2> concept CAsArrayRangeOfExactly = CArrayRangeOfExactly<TRangeOfType<T1>, T2>;
template<typename T> concept CAsInfiniteRange = CInfiniteRange<TRangeOfType<T>>;
template<typename T> concept CAsCharRange = CCharRange<TRangeOfType<T>>;
template<typename T> concept CAsForwardCharRange = CForwardCharRange<TRangeOfType<T>>;
template<typename T> concept CAsOutputCharRange = COutputCharRange<TRangeOfType<T>>;

template<typename T> using TReturnValueTypeOfAs = TReturnValueTypeOf<TRangeOfType<T>>;
template<typename T> using TValueTypeOfAs = TValueTypeOf<TRangeOfType<T>>;

template<typename R> concept CAsAccessibleRange = CAccessibleRange<TRangeOfType<R>>;
template<typename R> concept CAsConsumableRange = CConsumableRange<TRangeOfType<R>>;
template<typename R, typename T> concept CAsConsumableRangeOf = CConsumableRangeOf<TRangeOfType<R>, T>;

template<typename R> using CAsAccessibleRangeT = TBool<CAsAccessibleRange<R>>;

template<typename P, typename... Rs> concept CAsElementPredicate = CElementPredicate<P, TRangeOfType<Rs>...>;
template<typename P, typename... Rs> concept CAsElementAsPredicate = CElementAsPredicate<P, TRangeOfType<Rs>...>;


template<typename R,
	typename AsR = TRangeOfType<R&&>
> forceinline Requires<
	CInputRange<TRemoveConstRef<AsR>> ||
	COutputRange<TRemoveConst<AsR>> ||
	COutputCharRange<TRemoveConst<AsR>>,
AsR> ForwardAsRange(TRemoveReference<R>& t)
{return RangeOf(static_cast<R&&>(t));}

template<typename R,
	typename AsR = TRangeOfType<R&&>
> forceinline Requires<
	CInputRange<TRemoveConstRef<AsR>> ||
	COutputRange<TRemoveConst<AsR>> ||
	COutputCharRange<TRemoveConst<AsR>>,
AsR> ForwardAsRange(TRemoveReference<R>&& t)
{
	static_assert(!CLValueReference<R>, "Bad ForwardAsRange call!");
	return RangeOf(static_cast<R&&>(t));
}


template<typename R, typename T> forceinline Requires<
	CAsOutputRangeOf<TRemoveConst<R>, T>,
TRangeOfType<R&&>> ForwardAsOutputRangeOf(TRemoveReference<R>& t)
{return RangeOf(static_cast<R&&>(t));}

template<typename R, typename T> forceinline Requires<
	CAsOutputRangeOf<TRemoveConst<R>, T>,
TRangeOfType<R&&>> ForwardAsOutputRangeOf(TRemoveReference<R>&& t)
{
	static_assert(!CLValueReference<R>, "Bad ForwardAsOutputRangeOf call!");
	return RangeOf(static_cast<R&&>(t));
}

namespace Tags {
enum TKeepTerminator: bool {KeepTerminator = true};
}

INTRA_END
