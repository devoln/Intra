#pragma once

#include "Core/Functional.h"
#include "Core/CArray.h"
#include "Core/Range/Concepts.h"
#include "Core/Misc/RawMemory.h"
#include "Core/Range/Operations.h"

INTRA_CORE_RANGE_BEGIN
template<typename R1, typename R2> INTRA_NODISCARD constexpr forceinline Requires<
	(CFiniteRange<R1> && CInfiniteRange<R2>) ||
		(CInfiniteRange<R1> && CFiniteRange<R2>),
bool> Equals(R1&&, R2&&) {return false;}

template<typename R1, typename R2> INTRA_CONSTEXPR2 forceinline Requires<
	CTrivCopyCompatibleArrayWith<R1, R2>,
bool> Equals(const R1& r1, const R2& r2)
{
	return LengthOf(r1) == LengthOf(r2) &&
		Misc::BitsEqual(DataOf(r1), DataOf(r2), LengthOf(r1));
}

namespace z__R {

template<typename R1, typename R2, typename P> INTRA_CONSTEXPR2 bool EqualsAdvance(R1&& r1, R2&& r2, P pred)
{
	while(!r1.Empty() && !r2.Empty())
	{
		if(pred(r1.First(), r2.First()))
		{
			r1.PopFirst();
			r2.PopFirst();
			continue;
		}
		return false;
	}
	return r1.Empty() && r2.Empty();
}

}

template<typename R1, typename R2, typename P> INTRA_NODISCARD INTRA_CONSTEXPR2 Requires<
	!(CHasLength<R1> && CHasLength<R2>) &&
	CNonInfiniteForwardRange<R1> && CNonInfiniteInputRange<R2> &&
	CElementPredicate<P, R1, R2>,
bool> Equals(R1&& lhs, R2&& rhs, P pred)
{
	auto lhsCopy = Forward<R1>(lhs);
	auto rhsCopy = Forward<R2>(rhs);
	return z__R::EqualsAdvance(lhsCopy, rhsCopy, pred);
}

template<typename R1, typename R2, typename P> INTRA_NODISCARD INTRA_CONSTEXPR2 Requires<
	CAsForwardRangeWithLength<R1> &&
	CAsForwardRangeWithLength<R2> &&
	CAsElementAsPredicate<P, R1, R2>,
bool> Equals(R1&& lhs, R2&& rhs, P pred)
{
	if(lhs.Length() != rhs.Length()) return false;
	auto lhsCopy = Forward<R1>(lhs);
	auto rhsCopy = Forward<R2>(rhs);
	return z__R::EqualsAdvance(lhsCopy, rhsCopy, pred);
}

template<typename R1, typename R2> INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline Requires<
	!CTrivCopyCompatibleArrayWith<R1, R2> &&
	CForwardRange<R1> &&
	CForwardRange<R2>,
bool> Equals(R1&& lhs, R2&& rhs)
{return Equals(Forward<R1>(lhs), Forward<R2>(rhs), FEqual);}


template<typename R1, typename R2> INTRA_NODISCARD constexpr forceinline Requires<
	(!CInputRange<R1> || !CInputRange<R2>) &&
	CAsForwardRange<R1> && CAsForwardRange<R2>,
bool> Equals(R1&& lhs, R2&& rhs)
{return Equals(ForwardAsRange<R1>(lhs), ForwardAsRange<R2>(rhs));}

template<typename R1, typename R2, typename P> INTRA_NODISCARD constexpr forceinline Requires<
	(!CInputRange<R1> || !CInputRange<R2>) &&
	CAsForwardRange<R1> && CAsForwardRange<R2> &&
	CAsElementPredicate<P, R1, R2>,
bool> Equals(R1&& lhs, R2&& rhs, P pred)
{return Equals(ForwardAsRange<R1>(lhs), ForwardAsRange<R2>(rhs), pred);}


namespace z__R {

template<class R, class RW, class F> INTRA_CONSTEXPR2 bool StartsAdvanceWithAdvance(R& range, RW& what, const F& equivPred)
{
	while(!what.Empty())
	{
		if(range.Empty()) return false;
		if(!equivPred(range.First(), what.First())) return false;
		range.PopFirst();
		what.PopFirst();
	}
	return true;
}

}

template<typename R, typename RW, typename F> INTRA_CONSTEXPR2 forceinline Requires<
	CAccessibleRange<R> &&
	CConsumableRange<RW> &&
	CCallable<F, TValueTypeOf<RW>, TValueTypeOf<R>> &&
	!(CHasLength<R> && CHasLength<RW>),
bool> StartsWith(R&& range, RW&& what, F&& equivPred)
{
	auto rangeCopy = Forward<R>(range);
	auto whatCopy = Forward<RW>(what);
	return z__R::StartsAdvanceWithAdvance(rangeCopy, whatCopy, ForwardAsFunc<F>(equivPred));
}

template<typename R, typename RW> INTRA_CONSTEXPR2 forceinline Requires<
	CAccessibleRangeWithLength<R> &&
	CAccessibleRangeWithLength<RW> &&
	CConvertible<TValueTypeOf<RW>, TValueTypeOf<R>> &&
	!(CHasData<R> &&
		CHasData<RW> &&
		CTriviallyComparable<TValueTypeOf<R>>),
bool> StartsWith(R&& range, RW&& what)
{
	if(range.Length() < what.Length()) return false;
	auto rangeCopy = Forward<R>(range);
	auto whatCopy = Forward<RW>(what);
	return z__R::StartsAdvanceWithAdvance(rangeCopy, whatCopy);
}

template<typename R, typename RW> INTRA_MEM_CONSTEXPR forceinline Requires<
	CArrayClass<R> &&
	CArrayClass<RW> &&
	CSame<TArrayElement<RW>, TArrayElement<R>> &&
	CTriviallyComparable<TArrayElement<R>>,
bool> StartsWith(const R& range, const RW& what)
{
	return range.Length() >= what.Length() &&
		BitsEqual(range.Data(), what.Data(), what.Length())==0;
}

template<typename R, typename RW> constexpr forceinline Requires<
	!(CInputRange<R> && CInputRange<RW>) &&
	CAsForwardRange<R> &&
	CAsForwardRange<RW>,
bool> StartsWith(R&& range, RW&& what)
{
	return StartsWith(
		ForwardAsRange<R>(range),
		ForwardAsRange<RW>(what));
}

template<typename R, typename W> constexpr forceinline Requires<
	CForwardRange<R> &&
	!CAsInputRange<W>,
bool> StartsWith(const R& range, const W& what)
{
	return !range.Empty() &&
		range.First() == what;
}

template<typename R, typename W> constexpr forceinline Requires<
	!CInputRange<R> &&
	CAsForwardRange<R> &&
	!CAsInputRange<W>,
bool> StartsWith(R&& range, const W& what)
{
	return StartsWith(
		ForwardAsRange<R>(range),
		what);
}


/** Checks whether ``range`` starts with ``what`` prefix and consume it if it does.

  If ``range`` starts with ``what``, then ``range``'s start moves to the position immediately after occurence of ``what``.
  Otherwise ``range`` remains unchanged.
  @returns true if prefix was consumed.
*/
template<typename R, typename RW> INTRA_CONSTEXPR2 forceinline Requires<
	CForwardRange<R> &&
	CAsConsumableRange<RW>,
bool> ConsumePrefix(R& range, RW&& prefix)
{
	auto whatCopy = ForwardAsRange<RW>(prefix);
	bool result = StartsWith(range, prefixCopy);
	if(result) PopFirstExactly(range, Count(prefixCopy));
	return result;
}

/** Checks whether ``range`` starts with ``prefix`` consuming matching elements.

  If it does, then ``range``'s start moves to the position immediately after occurence of ``prefix``.
  Otherwise ``range`` remains unchanged.
  @returns true if whole ``prefix`` was consumed.
*/
template<typename R, typename RW> INTRA_CONSTEXPR2 forceinline Requires<
	CInputRange<R> &&
	CAsConsumableRange<RW>,
bool> StartsAdvanceWith(R& range, RW&& prefix)
{
	auto prefixCopy = ForwardAsRange<RW>(prefix);
	return z__R::StartsAdvanceWithAdvance(range, prefixCopy);
}

template<typename R, typename RWs> INTRA_CONSTEXPR2 Requires<
	CForwardRange<R> &&
	CNonInfiniteInputRange<RWs> &&
	CNonInfiniteForwardRange<TValueTypeOf<RWs>> &&
	CConvertible<TValueTypeOf<TValueTypeOf<RWs>>, TValueTypeOf<R>>,
bool> StartsWithAnyAdvance(const R& range, RWs& subranges, Optional<size_t&> oSubrangeIndex = null)
{
	if(oSubrangeIndex) oSubrangeIndex.Unwrap() = 0;
	while(!subranges.Empty())
	{
		if(StartsWith(range, subranges.First())) return true;
		if(oSubrangeIndex) oSubrangeIndex.Unwrap()++;
		subranges.PopFirst();
	}
	return false;
}

template<typename R, typename RWs> INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline Requires<
	CAsForwardRange<R> &&
	CAsConsumableRange<RWs> &&
	CAsNonInfiniteForwardRange<TValueTypeOfAs<RWs>> &&
	CConvertible<TValueTypeOfAs<TValueTypeOfAs<RWs>>, TValueTypeOfAs<R>>,
bool> StartsWithAny(R&& range, RWs&& subranges, Optional<size_t&> oSubrangeIndex = null)
{
	auto subrangesCopy = ForwardAsRange<RWs>(subranges);
	return StartsWithAnyAdvance(ForwardAsRange<R>(range), subrangesCopy, oSubrangeIndex);
}

template<typename R, typename RWs,
	typename W = TValueTypeOfAs<RWs>
> INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline Requires<
	CForwardRange<R> &&
	CAsConsumableRange<RWs> &&
	CAsNonInfiniteForwardRange<W> &&
	CConvertible<TValueTypeOfAs<W>, TValueTypeOf<R>>,
bool> StartsAdvanceWithAny(R& range, RWs&& subranges, Optional<size_t&> oSubrangeIndex = null)
{
	auto subrangesCopy = ForwardAsRange<RWs>(subranges);
	bool result = StartsWithAnyAdvance(range, subrangesCopy, oSubrangeIndex);
	if(result) PopFirstExactly(range, Count(subrangesCopy.First()));
	return result;
}


namespace z__R {

template<typename R, typename RW> INTRA_CONSTEXPR2 bool EndsAdvanceWithAdvance(R& range, RW& what)
{
	while(!what.Empty())
	{
		if(range.Empty()) return false;
		if(range.Last() != what.Last()) return false;
		range.PopLast();
		what.PopLast();
	}
	return true;
}

}

template<typename R, typename RW> INTRA_NODISCARD INTRA_MEM_CONSTEXPR forceinline Requires<
	CArrayClass<R> &&
	CArrayClass<RW> &&
	CSame<TArrayElement<R>, TArrayElement<RW>>,
bool> EndsWith(const R& range, const RW& what)
{
	return range.Length() <= what.Length() &&
		BitsEqual(
			range.Data()+range.Length()-what.Length(),
			what.Data(), what.Length()
		);
}

template<typename R, typename RW> INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline Requires<
	CBidirectionalRange<R> &&
	CBidirectionalRange<RW> &&
	!(CHasLength<R> && CHasLength<RW>),
bool> EndsWith(R&& range, RW&& what)
{
	auto rangeCopy = Forward<R>(range);
	auto whatCopy = Forward<RW>(what);
	return z__R::EndsAdvanceWithAdvance(range, what);
}

template<typename R, typename RW> INTRA_CONSTEXPR2 forceinline Requires<
	CBidirectionalRangeWithLength<R> &&
	CBidirectionalRangeWithLength<RW> &&
	!(CHasData<R> && CHasData<RW>),
bool> EndsWith(R&& range, RW&& what)
{
	if(range.Length() < what.Length()) return false;
	auto rangeCopy = Forward<R>(range);
	auto whatCopy = Forward<RW>(what);
	return z__R::EndsAdvanceWithAdvance(rangeCopy, whatCopy);
}

template<typename R, typename RW> INTRA_NODISCARD INTRA_MEM_CONSTEXPR forceinline Requires<
	(!CInputRange<R> || !CInputRange<RW>) &&
	CAsBidirectionalRange<R> &&
	CAsBidirectionalRange<RW>,
bool> EndsWith(R&& range, RW&& what)
{return EndsWith(ForwardAsRange<R>(range), ForwardAsRange<RW>(what));}

template<typename R, typename W> INTRA_NODISCARD constexpr forceinline Requires<
	CBidirectionalRange<R> &&
	!CAsInputRange<W>,
bool> EndsWith(const R& range, const W& what)
{return !range.Empty() && range.Last() == what;}

template<typename R, typename W> INTRA_NODISCARD constexpr forceinline Requires<
	!CInputRange<R> &&
	CAsBidirectionalRange<R> &&
	!CAsInputRange<W>,
bool> EndsWith(R&& range, const W& what)
{return EndsWith(ForwardAsRange<R>(range), what);}


template<typename R1, typename R2, typename P> INTRA_CONSTEXPR2 Requires<
	CAsForwardRange<R1> &&
	CAsForwardRange<R2> &&
	CAsElementAsPredicate<P, const TValueTypeOfAs<R1>, const TValueTypeOfAs<R2>>,
int> LexCompare(R1&& r1, R2&& r2, P&& cmdPred)
{
	auto&& range1 = ForwardAsRange<R1>(r1);
	auto&& range2 = ForwardAsRange<R2>(r2);
	auto&& pred = ForwardAsFunc<P>(cmdPred);
	while(!range1.Empty() && !range2.Empty())
	{
		auto&& a = Next(range1);
		auto&& b = Next(range2);
		if(pred(a, b)) return -1;
		if(pred(b, a)) return 1;
	}
	if(range1.Empty())
	{
		if(range2.Empty()) return 0;
		return -1;
	}
	return 1;
}

template<typename R1, typename R2,
	typename T1 = TArrayElement<R1>,
	typename T2 = TArrayElement<R2>>
INTRA_CONSTEXPR2 Requires<
	CIntegral<T1> &&
	CSame<T1, T2> &&
	CArrayClass<R1> &&
	CArrayClass<R2> &&
	(sizeof(T1) == 1 || INTRA_PLATFORM_ENDIANESS == INTRA_PLATFORM_ENDIANESS_BigEndian),
int> LexCompare(const R1& r1, const R2& r2)
{
	const size_t l1 = LengthOf(r1), l2 = LengthOf(r2);
	const int result = BitsEqual(DataOf(r1), DataOf(r2), FMin(l1, l2));
	return result == 0? FCmp(l1, l2): result;
}

template<typename R1, typename R2> INTRA_CONSTEXPR2 Requires<
	CForwardRange<R1> &&
	CForwardRange<R2> &&
	!(
		CIntegral<TValueTypeOf<R1>> &&
		CSame<TArrayElement<R1>, TArrayElement<R2>> &&
		CArrayClass<R1> &&
		CArrayClass<R2> &&
		(sizeof(TArrayElement<R1>) == 1 ||
			INTRA_PLATFORM_ENDIANESS == INTRA_PLATFORM_ENDIANESS_BigEndian)
	),
int> LexCompare(const R1& r1, const R2& r2)
{return LexCompare(r1, r2, FLess);}
INTRA_CORE_RANGE_END
