#pragma once

#include "Intra/Functional.h"
#include "Intra/CompoundTypes.h"
#include "Intra/Concepts.h"

namespace Intra {
INTRA_BEGIN
INTRA_IGNORE_WARN_DEFAULT_CTOR_IMPLICITLY_DELETED
INTRA_IGNORE_WARN_ASSIGN_IMPLICITLY_DELETED

/// Non-owning reference to an array.
template<typename T> struct Span
{
	Span() = default;
	Span(const Span&) = default;

	template<CArrayList L> requires !CSameUnqualRef<L, Span>
	constexpr Span(L&& arr) noexcept: Span(Unsafe, DataOf(arr), LengthOf(arr)) {}

	constexpr Span(TUnsafe, T* begin, T* end) noexcept: Begin(begin), End(end) {INTRA_PRECONDITION(end >= begin);}
	constexpr Span(TUnsafe, T* begin, Size length) noexcept: Begin(begin), End(Begin + size_t(length)) {}

	[[nodiscard]] INTRA_FORCEINLINE constexpr index_t Length() const noexcept {return End - Begin;}
	[[nodiscard]] INTRA_FORCEINLINE constexpr bool Empty() const noexcept {return Begin >= End;}
	[[nodiscard]] INTRA_FORCEINLINE constexpr T* Data() const noexcept {return Begin;}

	[[nodiscard]] INTRA_FORCEINLINE constexpr T* begin() const noexcept {return Begin;}
	[[nodiscard]] INTRA_FORCEINLINE constexpr T* end() const noexcept {return End;}

	[[nodiscard]] constexpr T& First() const
	{
	    INTRA_PRECONDITION(!Empty());
	    return *Begin;
	}

	constexpr void PopFirst()
	{
	    INTRA_PRECONDITION(!Empty());
	    Begin++;
	}

	[[nodiscard]] constexpr T& Last() const
	{
	    INTRA_PRECONDITION(!Empty());
	    return End[-1];
	}

	constexpr void PopLast()
	{
	    INTRA_PRECONDITION(!Empty());
	    End--;
	}

	constexpr index_t PopFirstCount(ClampedSize count) noexcept
	{
		const auto poppedElements = Min(index_t(count), Length());
		Begin += poppedElements;
		return poppedElements;
	}

	constexpr index_t PopLastCount(ClampedSize count) noexcept
	{
		const auto poppedElements = Min(index_t(count), Length());
		End -= poppedElements;
		return poppedElements;
	}

	[[nodiscard]] constexpr Span Take(ClampedSize count) const noexcept
	{
	    return Span(Unsafe, Begin, Min(index_t(count), Length()));
	}

	[[nodiscard]] constexpr T& operator[](Index index) const
	{
		INTRA_PRECONDITION(index < Length());
		return Begin[size_t(index)];
	}

	T* Begin = nullptr;
	T* End = nullptr;
};
template<class R> Span(R&&) -> Span<TArrayElementKeepConst<R>>;

constexpr auto ConstSpanOf = []<CArrayList L>(L&& list) noexcept {return Span<const TArrayListValue<L>>(list);};

template<typename T> concept CSpan = CInstanceOfTemplate<TUnqualRef<T>, Span>;

inline namespace Literals {
[[nodiscard]] constexpr auto operator""_span(const char* str, size_t len) noexcept
{
	return Span(Unsafe, str, len);
}

[[nodiscard]] constexpr auto operator""_span(const wchar_t* str, size_t len) noexcept
{
	return Span(Unsafe, str, len);
}

[[nodiscard]] constexpr auto operator""_span(const char16_t* str, size_t len) noexcept
{
	return Span(Unsafe, str, len);
}

[[nodiscard]] constexpr auto operator""_span(const char32_t* str, size_t len) noexcept
{
	return Span(Unsafe, str, len);
}

#ifdef __cpp_char8_t
[[nodiscard]] constexpr auto operator""_span(const char8_t* str, size_t len) noexcept
{
	return Span(Unsafe, str, len);
}
#endif
}

/** This class is supposed to be used as an output range or a stream.

  Unlike Span, it remembers its original position,
  which allows to request the range of all written elements.
  It also can be considered as an input range of not yet written elements.
*/
template<typename T> class SpanOutput: private Span<T>
{
	T* mBegin = nullptr;
public:
	SpanOutput() = default;

	template<CArrayList R> requires CSame<TArrayListValue<R>, T>
	constexpr SpanOutput(R&& dst): Span<T>(dst), mBegin(Span<T>::Begin) {}

	/// Reset this range to its original state to overwrite all elements written earlier.
	constexpr void Reset() noexcept {Span<T>::Begin = mBegin;}

	/// Get a range of all written data.
	[[nodiscard]] constexpr Span<T> WrittenRange() const noexcept
	{
		return Span<T>(Unsafe, mBegin, Span<T>::Begin);
	}

	/// @return Number of elements written after last Reset() or construction.
	[[nodiscard]] constexpr index_t Position() const noexcept {return Span<T>::Begin - mBegin;}

	[[nodiscard]] constexpr bool IsInitialized() const noexcept {return mBegin != nullptr;}

	[[nodiscard]] constexpr bool Full() const {return Span<T>::Empty();}

	using Span<T>::Data;
	using Span<T>::Length;
	using Span<T>::PopFirst;
	using Span<T>::PopFirstCount;

	template<CMoveAssignable U> void Put(U&& value)
	{
		INTRA_PRECONDITION(!Full());
		*Span<T>::Begin++ = INTRA_FWD(value);
	}
};


template<typename T> struct LinkedNode
{
	LinkedNode* Next;
	[[no_unique_address]] T Value;

	[[nodiscard]] constexpr LinkedNode* NextListNode() const {return Next;}
};

template<typename T, typename NodeType> struct LinkedRange
{
	using Node = NodeType;

	LinkedRange() = default;

	constexpr LinkedRange(Node* first): FirstNode(first) {}

	[[nodiscard]] constexpr bool Empty() const {return FirstNode == nullptr;}

	constexpr void PopFirst()
	{
		INTRA_PRECONDITION(!Empty());
		FirstNode = FirstNode->NextListNode();
	}

	[[nodiscard]] constexpr T& First() const
	{
		INTRA_PRECONDITION(!Empty());
		if constexpr(CSame<T, Node>) return *FirstNode;
		else FirstNode->Value;
	}

	[[nodiscard]] constexpr bool operator==(const LinkedRange& rhs) const {return FirstNode == rhs.FirstNode;}

	Node* FirstNode = nullptr;
};


template<typename T> struct BidirectionalLinkedNode
{
	BidirectionalLinkedNode* Prev;
	BidirectionalLinkedNode* Next;
	T Value;

	[[nodiscard]] BidirectionalLinkedNode* PrevListNode() const {return Prev;}
	[[nodiscard]] BidirectionalLinkedNode* NextListNode() const {return Next;}
};

template<typename T, typename NodeType> struct BidirectionalLinkedRange: private LinkedRange<T, NodeType>
{
	using BASE = LinkedRange<T, NodeType>;
	using Node = NodeType;

	BidirectionalLinkedRange() = default;

	constexpr BidirectionalLinkedRange(Node* first, Node* last): BASE(first), LastNode(last) {}

	using BASE::Empty;
	using BASE::First;

	constexpr void PopFirst()
	{
		INTRA_PRECONDITION(!Empty());
		const bool last = BASE::FirstNode == LastNode;
		BASE::PopFirst();
		if(last) LastNode = BASE::FirstNode = nullptr;
	}

	constexpr void PopLast()
	{
		INTRA_PRECONDITION(!Empty());
		const bool last = BASE::FirstNode == LastNode;
		LastNode = LastNode->PrevListNode();
		if(last) LastNode = BASE::FirstNode = nullptr;
	}

	[[nodiscard]] constexpr T& Last() const
	{
		INTRA_PRECONDITION(!Empty());
		if constexpr(CSame<T, Node>) return *LastNode;
		else LastNode->Value;
	}

	[[nodiscard]] constexpr bool operator==(const BidirectionalLinkedRange& rhs) const
	{
		return BASE::FirstNode == rhs.FirstNode && LastNode == rhs.LastNode;
	}

private:
	Node* LastNode = nullptr;
};


constexpr auto ForEach = []<typename F>(F&& f) {
	return [f = ForwardAsFunc<F>(f)]<typename Collection>(Collection&& collection)
		requires(CConsumableList<Collection> || CStaticLengthContainer<Collection>)
	{
		if constexpr(CConsumableList<Collection>)
		{
			auto range = RangeOf(INTRA_FWD(collection));
			index_t index = 0;
			while(!range.Empty())
			{
				if constexpr(CCallable<decltype(f), TListValueRef<Collection>, index_t>)
					f(range.First(), index++);
				else f(range.First());
				range.PopFirst();
			}
		}
		else collection|ForEachField(f);
	};
};

constexpr auto First = []<CList L>(L&& list) -> decltype(auto) {
	if constexpr(CHasFirst<L>) return list.First();
	else return RangeOf(INTRA_FWD(list)).First();
};
constexpr auto Last = []<CBidirectionalList L>(L&& list) -> decltype(auto) {
	if constexpr(CHasLast<L>) return list.Last();
	else return RangeOf(INTRA_FWD(list)).Last();
};
constexpr auto IsEmpty = []<CHasEmpty R>(R&& range) noexcept {return range.Empty();};
constexpr auto Length = []<CHasLengthOf R>(R&& range) noexcept {return LengthOf(range);};

constexpr auto IsFull = []<class R>(R&& range) noexcept requires CHasFull<R> || CAssignableRange<R> {
	if constexpr(CHasFull<R>) return range.Full();
	else return range.Empty();
};

constexpr auto PopFirst = []<CHasPopFirst R>(R&& range) {range.PopFirst();};
constexpr auto PopLast = []<CHasPopLast R>(R&& range) {range.PopLast();};

constexpr auto PopFirstCount = [](ClampedSize maxElementsToPop) noexcept {
	return [maxElementsToPop]<CRange R>(R&& range) requires(!CConst<TRemoveReference<R>>)
	{
		if constexpr(CHasPopFirstCount<R>) return range.PopFirstCount(maxElementsToPop);
		else
		{
			size_t poppedElements = 0;
			while(!range.Empty())
			{
				if(poppedElements < maxElementsToPop) break;
				range.PopFirst();
				poppedElements++;
			}
			return poppedElements;
		}
	};
};

constexpr auto PopLastCount = [](ClampedSize maxElementsToPop) noexcept {
	return [maxElementsToPop]<CBidirectionalRange R>(R&& range) requires(!CConst<TRemoveReference<R>>)
	{
		if constexpr(CHasPopFirstCount<R>) return range.PopLastCount(maxElementsToPop);
		else
		{
			size_t poppedElements = 0;
			while(!range.Empty())
			{
				if(poppedElements < maxElementsToPop) break;
				range.PopLast();
				poppedElements++;
			}
			return poppedElements;
		}
	};
};

constexpr auto PopFirstAllCount = [](ClampedSize maxElementsToPop) noexcept {
	return [maxElementsToPop]<CRange R>(R&& range) requires(!CConst<TRemoveReference<R>>)
	{
		if constexpr(CHasPopFirstCount<R>) return range.PopFirstCount(MaxValueOf<size_t>);
		else
		{
			size_t poppedElements = 0;
			while(!range.Empty())
			{
				range.PopFirst();
				poppedElements++;
			}
			return poppedElements;
		}
	};
};

constexpr auto Next = []<CRange R>(R&& range) -> decltype(auto)
	requires CHasNext<R> || CCopyConstructible<TRangeValue<R>>
{
	if constexpr(CHasNext<R>) return range.Next();
	else
	{
		auto res = range.First();
		range.PopFirst();
		return res;
	}
};

constexpr auto PopFirstExactly = [](Size numElementsToPop) noexcept {
	return [numElementsToPop]<CRange R>(R&& range) requires(!CConst<TRemoveReference<R>>)
	{
		[[maybe_unused]] index_t poppedElements = range|PopFirstCount(numElementsToPop);
		INTRA_POSTCONDITION(poppedElements == numElementsToPop);
	};
};
constexpr auto PopLastExactly = [](Size numElementsToPop) noexcept {
	return [numElementsToPop]<CBidirectionalRange R>(R&& range) requires(!CConst<TRemoveReference<R>>)
	{
		[[maybe_unused]] index_t poppedElements = range|PopLastCount(numElementsToPop);
		INTRA_POSTCONDITION(poppedElements == numElementsToPop);
	};
};

constexpr auto PopWhile = [](auto pop, auto&& pred) noexcept {
	return [pred = FunctorOf(INTRA_FWD(pred))]<CRange R>(R&& range) requires
		CCallable<decltype(pop), R&&> &&
		CCallable<decltype(pred), TRangeValue<R>>
	{
		index_t poppedElements = 0;
		while(!range.Empty() && pred(range.First()))
		{
			pop(range);
			poppedElements++;
		}
		return poppedElements;
	};
};
constexpr auto PopFirstWhile = [](auto&& pred) noexcept {return PopWhile(PopFirst, FNot(INTRA_FWD(pred)));};
constexpr auto PopLastWhile = [](auto&& pred) noexcept {return PopWhile(PopLast, FNot(INTRA_FWD(pred)));};
constexpr auto PopFirstUntil = [](auto&& pred) noexcept {return PopFirstWhile(FNot(INTRA_FWD(pred)));};
constexpr auto PopLastUntil = [](auto&& pred) noexcept {return PopLastWhile(FNot(INTRA_FWD(pred)));};

constexpr auto Count = []<typename L>(L&& list) requires CHasLengthOf<L> || CConsumableList<L>
{
	if constexpr(CHasLengthOf<L>) return LengthOf(list);
	else return Dup(RangeOf(INTRA_FWD(list)))|PopFirstAllCount;
};

constexpr auto FunctionalRangeOp = [](auto inplaceModifyingOp) {
	return [inplaceModifyingOp]<CAccessibleList L>(L&& list) -> decltype(auto)
		requires CCallable<decltype(inplaceModifyingOp), TRangeOfRef<L>>
	{
		auto result = RangeOf(INTRA_FWD(list));
		inplaceModifyingOp(result);
		return result;
	};
};

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
constexpr auto DropUntil = [](auto&& pred) {
	return FunctionalRangeOp(PopFirstUntil(INTRA_FWD(pred)));
};
constexpr auto DropLastUntil = [](auto&& pred) {
	return FunctionalRangeOp(PopLastUntil(INTRA_FWD(pred)));
};

#if INTRA_CONSTEXPR_TEST
static_assert(CRange<Span<const int>>);
static_assert(CHasFirst<Span<const int>>);
static_assert(CHasPopFirst<Span<const int>>);
static_assert(CHasEmpty<Span<const int>>);
static_assert(CNonConstRValueReference<Span<const int>> || CCopyConstructible<TRemoveReference<Span<const int>>>);
static_assert(CAccessibleRange<Span<const int>>);
static_assert(Span<const int>()|DropExactly(ClampedSize(Index(0)))|IsEmpty);
static_assert(Drop(2).MaxElementsToPop == 2);
static_assert(CCallable<decltype(PopFirstCount(0)), Span<const int>>);
#endif

constexpr auto Contains = []<typename P>(P&& pred) {
	return [drop = DropUntil(INTRA_FWD(pred))](auto&& range) -> decltype(range|drop|FNot(IsEmpty)) {
		return range|drop|FNot(IsEmpty);
	};
};

constexpr auto Tail = [](ClampedSize maxLength) noexcept {
	return [maxLength]<CNonInfiniteForwardList L>(L&& list) {
		return list|Drop((list|Count) - maxLength);
	};
};

constexpr auto AtIndex = [](Index index) noexcept {
	return [index]<typename L>(L&& list) -> decltype(auto) requires CHasIndex<L> || CAccessibleList<L> {
		if constexpr(CHasIndex<L>) return list[At];
		else return INTRA_FWD(list)|DropExactly(At)|First;
	};
};

constexpr auto TryFirst = []<CList L>(L&& list)
{
	if(RangeOf(list).Empty()) return Undefined;
	return list|First;
};
constexpr auto TryLast = []<CBidirectionalList L>(L&& list)
{
	if(RangeOf(list).Empty()) return Undefined;
	return list|Last;
};
constexpr auto TryNext = []<CList L>(L&& list)
{
	if(RangeOf(list).Empty()) return Undefined;
	return list|Next;
};

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

constexpr auto TryPut = [](auto&& value) {
	return [value = INTRA_FWD(value)]<COutputOf<decltype(value)> O>(O&& out) {
		if constexpr(CHasTryPut<O, decltype(value)>) return out.TryPut(value);
		else
		{
			if constexpr(CHasFull<O>) if(out.Full()) return false;
			out|PutRef(Unsafe, value);
			return true;
		}
	};
};

constexpr auto PopWhileMatch = [](auto&& matchPred, auto get1, auto pop1, auto get2, auto pop2)
{
	return [=]<CRange R1, CRange R2>(R1&& range1, R2&& range2) ->
		decltype(matchPred(get1(range1), get2(range2)), index_t())
		requires CCallable<decltype(pop1), R1&&> && CCallable<decltype(pop2), R2&&>
	{
		index_t numPopped = 0;
		while(!range1.Empty() && !range2.Empty() &&
			matchPred(get1(range1), get2(range2)))
		{
			pop1(range1);
			pop2(range2);
			numPopped++;
		}
		return numPopped;
	};
};
constexpr auto PopFirstWhileEqual = PopWhileMatch(Equal, First, PopFirst, First, PopFirst);
constexpr auto PopLastWhileEqual = PopWhileMatch(Equal, Last, PopLast, Last, PopLast);
constexpr auto PopFirstUntilEqual = PopWhileMatch(NotEqual, First, PopFirst, First, PopFirst);
constexpr auto PopLastUntilEqual = PopWhileMatch(NotEqual, Last, PopLast, Last, PopLast);

namespace z_D {
constexpr auto GenericListsMatch = [](auto&& matchPred,
	auto get1, auto pop1, auto get2, auto pop2, auto prefixLengthPredicate) requires
	CAnyOf<decltype(get1), decltype(First), decltype(Last)> &&
	CAnyOf<decltype(get2), decltype(First), decltype(Last)> &&
	CAnyOf<decltype(pop1), decltype(PopFirst), decltype(PopLast)> &&
	CAnyOf<decltype(pop2), decltype(PopFirst), decltype(PopLast)> &&
	CSame<decltype(get1), decltype(First)> == CSame<decltype(pop1), decltype(PopFirst)> &&
	CSame<decltype(get2), decltype(First)> == CSame<decltype(pop2), decltype(PopFirst)> &&
	CAnyOf<decltype(prefixLengthPredicate), decltype(LEqual), decltype(Equal), decltype(GEqual)>
{
	return [=, matchPred = INTRA_FWD(matchPred)]<CList L1, CList L2>(L1&& list1, L2&& list2)
		requires(requires(PopWhileMatch(matchPred, get1, pop1, get2, pop2)(RangeOf(INTRA_FWD(list1)), RangeOf(INTRA_FWD(list2)))))
	{
		if constexpr(CSameArrays<L1, L2> && CTriviallyEqualComparable<TListValue<L1>> &&
			CSame<decltype(get1), decltype(get2)> &&
			CSame<TRemoveReference<decltype(matchPred)>, decltype(Equal)>)
			if(!IsConstantEvaluated())
			{
				if(!prefixLengthPredicate(LengthOf(list1), LengthOf(list2))) return false;
				auto data1 = DataOf(list1);
				auto data2 = DataOf(list2);
				if constexpr(CSame<TGet2, decltype(Last)>)
				{
					if constexpr(CSame<decltype(prefixLengthPredicate), decltype(LEqual)>)
						data2 += LengthOf(list2) - LengthOf(list1);
					else if constexpr(CSame<decltype(prefixLengthPredicate), decltype(GEqual)>)
						data1 += LengthOf(list1) - LengthOf(list2);
				}
				auto length = CSame<decltype(prefixLengthPredicate), decltype(GEqual)>? LengthOf(list2): LengthOf(list1);
				return __builtin_memcmp(data1, data2, length*sizeof(T1)) == 0;
			}
		if constexpr(CSameNotVoid<TRawUnicodeUnit<Range1>, TRawUnicodeUnit<L>>)
		{
			return list2.RawUnicodeUnits()|TMatch(mRange1.RawUnicodeUnits());
		}
		else
		{
			auto range1 = RangeOf(INTRA_FWD(list1));
			auto range2 = RangeOf(INTRA_FWD(list2));
			PopWhileMatch(matchPred, get1, pop1, get2, pop2)(range1, range2);
			if constexpr(CSame<decltype(prefixLengthPredicate), decltype(LEqual)>) return range1.Empty();
			else if constexpr(CSame<decltype(prefixLengthPredicate), decltype(GEqual)>) return range2.Empty();
			else return range1.Empty() && range2.Empty();
		}
	};
};
}

constexpr auto MatchesWith = []<CList L2>(L2&& list)
{
	return Bind(
		z_D::GenericListsMatch(Equal, First, PopFirst, First, PopFirst, Equal),
		INTRA_FWD(list));
};
constexpr auto StartsWith = []<CList L2>(L2&& prefixList)
{
	return Bind(
		z_D::GenericListsMatch(Equal, First, PopFirst, First, PopFirst, GEqual),
		INTRA_FWD(prefixList));
};
constexpr auto EndsWith = []<CList L2>(L2&& suffixList)
{
	return Bind(
		z_D::GenericListsMatch(Equal, Last, PopLast, Last, PopLast, GEqual),
		INTRA_FWD(suffixList));
};

template<class Range2, class ComparePred = decltype(Less)> class LexCompareTo
{
	Range2 mRange2;
	using T2 = TRangeValue<Range2>;
	[[no_unique_address]] ComparePred mComparePred;
public:
	template<CForwardList L> explicit constexpr LexCompareTo(L&& list2): mRange2(RangeOf(INTRA_FWD(list2))) {}

	template<CForwardList L, typename ComparePredicate>
	constexpr LexCompareTo(L&& list2, ComparePredicate&& comparePredicate):
		mComparePred(ForwardAsFunc<ComparePredicate>(comparePredicate)),
		mRange2(RangeOf(INTRA_FWD(list2))) {}

	template<CConsumableList L> [[nodiscard]] constexpr int operator()(L&& list1) const
	{
		if constexpr(CSameArrays<L, Range2> &&
			CByteByByteLexicographicallyComparable<T2> &&
			CAnyOf<ComparePred, decltype(Less), decltype(Greater)>)
			if(!IsConstantEvaluated()) //__builtin_memcmp doesn't work in constexpr with GCC with heap allocated memory
		{
			const auto res = __builtin_memcmp(DataOf(list1), DataOf(mRange2),
				size_t(Min(LengthOf(list1), LengthOf(mRange2)))*sizeof(T2));
			if(res == 0) res = Cmp(LengthOf(mRange2), LengthOf(list1));
			if constexpr(CSame<ComparePred, decltype(Greater)>) res = -res;
			return res;
		}

		if constexpr(CSameNotVoid<TRawUnicodeUnit<L>, TRawUnicodeUnit<Range2>> &&
			CAnyOf<ComparePred, decltype(Less), decltype(Greater)>)
		{
			return list1.RawUnicodeUnits()|LexCompareTo(mRange2.RawUnicodeUnits());
		}
		else
		{
			auto range1 = RangeOf(INTRA_FWD(list1));
			auto range2 = mRange2;
			for(;;)
			{
				if(range2.Empty()) return range1.Empty()? 1: 0;
				if(range1.Empty()) return -1;
				auto&& a = Next(range1);
				auto&& b = Next(range2);
				if(mComparePred(a, b)) return -1;
				if constexpr(CSame<ComparePred, decltype(Less)> && CBasicIntegral<T2>)
				{
					//Unlike b < a this helps most compilers to reuse the result of the previous cmp instruction
					if(a > b) return 1;
				}
				else if(mComparePred(b, a)) return 1;
			}
		}
	}
};
template<typename R2, typename ComparePredicate>
LexCompareTo(R2, ComparePredicate) -> LexCompareTo<TRangeOf<R2>, TFunctorOf<ComparePredicate>>;
template<typename R2> LexCompareTo(R2) -> LexCompareTo<TRangeOf<R2>>;


/**
  May be faster than WriteTo(src).ConsumeFrom(dst) for short arrays of trivial types
  when their length is not known at compile time.
*/
template<CRange R, class OR, typename P = decltype(Never)>
requires (!CConst<R>) && COutputOf<OR, TRangeValue<R>>
constexpr index_t ReadWriteByOne(R& src, OR& dst)
{
	index_t minLen = 0;
	while(!src.Empty())
	{
		if constexpr(CHasFull<OR> && !CInfiniteRange<OR>)
			if(dst.Full()) break;
		dst|Put(Next(src));
		minLen++;
	}
	return minLen;
}

template<class Dst> struct CopyTo
{
	Dst Dest;
	template<typename R> requires CAsOutputRange<R> || CAssignableList<R>
	constexpr CopyTo(R&& dst): Dest(RangeOf(INTRA_FWD(dst))) {}

	/** Consume the first Min(Count(src), Count(Dest)) of `src` and put them into `Dest`.
	  `Dest` and `src` may overlap only if `src` elements come after `Dest` elements
	  @return The number of copied elements.
	**/
	template<CRange Src> constexpr index_t ConsumeFrom(Src&& src)
	{
		static_assert(COutputOf<Dst, TRangeValue<Src>> || CAssignableRange<Dst>);
		static_assert(!CInfiniteRange<Dst> || !CInfiniteRange<Src>);
		if constexpr(CArrayList<Src> &&
			CTriviallyCopyable<TArrayListValue<Src>> &&
			CSame<TArrayListValue<Src>, TArrayListValue<Dst>>)
		{
			const auto minLen = Min(LengthOf(src), LengthOf(Dest));
			BitwiseCopy(Unsafe, DataOf(Dest), DataOf(src), minLen);
			Dest|PopFirstExactly(minLen);
			src|PopFirstExactly(minLen);
			return minLen;
		}
		else if constexpr(CHasReadWriteMethod<Src&, Dst&>) return src.ReadWrite(Dest);
		else if constexpr(CHasMethodPutAll<Dst&, Src&>) return Dest.PutAll(src);
		else return ReadWriteByOne(src, Dest);
	}

	/** Put the first Min(Count(src), Count(Dest)) of \p src into Dest.
	  `Dest` and `src` may overlap only if `src` elements come after Dest elements
	  @return The number of copied elements.
	*/
	template<CList Src> constexpr index_t From(Src&& src)
	{
		if constexpr(CRange<Src> && CRValueReference<Src&&>) return ConsumeFrom(src);
		else return ConsumeFrom(RangeOf(src));
	}

	template<CList Src> constexpr index_t operator()(Src&& src)
	{
		return From(INTRA_FWD(src));
	}
};
template<typename Dst> CopyTo(Dst&&) -> CopyTo<TRangeOf<Dst&&>>;

template<class Dst> requires COutput<Dst> || CAssignableList<Dst>
[[nodiscard]] constexpr auto WriteTo(Dst& dst) {return CopyTo<Dst&>(dst);}




template<class F, typename T> struct TReducer
{
	[[no_unique_address]] F Func;
	T Value;

	template<typename F1, typename T> constexpr TReducer(F1&& f, T&& seed):
		Func(ForwardAsFunc<F1>(f)), Value(INTRA_FWD(seed)) {}

	template<CConsumableList L> constexpr auto operator()(L&& list)
	{
		if constexpr(CRange<L> && !CConst<L>)
		{
			auto res = Move(Value);
			while(!range.Empty()) res = Func(res, Next(range));
			Value = Move(res);
			return Value;
		}
		else return operator()(Dup(ForwardAsRange<R>(range)));
	}
};
template<class F, typename T> TReducer(F, T) -> TReducer<TFunctorOf<F>, T>;

template<class F> struct Reduce
{
	[[no_unique_address]] F Func;

	template<typename F1> constexpr Reduce(F&& f): F(ForwardAsFunc<F1>(f)) {}

	template<CConsumableList L>	constexpr auto operator()(L&& list)
	{
		if constexpr(CRange<L> && !CConst<L>)
		{
			TReducer<F&, TResultOf<F, TRangeValue<L>, TRangeValue<L>>> reducer(Func, Next(range));
			return reducer(list);
		}
		else return operator()(Dup(ForwardAsRange<L>(list)));
	}
};


template<typename F> class Generate
{
	[[no_unique_address]] F mFunc;
	TResultOf<F> mFirst;
public:
	using TagAnyInstanceInfinite = TTag<>;

	constexpr Generate(F f): mFunc(INTRA_MOVE(f)), mFirst(mFunc()) {}
	[[nodiscard]] constexpr bool Empty() const {return false;}
	[[nodiscard]] constexpr const auto& First() const {return mFirst;}
	constexpr void PopFirst() {mFirst = mFunc();}
};

template<typename F, typename T, size_t N> class Recurrence
{
	[[no_unique_address]] TSelect<size_t, EmptyType, (N > 2)> mIndex = 0;
	[[no_unique_address]] F mFunc;
	Array<T, N> mState;
public:
	using TagAnyInstanceInfinite = TTag<>;

	template<typename... Ts, CCallable<Ts...> F1> requires (sizeof...(Ts) == N)
		constexpr Recurrence(F1&& function, Ts&&... initialSequence):
		mFunc(INTRA_FWD(function)),
		mState{INTRA_FWD(initialSequence)...} {}

	[[nodiscard]] constexpr T First() const
	{
		if constexpr(N <= 2) return mState[0];
		else return mState[mIndex];
	}
	constexpr void PopFirst()
	{
		if constexpr(N == 1) mState[0] = mFunc(mState[0]);
		else if constexpr(N == 2)
		{
			mState[0] = mFunc(mState[0], mState[1]);
			Swap(mState[0], mState[1]);
		}
		else [&]<size_t... Is>(TIndexSeq<Is...>)
		{
			auto& nextVal = mState[mIndex++];
			if(mIndex == N) mIndex = 0;
			nextVal = mFunc(mState[mIndex], mState[(mIndex+(1+Is) - (mIndex >= N-(1+Is)? N: 0))...]);
		}();
	}
	[[nodiscard]] constexpr bool Empty() const noexcept {return false;}
};
template<typename F, typename... Ts>
Recurrence(F&&, Ts&&...) -> Recurrence<TFunctorOf<F&&>, TCommon<Ts...>, sizeof...(Ts)>;

template<typename F> class Sequence
{
	[[no_unique_address]] F mFunc;
public:
	using TagAnyInstanceInfinite = TTag<>;

	Sequence() = default;
	template<CAsCallable<size_t> F1> constexpr Sequence(F1&& function, size_t offset = 0):
		mFunc(function), Offset(offset) {}

	[[nodiscard]] constexpr auto First() const {return mFunc(Offset);}
	constexpr void PopFirst() noexcept {Offset++;}
	[[nodiscard]] constexpr bool Empty() const noexcept {return false;}
	[[nodiscard]] constexpr auto operator[](size_t index) const {return mFunc(Offset + index);}

	[[nodiscard]] constexpr index_t PopFirstCount(ClampedSize maxElementsToPop) noexcept
	{
		Offset += size_t(maxElementsToPop);
		return index_t(maxElementsToPop);
	}

	size_t Offset = 0;
};
template<typename F> Sequence(F) -> Sequence<TFunctorOf<F>>;

constexpr auto Repeat = [](auto&& value) {
	return Sequence(FRepeat(INTRA_FWD(value)));
};

template<typename T> struct REmptyRange
{
	[[nodiscard]] constexpr bool Empty() const noexcept {return true;}
	[[nodiscard]] constexpr index_t Length() const noexcept {return 0;}
	[[nodiscard]] constexpr T First() const {INTRA_PRECONDITION(!Empty());}
	[[nodiscard]] constexpr T Last() const {INTRA_PRECONDITION(!Empty());}
	constexpr void PopFirst() const {INTRA_PRECONDITION(!Empty());}
	constexpr void PopLast() const {INTRA_PRECONDITION(!Empty());}
	constexpr T operator[](size_t) const {INTRA_PRECONDITION(!Empty());}
	[[nodiscard]] constexpr index_t PopFirstCount(ClampedSize) const noexcept {return 0;}
	constexpr T* Data() const noexcept {return nullptr;}
	constexpr REmptyRange Take(ClampedSize) {return *this;}
};
template<typename T> constexpr REmptyRange<T> EmptyRange;

constexpr struct {template<typename T> constexpr void Put(T&&) const {}} NullSink;

constexpr auto IotaInf = []<CNumber T = int, CNumber S = int>(T first = 0, S step = 1) {
	return Sequence([=](auto index) {return first + step*index;});
};

/** Range that counts all elements that are put into it.
Useful for example for counting result string length before conversion to it to avoid reallocation.
*/
template<typename T = int, typename CounterT = index_t> struct RCounter
{
	using TagAnyInstanceInfinite = TTag<>;

	[[nodiscard]] constexpr bool Empty() const noexcept {return false;}
	[[nodiscard]] constexpr T First() const noexcept(noexcept(T())) {return T();}
	constexpr void Put(const T&) noexcept {Counter++;}
	constexpr void PopFirst() noexcept {Counter++;}
	[[nodiscard]] constexpr index_t PopFirstCount(ClampedSize elementsToPop) const noexcept
	{
		Counter += CounterT(elementsToPop);
		return index_t(elementsToPop);
	}

	CounterT Counter = 0;
};
template<typename T = int, typename CounterT = index_t> struct RCeilCounter: RCounter<T, CounterT> {};
template<typename T> concept CCounter = CInstanceOfTemplate<TRemoveConstRef<T>, RCounter>;
template<typename T> concept CCeilCounter = CInstanceOfTemplate<TRemoveConstRef<T>, RCeilCounter>;




template<class T> concept CHasTakeMethod = requires(T x) {x.Take(index_t());};

template<CRange R, bool Exactly> struct RTake: CopyableIf<!CReference<R>>
{
	using TagAnyInstanceFinite = TTag<>;

	constexpr RTake() = default;

	template<CList L> constexpr RTake(L&& list, ClampedSize count):
		mOriginalRange(RangeOf(INTRA_FWD(list))), mLen(size_t(count))
	{
		if constexpr(CHasLength<R>)
		{
			if constexpr(Exactly) INTRA_PRECONDITION(count <= mOriginalRange.Length());
			else mLen = Min(mLen, size_t(mOriginalRange.Length()));
		}
	}

	/*RTake(RTake&&) = default;
	RTake(const RTake&) = default;
	RTake& operator=(RTake&&) = default;
	RTake& operator=(const RTake&) = default;*/

	[[nodiscard]] constexpr bool Empty() const
	{
		if constexpr(Exactly || CHasLength<R> || CInfiniteRange<R>)
			return mLen == 0;
		else return mLen == 0 || mOriginalRange.Empty();
	}


	[[nodiscard]] constexpr TRangeValueRef<R> First() const
	{
		INTRA_PRECONDITION(!Empty());
		return mOriginalRange.First();
	}

	constexpr void PopFirst()
	{
		INTRA_PRECONDITION(!Empty());
		mOriginalRange.PopFirst();
		mLen--;
	}
	
	[[nodiscard]] constexpr TRangeValueRef<R> Last() const requires CHasIndex<R>
	{
		INTRA_PRECONDITION(!Empty());
		return mOriginalRange[mLen-1];
	}

	constexpr void PopLast() requires CHasIndex<R>
	{
		INTRA_PRECONDITION(!Empty());
		mLen--;
	}

	[[nodiscard]] constexpr TRangeValueRef<R> operator[](Index index) const requires CHasIndex<R>
	{
		INTRA_PRECONDITION(size_t(index) < mLen);
		return mOriginalRange[index];
	}

	constexpr index_t Length() const noexcept requires CHasLength<R> || CInfiniteRange<R> {return index_t(mLen);}

	[[nodiscard]] constexpr index_t LengthLimit() const noexcept {return index_t(mLen);}
	
	[[nodiscard]] constexpr RTake Take(Size count) const
	{
		return RTake(mOriginalRange, Min(size_t(count), mLen));
	}

private:
	R mOriginalRange;
	size_t mLen = 0;
};

constexpr auto Take = [](ClampedSize maxCount) noexcept {
	return [maxCount]<typename L>(L&& list) requires(CList<L> || CAdvance<L>) {
		if constexpr(CAdvance<L>)
		{
			if constexpr(CHasTakeMethod<decltype(list.RangeRef)> && CHasPopFirstCount<decltype(list.RangeRef)>)
			{
				auto res = list.RangeRef.Take(maxCount);
				list.RangeRef.PopFirstCount(maxCount);
				return res;
			}
			else return RTake<decltype(list.RangeRef), false>(list.RangeRef, maxCount);
		}
		else if constexpr(CHasTakeMethod<TRangeOfRef<R>>) return RangeOf(INTRA_FWD(list)).Take(maxCount);
		else return RTake<TRangeOf<R>, false>(RangeOf(INTRA_FWD(list)), maxCount);
	};
};

constexpr auto TakeExactly = [](Size numElementsToTake) noexcept {
	return [numElementsToTake]<typename L>(L&& list) requires(CList<L> || CAdvance<L>) {
		if constexpr(CAdvance<R>)
		{
			if constexpr(CHasTakeMethod<decltype(list.RangeRef)> && CHasPopFirstCount<decltype(list.RangeRef)>)
			{
				auto res = list.RangeRef.Take(numElementsToTake);
				auto numElementsTaken = list.RangeRef.PopFirstCount(numElementsToTake);
				INTRA_DEBUG_ASSERT(numElementsTaken == numElementsToTake);
				return res;
			}
			else return RTake<decltype(list.RangeRef), true>(list.RangeRef, numElementsToTake);
		}
		else if constexpr(CHasTakeMethod<TRangeOfRef<R>>)
		{
			if constexpr(CHasLength<TRangeOf<R>>) INTRA_PRECONDITION(numElementsToTake <= RangeOf(list).Length());
			return RangeOf(INTRA_FWD(list)).Take(numElementsToTake);
		}
		else return RTake<TRangeOf<R>, true>(RangeOf(INTRA_FWD(list)), numElementsToTake);
	};
};

INTRA_DEFINE_SAFE_DECLTYPE(TTakeResult, Take(0)(Val<T>()));
INTRA_DEFINE_SAFE_DECLTYPE(TTakeExactlyResult, TakeExactly(0)(Val<T>()));


template<class R, class P, bool TestSubranges> class RTakeUntil
{
	R mRange;
	[[no_unique_address]] P mPred;
	bool mEmpty = false;
public:
	constexpr RTakeUntil(R range, P pred): mPred(Move(pred)), mRange(Move(range)) {}

	[[nodiscard]] constexpr decltype(auto) First() const
	{
		INTRA_PRECONDITION(!Empty());
		return mRange.First();
	}

	constexpr void PopFirst()
	{
		mRange.PopFirst();
		mEmpty = mRange.Empty();
		if(mEmpty) return;
		if constexpr(TestSubranges) mEmpty = mPred(mRange);
		else mEmpty = mPred(mRange.First());
	}

	[[nodiscard]] constexpr bool Empty() const noexcept {return mEmpty;}

	[[nodiscard]] constexpr decltype(auto) Next()
	{
		INTRA_PRECONDITION(!Empty());
		decltype(auto) res = mRange|Intra::Next;
		if constexpr(CCallable<P, TRangeValue<R>>)
			mEmpty = mRange.Empty() || mPred(res);
		return res;
	}
};

constexpr auto TakeUntil = [](auto&& pred) {
	return [pred = ForwardAsFunc<decltype(pred)>(pred)]<CList L>(L&& list) {
		return RTakeUntil(RangeOf(INTRA_FWD(list)), pred);
	};
};

constexpr auto TakeUntilEagerly = [](auto&& pred) {
	return [pred = ForwardAsFunc<decltype(pred)>(pred)]<CForwardList L>(L&& list) {
		return r|Take(r|TakeUntil(pred)|Count);
	};
};

INTRA_IGNORE_WARN_ASSIGN_IMPLICITLY_DELETED
template<typename R> struct RCycle
{
	static_assert(CForwardRange<R> && !CInfiniteRange<R>);
	using TagAnyInstanceInfinite = TTag<>;

	constexpr RCycle(R range) requires !CRandomAccessRange<R>:
		mOriginalRange(Move(range)), mOffsetRange(mOriginalRange) {INTRA_PRECONDITION(!mOriginalRange.Empty());}

	constexpr RCycle(R range) requires CRandomAccessRange<R>:
		mOriginalRange(Move(range)) {INTRA_PRECONDITION(!mOriginalRange.Empty());}

	[[nodiscard]] constexpr bool Empty() const {return false;}

	[[nodiscard]] constexpr decltype(auto) First() const
	{
		if constexpr(CRandomAccessRange<R>) return mOriginalRange[mOffsetRange.Counter];
		else return mOffsetRange.First();
	}

	constexpr void PopFirst()
	{
		mOffsetRange.PopFirst();
		if constexpr(CRandomAccessRange<R>)
		{
			if constexpr(CHasLength<R>)
				if(mOffsetRange.Counter == size_t(mOriginalRange.Length())) mOffsetRange.Counter = 0;
		}
		else if(mOffsetRange.Empty()) mOffsetRange = mOriginalRange;
	}

	constexpr index_t PopFirstCount(ClampedSize elementsToPop) const requires CRandomAccessRange<R> || CHasPopFirstCount<R> || CHasLength<R>
	{
		if constexpr(CRandomAccessRange<R>)
		{
			mOffsetRange.Counter += size_t(elementsToPop);
			mOffsetRange.Counter %= size_t(mOriginalRange.Length());
		}
		else
		{
			auto leftToPop = size_t(elementsToPop);
			if constexpr(CHasLength<R>)
			{
				leftToPop %= mOriginalRange.Length();
				const auto offLen = size_t(mOffsetRange.Length());
				if(offLen < leftToPop)
				{
					mOffsetRange = mOriginalRange;
					leftToPop -= offLen;
				}
				mOffsetRange|PopFirstExactly(leftToPop);
			}
			else while(leftToPop)
			{
				leftToPop -= mOffsetRange|Intra::PopFirstCount(leftToPop);
				if(mOffsetRange.Empty()) mOffsetRange = mOriginalRange;
			}
		}
		return index_t(elementsToPop);
	}

	[[nodiscard]] constexpr decltype(auto) operator[](Index index) const requires CRandomAccessRange<R>
	{
		size_t i = mOffsetRange.Counter + size_t(index);
		i %= size_t(mOriginalRange.Length());
		return mOriginalRange[i];
	}

private:
	R mOriginalRange;
	TSelect<RCounter<int, size_t>, R, CRandomAccessRange<R>> mOffsetRange;
};

constexpr auto Cycle = []<CList L>(L&& list) requires(CForwardList<L> || CInfiniteList<L>) {
	if constexpr(CInfiniteList<R>) return RangeOf(INTRA_FWD(list));
	else return RCycle(RangeOf(INTRA_FWD(list)));
};

template<typename R, typename F> struct RMap
{
	using TagAnyInstanceFinite = TTag<CFiniteRange<R>>;
	using TagAnyInstanceInfinite = TTag<CInfiniteRange<R>>;

	template<CList L, CAsCallable<TListValueRef<L>> F1>
	constexpr RMap(L&& list, F1&& f): Func(ForwardAsFunc<F1>(f)), OriginalRange(RangeOf(INTRA_FWD(list))) {}

	[[nodiscard]] constexpr decltype(auto) First() const {return Func(OriginalRange.First());}
	constexpr void PopFirst() {OriginalRange.PopFirst();}
	[[nodiscard]] constexpr bool Empty() const {return OriginalRange.Empty();}

	constexpr decltype(auto) Last() const requires CHasLast<R> {return Func(OriginalRange.Last());}
	constexpr void PopLast() requires CHaslPopLast<R> {OriginalRange.PopLast();}
	constexpr decltype(auto) operator[](size_t index) const {return Func(OriginalRange[index]);}
	[[nodiscard]] constexpr index_t Length() const requires CHasLength<R> {return OriginalRange.Length();}

	[[nodiscard]] constexpr auto PopFirstCount(ClampedSize numElementsToPop) requires CHasPopFirstCount<R>
	{
		return OriginalRange.PopFirstCount(numElementsToPop);
	}
	[[nodiscard]] constexpr auto PopLastCount(ClampedSize numElementsToPop) requires CHasPopLastCount<R>
	{
		return OriginalRange.PopLastCount(numElementsToPop);
	}

	[[no_unique_address]] F Func;
	R OriginalRange;
};

constexpr auto Map = []<typename F>(F&& f) {
	return [func = ForwardAsFunc<F>(f)]<CAccessibleList L>(L&& list) {
		return RMap<TRangeOf<L>, TFunctorOf<F>>(RangeOf(INTRA_FWD(list)), func);
	};
};

template<size_t N> constexpr auto Unzip = []<CList L>(L&& list) requires CStaticLengthContainer<L> {
	return INTRA_FWD(list)|Map(TFieldAt<N>);
};

INTRA_IGNORE_WARN_COPY_MOVE_IMPLICITLY_DELETED
template<typename R, class P> class RFilter
{
	using TagAnyInstanceFinite = TTag<CFiniteRange<R>>;
	using TagAnyInstanceInfinite = TTag<CInfiniteRange<R>>;

	template<CConsumableList L> constexpr RFilter(L&& list, P&& filterPredicate):
		mPred(ForwardAsFunc<P>(filterPredicate)), mOriginalRange(RangeOf(INTRA_FWD(list)))
	{skipFalsesFront(mOriginalRange, filterPredicate);}


	[[nodiscard]] constexpr decltype(auto) First() const
	{
		INTRA_PRECONDITION(!Empty());
		return mOriginalRange.First();
	}

	constexpr void PopFirst()
	{
		mOriginalRange.PopFirst();
		mOriginalRange|PopFirstUntil(mPred);
	}

	[[nodiscard]] constexpr decltype(auto) Last() const requires CBidirectionalRange<R>
	{
		INTRA_PRECONDITION(!Empty());
		auto&& b = mOriginalRange.Last();
		if(mPred(b)) return b;

		auto copy = mOriginalRange;
		copy.PopLast();
		copy|PopLastUntil(mPred);
		return copy.Last();
	}

	constexpr void PopLast() requires CBidirectionalRange<R>
	{
		mOriginalRange|PopLastUntil(mPred);
		mOriginalRange.PopLast();
	}

	[[nodiscard]] constexpr bool Empty() const {return mOriginalRange.Empty();}

private:
	R mOriginalRange;
	[[no_unique_address]] P mPred;
};

constexpr auto Filter = []<typename P>(P&& pred) {
	return [pred = ForwardAsFunc<P>(pred)]<CConsumableList L>(L&& list) {
		return RFilter<TRangeOf<L>, P>(INTRA_FWD(list), pred);
	};
};

template<typename R> struct RRetro
{
	R OriginalRange;

	[[nodiscard]] constexpr bool Empty() const {return OriginalRange.Empty();}
	[[nodiscard]] constexpr decltype(auto) First() const {return OriginalRange.Last();}
	constexpr void PopFirst() {OriginalRange.PopLast();}
	[[nodiscard]] constexpr decltype(auto) Last() const {return OriginalRange.First();}
	constexpr void PopLast() {OriginalRange.PopFirst();}
	
	[[nodiscard]] constexpr decltype(auto) operator[](Index index)
		requires CHasIndex<R> && CHasLength<R>
	{
		return OriginalRange[Length()-1-index];
	}

	[[nodiscard]] constexpr decltype(auto) operator[](Index index) const
		requires CHasIndex<R> && CHasLength<R>
	{
		return OriginalRange[Length()-1-index];
	}

	[[nodiscard]] constexpr auto Length() const requires CHasLength<R> {return OriginalRange.Length();}

	[[nodiscard]] constexpr index_t PopFirstCount(ClampedSize maxElementsToPop) requires CHasPopLastCount<R>
	{
		return OriginalRange.PopLastCount(maxElementsToPop);
	}

	[[nodiscard]] constexpr index_t PopLastCount(ClampedSize maxElementsToPop) requires CHasPopFirstCount<R>
	{
		return OriginalRange.PopFirstCount(maxElementsToPop);
	}
};

constexpr auto Retro = []<CBidirectionalList L>(L&& list) {
	if constexpr(CInstanceOfTemplate<TRemoveConstRef<L>, RRetro>) return list.OriginalRange;
	else return RRetro{RangeOf(INTRA_FWD(list))};
};

template<typename R> struct RChunks
{
	using TagAnyInstanceFinite = TTag<CFiniteRange<R>>;
	using TagAnyInstanceInfinite = TTag<CInfiniteRange<R>>;

	template<CList L> constexpr RChunks(L&& list, ClampedSize chunkLen):
		mOriginalRange(RangeOf(INTRA_FWD(list))), mChunkLen(chunkLen)
	{
		INTRA_PRECONDITION(chunkLen >= 1);
	}

	[[nodiscard]] auto First() const {return mOriginalRange|Take(mChunkLen);}
	void PopFirst() {mOriginalRange|PopFirstCount(mChunkLen);}
	[[nodiscard]] bool Empty() const {return mChunkLen == 0 || mOriginalRange.Empty();}

	[[nodiscard]] constexpr index_t Length() const requires CHasLength<R>
	{
		return (mOriginalRange.Length() + mChunkLen - 1) / mChunkLen;
	}

	[[nodiscard]] constexpr auto operator[](Index index) const requires CHasPopFirstCount<R> && CHasLength<R>
	{
		INTRA_PRECONDITION(index < Length());
		return mOriginalRange|Drop(index*mChunkLen)|Take(mChunkLen);
	}

	[[nodiscard]] constexpr auto Last() const requires CHasPopFirstCount<R> && CHasLength<R>
	{
		INTRA_PRECONDITION(!Empty());
		size_t numToTake = size_t(mOriginalRange.Length()) % mChunkLen;
		if(numToTake == 0) numToTake = mChunkLen;
		return mOriginalRange|Tail(numToTake);
	}

	constexpr void PopLast() requires CHasPopLast<R> && CHasLength<R>
	{
		INTRA_PRECONDITION(!Empty());
		size_t numToPop = size_t(mOriginalRange.Length()) % mChunkLen;
		if(numToPop == 0) numToPop = mChunkLen;
		mOriginalRange|PopLastExactly(numToPop);
	}

private:
	R mOriginalRange;
	size_t mChunkLen;
};

constexpr auto Chunks = [](ClampedSize chunkLen) {
	return [chunkLen]<CForwardList L>(L&& list) {
		return RChunks<TRangeOf<L>>>(RangeOf(INTRA_FWD(list)), chunkLen);
	};
};

INTRA_IGNORE_WARN_ASSIGN_IMPLICITLY_DELETED
template<typename R> struct RStride
{
	using TagAnyInstanceFinite = TTag<CFiniteRange<R>>;
	using TagAnyInstanceInfinite = TTag<CInfiniteRange<R>>;

	RStride() = default;

	constexpr RStride(R range, Size strideStep):
		mOriginalRange(Move(range)), mStep(strideStep)
	{
		INTRA_PRECONDITION(strideStep > 0);
		if constexpr(CHasPopLast<R> && CHasLength<R>)
		{
			const size_t len = size_t(mOriginalRange.Length());
			if(len == 0) return;
			mOriginalRange|PopLastExactly((len-1) % mStep);
		}
	}

	[[nodiscard]] constexpr decltype(auto) First() const {return mOriginalRange.First();}
	constexpr void PopFirst() {mOriginalRange|Intra::PopFirstCount(mStep);}
	[[nodiscard]] constexpr bool Empty() const {return mOriginalRange.Empty();}

	[[nodiscard]] constexpr decltype(auto) Last() const requires CHasLast<R> {return mOriginalRange.Last();}
	constexpr void PopLast() requires CHasPopLast<R> {mOriginalRange|Intra::PopLastCount(mStep);}
	[[nodiscard]] constexpr decltype(auto) operator[](Index index) const requires CHasIndex<R> {return mOriginalRange[index*mStep];}

	[[nodiscard]] constexpr index_t PopFirstCount(ClampedSize maxElementsToPop) requires CHasPopFirstCount<R>
	{
		const auto originalElementsPopped = mOriginalRange.PopFirstCount(mStep*size_t(maxElementsToPop));
		return index_t((size_t(originalElementsPopped) + mStep - 1) / mStep);
	}

	[[nodiscard]] constexpr index_t PopLastCount(ClampedSize maxElementsToPop) requires CHasPopLastCount<R>
	{
		const auto originalElementsPopped = mOriginalRange.PopLastCount(mStep*size_t(maxElementsToPop));
		return index_t((size_t(originalElementsPopped) + mStep - 1) / mStep);
	}

	[[nodiscard]] constexpr index_t Length() const requires CHasLength<R>
	{
		return index_t((size_t(mOriginalRange.Length()) + mStep - 1) / mStep);
	}

	[[nodiscard]] constexpr auto Stride(Size step) const {return RStride{mOriginalRange, mStep*size_t(step)};}

private:
	R mOriginalRange;
	size_t mStep = 0;
};

constexpr auto Stride = [](Size step) {
	return [step]<CAccessibleList L>(L&& list) {
		if constexpr(CInstanceOfTemplate<TRemoveConstRef<L>, RStride>) return list.Stride(step);
		else return RStride<TRangeOf<L>>(RangeOf(INTRA_FWD(list)), step);
	};
};

template<class RangeOfLists> class RJoin
{
	using R = TRangeOf<TRangeValueRef<RangeOfLists>>;

	RangeOfLists mLists;
	R mCurrentRange;
	[[no_unique_address]] TSelect<bool, EmptyType, CInfiniteRange<R>> mCreatedEmpty;
public:
	using TagAnyInstanceFinite = TTag<CFiniteRange<RangeOfLists> && CFiniteRange<R>>;
	using TagAnyInstanceInfinite = TTag<CInfiniteRange<RangeOfLists>>;

	template<CAccessibleList L> requires CAccessibleRange<TRangeValue<RangeOfLists>>
	constexpr RJoin(L&& listOfLists):
		mLists(RangeOf(INTRA_FWD(listOfLists))),
		mCreatedEmpty(mLists.Empty())
	{
		goToNearestNonEmptyElement();
	}

	constexpr decltype(auto) First() const
	{
		INTRA_PRECONDITION(!Empty());
		return mCurrentRange.First();
	}
	constexpr void PopFirst()
	{
		INTRA_PRECONDITION(!Empty());
		mCurrentRange.PopFirst();
		goToNearestNonEmptyElement();
	}
	[[nodiscard]] constexpr bool Empty() const
	{
		if constexpr(CInfiniteRange<R>) return mCreatedEmpty;
		else return mCurrentRange.Empty();
	}

	[[nodiscard]] constexpr decltype(auto) operator[](Index index) const
		requires CHasIndex<R> && (CInfiniteRange<R> || CHasLength<R> && CCopyConstructible<RangeOfLists>)
	{
		INTRA_PRECONDITION(!Empty());
		if constexpr(CInfiniteRange<R>) return mCurrentRange[index];
		else
		{
			INTRA_PRECONDITION(index < Length());
			size_t curIndex = size_t(index);
			size_t curLen = size_t(mCurrentRange.Length());
			if(curIndex < curLen) return mCurrentRange[curIndex];
			curIndex -= curLen;
			auto rr = mLists;
			for(;;)
			{
				auto range = RangeOf(rr.First());
				curLen = range.Length();
				if(curIndex < curLen) return range[curIndex];
				curIndex -= curLen;
				rr.PopFirst();
			}
		}
	}

	[[nodiscard]] constexpr index_t Length() const
		requires CHasLength<R> && CCopyConstructible<RangeOfLists>
	{
		auto result = mCurrentRange.Length();
		for(auto&& range: mLists) result += range|Intra::Length;
		return result;
	}

private:
	constexpr void goToNearestNonEmptyElement()
	{
		while(mCurrentRange.Empty() && !mLists.Empty())
		{
			mCurrentRange = RangeOf(mLists.First());
			mLists.PopFirst();
		}
	}
};
constexpr auto Join = []<CList ListOfLists>(ListOfLists&& lists) {
	if constexpr(CInfiniteList<ListsOfLists> && CInfiniteList<TListValue<ListOfLists>>)
	return RJoin<TRangeOf<ListOfLists>>(INTRA_FWD(lists));
};


template<CAccessibleRange R, CAssignableList B> class RBuffered
{
	static_assert(CSame<TRangeValueRef<R>, TListValueRef<B>>);
public:
	template<CAccessibleList L, CAssignableList LBuf> constexpr RBuffered(L&& list, LBuf&& buffer):
		mOriginalRange(RangeOf(INTRA_FWD(list))), mBuffer(INTRA_FWD(buffer)) {loadBuffer();}

	//External buffer referred by range cannot be copied
	RBuffered(const RBuffered& rhs) requires(!CRange<B>) = default;
	RBuffered& operator=(const RBuffered& rhs) requires(!CRange<B>) = default;
	RBuffered(RBuffered&& rhs) = default;
	RBuffered& operator=(RBuffered&& rhs) = default;


	[[nodiscard]] constexpr bool Empty() const
	{
		INTRA_POSTCONDITION(!mBufferRange.Empty() || mOriginalRange.Empty());
		return mBufferRange.Empty();
	}
	[[nodiscard]] constexpr auto First() const {return mBufferRange.First();}

	constexpr void PopFirst()
	{
		mBufferRange.PopFirst();
		if(!mBufferRange.Empty()) return;
		loadBuffer();
	}

	constexpr void PopFirstCount(ClampedSize n)
	{
		const auto leftToPop = size_t(n) - size_t(mBufferRange|Intra::PopFirstCount(n));
		if(!mBufferRange.Empty()) return;
		mOriginalRange|Intra::PopFirstCount(leftToPop);
		loadBuffer();
	}

	template<COutputOf<TRangeValue<R>> Dst> constexpr void ReadWrite(Dst&& dst)
	{
		if constexpr(CSameArrays<R, Dst>)
		{
			Advance(mBufferRange)|WriteTo(dst);
			Advance(mOriginalRange)|WriteTo(dst);
			if(mBufferRange.Empty()) loadBuffer();
		}
		else while(!Empty())
		{
			Advance(mBufferRange)|WriteTo(dst);
			if(mBufferRange.Empty()) loadBuffer();
			else break;
		}
	}

	[[nodiscard]] constexpr decltype(auto) operator[](Index index) requires CHasIndex<R>
	{
		if(size_t(index) < size_t(mBufferRange.Length())) return mBufferRange[index];
		return mOriginalRange[size_t(index) - size_t(mBufferRange.Length())];
	}

private:
	void loadBuffer()
	{
		INTRA_PRECONDITION(mBufferRange.Empty());
		mBufferRange = mBuffer|Take(Advance(mOriginalRange)|CopyTo(mBuffer));
	}

	R mOriginalRange;
	B mBuffer;
	TTakeResult<B> mBufferRange;
};
template<CAccessibleList L, CAssignableList LBuf> RBuffered(L&&, LBuf&&) -> RBuffered<TRangeOf<L>, TUnqualRef<LBuf>>;

constexpr auto Buffered = []<CAssignableList B>(B&& buffer) {
	return [buffer = INTRA_FWD(buffer)]<CAccessibleList L>(L&& list) {
		return RBuffered<TRangeOf<L>, decltype(buffer)>(INTRA_FWD(list), buffer);
	};
};

constexpr auto Iota = []<typename T>(T begin, T end, auto step = 1) {
	return IotaInf(begin, step)|Take((end - begin + step - 1) / step);
};

#if INTRA_CONSTEXPR_TEST
static_assert(CRandomAccessRange<decltype(Iota(1, 2, 3))>);
#endif


template<class... Rs> class Chain
{
	Tuple<Rs...> mRanges;
	size_t mIndex = 0;
	[[no_unique_address]] TConditionalField<size_t, (CBidirectionalRange<Rs> && ...)> mLastIndex;
public:
	using TagAnyInstanceFinite = TTag<(CFiniteRange<Rs> && ...)>;
	using TagAnyInstanceInfinite = TTag<(CInfiniteRange<Rs> || ...)>;

	template<CAccessibleList... Ls> constexpr Chain(Ls&&... lists): mRanges(ForwardAsRange<Ls>(lists)...) {}

	[[nodiscard]] constexpr decltype(auto) First() const {return ForFieldAtRuntime(mIndex, Intra::First)(mRanges);}
	constexpr void PopFirst()
	{
		INTRA_PRECONDITION(!Empty());
		ForFieldAtRuntime(mIndex, Intra::PopFirst)(mRanges);
		if(ForFieldAtRuntime(mIndex, Intra::IsEmpty)(mRanges)) mIndex++;
	}
	[[nodiscard]] constexpr bool Empty() const
	{
		if constexpr((CBidirectionalRange<Rs> && ...))
			return mIndex > mLastIndex || mIndex == mLastIndex && ForFieldAtRuntime(mIndex, Intra::IsEmpty)(mRanges);
		else return mIndex >= sizeof...(Rs);
	}

	[[nodiscard]] constexpr decltype(auto) Last() const requires (CBidirectionalRange<Rs> && ...)
	{
		return ForFieldAtRuntime(mLastIndex, Intra::Last)(mRanges);
	}

	constexpr void PopLast() requires (CBidirectionalRange<Rs> && ...)
	{
		INTRA_PRECONDITION(!Empty());
		ForFieldAtRuntime(mLastIndex, Intra::PopLast)(mRanges);
		if(ForFieldAtRuntime(mLastIndex, Intra::IsEmpty)(mRanges)) mLastIndex--;
	}

	[[nodiscard]] constexpr index_t Length() const requires (CHasLength<Rs> && ...)
	{
		auto acc = Accum(Add, Count, 0);
		ForEach(FRef(acc))(mRanges);
		return acc.Result;
	}

	[[nodiscard]] constexpr decltype(auto) operator[](Index index) const requires ((CHasIndex<Rs> && CHasLength<Rs>) && ...)
	{
		INTRA_PRECONDITION(index < Length());
		const auto getByIndex = [&]<size_t I>(auto& getByIndex, size_t index, TIndex<I>) -> decltype(auto) {
			auto& range = At<I>(mRanges);
			if constexpr(I == sizeof...(Rs) - 1) return range[index];
			else
			{
				auto len = size_t(range.Length());
				if(index < len) return range[index];
				else return getByIndex(getByIndex, index - len, TIndex<I+1>());
			}
		};
		return getByIndex(getByIndex, index, TIndex<0>());
	}

	constexpr index_t PopFirstCount(ClampedSize maxElementsToPop) const requires (CHasPopFirstCount<Rs> && ...)
	{
		size_t elemsLeftToPop = maxElementsToPop;
		ForEach([&](auto& range) {
			elemsLeftToPop -= range.PopFirstCount(elemsLeftToPop);
		})(mRanges);
		return index_t(maxElementsToPop - elemsLeftToPop);
	}

	constexpr index_t PopLastCount(ClampedSize maxElementsToPop) const requires (CHasPopLastCount<Rs> && ...)
	{
		size_t elemsLeftToPop = maxElementsToPop;
		ForEach([&](auto& range) {
			elemsLeftToPop -= range.PopLastCount(elemsLeftToPop);
		})(mRanges);
		return index_t(maxElementsToPop - elemsLeftToPop);
	}
};
template<CList... Ls> Chain(Ls&&...) -> Chain<TRangeOf<Ls&&>...>;

template<class... Rs> struct RoundRobin
{
	static_assert(sizeof...(Rs) != 0);
	using TagAnyInstanceInfinite = TTag<(CInfiniteRange<Rs> || ...)>;
	using TagAnyInstanceFinite = TTag<(CFiniteRange<Rs> && ...)>;

	RoundRobin() = default;
	template<CList... Ls> constexpr RoundRobin(Ls&&... lists): mRanges(ForwardAsRange<Rs>(lists)...)
	{
		while(mCurrent < sizeof...(Rs) && ForFieldAtRuntime(mCurrent, IsEmpty)(mRanges))
			mCurrent++;
	}

	constexpr decltype(auto) First() const
	{
		INTRA_PRECONDITION(!Empty());
		return ForFieldAtRuntime(mCurrent, Intra::First)(mRanges);
	}

	constexpr void PopFirst()
	{
		INTRA_PRECONDITION(!Empty());
		ForFieldAtRuntime(mCurrent, Intra::PopFirst)(mRanges);
		mCurrent++;
		mCurrent %= sizeof...(Rs);
		auto startIndex = mCurrent;
		while(ForFieldAtRuntime(mCurrent, IsEmpty)(mRanges))
		{
			mCurrent++;
			mCurrent %= sizeof...(Rs);
			if(mCurrent == startIndex)
			{
				mCurrent = sizeof...(Rs);
				return;
			}
		}
	}

	[[nodiscard]] constexpr bool Empty() const {return mCurrent == sizeof...(Rs);}

	[[nodiscard]] constexpr index_t Length() const requires (CHasLength<Rs> && ...)
	{
		auto accum = Accum(Add, Count, index_t(0));
		ForEachField(FRef(accum))(mRanges);
		return accum.Result;
	}

//private:
	Tuple<Rs...> mRanges;
	size_t mCurrent = 0;
};
template<CList... Ls> RoundRobin(Ls&&...) -> RoundRobin<TRangeOf<Ls&&>...>;

#if INTRA_CONSTEXPR_TEST
static_assert(CRange<TUnqualRef<Chain<Span<int>>>>);
static_assert(CRange<TUnqualRef<decltype(Chain(Span<int>()))>>);
static_assert(CRange<TUnqualRef<decltype(Chain(Array{1, 2, 3, 4}))>>);
static_assert(CSame<int, TUnqualRef<decltype(Chain(Array{1, 2, 3, 4}, Array{1, 2, 3, 4}).First())>>);
static_assert(CRandomAccessRange<decltype(Chain(Array{1, 2, 3, 4}))>);
static_assert(!CArrayList<const Intra::Tuple<Intra::Span<int>>&>);
static_assert(CSame<RoundRobin<Span<int>, Span<int>>, decltype(RoundRobin<Span<int>, Span<int>>(Span<int>(), Span<int>()))>);
static_assert(Array{1, 2, 3}|MatchesWith(Array{1, 2, 3}));
static_assert(!RoundRobin(Array{1}).Empty());
static_assert(RoundRobin(Array{1}).Length() == 1);
static_assert(RoundRobin(Array{1}).First() == 1);
static_assert(RoundRobin(Array{1})|MatchesWith(Array{1}));
static_assert(RoundRobin(Array{1}, Array{10})|MatchesWith(Array{1, 10}));
static_assert(RoundRobin(Array{1, 2}, Array{10})|MatchesWith(Array{1, 10, 2}));
static_assert(RoundRobin(Array{1, 2, 3}, Array{10, 20, 30, 40})|MatchesWith(Array{1, 10, 2, 20, 3, 30, 40}));
static_assert(RoundRobin(Array{0}, Array{10u, 20u, 30u, 40u})|MatchesWith(Array{0u, 10u, 20u, 30u, 40u}));
#endif

template<class LengthPolicy = decltype(Min), CRange... Rs>
class Zip: CopyableIf<!(CReference<Rs> || ...)>
{
	static_assert(CAnyOf<LengthPolicy,
		decltype(Min), //Length = Min(Rs::Length...)
		decltype(Equal), //Require all Rs::Length to be equal
		decltype(Max) //Length = Max(Rs::Length...), empty range elements are default constructed
	>);
	static_assert(!CSame<LengthPolicy, decltype(Max)> || (CConstructible<TRangeValueRef<Rs>> && ...));
	Tuple<Rs...> mRanges;
	static constexpr bool cacheLengthInConstructor = sizeof...(Rs) != 0 && !CSame<LengthPolicy, decltype(Equal)> && ((CHasLength<Rs> && !CReference<Rs>) && ...);
	[[no_unique_address]] TConditionalField<size_t, cacheLengthInConstructor> mLen;

	static constexpr size_t IndexOfFirstRangeWithLength = IndexOfFirstTrueArg<0>(CHasLength<Rs>...);
public:
	using TagAnyInstanceFinite = TTag<(CFiniteRange<Rs> && ...) ||
		CSame<LengthPolicy, decltype(Min)> && (CFiniteRange<Rs> || ...)>;
	using TagAnyInstanceInfinite = TTag<(CInfiniteRange<Rs> && ...) ||
		CSame<LengthPolicy, decltype(Max)> && (CInfiniteRange<Rs> || ...)>;

	static_assert(!(CSame<LengthPolicy, decltype(Equal)> &&
		(CFiniteRange<Rs> || ...) && (CInfiniteRange<Rs> || ...)),
		"All ranges are required to have the same length but Rs contains both finite and infinite ranges!");

	template<CAccessibleList... Ls> [[nodiscard]] constexpr Zip(Rs&&... ranges):
		mRanges(ForwardAsRange<Rs>(ranges)...)
	{
		if constexpr(cacheLengthInConstructor)
		{
			mLen = mRanges|ForEachFieldToArray(Count)|ApplyPackedArgs(LengthPolicy());
		}

		//TODO: implement Min and Max policies
		if constexpr(CSame<LengthPolicy, decltype(Equal)>)
		{
			if(IsConstantEvaluated() || Config::DebugCheckLevel >= 1)
			{
				//Try to check the requirement early, at construction
				if constexpr((CFiniteRange<Rs> && !CReference<Rs>) || ...)
				{
					bool allRangeLengthsAreNotEqual = false;
					if constexpr(CHasLength<Rs> && ...)
					{
						index_t length = -1;
						mRanges|ForEach([&](auto& range) {
							auto len = range.Length();
							if(length == -1) length = len;
							if(len != length) allRangeLengthsAreNotEqual = true;
						});
					}
					else if constexpr(Config::DebugCheckLevel >= 2 && CCopyConstructible<Zip>)
					{
						auto copy = *this;
						for(;;)
						{
							auto anyEmpty = AccumAny(IsEmpty);
							auto allEmpty = AccumAll(IsEmpty);
							copy.mRanges|ForEach(anyEmpty);
							copy.mRanges|ForEach(allEmpty);
							if(!anyEmpty.Result) continue;
							if(anyEmpty.Result != allEmpty.Result) allRangeLengthsAreNotEqual = true;
							break;
						}
					}
					INTRA_PRECONDITION(!allRangeLengthsAreNotEqual);
				}
			}
		}
	}

	[[nodiscard]] constexpr auto First() const
	{
		if constexpr(CSame<LengthPolicy, decltype(Max)>) return mRanges|ForEach([](auto&));
		else return mRanges|ForEach(Intra::First);
	}
	constexpr void PopFirst() {mRanges|ForEach(Intra::PopFirst);}
	[[nodiscard]] constexpr bool Empty() const
	{
		return At<0>(mRanges).Empty();
	}

	[[nodiscard]] constexpr auto Last() const requires(CHasLast<Rs> && ...)
	{return OriginalRanges|ForEach(Intra::Last);}
	constexpr void PopLast() {mRanges|ForEach(Intra::PopLast);}


	[[nodiscard]] constexpr index_t Length() const requires CHasLength<TPackFirst<Rs...>>
	{
		return At<0>(mRanges).Length();
	}

	[[nodiscard]] constexpr auto operator[](Index index) const
	{return mRanges|ForEach(AtIndex(index));}

	[[nodiscard]] constexpr index_t PopFirstCount(ClampedSize maxElementsToPop)
	{return At<0>(mRanges|ForEach(Intra::PopFirstCount(maxElementsToPop)));}
};

} INTRA_END
