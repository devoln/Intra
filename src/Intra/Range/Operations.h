#pragma once

#include "Intra/Assert.h"
#include "Intra/Functional.h"
#include "Intra/Container/Optional.h"
#include "Intra/Range/Concepts.h"
#include "Intra/Container/Tuple.h"
#include "Intra/TypeSafe.h"

#include "Intra/Range/Span.h"

INTRA_BEGIN
INTRA_IGNORE_WARN_DEFAULT_CTOR_IMPLICITLY_DELETED
constexpr auto ForEach = []<typename F>(F&& f) {
	return [f = ForwardAsFunc<F>(f)]<typename Collection>(Collection&& collection)
		requires(CConsumableList<Collection> || CStaticLengthContainer<Collection>)
	{
		if constexpr(CConsumableList<Collection>)
		{
			auto range = ForwardAsRange<Collection>(collection);
			index_t index = 0;
			while(!range.Empty())
			{
				if constexpr(CCallable<decltype(f), TListValueRef<Collection>, index_t>)
					f(range.First(), index++);
				else f(range.First());
				range.PopFirst();
			}
		}
		else 
		{
			return [&]<size_t... I>(TIndexSeq<I...>)
			{
				if constexpr(CCallable<decltype(f), decltype(FieldAt<I>(collection)), index_t> && ...)
				{
					constexpr bool isAnyReturnTypeVoid = (CVoid<decltype(f(FieldAt<I>(collection), I))> || ...);
					if constexpr(isAnyReturnTypeVoid) (f(FieldAt<I>(collection)), ...);
					else return Tuple(f(FieldAt<I>(collection))...);
				}
				else
				{
					constexpr bool isAnyReturnTypeVoid = (CVoid<decltype(f(FieldAt<I>(collection)))> || ...);
					if constexpr(isAnyReturnTypeVoid) (f(FieldAt<I>(collection)), ...);
					else return Tuple(f(FieldAt<I>(collection))...);
				}
			}(TMakeIndexSeq<StaticLength<Collection>>());
		}
	};
};

constexpr auto First = []<CList R>(R&& range) {
	if constexpr(CHasFirst<R>) return range.First();
	else return ForwardAsRange<R>(range).First();
};
constexpr auto Last = []<CBidirectionalList R>(R&& range) {
	if constexpr(CHasLast<R>) return range.Last();
	else return ForwardAsRange<R>(range).Last();
};
constexpr auto IsEmpty = []<CHasEmpty R>(R&& range) noexcept {return range.Empty();};
constexpr auto Length = []<CHasLengthOf R>(R&& range) noexcept {return LengthOf(range);};

constexpr auto IsFull = []<CAssignableRange R>(R&& range) noexcept {
	if constexpr(CHasFull<R>) return range.Full();
	else return range.Empty();
};

constexpr auto PopFirst = []<CHasPopFirst R>(R&& range) {range.PopFirst();};
constexpr auto PopLast = []<CHasPopLast R>(R&& range) {range.PopLast();};

template<class TPop, bool PopUntilEmpty = false> struct TPopCount
{
	ClampedLongSize MaxElementsToPop;
	explicit constexpr TPopCount(ClampedLongSize maxElementsToPop) noexcept:
		MaxElementsToPop(maxElementsToPop) {}

	template<class R> requires CCallable<TPop, R&>
	constexpr auto operator()(R&& range) const
	{
		static_assert(!(PopUntilEmpty && CInfiniteRange<R>));
		if constexpr(CSame<TPop, decltype(First)> && CHasPopFirstCount<R>)
		{
			if constexpr(!PopUntilEmpty) return range.PopFirstCount(MaxElementsToPop);
			else return range.PopFirstCount(MaxValueOf<decltype(range.PopFirstCount(0))>);
		}
		else if constexpr(CSame<TPop, decltype(Last)> && CHasPopLastCount<R>)
		{
			if constexpr(!PopUntilEmpty) return range.PopLastCount(MaxElementsToPop);
			else return range.PopLastCount(MaxValueOf<decltype(range.PopLastCount(0))>);
		}
		else
		{
			if constexpr(CSame<index_t, int64>)
				return index_t(impl(range, size_t(MaxElementsToPop)));
			else
			{
				int64 res = 0;
				auto leftElementsToPop = uint64(MaxElementsToPop);
				while((PopUntilEmpty || res != int64(MaxElementsToPop)) && !range.Empty())
					res += impl(range, size_t(Overflow::Saturate<uint64>(leftElementsToPop)));
				return res;
			}
		}
	}

private:
	template<class R> constexpr size_t impl(R& range, size_t maxElementsToPop) const
	{
		size_t poppedElements = 0;
		TPop pop;
		while(!range.Empty())
		{
			if constexpr(!PopUntilEmpty)
				if(poppedElements < maxElementsToPop) break;
			pop(range);
			poppedElements++;
		}
		return poppedElements;
	}
};
using PopFirstCount = TPopCount<decltype(First)>;
using PopLastCount = TPopCount<decltype(Last)>;
constexpr TPopCount<decltype(First), true> PopFirstAllCount(0);

constexpr struct
{
	template<CRange R> requires CHasNext<R> || CCopyConstructible<TRangeValue<R>>
	[[nodiscard]] constexpr decltype(auto) operator()(R&& range) const
	{
		if constexpr(CHasNext<R>) return range.Next();
		else
		{
			auto res = First(range);
			PopFirst(range);
			return res;
		}
	}
} Next;

template<class TPop> struct TPopExactly
{
	LongSize NumElementsToPop;
	explicit constexpr TPopExactly(LongSize numElementsToPop) noexcept: NumElementsToPop(numElementsToPop) {}

	template<class R> requires CCallable<TPop, R&>
	constexpr void operator()(R&& range) const
	{
		auto poppedElements = range|TPopCount<TPop>(NumElementsToPop);
		INTRA_POSTCONDITION(int64(poppedElements) == int64(NumElementsToPop));
		(void)poppedElements;
	}
};
using PopFirstExactly = TPopExactly<decltype(First)>;
using PopLastExactly = TPopExactly<decltype(Last)>;

template<class P> struct PopFirstWhile: P
{
	template<typename P1> explicit constexpr PopFirstWhile(P1&& pred): P(INTRA_FWD(pred)) {}

	template<CRange R> requires (!CConst<R>) && CCallable<P&&, TRangeValue<R>>
	constexpr auto operator()(R&& range) const
	{
		while(!range.Empty() && P::operator()(range.First()))
			range.PopFirst();
		return range;
	}
};
template<typename P1> PopFirstWhile(P1&&) -> PopFirstWhile<TRemoveConstRef<TFunctorOf<P1>>>;
constexpr auto PopFirstUntil = [](auto&& pred) {return PopFirstWhile(FNot(INTRA_FWD(pred)));};

/** Pop elements of \p range one by one until the first element for which \p pred returns true.
@param pred A predicate taking an element of the range.
@note The found element index equals to the number of the false evaluations of \p pred.
@return The remaining part of \p range including found element.
*/
template<class P> struct PopLastWhile: P
{
	template<typename P1> explicit constexpr PopLastWhile(P1&& pred): P(INTRA_FWD(pred)) {}

	template<CBidirectionalRange R> requires (!CConst<R>) && CCallable<P&&, TRangeValue<R>>
		constexpr auto operator()(R&& range) const
	{
		while(!range.Empty() && P::operator()(range.Last()))
			range.PopLast();
		return range;
	}
};
template<typename P1> PopLastWhile(P1&&) -> PopLastWhile<TRemoveConstRef<TFunctorOf<P1>>>;
constexpr auto PopLastUntil = [](auto&& pred) {return PopLastWhile(FNot(INTRA_FWD(pred)));};

constexpr struct
{
	template<typename R> requires CConsumableList<R> || CHasLengthOf<R>
	[[nodiscard]] constexpr auto operator()(R&& range) const
	{
		if constexpr(CHasLengthOf<R>) return LengthOf(range);
		else if constexpr(CRange<R> && CNonConstRValueReference<R>) return range|PopFirstAllCount;
		else return operator()(Dup(ForwardAsRange<R>(range)));
	}
} Count;

template<class TInplaceOp> struct FunctionalRangeOp: TInplaceOp
{
	template<typename... Args> requires CConstructible<TInplaceOp, Args...>
	explicit constexpr FunctionalRangeOp(Args&&... args): TInplaceOp(INTRA_FWD(args)...) {}

	template<CAccessibleList R> requires CCallable<TInplaceOp, TRangeOfRef<R>>
	[[nodiscard]] constexpr decltype(auto) operator()(R&& r) const
	{
		auto result = ForwardAsRange<R>(r);
		TInplaceOp::operator()(result);
		return result;
	}
};
template<class Op> FunctionalRangeOp(Op) -> FunctionalRangeOp<Op>;

constexpr auto Drop = [](ClampedSize maxElementsToDrop) noexcept {
	return FunctionalRangeOp(PopFirstCount(maxElementsToDrop));
};
constexpr auto DropLast = [](ClampedSize maxElementsToDrop) noexcept {
	return FunctionalRangeOp(PopLastCount(maxElementsToDrop));
};
constexpr auto DropExactly = [](ClampedSize maxElementsToDrop) {
	return FunctionalRangeOp(PopFirstExactly(maxElementsToDrop));
};
constexpr auto DropLastExactly = [](ClampedSize maxElementsToDrop) {
	return FunctionalRangeOp(PopLastExactly(maxElementsToDrop));
};
constexpr auto DropWhile = [](auto&& pred) noexcept {
	return FunctionalRangeOp(PopFirstWhile(INTRA_FWD(pred)));
};
constexpr auto DropLastWhile = [](auto&& pred) noexcept {
	return FunctionalRangeOp(PopLastWhile(INTRA_FWD(pred)));
};

#if INTRA_CONSTEXPR_TEST
static_assert(CSpan<int>()|DropExactly(ClampedSize(Index(0)))|IsEmpty);
static_assert(Drop(2).MaxElementsToPop == 2);
static_assert(CCallable<PopFirstCount, CSpan<int>>);
#endif

/// Drop all elements until `pred` is true or range is empty.
/// @param pred Predicate.
constexpr auto DropUntil = [](auto&& pred) {
	return FunctionalRangeOp(PopFirstUntil(INTRA_FWD(pred)));
};
constexpr auto DropLastUntil = [](auto&& pred) {
	return FunctionalRangeOp(PopLastUntil(INTRA_FWD(pred)));
};

constexpr auto Contains = []<typename P>(P&& pred) {
	return [drop = DropUntil(ForwardAsFunc<P>(pred))](auto&& range) -> decltype(range|drop|FNot(IsEmpty)) {
		return range|drop|FNot(IsEmpty);
	};
};

struct Tail
{
	ClampedSize MaxLength;

	explicit constexpr Tail(ClampedSize maxLength) noexcept: MaxLength(maxLength) {}

	template<CNonInfiniteForwardList R>
	[[nodiscard]] constexpr decltype(auto) operator()(R&& range) const
	{
		if constexpr(CHasLength<TRangeOfRef<R>>) return range|Drop(range.Length() - index_t(MaxLength));
		else if constexpr(CForwardRange<R> && CNonConstRValueReference<R>)
		{
			auto r = ForwardAsRange<R>(range);
			auto temp = r|Drop(MaxLength);
			while(!temp.Empty())
			{
				temp.PopFirst();
				r.PopFirst();
			}
			return r;
		}
		else return operator()(Dup(ForwardAsRange<R>(range)));
	}
};

struct AtIndex
{
	Index At;
	explicit constexpr AtIndex(Index index) noexcept: At(index) {}

	template<typename R> requires CHasIndex<R> || CAccessibleList<R>
	[[nodiscard]] constexpr decltype(auto) operator()(R&& range) const
	{
		if constexpr(CHasIndex<R>) return range[At];
		else return ForwardAsRange<R>(range)|DropExactly(At)|First;
	}
};

template<typename TGet> struct TRangeTryGet
{
	template<typename R> requires CCallable<TGet, R&&>
	[[nodiscard]] constexpr Optional<TRangeValue<R>> operator()(R&& range) const
	{
		if(range.Empty()) return null;
		return get(Forward<R>(range));
	}
private:
	static constexpr TGet get{};
};
constexpr TRangeTryGet<decltype(First)> TryFirst;
constexpr TRangeTryGet<decltype(Last)> TryLast;
constexpr TRangeTryGet<decltype(Next)> TryNext;

constexpr auto PutRef = [](TUnsafe, auto&& value) {
	return [&]<COutputOf<decltype(value)> O>(O&& out) {
		if constexpr(CHasPut<R, T>) out.Put(INTRA_FWD(value));
		else
		{
			out.First() = INTRA_FWD(value);
			out.PopFirst();
		}
	};
};
constexpr auto PutOnce = [](auto&& value) {
	return [value = INTRA_FWD(value)]<COutputOf<decltype(value)> O>(O&& out) {
		out|PutRef(Unsafe, INTRA_MOVE(value));
	};
};
constexpr auto Put = [](auto&& value) {
	return [value = INTRA_FWD(value)]<COutputOf<decltype(value)> O>(O&& out) {
		out|PutRef(Unsafe, value);
	};
};

template<class MatchPred,
	class TGet1 = decltype(First), class TPop1 = decltype(PopFirst),
	class TGet2 = TGet1, class TPop2 = TPop1>
class TPopWhileMatch: MatchPred
{
	static constexpr TGet1 get1{};
	static constexpr TGet2 get2{};
	static constexpr TPop1 pop1{};
	static constexpr TPop2 pop2{};

	template<class R1, class R2, class P>
	static constexpr void impl(R1& range1, R2& range2, P& matchPred)
	{
		while(!range1.Empty() && !range2.Empty() &&
			matchPred(get1(range1), get2(range2)))
		{
			pop1(range1);
			pop2(range2);
		}
	}
public:
	constexpr TPopWhileMatch() = default;

	template<typename MatchPredicate>
	explicit constexpr TPopWhileMatch(MatchPredicate&& matchPredicate):
		MatchPred(ForwardAsFunc<MatchPredicate>(matchPredicate)) {}

	template<class R1, class R2> requires
		CCallable<TGet1, R1&&> && CCallable<TGet2, R2&&> &&
		CCallable<TPop1, R1&&> && CCallable<TPop2, R2&&> &&
		CCallable<MatchPred, TRangeValueRef<R1>, TRangeValueRef<R2>>
	constexpr void operator()(R1&& range1, R2&& range2)
	{
		impl(range1, range2, static_cast<MatchPred&>(*this));
	}

	template<class R1, class R2> requires
		CCallable<TGet1, R1&&> && CCallable<TGet2, R2&&> &&
		CCallable<TPop1, R1&&> && CCallable<TPop2, R2&&> &&
		CCallable<const MatchPred, TRangeValueRef<R1>, TRangeValueRef<R2>>
	constexpr void operator()(R1&& range1, R2&& range2) const
	{
		impl(range1, range2, static_cast<const MatchPred&>(*this));
	}
};
template<typename MatchPredicate> TPopWhileMatch(MatchPredicate) -> TPopWhileMatch<TFunctorOf<MatchPredicate>>;
constexpr TPopWhileMatch<decltype(Equal)> PopFirstWhileEqual;
constexpr TPopWhileMatch<decltype(Equal), decltype(Last), decltype(PopLast)> PopLastWhileEqual;

template<class Range1, class MatchPred,
	class TGet1, class TPop1,
	class TPrefixLengthPredicate,
	class TGet2 = TGet1, class TPop2 = TPop1>
class TMatch: MatchPred
{
	static_assert(CAnyOf<TGet1, decltype(First), decltype(Last)>);
	static_assert(CAnyOf<TGet2, decltype(First), decltype(Last)>);
	static_assert(CAnyOf<TPop1, decltype(PopFirst), decltype(PopLast)>);
	static_assert(CAnyOf<TPop2, decltype(PopFirst), decltype(PopLast)>);
	static_assert(CSame<TGet1, decltype(First)> == CSame<TPop1, decltype(PopFirst)>);
	static_assert(CSame<TGet2, decltype(First)> == CSame<TPop2, decltype(PopFirst)>);
	static_assert(CAnyOf<TPrefixLengthPredicate, decltype(LEqual), decltype(Equal), decltype(GEqual)>);

	static constexpr TGet1 get1{};
	static constexpr TGet1 get2{};
	static constexpr TPop1 pop1{};
	static constexpr TPop1 pop2{};
	static constexpr TPrefixLengthPredicate prefixLengthPredicate{};

	using T1 = TRangeValue<Range1>;

	Range1 mRange1;
public:
	template<typename R1> requires CForwardList<R1>
	explicit constexpr TMatch(R1&& r1): mRange1(ForwardAsRange<R1>(r1)) {}

	template<typename R1, typename MatchPredicate> requires CForwardList<R1>
	constexpr TMatch(R1&& r1, MatchPredicate&& matchPredicate):
		MatchPred(ForwardAsFunc<MatchPredicate>(matchPredicate)),
		mRange1(ForwardAsRange<R1>(r1)) {}

	template<CConsumableList R2> [[nodiscard]] constexpr bool operator()(R2&& r2) const
	{
		static_assert(!(
			CSame<TPrefixLengthPredicate, decltype(Equal)> &&
			CInfiniteRange<Range1> &&
			CInfiniteList<R2>
		));

		if constexpr(CArrayList<Range1> && CArrayList<R2> &&
			CSame<T1, TArrayElement<R2>> &&
			CTriviallyEqualComparable<T1> &&
			CSame<TPop1, TPop2> && CSame<TGet1, TGet2> &&
			CSame<MatchPred, decltype(Equal)>)
			if(!IsConstantEvaluated(*this, r2))
		{
			if(!prefixLengthPredicate(LengthOf(mRange1), LengthOf(r2))) return false;
			auto data1 = DataOf(mRange1);
			auto data2 = DataOf(r2);
			if constexpr(CSame<TGet2, decltype(Last)>)
			{
				if constexpr(CSame<TPrefixLengthPredicate, decltype(LEqual)>)
					data2 += LengthOf(r2) - LengthOf(mRange1);
				else if constexpr(CSame<TPrefixLengthPredicate, decltype(GEqual)>)
					data1 += LengthOf(mRange1) - LengthOf(r2);
			}
			auto length = CSame<TPrefixLengthPredicate, decltype(GEqual)>? LengthOf(r2): LengthOf(mRange1);
			return __builtin_memcmp(data1, data2, length*sizeof(T1)) == 0;
		}
		if constexpr(CSameNotVoid<TRawUnicodeUnit<Range1>, TRawUnicodeUnit<R2>> && CSame<MatchPred, decltype(Equal)>)
        {
		    return r2.RawUnicodeUnits()|TMatch(mRange1.RawUnicodeUnits());
        }
		else
		{
			auto range1 = mRange1;
			auto range2 = ForwardAsRange<R2>(r2);
			const auto popWhileMatch = TPopWhileMatch<MatchPred, TGet1, TPop1, TGet2, TPop2>(MatchPredicate());
			popWhileMatch(range1, range2);
			if constexpr(CSame<TPrefixLengthPredicate, decltype(LEqual)>) return range1.Empty();
			else if constexpr(CSame<TPrefixLengthPredicate, decltype(GEqual)>) return range2.Empty();
			else return range1.Empty() && range2.Empty();
		}
	}

	[[nodiscard]] constexpr auto& MatchPredicate() noexcept {return static_cast<MatchPred&>(*this);}
	[[nodiscard]] constexpr auto& MatchPredicate() const noexcept {return static_cast<const MatchPred&>(*this);}
};

template<class PrefixRange, class MatchPred = decltype(Equal)>
class StartsWith: public TMatch<PrefixRange, MatchPred, decltype(First), decltype(PopFirst), decltype(LEqual)>
{
public:
	using Parent = TMatch<PrefixRange, MatchPred, decltype(First), decltype(PopFirst), decltype(LEqual)>;
	using Parent::Parent;
};
template<typename RPrefix, typename MatchPredicate>
StartsWith(RPrefix, MatchPredicate) -> StartsWith<TRangeOf<RPrefix>, TFunctorOf<MatchPredicate>>;

template<class SuffixRange, class MatchPred = decltype(Equal)>
class EndsWith: public TMatch<SuffixRange, MatchPred, decltype(Last), decltype(PopLast), decltype(LEqual)>
{
public:
	using Parent = TMatch<SuffixRange, MatchPred, decltype(Last), decltype(PopLast), decltype(LEqual)>;
	using Parent::Parent;
};
template<typename RSuffix, typename MatchPredicate>
EndsWith(RSuffix, MatchPredicate) -> EndsWith<TRangeOf<RSuffix>, TFunctorOf<MatchPredicate>>;

template<class Range1, class MatchPred = decltype(Equal)>
class MatchesWith: public TMatch<Range1, MatchPred, decltype(First), decltype(PopFirst), decltype(Equal)>
{
public:
	using Parent = TMatch<Range1, MatchPred, decltype(First), decltype(PopFirst), decltype(Equal)>;
	using Parent::Parent;
};
template<typename R1, typename MatchPredicate>
MatchesWith(R1, MatchPredicate) -> MatchesWith<TRangeOf<R1>, TFunctorOf<MatchPredicate>>;
template<typename R1>
MatchesWith(R1) -> MatchesWith<TRangeOf<R1>>;

template<class Range2, class ComparePred = decltype(Less)> class LexCompareTo: ComparePred
{
	Range2 mRange2;
	using T2 = TRangeValue<Range2>;
public:
	template<CForwardList R2> explicit constexpr LexCompareTo(R2&& r2): mRange2(ForwardAsRange<R2>(r2)) {}

	template<CForwardList R2, typename ComparePredicate>
	constexpr LexCompareTo(R2&& r2, ComparePredicate&& comparePredicate):
		ComparePred(ForwardAsFunc<ComparePredicate>(comparePredicate)),
		mRange2(ForwardAsRange<R2>(r2)) {}

	template<CConsumableList R1>	[[nodiscard]] constexpr int operator()(R1&& r1) const
	{
		if constexpr(CSameArrays<R1, Range2> &&
			CByteByByteLexicographicallyComparable<T2> &&
			CAnyOf<ComparePred, decltype(Less), decltype(Greater)>)
			if(!IsConstantEvaluated(*this, r1)) //__builtin_memcmp doesn't work in constexpr with GCC with heap allocated memory
		{
			const auto res = __builtin_memcmp(DataOf(r1), DataOf(mRange2),
				size_t(Min(LengthOf(r1), LengthOf(mRange2)))*sizeof(T2));
			if(res == 0) res = Cmp(LengthOf(mRange2), LengthOf(r1));
			if constexpr(CSame<ComparePred, decltype(Greater)>) res = -res;
			return res;
		}

		if constexpr(CSameNotVoid<TRawUnicodeUnit<R1>, TRawUnicodeUnit<Range2>> &&
			CAnyOf<ComparePred, decltype(Less), decltype(Greater)>)
        {
            return r1.RawUnicodeUnits()|LexCompareTo(mRange2.RawUnicodeUnits());
        }
		else
		{
			auto range1 = ForwardAsRange<R1>(r1);
			auto range2 = mRange2;
			auto& pred = ComparePredicate();
			for(;;)
			{
				if(range2.Empty()) return range1.Empty()? 1: 0;
				if(range1.Empty()) return -1;
				auto&& a = Next(range1);
				auto&& b = Next(range2);
				if(pred(a, b)) return -1;
				if constexpr(CSame<ComparePred, decltype(Less)> && CIntegral<T2>)
				{
					//Unlike b < a this helps most compilers to reuse the result of the previous cmp instruction
					if(a > b) return 1;
				}
				else if(pred(b, a)) return 1;
			}
		}
	}

	[[nodiscard]] constexpr auto& ComparePredicate() noexcept {return static_cast<ComparePred&>(*this);}
	[[nodiscard]] constexpr auto& ComparePredicate() const noexcept {return static_cast<const ComparePred&>(*this);}
};
template<typename R2, typename ComparePredicate>
LexCompareTo(R2, ComparePredicate) -> LexCompareTo<TRangeOf<R2>, TFunctorOf<ComparePredicate>>;
template<typename R2> LexCompareTo(R2) -> LexCompareTo<TRangeOf<R2>>;

INTRA_END
