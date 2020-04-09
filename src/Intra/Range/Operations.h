#pragma once

#include "Intra/Assert.h"
#include "Intra/Functional.h"
#include "Intra/Container/Optional.h"
#include "Intra/Range/Concepts.h"
#include "Intra/Container/Tuple.h"
#include "Intra/TypeSafe.h"

INTRA_BEGIN

constexpr struct
{
	template<class R, class = Requires<CHasFirst<R> || CAsInputRange<R>>>
	[[nodiscard]] constexpr decltype(auto) operator()(R&& range) const
	{
		if constexpr(CHasFirst<R>) return range.First();
		else operator()(ForwardAsRange<R>(range));
	}
} First;

constexpr struct
{
	template<class R, class = Requires<CHasLast<R> || CAsBidirectionalRange<R>>>
	[[nodiscard]] constexpr decltype(auto) operator()(R&& range) const
	{
		if constexpr(CHasLast<R>) return range.Last();
		else operator()(ForwardAsRange<R>(range));
	}
} Last;

constexpr struct
{
	template<typename R, typename = Requires<CHasEmpty<R>>>
	[[nodiscard]] constexpr bool operator()(R&& range) const noexcept {return range.Empty();}
} IsEmpty;

constexpr struct
{
	template<typename R, typename = Requires<CHasFull<R> || CAssignableRange<R> && CHasEmpty<R>>>
	[[nodiscard]] constexpr bool operator()(R&& range) const noexcept
	{
		if constexpr(CHasFull<R>) return range.Full();
		else return range.Empty();
	}
} IsFull;

constexpr struct
{
	template<class R, class = Requires<CInputRange<R> && !CConst<R>>>
	constexpr void operator()(R&& range) const {range.PopFirst();}
} PopFirst;

constexpr struct
{
	template<class R, class = Requires<CBidirectionalRange<R> && !CConst<R>>>
	constexpr void operator()(R&& range) const {range.PopLast();}
} PopLast;

template<class TPop, bool PopUntilEmpty = false> struct TPopCount
{
	ClampedLongSize MaxElementsToPop;
	explicit constexpr TPopCount(ClampedLongSize maxElementsToPop) noexcept: MaxElementsToPop(maxElementsToPop) {}

	template<class R, class = Requires<CCallable<TPop, R&>>>
	constexpr auto operator()(R&& range) const
	{
		static_assert(!(PopUntilEmpty && CInfiniteInputRange<R>));
		if constexpr(CSame<TPop, decltype(First)> && CHasPopFirstCount<R>)
		{
			if constexpr(!PopUntilEmpty) return range.PopFirstCount(MaxElementsToPop);
			else return range.PopFirstCount(LMaxOf<decltype(range.PopFirstCount(0))>);
		}
		else if constexpr(CSame<TPop, decltype(Last)> && CHasPopLastCount<R>)
		{
			if constexpr(!PopUntilEmpty) return range.PopLastCount(MaxElementsToPop);
			else return range.PopLastCount(LMaxOf<decltype(range.PopLastCount(0))>);
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
	template<class R> size_t impl(R& range, size_t maxElementsToPop)
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
	template<class R, class = Requires<CInputRange<R> && (CHasNext<R> || CCopyConstructible<TValueTypeOf<R>>)>>
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

	template<class R, class = Requires<CCallable<TPop, R&>>>
	constexpr void operator()(R&& range) const
	{
		auto poppedElements = range|TPopCount<TPop>(NumElementsToPop);
		INTRA_POSTCONDITION(int64(poppedElements) == int64(NumElementsToPop));
		(void)poppedElements;
	}
};
using PopFirstExactly = TPopExactly<decltype(First)>;
using PopLastExactly = TPopExactly<decltype(Last)>;

constexpr struct
{
	template<typename R, typename = Requires<CAsConsumableRange<R> || CHasLengthOf<R>>>
	[[nodiscard]] constexpr auto operator()(R&& range) const
	{
		if constexpr(CHasLengthOf<R>) return LengthOf(range);
		else if constexpr(CInputRange<R> && CNCRValueReference<R>) return range|PopFirstAllCount;
		else return operator()(Dup(ForwardAsRange<R>(range)));
	}
} Count;

template<class TPop> struct TDrop
{
	ClampedSize MaxElementsToSkip;
	explicit constexpr TDrop(ClampedSize maxElementsToSkip) noexcept: MaxElementsToSkip(maxElementsToSkip) {}
	template<typename R, typename = Requires<CAsAccessibleRange<R> && CCallable<TPop, R&>>>
	[[nodiscard]] constexpr decltype(auto) operator()(R&& range) const
	{
		auto result = ForwardAsRange<R>(range);
		result|TPopCount<TPop>(MaxElementsToSkip);
		return result;
	}
};
using Drop = TDrop<decltype(PopFirst)>;
using DropLast = TDrop<decltype(PopLast)>;

template<class TPop> struct TDropExactly
{
	Size NumElementsToSkip;
	explicit constexpr TDropExactly(Size numElementsToSkip) noexcept: NumElementsToSkip(numElementsToSkip) {}
	template<typename R, typename = Requires<CAsAccessibleRange<R> && CCallable<TPop, R&>>>
	[[nodiscard]] constexpr decltype(auto) operator()(R&& range) const
	{
		auto result = ForwardAsRange<R>(range);
		result|TPopExactly<TPop>(NumElementsToSkip);
		return result;
	}
};
using DropExactly = TDropExactly<decltype(PopFirst)>;
using DropLastExactly = TDropExactly<decltype(PopLast)>;

struct Tail
{
	ClampedSize MaxLength;

	explicit constexpr Tail(ClampedSize maxLength) noexcept: MaxLength(maxLength) {}

	template<typename R, typename = Requires<CAsNonInfiniteForwardRange<R>>>
	[[nodiscard]] constexpr decltype(auto) operator()(R&& range) const
	{
		if constexpr(CHasLength<TRangeOfRef<R>>) return range|Drop(range.Length() - index_t(MaxLength));
		else if constexpr(CForwardRange<R> && CNCRValueReference<R>)
		{
			auto r = ForwardAsRange<R>(range);
			auto temp = r;
			temp|PopFirstCount(MaxLength);
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
	template<typename R, typename = Requires<CHasIndex<R> || CAsAccessibleRange<R>>>
	[[nodiscard]] constexpr decltype(auto) operator()(R&& range) const
	{
		if constexpr(CHasIndex<R>) return range[At];
		else return ForwardAsRange<R>(range)|DropExactly(At)|First;
	}
};

template<typename TGet> struct TTry
{
	template<typename R, typename = Requires<CCallable<TGet, R&&>>>
	[[nodiscard]] constexpr Optional<TValueTypeOf<R>> operator()(R&& range) const
	{
		if(range.Empty()) return null;
		return get(Forward<R>(range));
	}
private:
	static constexpr TGet get{};
};
constexpr TTry<decltype(First)> TryFirst;
constexpr TTry<decltype(Last)> TryLast;
constexpr TTry<decltype(Next)> TryNext;

template<class R, typename T, class = Requires<CHasPut<R, T> || CAssignableRange<R>>>
constexpr void Put(R&& range, T&& val)
{
	if constexpr(CHasPut<R, T>) range.Put(val);
	else
	{
		range.First() = Forward<T>(val);
		range.PopFirst();
	}
}

template<class MatchPred,
	class TGet1 = decltype(First), class TPop1 = decltype(PopFirst),
	class TGet2 = TGet1, class TPop2 = TPop1>
class TPopWhileMatch: MatchPred
{
	static constexpr TGet1 get1{};
	static constexpr TGet2 get2[};
	static constexpr TPop1 pop1{};
	static constexpr TPop2 pop2{};
	template<class R1, class R2, class P> static constexpr void impl(R1& range1, R2& range2, P& matchPred)
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

	template<class R1, class R2, class = Requires<
		CCallable<TGet1, R1&&> && CCallable<TGet2, R2&&> &&
		CCallable<TPop1, R1&&> && CCallable<TPop2, R2&&> &&
		CCallable<MatchPred, TReturnValueTypeOf<R1>, TReturnValueTypeOf<R2>>
	>> constexpr void operator()(R1&& range1, R2&& range2)
	{
		impl(range1, range2, static_cast<MatchPred&>(*this));
	}

	template<class R1, class R2, class = Requires<
		CCallable<TGet1, R1&&> && CCallable<TGet2, R2&&> &&
		CCallable<TPop1, R1&&> && CCallable<TPop2, R2&&> &&
		CCallable<const MatchPred, TReturnValueTypeOf<R1>, TReturnValueTypeOf<R2>>
	>> constexpr void operator()(R1&& range1, R2&& range2) const
	{
		impl(range1, range2, static_cast<const MatchPred&>(*this));
	}
};
template<typename MatchPredicate> TPopWhileMatch(MatchPredicate) -> TPopWhileMatch<TFunctorOf<MatchPredicate>>;
constexpr TPopWhileMatch PopFirstWhileEqual;
constexpr TPopWhileMatch<TFEqual, decltype(Last), decltype(PopLast)> PopLastWhileEqual;

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
	static_assert(CAnyOf<TPrefixLengthPredicate, decltype(FLEqual), decltype(FEqual), decltype(FGEqual)>);

	static constexpr TGet1 get1{};
	static constexpr TGet1 get2{};
	static constexpr TPop1 pop1{};
	static constexpr TPop1 pop2{};
	static constexpr TPrefixLengthPredicate prefixLengthPredicate{};

	using T1 = TValueTypeOf<Range1>;

	Range1 mRange1;
public:
	template<typename R1, typename = Requires<CAsForwardRange<R1>>>
	explicit constexpr TMatch(R1&& r1): mRange1(ForwardAsRange<R1>(r1)) {}

	template<typename R1, typename MatchPredicate, typename = Requires<CAsForwardRange<R1>>>
	constexpr TMatch(R1&& r1, MatchPredicate&& matchPredicate):
		MatchPred(ForwardAsFunc<MatchPredicate>(matchPredicate)),
		mRange1(ForwardAsRange<R1>(r1)) {}

	template<typename R2, typename = Requires<CAsConsumableRange<R2>>>
	[[nodiscard]] constexpr bool operator()(R2&& r2) const
	{
		static_assert(!(
			CSame<TPrefixLengthPredicate, TFEqual> &&
			CInfiniteInputRange<Range1> &&
			CAsInfiniteInputRange<R2>
		));

		if constexpr(CArrayClass<Range1> && CArrayClass<R2> &&
			CSame<T1, TArrayElement<R2>> &&
			CTriviallyEqualComparable<T1> &&
			CSame<TPop1, TPop2> && CSame<TGet1, TGet2> &&
			CSame<MatchPred, TFEqual>)
		{
			if(!prefixLengthPredicate(LengthOf(mRange1), LengthOf(r2))) return false;
			auto data1 = DataOf(mRange1);
			auto data2 = DataOf(r2);
			if constexpr(CSame<TGet2, decltype(Last)>)
			{
				if constexpr(CSame<TPrefixLengthPredicate, decltype(FLEqual)>)
					data2 += LengthOf(r2) - LengthOf(mRange1);
				else if constexpr(CSame<TPrefixLengthPredicate, decltype(FGEqual)>)
					data1 += LengthOf(mRange1) - LengthOf(r2);
			}
			auto length = CSame<TPrefixLengthPredicate, decltype(FGEqual)>? LengthOf(r2): LengthOf(mRange1);
			return __builtin_memcmp(data1, data2, length*sizeof(T1)) == 0;
		}
		else if constexpr(CSameNotVoid<TRawUnicodeUnit<Range1>, TRawUnicodeUnit<R2>> && CSame<MatchPred, TFEqual>)
        {
		    return r2.RawUnicodeUnits()|TMatch(mRange1.RawUnicodeUnits());
        }
		else
		{
			auto range1 = mRange1;
			auto range2 = ForwardAsRange<R2>(r2);
			TPopWhileMatch<MatchPred, TGet1, TPop1, TGet2, TPop2>(MatchPredicate())(range1, range2);
			if constexpr(CSame<TPrefixLengthPredicate, TFLEqual>) return range1.Empty();
			else if constexpr(CSame<TPrefixLengthPredicate, TFGEqual>) return range2.Empty();
			else return range1.Empty() && range2.Empty();
		}
	}

	[[nodiscard]] constexpr auto& MatchPredicate() noexcept {return static_cast<MatchPred&>(*this);}
	[[nodiscard]] constexpr auto& MatchPredicate() const noexcept {return static_cast<const MatchPred&>(*this);}
};

template<class PrefixRange, class MatchPred = TFEqual>
class StartsWith: TMatch<PrefixRange, MatchPred, decltype(First), decltype(PopFirst), TFLEqual>
{
public:
	using Parent = TMatch<PrefixRange, MatchPred, decltype(First), decltype(PopFirst), TFLEqual>;
	using Parent::Parent;
};
template<typename RPrefix, typename MatchPredicate>
StartsWith(RPrefix, MatchPredicate) -> StartsWith<TRangeOfRef<RPrefix>, TFunctorOf<MatchPredicate>>;

template<class SuffixRange, class MatchPred = TFEqual>
class EndsWith: TMatch<SuffixRange, MatchPred, decltype(Last), decltype(PopLast), TFLEqual>
{
public:
	using Parent = TMatch<SuffixRange, MatchPred, decltype(Last), decltype(PopLast), TFLEqual>;
	using Parent::Parent;
};
template<typename RSuffix, typename MatchPredicate>
EndsWith(RSuffix, MatchPredicate) -> EndsWith<TRangeOfRef<RSuffix>, TFunctorOf<MatchPredicate>>;

template<class Range1, class MatchPred = TFEqual>
class Equals: TMatch<Range1, MatchPred, decltype(First), decltype(PopFirst), TFEqual>
{
public:
	using Parent = TMatch<Range1, MatchPred, decltype(First), decltype(PopFirst), TFEqual>;
	using Parent::Parent;
};
template<typename R1, typename MatchPredicate>
Equals(R1, MatchPredicate) -> Equals<TRangeOfRef<R1>, TFunctorOf<MatchPredicate>>;

template<class Range2, class ComparePred = TFLess> class LexCompareTo: ComparePred
{
	Range2 mRange2;
	using T2 = TValueTypeOf<Range2>;
public:
	template<typename R2, typename = Requires<CAsForwardRange<R2>>>
	explicit constexpr LexCompareTo(R2&& r2): mRange2(ForwardAsRange<R2>(r2)) {}

	template<typename R2, typename ComparePredicate, typename = Requires<CAsForwardRange<R2>>>
	constexpr LexCompareTo(R2&& r2, ComparePredicate&& comparePredicate):
		ComparePred(ForwardAsFunc<ComparePredicate>(comparePredicate)),
		mRange2(ForwardAsRange<R2>(r2)) {}

	template<class R1, class = Requires<CAsConsumableRange<R1>>>
	[[nodiscard]] constexpr int operator()(R1&& r1) const
	{
		if constexpr(CArrayClass<R1> && CArrayClass<Range2> &&
			CSame<TArrayElement<R1>, T2> &&
			CByteByByteLexicographicallyComparable<T2> &&
			CSame<ComparePred, TFLess>)
		{
			return __builtin_memcmp(DataOf(r1), DataOf(mRange2),
				size_t(FMin(LengthOf(r1), LengthOf(mRange2)))*sizeof(T2));
		}
		else if constexpr(CSameNotVoid<TRawUnicodeUnit<R1>, TRawUnicodeUnit<Range2>> && CSame<MatchPred, TFLess>)
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
				if constexpr(CSame<ComparePred, TFLess> && CIntegral<T2>)
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
LexCompareTo(R2, ComparePredicate) -> LexCompareTo<TRangeOfRef<R2>, TFunctorOf<ComparePredicate>>;

INTRA_END
