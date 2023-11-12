#pragma once

#include <Intra/Concepts.h>
#include <Intra/Functional.h>
#include <Intra/Container/Compound.h>

namespace Intra { INTRA_BEGIN
INTRA_IGNORE_WARN_DEFAULT_CTOR_IMPLICITLY_DELETED
INTRA_IGNORE_WARN_ASSIGN_IMPLICITLY_DELETED

struct LengthInfo
{
	enum DimType: uint64 {HasKnownLength = MaxValueOf<uint64> - 3, Finite, Unknown, Infinite};
	[[nodiscard]] INTRA_FORCEINLINE constexpr DimType Type() const {return Max(DimType(RawValue), HasKnownLength);}
	[[nodiscard]] INTRA_FORCEINLINE constexpr uint64 Value() const {INTRA_PRECONDITION(Type() == DimType::HasKnownLength); return RawValue;}
	[[nodiscard]] INTRA_FORCEINLINE constexpr bool IsKnown() const {return RawValue <= uint64(HasKnownLength);}

	[[nodiscard]] INTRA_FORCEINLINE constexpr LengthInfo(uint64 x): RawValue(x) {}
	[[nodiscard]] INTRA_FORCEINLINE constexpr LengthInfo(DimType type): RawValue(uint64(type)) {}

	uint64 RawValue;
};

/// Non-owning reference to an array.
template<typename T> struct Span
{
	Span() = default;
	Span(const Span&) = default;

	template<CConvertibleToSpan L> requires (!CSameUnqualRef<L, Span>)
	INTRA_FORCEINLINE constexpr Span(L&& arr) noexcept: Span(Unsafe, Intra::Data(arr), Intra::Length(arr)) {}

	constexpr Span(TUnsafe, T* begin, T* end) noexcept: Begin(begin), End(end) {INTRA_PRECONDITION(end >= begin);}
	INTRA_FORCEINLINE constexpr Span(TUnsafe, T* begin, Size length) noexcept: Begin(begin), End(Begin + size_t(length)) {}

	[[nodiscard]] INTRA_FORCEINLINE constexpr T* Data() const noexcept {return Begin;}
	[[nodiscard]] INTRA_FORCEINLINE constexpr index_t Length() const noexcept {return End - Begin;}
	[[nodiscard]] INTRA_FORCEINLINE constexpr bool Empty() const noexcept {return Begin >= End;}

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

	INTRA_FORCEINLINE constexpr index_t PopFirstCount(ClampedSize count) noexcept
	{
		const auto poppedElements = Min(index_t(count), Length());
		Begin += poppedElements;
		return poppedElements;
	}

	INTRA_FORCEINLINE constexpr index_t PopLastCount(ClampedSize count) noexcept
	{
		const auto poppedElements = Min(index_t(count), Length());
		End -= poppedElements;
		return poppedElements;
	}

	[[nodiscard]] INTRA_FORCEINLINE constexpr Span Take(ClampedSize count) const noexcept
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

INTRA_DEFINE_FUNCTOR(ConstSpanOf)<CConvertibleToSpan L>(L&& list) noexcept {return Span<const TArrayListValue<L>>(list);};

template<typename T> concept CSpan = CInstanceOfTemplate<TUnqualRef<T>, Span>;

inline namespace Literals {
[[nodiscard]] INTRA_FORCEINLINE constexpr auto operator""_span(const char* str, size_t len) noexcept {return Span(Unsafe, str, len);}
[[nodiscard]] INTRA_FORCEINLINE constexpr auto operator""_span(const wchar_t* str, size_t len) noexcept {return Span(Unsafe, str, len);}
[[nodiscard]] INTRA_FORCEINLINE constexpr auto operator""_span(const char16_t* str, size_t len) noexcept {return Span(Unsafe, str, len);}
[[nodiscard]] INTRA_FORCEINLINE constexpr auto operator""_span(const char32_t* str, size_t len) noexcept {return Span(Unsafe, str, len);}
#ifdef __cpp_char8_t
[[nodiscard]] INTRA_FORCEINLINE constexpr auto operator""_span(const char8_t* str, size_t len) noexcept {return Span(Unsafe, str, len);}
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

	template<CConvertibleToSpan R> requires CSame<TArrayListValue<R>, T>
	INTRA_FORCEINLINE constexpr SpanOutput(R&& dst): Span<T>(dst), mBegin(Span<T>::Begin) {}

	/// Reset this range to its original state to overwrite all elements written earlier.
	INTRA_FORCEINLINE constexpr void Reset() noexcept {Span<T>::Begin = mBegin;}

	/// Get a range of all written data.
	[[nodiscard]] INTRA_FORCEINLINE constexpr Span<T> WrittenRange() const noexcept
	{
		return Span<T>(Unsafe, mBegin, Span<T>::Begin);
	}

	/// @return Number of elements written after last Reset() or construction.
	[[nodiscard]] INTRA_FORCEINLINE constexpr index_t Position() const noexcept {return Span<T>::Begin - mBegin;}

	[[nodiscard]] INTRA_FORCEINLINE constexpr bool IsInitialized() const noexcept {return mBegin != nullptr;}

	[[nodiscard]] INTRA_FORCEINLINE constexpr bool Full() const {return Span<T>::Empty();}

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
	INTRA_NO_UNIQUE_ADDRESS T Value;

	[[nodiscard]] INTRA_FORCEINLINE constexpr LinkedNode* NextListNode() const {return Next;}
};

template<typename T, typename NodeType> struct LinkedRange
{
	using Node = NodeType;

	LinkedRange() = default;

	INTRA_FORCEINLINE constexpr LinkedRange(Node* first): FirstNode(first) {}

	[[nodiscard]] INTRA_FORCEINLINE constexpr bool Empty() const {return FirstNode == nullptr;}

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

	bool operator==(const LinkedRange&) const = default;
	bool operator!=(const LinkedRange&) const = default;

	Node* FirstNode = nullptr;
};


template<typename T> struct BidirectionalLinkedNode
{
	BidirectionalLinkedNode* Prev;
	BidirectionalLinkedNode* Next;
	T Value;

	[[nodiscard]] INTRA_FORCEINLINE constexpr BidirectionalLinkedNode* PrevListNode() const {return Prev;}
	[[nodiscard]] INTRA_FORCEINLINE constexpr BidirectionalLinkedNode* NextListNode() const {return Next;}
};

template<typename T, typename NodeType> struct BidirectionalLinkedRange: private LinkedRange<T, NodeType>
{
	using BASE = LinkedRange<T, NodeType>;
	using Node = NodeType;

	BidirectionalLinkedRange() = default;

	INTRA_FORCEINLINE constexpr BidirectionalLinkedRange(Node* first, Node* last): BASE(first), LastNode(last) {}

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
		const bool last = this->FirstNode == LastNode;
		LastNode = LastNode->PrevListNode();
		if(last) LastNode = BASE::FirstNode = nullptr;
	}

	[[nodiscard]] constexpr T& Last() const
	{
		INTRA_PRECONDITION(!Empty());
		if constexpr(CSame<T, Node>) return *LastNode;
		else LastNode->Value;
	}

	bool operator==(const BidirectionalLinkedRange&) const = default;
	bool operator!=(const BidirectionalLinkedRange&) const = default;

private:
	Node* LastNode = nullptr;
};

INTRA_DEFINE_FUNCTOR(ForEach)(auto&& f) {
	return [f = FunctorOf(INTRA_FWD(f))]<typename Collection>(Collection&& collection)
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
		else ForEachField(f)(collection);
	};
};

INTRA_DEFINE_FUNCTOR(First)<CList L>(L&& list) -> decltype(auto) {
	if constexpr(CHasFirst<L>) return list.First();
	else return RangeOf(INTRA_FWD(list)).First();
};
INTRA_DEFINE_FUNCTOR(Last)<CBidirectionalList L>(L&& list) -> decltype(auto) {
	if constexpr(CHasLast<L>) return list.Last();
	else return RangeOf(INTRA_FWD(list)).Last();
};
INTRA_DEFINE_FUNCTOR(IsEmpty)<CHasEmpty R>(R&& range) noexcept {return range.Empty();};

INTRA_DEFINE_FUNCTOR(IsFull)<class R>(R&& range) noexcept requires CHasFull<R> || CAssignableRange<R> {
	if constexpr(CHasFull<R>) return range.Full();
	else return range.Empty();
};

INTRA_DEFINE_FUNCTOR(PopFirst)(CHasPopFirst auto&& range) {range.PopFirst();};
INTRA_DEFINE_FUNCTOR(PopLast)(CHasPopLast auto&& range) {range.PopLast();};

INTRA_DEFINE_FUNCTOR(PopFirstCount)(ClampedLongSize maxElementsToPop) noexcept {
	return [maxElementsToPop]<CRange R>(R&& range) requires(!CConst<TRemoveReference<R>>)
	{
		if constexpr(CHasPopFirstCount<R>)
			return range.PopFirstCount(maxElementsToPop);
		else
		{
			const size_t maxElemsToPop = size_t(maxElementsToPop); // optimize for 32 bit, ignoring cases of huge ranges without PopFirstCount method
			size_t poppedElements = 0;
			while(!range.Empty())
			{
				if(poppedElements < maxElemsToPop) break;
				range.PopFirst();
				poppedElements++;
			}
			return index_t(poppedElements);
		}
	};
};

INTRA_DEFINE_FUNCTOR(PopLastCount)(ClampedLongSize maxElementsToPop) noexcept {
	return [maxElementsToPop]<CBidirectionalRange R>(R&& range) requires(!CConst<TRemoveReference<R>>)
	{
		if constexpr(CHasPopFirstCount<R>)
			return range.PopLastCount(maxElementsToPop);
		else
		{
			const size_t maxElemsToPop = size_t(maxElementsToPop); // optimize for 32 bit, ignoring cases of huge ranges without PopLastCount method
			size_t poppedElements = 0;
			while(!range.Empty())
			{
				if(poppedElements < maxElemsToPop) break;
				range.PopLast();
				poppedElements++;
			}
			return index_t(poppedElements);
		}
	};
};

INTRA_DEFINE_FUNCTOR(PopFirstAllCount)<CRange R>(R&& range) requires(!CConst<TRemoveReference<R>>) {
	if constexpr(CHasPopFirstCount<R>)
		return range.PopFirstCount(MaxValueOf<size_t>);
	else
	{
		size_t poppedElements = 0;
		while(!range.Empty())
		{
			range.PopFirst();
			poppedElements++;
		}
		return index_t(poppedElements);
	}
};

INTRA_DEFINE_FUNCTOR(Next)<CRange R>(R&& range) -> decltype(auto)
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

INTRA_DEFINE_FUNCTOR(PopFirstExactly)(Size numElementsToPop) noexcept {
	return [numElementsToPop]<CRange R>(R&& range) requires(!CConst<TRemoveReference<R>>)
	{
		[[maybe_unused]] index_t poppedElements = range|PopFirstCount(numElementsToPop);
		INTRA_POSTCONDITION(size_t(poppedElements) == numElementsToPop);
	};
};
INTRA_DEFINE_FUNCTOR(PopLastExactly)(Size numElementsToPop) noexcept {
	return [numElementsToPop]<CBidirectionalRange R>(R&& range) requires(!CConst<TRemoveReference<R>>)
	{
		[[maybe_unused]] index_t poppedElements = range|PopLastCount(numElementsToPop);
		INTRA_POSTCONDITION(size_t(poppedElements) == numElementsToPop);
	};
};

INTRA_DEFINE_FUNCTOR(PopWhile)(auto pop, auto&& pred) noexcept {
	return [pop, pred = FunctorOf(INTRA_FWD(pred))]<CRange R>(R&& range) requires
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
INTRA_DEFINE_FUNCTOR(PopFirstWhile)(auto&& pred) noexcept {return PopWhile(PopFirst, FNot(INTRA_FWD(pred)));};
INTRA_DEFINE_FUNCTOR(PopLastWhile)(auto&& pred) noexcept {return PopWhile(PopLast, FNot(INTRA_FWD(pred)));};
INTRA_DEFINE_FUNCTOR(PopFirstUntil)(auto&& pred) noexcept {return PopFirstWhile(FNot(INTRA_FWD(pred)));};
INTRA_DEFINE_FUNCTOR(PopLastUntil)(auto&& pred) noexcept {return PopLastWhile(FNot(INTRA_FWD(pred)));};
INTRA_DEFINE_FUNCTOR(Count)<typename L>(L&& list) requires CHasLength<L> || CConsumableList<L>
{
	if constexpr(CHasLength<L>) return Length(list);
	else return Dup(RangeOf(INTRA_FWD(list)))|PopFirstAllCount;
};

INTRA_DEFINE_FUNCTOR(FunctionalRangeOp)(auto inplaceModifyingOp) {
	return [inplaceModifyingOp]<CAccessibleList L>(L&& list) -> decltype(auto)
		requires CCallable<decltype(inplaceModifyingOp), TRangeOfRef<L>>
	{
		auto result = RangeOf(INTRA_FWD(list));
		inplaceModifyingOp(result);
		return result;
	};
};

INTRA_DEFINE_FUNCTOR(Drop)(ClampedSize maxElementsToDrop) noexcept {
	return FunctionalRangeOp(PopFirstCount(maxElementsToDrop));
};
INTRA_DEFINE_FUNCTOR(DropLast)(ClampedSize maxElementsToDrop) noexcept {
	return FunctionalRangeOp(PopLastCount(maxElementsToDrop));
};
INTRA_DEFINE_FUNCTOR(DropExactly)(ClampedSize maxElementsToDrop) {
	return FunctionalRangeOp(PopFirstExactly(maxElementsToDrop));
};
INTRA_DEFINE_FUNCTOR(DropLastExactly)(ClampedSize maxElementsToDrop) {
	return FunctionalRangeOp(PopLastExactly(maxElementsToDrop));
};
INTRA_DEFINE_FUNCTOR(DropWhile)(auto&& pred) noexcept {
	return FunctionalRangeOp(PopFirstWhile(INTRA_FWD(pred)));
};
INTRA_DEFINE_FUNCTOR(DropLastWhile)(auto&& pred) noexcept {
	return FunctionalRangeOp(PopLastWhile(INTRA_FWD(pred)));
};
INTRA_DEFINE_FUNCTOR(DropUntil)(auto&& pred) {
	return FunctionalRangeOp(PopFirstUntil(INTRA_FWD(pred)));
};
INTRA_DEFINE_FUNCTOR(DropLastUntil)(auto&& pred) {
	return FunctionalRangeOp(PopLastUntil(INTRA_FWD(pred)));
};

#if INTRA_CONSTEXPR_TEST
static_assert(CRange<Span<const int>>);
static_assert(CHasFirst<Span<const int>>);
static_assert(CHasPopFirst<Span<const int>>);
static_assert(CHasEmpty<Span<const int>>);
static_assert(CNonConstRValueReference<Span<const int>> || CCopyConstructible<TRemoveReference<Span<const int>>>);
static_assert(CAccessibleRange<Span<const int>>);
static_assert(Span<const int>()|IsEmpty);
//static_assert(Span<const int>()|DropExactly(ClampedSize(Index(0)))|IsEmpty);
static_assert(CCallable<decltype(PopFirstCount(0)), Span<const int>>);
#endif

INTRA_DEFINE_FUNCTOR(Contains)<typename P>(P&& pred) {
	return [drop = DropUntil(INTRA_FWD(pred))](auto&& range) -> decltype(range|DropUntil(INTRA_FWD(pred))|FNot(IsEmpty)) {
		return range|drop|FNot(IsEmpty);
	};
};

INTRA_DEFINE_FUNCTOR(Tail)(ClampedSize maxLength) noexcept {
	return [maxLength]<CNonInfiniteForwardList L>(L&& list) {
		return list|Drop((list|Count) - maxLength);
	};
};

INTRA_DEFINE_FUNCTOR(AtIndex)(Index index) noexcept {
	return [index]<typename L>(L&& list) -> decltype(auto) requires CHasIndex<L> || CAccessibleList<L> {
		if constexpr(CHasIndex<L>) return list[index];
		else return INTRA_FWD(list)|DropExactly(index)|First;
	};
};

INTRA_DEFINE_FUNCTOR(TryFirst)<CList L>(L&& list) -> Optional<TListValueRef<L>>
{
	if(RangeOf(list).Empty()) return Undefined;
	return list|First;
};
INTRA_DEFINE_FUNCTOR(TryLast)<CBidirectionalList L>(L&& list) -> Optional<TListValueRef<L>>
{
	if(RangeOf(list).Empty()) return Undefined;
	return list|Last;
};
INTRA_DEFINE_FUNCTOR(TryNext)<CList L>(L&& list) -> Optional<TListValueRef<L>>
{
	if(RangeOf(list).Empty()) return Undefined;
	return list|Next;
};

INTRA_DEFINE_FUNCTOR(PutRef)<typename T>(TUnsafe, T&& value) {
	return [&]<COutputOf<T> O>(O&& out) {
		if constexpr(z_D::CHasMethodPut<O, T>) out.Put(INTRA_FWD(value));
		else
		{
			out.First() = INTRA_FWD(value);
			out.PopFirst();
		}
	};
};

INTRA_DEFINE_FUNCTOR(PutOnce)(auto&& value) {
	return [value = INTRA_FWD(value)](COutputOf<decltype(value)> auto&& out) {
		out|PutRef(Unsafe, INTRA_MOVE(value));
	};
};
INTRA_DEFINE_FUNCTOR(Put)(auto&& value) {
	return [value = INTRA_FWD(value)](COutputOf<decltype(value)> auto&& out) {
		out|PutRef(Unsafe, value);
	};
};

INTRA_DEFINE_FUNCTOR(TryPut)(auto&& value) {
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

INTRA_DEFINE_FUNCTOR(PopWhileMatch)(auto&& matchPred, auto get1, auto pop1, auto get2, auto pop2)
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
INTRA_DEFINE_FUNCTOR(GenericListsMatch)(auto&& matchPred,
	const auto get1, const auto pop1, const auto get2, const auto pop2, const auto prefixLengthPredicate) requires
	CAnyOf<decltype(get1), decltype(First), decltype(Last)> &&
	CAnyOf<decltype(get2), decltype(First), decltype(Last)> &&
	CAnyOf<decltype(pop1), decltype(PopFirst), decltype(PopLast)> &&
	CAnyOf<decltype(pop2), decltype(PopFirst), decltype(PopLast)> &&
	(CSame<decltype(get1), decltype(First)> == CSame<decltype(pop1), decltype(PopFirst)>) &&
	(CSame<decltype(get2), decltype(First)> == CSame<decltype(pop2), decltype(PopFirst)>) &&
	CAnyOf<decltype(prefixLengthPredicate), decltype(LEqual), decltype(Equal), decltype(GEqual)>
{
	return [=, matchPred = INTRA_FWD(matchPred)]<CList L1, CList L2>(L1&& list1, L2&& list2)
		requires(requires{PopWhileMatch(matchPred, get1, pop1, get2, pop2)(RangeOf(INTRA_FWD(list1)), RangeOf(INTRA_FWD(list2)));})
	{
		if constexpr(CSameArrays<L1, L2> && CTriviallyEqualComparable<TListValue<L1>> &&
			CSame<decltype(get1), decltype(get2)> && CSame<decltype(matchPred), decltype(Equal)>)
			if(!IsConstantEvaluated())
			{
				if(!prefixLengthPredicate(Length(list1), Length(list2))) return false;
				auto data1 = Data(list1);
				auto data2 = Data(list2);
				if constexpr(VSameTypes(get2, Last))
				{
					if constexpr(VSameTypes(prefixLengthPredicate, LEqual))
						data2 += Length(list2) - Length(list1);
					else if constexpr(VSameTypes(prefixLengthPredicate, GEqual))
						data1 += Length(list1) - Length(list2);
				}
				auto length = VSameTypes(prefixLengthPredicate, GEqual)? Length(list2): Length(list1);
				return __builtin_memcmp(data1, data2, size_t(length) * sizeof(TListValue<L1>)) == 0;
			}
		if constexpr([]{
			if constexpr(CUnicodeList<L1> && CUnicodeList<L2>)
				return CSame<TRawUnicodeUnit<L1>, TRawUnicodeUnit<L2>>;
			else return false;
		}())
		{
			return list2.RawUnicodeUnits()|TMatch(list1.RawUnicodeUnits());
		}
		else
		{
			auto range1 = RangeOf(INTRA_FWD(list1));
			auto range2 = RangeOf(INTRA_FWD(list2));
			PopWhileMatch(matchPred, get1, pop1, get2, pop2)(range1, range2);
			if constexpr(VSameTypes(prefixLengthPredicate, LEqual)) return range1.Empty();
			else if constexpr(VSameTypes(prefixLengthPredicate, GEqual)) return range2.Empty();
			else return range1.Empty() && range2.Empty();
		}
	};
};
}

INTRA_DEFINE_FUNCTOR(MatchesWith)(CList auto&& list)
{
	return Bind(
		z_D::GenericListsMatch(Equal, First, PopFirst, First, PopFirst, Equal),
		INTRA_FWD(list));
};
INTRA_DEFINE_FUNCTOR(StartsWith)(CList auto&& prefixList)
{
	return Bind(
		z_D::GenericListsMatch(Equal, First, PopFirst, First, PopFirst, GEqual),
		INTRA_FWD(prefixList));
};
INTRA_DEFINE_FUNCTOR(EndsWith)(CList auto&& suffixList)
{
	return Bind(
		z_D::GenericListsMatch(Equal, Last, PopLast, Last, PopLast, GEqual),
		INTRA_FWD(suffixList));
};

template<class Range2, class ComparePred = decltype(Less)> class LexCompareTo
{
	Range2 mRange2;
	using T2 = TRangeValue<Range2>;
	INTRA_NO_UNIQUE_ADDRESS ComparePred mComparePred;
public:
	template<CForwardList L> explicit constexpr LexCompareTo(L&& list2): mRange2(RangeOf(INTRA_FWD(list2))) {}

	template<CForwardList L, typename ComparePredicate>
	constexpr LexCompareTo(L&& list2, ComparePredicate&& comparePredicate):
		mComparePred(FunctorOf(INTRA_FWD(comparePredicate))),
		mRange2(RangeOf(INTRA_FWD(list2))) {}

	template<CConsumableList L> [[nodiscard]] constexpr int operator()(L&& list1) const
	{
		if constexpr(CSameArrays<L, Range2> &&
			CByteByByteLexicographicallyComparable<T2> &&
			CAnyOf<ComparePred, decltype(Less), decltype(Greater)>)
			if(!IsConstantEvaluated()) //__builtin_memcmp doesn't work in constexpr with GCC with heap allocated memory
			{
				const auto res = __builtin_memcmp(DataOf(list1), DataOf(mRange2),
					size_t(Min(Length(list1), Length(mRange2)))*sizeof(T2));
				if(res == 0) res = Cmp(Length(mRange2), Length(list1));
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
template<CList L, typename ComparePredicate> LexCompareTo(L, ComparePredicate) -> LexCompareTo<TRangeOf<L>, TFunctorOf<ComparePredicate>>;
template<CList L> LexCompareTo(L) -> LexCompareTo<TRangeOf<L>>;

template<CRange R, COutputOf<TRangeValue<R>> OR> requires (!CConst<R>)
constexpr index_t StreamToGeneric(R& src, OR& dst)
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
	template<typename R> requires COutput<R> || CAssignableList<R>
	constexpr CopyTo(R&& dst): Dest(RangeOf(INTRA_FWD(dst))) {}

	/** Consume the first Min(Count(src), Count(Dest)) of `src` and put them into `Dest`.
	  `Dest` and `src` may overlap only if `src` elements come after `Dest` elements
	  @return The number of copied elements.
	**/
	template<CRange Src> constexpr index_t ConsumeFrom(Src&& src)
	{
		static_assert(COutputOf<Dst, TRangeValue<Src>> || CAssignableRange<Dst>);
		static_assert(!CInfiniteRange<Dst> || !CInfiniteRange<Src>);
		if constexpr(CConvertibleToSpan<Src> &&
			CTriviallyCopyable<TArrayListValue<Src>> &&
			CSame<TArrayListValue<Src>, TArrayListValue<Dst>>)
		{
			const auto minLen = Min(LengthOf(src), LengthOf(Dest));
			MemoryCopy(Unsafe, Data(Dest), Data(src), minLen);
			Dest|PopFirstExactly(minLen);
			src|PopFirstExactly(minLen);
			return minLen;
		}
		else if constexpr(CHasStreamToMethod<Src&, Dst&>) return src.StreamTo(Dest);
		else if constexpr(z_D::CHasMethodPutAll<Dst&, Src&>) return Dest.PutAll(src);
		else return StreamToGeneric(src, Dest);
	}

	/** Put the first Min(Count(src), Count(Dest)) of \p src into Dest.
	  `Dest` and `src` may overlap only if `src` elements come after Dest elements
	  @return The number of copied elements.
	*/
	template<class Src> requires CList<Src> || CAdvance<Src>
	INTRA_FORCEINLINE constexpr index_t From(Src&& src)
	{
		if constexpr(CRange<Src> && CRValueReference<Src&&>) return ConsumeFrom(src);
		else if constexpr(CAdvance<Src>) return ConsumeFrom(src.RangeRef);
		else return ConsumeFrom(RangeOf(src));
	}

	template<class Src> requires CList<Src> || CAdvance<Src>
	INTRA_FORCEINLINE constexpr index_t operator()(Src&& src) {return From(INTRA_FWD(src));}
};
template<CList Dst> CopyTo(Dst&&) -> CopyTo<TRangeOf<Dst&&>>;

template<class Dst> requires COutput<Dst> || CAssignableList<Dst>
[[nodiscard]] constexpr auto WriteTo(Dst& dst) {return CopyTo<Dst&>(dst);}




template<class F, typename T> struct TReducer
{
	INTRA_NO_UNIQUE_ADDRESS F Func;
	T Value;

	constexpr TReducer(auto&& f, auto&& seed):
		Func(FunctorOf(INTRA_FWD(f))), Value(INTRA_FWD(seed)) {}

	template<CConsumableList L> constexpr auto operator()(L&& list)
	{
		if constexpr(CRange<L> && !CConst<L>)
		{
			auto res = INTRA_MOVE(Value);
			while(!list.Empty()) res = Func(res, Next(list));
			Value = INTRA_MOVE(res);
			return Value;
		}
		else return operator()(Dup(RangeOf(INTRA_FWD(list))));
	}
};
template<class F, typename T> TReducer(F, T) -> TReducer<TFunctorOf<F>, T>;

template<class F> struct Reduce
{
	INTRA_NO_UNIQUE_ADDRESS F Func;

	INTRA_FORCEINLINE constexpr Reduce(auto&& f): F(FunctorOf(INTRA_FWD(f))) {}

	template<CConsumableList L>	constexpr auto operator()(L&& list)
	{
		if constexpr(CRange<L> && !CConst<L>)
		{
			TReducer<F&, TResultOf<F, TRangeValue<L>, TRangeValue<L>>> reducer(Func, Next(list));
			return reducer(list);
		}
		else return operator()(Dup(RangeOf(INTRA_FWD(list))));
	}
};


template<typename F> class Generate
{
	INTRA_NO_UNIQUE_ADDRESS F mFunc;
	TResultOf<F> mFirst;
public:
	using TagAnyInstanceInfinite = TTag<>;

	INTRA_FORCEINLINE constexpr Generate(F f): mFunc(INTRA_MOVE(f)), mFirst(mFunc()) {}
	[[nodiscard]] INTRA_FORCEINLINE constexpr bool Empty() const {return false;}
	[[nodiscard]] INTRA_FORCEINLINE constexpr const auto& First() const {return mFirst;}
	INTRA_FORCEINLINE constexpr void PopFirst() {mFirst = mFunc();}
};

template<typename F, typename T, size_t N> class Recurrence
{
	INTRA_NO_UNIQUE_ADDRESS TSelect<size_t, EmptyType, (N > 2)> mIndex = 0;
	INTRA_NO_UNIQUE_ADDRESS F mFunc;
	Array<T, N> mState;
public:
	using TagAnyInstanceInfinite = TTag<>;

	template<typename... Ts, CCallable<Ts...> F1> requires (sizeof...(Ts) == N)
		constexpr Recurrence(F1&& function, Ts&&... initialSequence):
		mFunc(INTRA_FWD(function)),
		mState{INTRA_FWD(initialSequence)...} {}

	[[nodiscard]] INTRA_FORCEINLINE constexpr T First() const
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
			nextVal = mFunc(mState[mIndex], mState[(mIndex + (1 + Is) - (mIndex >= N - (1 + Is)? N: 0))]...);
		}();
	}
	[[nodiscard]] INTRA_FORCEINLINE constexpr bool Empty() const noexcept {return false;}
};
template<typename F, typename... Ts>
Recurrence(F&&, Ts&&...) -> Recurrence<TFunctorOf<F&&>, TCommon<Ts...>, sizeof...(Ts)>;

template<CNumber TOffset, CAsCallable<TOffset> F> class Sequence
{
	INTRA_NO_UNIQUE_ADDRESS F mFunc;
public:
	using TagAnyInstanceInfinite = TTag<>;

	Sequence() = default;
	constexpr Sequence(CAsCallable<TOffset> auto&& function, CNumber auto&& offset = 0):
		mFunc(FunctorOf(INTRA_FWD(function))), Offset(INTRA_FWD(offset)) {}

	[[nodiscard]] INTRA_FORCEINLINE constexpr auto First() const {return mFunc(Offset);}
	INTRA_FORCEINLINE constexpr void PopFirst() noexcept {Offset++;}
	[[nodiscard]] INTRA_FORCEINLINE constexpr bool Empty() const noexcept {return false;}
	[[nodiscard]] INTRA_FORCEINLINE constexpr auto operator[](CNumber auto&& index) const {return mFunc(Offset + INTRA_FWD(index));}

	[[nodiscard]] INTRA_FORCEINLINE constexpr auto PopFirstCount(CNumber auto&& maxElementsToPop) noexcept
	{
		Offset += maxElementsToPop;
		return INTRA_FWD(maxElementsToPop);
	}

	INTRA_NO_UNIQUE_ADDRESS TOffset Offset = 0;
};
template<CAsCallable<NDebugOverflow<int64>> F> Sequence(F) -> Sequence<NDebugOverflow<int64>, TUnqualRef<TFunctorOf<F>>>;
template<CNumber TOffset, CAsCallable<TOffset> F> Sequence(F, TOffset) -> Sequence<TUnqualRef<TOffset>, TUnqualRef<TFunctorOf<F>>>;

INTRA_DEFINE_FUNCTOR(Repeat)(auto&& value) {
	return Sequence(FRepeat(INTRA_FWD(value)));
};

template<typename T> struct REmptyRange
{
	[[nodiscard]] INTRA_FORCEINLINE constexpr bool Empty() const noexcept {return true;}
	[[nodiscard]] INTRA_FORCEINLINE constexpr index_t Length() const noexcept {return 0;}
	[[nodiscard]] constexpr T First() const {INTRA_PRECONDITION(!Empty());}
	[[nodiscard]] constexpr T Last() const {INTRA_PRECONDITION(!Empty());}
	constexpr void PopFirst() const {INTRA_PRECONDITION(!Empty());}
	constexpr void PopLast() const {INTRA_PRECONDITION(!Empty());}
	constexpr T operator[](CNumber auto&&) const {INTRA_PRECONDITION(!Empty());}
	[[nodiscard]] INTRA_FORCEINLINE constexpr index_t PopFirstCount(ClampedSize) const noexcept {return 0;}
	INTRA_FORCEINLINE constexpr T* Data() const noexcept {return nullptr;}
	INTRA_FORCEINLINE constexpr REmptyRange Take(CNumber auto&&) {return *this;}
};
template<typename T> constexpr REmptyRange<T> EmptyRange;

constexpr struct {template<typename T> constexpr void Put(T&&) const {}} NullSink;

INTRA_DEFINE_FUNCTOR(IotaInf)<CNumber T = NDebugOverflow<int64>, CNumber S = T>(T first = 0, S step = 1) {
	return Sequence([=](CNumber auto&& index) {return T(first + step * index);}, NDebugOverflow<int64>(0));
};

/** Range that counts all elements that are put into it.
Useful for example for counting result string length before conversion to it to avoid reallocation.
*/
template<typename T = int, typename CounterT = index_t> struct RCounter
{
	using TagAnyInstanceInfinite = TTag<>;

	[[nodiscard]] INTRA_FORCEINLINE constexpr bool Empty() const noexcept {return false;}
	[[nodiscard]] INTRA_FORCEINLINE constexpr T First() const noexcept(noexcept(T())) {return T();}
	INTRA_FORCEINLINE constexpr void Put(const T&) noexcept {Counter++;}
	INTRA_FORCEINLINE constexpr void PopFirst() noexcept {Counter++;}
	[[nodiscard]] INTRA_FORCEINLINE constexpr auto PopFirstCount(CNumber auto&& elementsToPop) const noexcept
	{
		Counter += NDebugOverflow<CounterT>(elementsToPop);
		return INTRA_FWD(elementsToPop);
	}

	CounterT Counter = 0;
};
template<typename T = int, typename CounterT = index_t> struct RCeilCounter: RCounter<T, CounterT> {};
template<typename T> concept CCounter = CInstanceOfTemplate<TRemoveConstRef<T>, RCounter>;
template<typename T> concept CCeilCounter = CInstanceOfTemplate<TRemoveConstRef<T>, RCeilCounter>;




template<class T> concept CHasTakeMethod = requires(T x) {x.Take(index_t());};

template<CRange R, bool Exactly, CNumber TLen = size_t> struct RTake: CopyableIf<!CReference<R>>
{
	using TagAnyInstanceFinite = TTag<>;

	constexpr RTake() = default;

	INTRA_FORCEINLINE constexpr RTake(CList auto&& list, CNumber auto&& count):
		mOriginalRange(RangeOf(INTRA_FWD(list))), mLen(count)
	{
		if constexpr(CHasLength<R>)
		{
			if constexpr(Exactly) {INTRA_PRECONDITION(count <= mOriginalRange.Length());}
			else mLen = Min(mLen, mOriginalRange.Length());
		}
	}

	[[nodiscard]] INTRA_FORCEINLINE constexpr bool Empty() const
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
		return mOriginalRange[mLen - 1];
	}

	constexpr void PopLast() requires CHasIndex<R>
	{
		INTRA_PRECONDITION(!Empty());
		mLen--;
	}

	[[nodiscard]] constexpr TRangeValueRef<R> operator[](CNumber auto&& index) const requires CHasIndex<R>
	{
		INTRA_PRECONDITION(0 <= index && index < mLen);
		return mOriginalRange[INTRA_FWD(index)];
	}

	[[nodiscard]] INTRA_FORCEINLINE constexpr auto Length() const noexcept requires CHasLength<R> || CInfiniteRange<R> {return mLen;}

	[[nodiscard]] INTRA_FORCEINLINE constexpr auto LengthLimit() const noexcept {return mLen;}
	
	[[nodiscard]] INTRA_FORCEINLINE constexpr RTake Take(CNumber auto&& count) const
	{
		return RTake(mOriginalRange, Min(count, mLen));
	}

private:
	R mOriginalRange;
	TLen mLen = 0;
};

INTRA_DEFINE_FUNCTOR(Take)(CNumber auto&& maxCount) noexcept {
	return [maxCount = INTRA_FWD(maxCount)]<typename L>(L&& list) requires(CList<L> || CAdvance<L>) {
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
		else if constexpr(CHasTakeMethod<TRangeOfRef<L>>) return RangeOf(INTRA_FWD(list)).Take(maxCount);
		else return RTake<TRangeOf<L>, false, TUnqualRef<decltype(maxCount)>>(RangeOf(INTRA_FWD(list)), maxCount);
	};
};

INTRA_DEFINE_FUNCTOR(TakeExactly)(CNumber auto&& numElementsToTake) noexcept {
	return [numElementsToTake = INTRA_FWD(numElementsToTake)]<typename L>(L&& list) requires(CList<L> || CAdvance<L>) {
		if constexpr(CAdvance<L>)
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
		else if constexpr(CHasTakeMethod<TRangeOfRef<L>>)
		{
			if constexpr(CHasLength<TRangeOf<L>>) INTRA_PRECONDITION(numElementsToTake <= RangeOf(list).Length());
			return RangeOf(INTRA_FWD(list)).Take(numElementsToTake);
		}
		else return RTake<TRangeOf<L>, true>(RangeOf(INTRA_FWD(list)), numElementsToTake);
	};
};

INTRA_DEFINE_SAFE_DECLTYPE(TTakeResult, Take(0)(Val<T>()));
INTRA_DEFINE_SAFE_DECLTYPE(TTakeExactlyResult, TakeExactly(0)(Val<T>()));


template<class R, class P, bool TestSubranges> class RTakeUntil
{
	R mRange;
	INTRA_NO_UNIQUE_ADDRESS P mPred;
	bool mEmpty = false;
public:
	constexpr RTakeUntil(R range, P pred): mPred(INTRA_MOVE(pred)), mRange(INTRA_MOVE(range)) {}

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

INTRA_DEFINE_FUNCTOR(TakeUntil)(auto&& pred) {
	return [pred = FunctorOf(INTRA_FWD(pred))](CList auto&& list) {
		return RTakeUntil(RangeOf(INTRA_FWD(list)), pred);
	};
};

INTRA_DEFINE_FUNCTOR(TakeUntilEagerly)(auto&& pred) {
	return [pred = FunctorOf(INTRA_FWD(pred))](CForwardList auto&& list) {
		return INTRA_FWD(list)|Take(list|TakeUntil(pred)|Count);
	};
};

INTRA_IGNORE_WARN_ASSIGN_IMPLICITLY_DELETED
template<typename R, CNumber TOffset = size_t> struct RCycle
{
	static_assert(CForwardRange<R> && !CInfiniteRange<R>);
	using TagAnyInstanceInfinite = TTag<>;

	constexpr RCycle(R range) requires (!CRandomAccessRange<R>):
		mOriginalRange(INTRA_MOVE(range)), mOffsetRange(mOriginalRange) {INTRA_PRECONDITION(!mOriginalRange.Empty());}

	constexpr RCycle(R range) requires CRandomAccessRange<R>:
		mOriginalRange(INTRA_MOVE(range)) {INTRA_PRECONDITION(!mOriginalRange.Empty());}

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

	constexpr auto PopFirstCount(CNumber auto&& elementsToPop) const requires CRandomAccessRange<R> || CHasPopFirstCount<R> || CHasLength<R>
	{
		if constexpr(CRandomAccessRange<R>)
		{
			mOffsetRange.Counter += elementsToPop;
			mOffsetRange.Counter %= mOriginalRange.Length();
		}
		else
		{
			auto leftToPop = elementsToPop;
			if constexpr(CHasLength<R>)
			{
				leftToPop %= mOriginalRange.Length();
				const auto offLen = mOffsetRange.Length();
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
		return INTRA_FWD(elementsToPop);
	}

	[[nodiscard]] constexpr decltype(auto) operator[](CNumber auto&& index) const requires CRandomAccessRange<R>
	{
		size_t i = mOffsetRange.Counter + index;
		i %= mOriginalRange.Length();
		return mOriginalRange[i];
	}

private:
	R mOriginalRange;
	TSelect<RCounter<int, size_t>, R, CRandomAccessRange<R>> mOffsetRange;
};

INTRA_DEFINE_FUNCTOR(Cycle)<CList L>(L&& list) requires(CForwardList<L> || CInfiniteList<L>) {
	if constexpr(CInfiniteList<L>) return RangeOf(INTRA_FWD(list));
	else return RCycle(RangeOf(INTRA_FWD(list)));
};

template<class R, typename F> struct RMap
{
	using TagAnyInstanceFinite = TTag<CFiniteRange<R>>;
	using TagAnyInstanceInfinite = TTag<CInfiniteRange<R>>;

	template<CList L> INTRA_FORCEINLINE constexpr RMap(L&& list, CAsCallable<TListValueRef<L>> auto&& f):
		Func(FunctorOf(INTRA_FWD(f))), OriginalRange(RangeOf(INTRA_FWD(list))) {}

	[[nodiscard]] INTRA_FORCEINLINE constexpr decltype(auto) First() const {return Func(OriginalRange.First());}
	INTRA_FORCEINLINE constexpr void PopFirst() {OriginalRange.PopFirst();}
	[[nodiscard]] INTRA_FORCEINLINE constexpr bool Empty() const {return OriginalRange.Empty();}

	constexpr INTRA_FORCEINLINE decltype(auto) Last() const requires CHasLast<R> {return Func(OriginalRange.Last());}
	constexpr INTRA_FORCEINLINE void PopLast() requires CHasPopLast<R> {OriginalRange.PopLast();}
	constexpr INTRA_FORCEINLINE decltype(auto) operator[](CNumber auto&& index) const {return Func(OriginalRange[index]);}
	[[nodiscard]] INTRA_FORCEINLINE constexpr auto Length() const requires CHasLength<R> {return OriginalRange.Length();}

	[[nodiscard]] INTRA_FORCEINLINE constexpr auto PopFirstCount(CNumber auto&& numElementsToPop) requires CHasPopFirstCount<R>
	{return OriginalRange.PopFirstCount(numElementsToPop);}
	[[nodiscard]] INTRA_FORCEINLINE constexpr auto PopLastCount(CNumber auto&& numElementsToPop) requires CHasPopLastCount<R>
	{return OriginalRange.PopLastCount(numElementsToPop);}

	INTRA_NO_UNIQUE_ADDRESS F Func;
	R OriginalRange;
};
template<CList L, CAsCallable<TListValueRef<L>> F> RMap(L, F) -> RMap<TRangeOf<L>, TFunctorOf<F>>;

INTRA_DEFINE_FUNCTOR(Map)(auto&& f) {
	return [func = FunctorOf(INTRA_FWD(f))](CAccessibleList auto&& list) mutable {
		return RMap(RangeOf(INTRA_FWD(list)), INTRA_MOVE(func));
	};
};

template<size_t N> constexpr auto Unzip = []<CList L>(L&& list) requires CStaticLengthContainer<L> {
	return INTRA_FWD(list)|Map(At<N>);
};

INTRA_IGNORE_WARN_COPY_MOVE_IMPLICITLY_DELETED
template<CRange R, class P> class RFilter
{
	using TagAnyInstanceFinite = TTag<CFiniteRange<R>>;
	using TagAnyInstanceInfinite = TTag<CInfiniteRange<R>>;

	constexpr RFilter(CConsumableList auto&& list, auto&& filterPredicate):
		mPred(FunctorOf(INTRA_FWD(filterPredicate))), mOriginalRange(RangeOf(INTRA_FWD(list)))
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
	INTRA_NO_UNIQUE_ADDRESS P mPred;
};

INTRA_DEFINE_FUNCTOR(Filter)<typename P>(P&& pred) {
	return [pred = FunctorOf(INTRA_FWD(pred))]<CConsumableList L>(L&& list) {
		return RFilter<TRangeOf<L>, P>(INTRA_FWD(list), pred);
	};
};

template<typename R> struct RRetro
{
	R OriginalRange;

	[[nodiscard]] INTRA_FORCEINLINE constexpr bool Empty() const {return OriginalRange.Empty();}
	[[nodiscard]] INTRA_FORCEINLINE constexpr decltype(auto) First() const {return OriginalRange.Last();}
	INTRA_FORCEINLINE constexpr void PopFirst() {OriginalRange.PopLast();}
	[[nodiscard]] INTRA_FORCEINLINE constexpr decltype(auto) Last() const {return OriginalRange.First();}
	INTRA_FORCEINLINE constexpr void PopLast() {OriginalRange.PopFirst();}
	
	[[nodiscard]] INTRA_FORCEINLINE constexpr decltype(auto) operator[](CNumber auto&& index) requires CHasIndex<R> && CHasLength<R>
	{
		return OriginalRange[Length() - 1 - index];
	}

	[[nodiscard]] INTRA_FORCEINLINE constexpr decltype(auto) operator[](CNumber auto&& index) const requires CHasIndex<R> && CHasLength<R>
	{
		return OriginalRange[Length() - 1 - index];
	}

	[[nodiscard]] INTRA_FORCEINLINE constexpr auto Length() const requires CHasLength<R> {return OriginalRange.Length();}

	[[nodiscard]] INTRA_FORCEINLINE constexpr auto PopFirstCount(CNumber auto&& maxElementsToPop) requires CHasPopLastCount<R>
	{
		return OriginalRange.PopLastCount(maxElementsToPop);
	}

	[[nodiscard]] INTRA_FORCEINLINE constexpr auto PopLastCount(CNumber auto&& maxElementsToPop) requires CHasPopFirstCount<R>
	{
		return OriginalRange.PopFirstCount(maxElementsToPop);
	}
};

INTRA_DEFINE_FUNCTOR(Retro)<CBidirectionalList L>(L&& list) {
	if constexpr(CInstanceOfTemplate<TRemoveConstRef<L>, RRetro>) return list.OriginalRange;
	else return RRetro{RangeOf(INTRA_FWD(list))};
};

template<typename R, CNumber TChunkLen = size_t> struct RChunks
{
	using TagAnyInstanceFinite = TTag<CFiniteRange<R>>;
	using TagAnyInstanceInfinite = TTag<CInfiniteRange<R>>;

	constexpr RChunks(CList auto&& list, CNumber auto&& chunkLen):
		mOriginalRange(RangeOf(INTRA_FWD(list))), mChunkLen(chunkLen)
	{
		INTRA_PRECONDITION(chunkLen >= 1);
	}

	[[nodiscard]] auto First() const {return mOriginalRange|Take(mChunkLen);}
	void PopFirst() {mOriginalRange|PopFirstCount(mChunkLen);}
	[[nodiscard]] INTRA_FORCEINLINE bool Empty() const {return mChunkLen == 0 || mOriginalRange.Empty();}

	[[nodiscard]] constexpr auto Length() const requires CHasLength<R>
	{
		return (mOriginalRange.Length() + mChunkLen - 1) / mChunkLen;
	}

	[[nodiscard]] constexpr auto operator[](CNumber auto&& index) const requires CHasPopFirstCount<R> && CHasLength<R>
	{
		INTRA_PRECONDITION(index < Length());
		return mOriginalRange|DropExactly(index * mChunkLen)|Take(mChunkLen);
	}

	[[nodiscard]] constexpr auto Last() const requires CHasPopFirstCount<R> && CHasLength<R>
	{
		INTRA_PRECONDITION(!Empty());
		auto numToTake = TChunkLen(mOriginalRange.Length() % mChunkLen);
		if(numToTake == 0) numToTake = mChunkLen;
		return mOriginalRange|Tail(numToTake);
	}

	constexpr void PopLast() requires CHasPopLast<R> && CHasLength<R>
	{
		INTRA_PRECONDITION(!Empty());
		auto numToPop = TChunkLen(mOriginalRange.Length() % mChunkLen);
		if(numToPop == 0) numToPop = mChunkLen;
		mOriginalRange|PopLastExactly(numToPop);
	}

private:
	R mOriginalRange;
	TChunkLen mChunkLen;
};

INTRA_DEFINE_FUNCTOR(Chunks)(ClampedSize chunkLen) {
	return [chunkLen]<CForwardList L>(L&& list) {
		return RChunks<TRangeOf<L>>(RangeOf(INTRA_FWD(list)), chunkLen);
	};
};

INTRA_IGNORE_WARN_ASSIGN_IMPLICITLY_DELETED
template<CAccessibleRange R> struct RStride
{
	using TagAnyInstanceFinite = TTag<CFiniteRange<R>>;
	using TagAnyInstanceInfinite = TTag<CInfiniteRange<R>>;

	RStride() = default;

	constexpr RStride(CAccessibleRange auto&& range, Size strideStep):
		mOriginalRange(INTRA_FWD(range)), mStep(strideStep)
	{
		INTRA_PRECONDITION(strideStep > 0);
		if constexpr(CHasPopLast<R> && CHasLength<R>)
		{
			const size_t len = size_t(mOriginalRange.Length());
			if(len == 0) return;
			mOriginalRange|PopLastExactly((len - 1) % mStep);
		}
	}

	[[nodiscard]] INTRA_FORCEINLINE constexpr decltype(auto) First() const {return mOriginalRange.First();}
	INTRA_FORCEINLINE constexpr void PopFirst() {mOriginalRange|Intra::PopFirstCount(mStep);}
	[[nodiscard]] INTRA_FORCEINLINE constexpr bool Empty() const {return mOriginalRange.Empty();}

	[[nodiscard]] INTRA_FORCEINLINE constexpr decltype(auto) Last() const requires CHasLast<R> {return mOriginalRange.Last();}
	INTRA_FORCEINLINE constexpr void PopLast() requires CHasPopLast<R> {mOriginalRange|Intra::PopLastCount(mStep);}
	[[nodiscard]] INTRA_FORCEINLINE constexpr decltype(auto) operator[](Index index) const requires CHasIndex<R> {return mOriginalRange[index*mStep];}

	[[nodiscard]] INTRA_FORCEINLINE constexpr auto PopFirstCount(CNumber auto&& maxElementsToPop) requires CHasPopFirstCount<R>
	{
		const auto originalElementsPopped = mOriginalRange.PopFirstCount(mStep * maxElementsToPop);
		return (originalElementsPopped + mStep - 1) / mStep;
	}

	[[nodiscard]] INTRA_FORCEINLINE constexpr auto PopLastCount(CNumber auto&& maxElementsToPop) requires CHasPopLastCount<R>
	{
		const auto originalElementsPopped = mOriginalRange.PopLastCount(mStep * maxElementsToPop);
		return index_t((size_t(originalElementsPopped) + mStep - 1) / mStep);
	}

	[[nodiscard]] INTRA_FORCEINLINE constexpr auto Length() const requires CHasLength<R>
	{
		return (mOriginalRange.Length() + mStep - 1) / mStep;
	}

	[[nodiscard]] INTRA_FORCEINLINE constexpr auto Stride(Size step) const {return RStride{mOriginalRange, mStep * step};}

private:
	R mOriginalRange;
	size_t mStep = 0;
};

INTRA_DEFINE_FUNCTOR(Stride)(Size step) {
	return [step]<CAccessibleList L>(L&& list) {
		if constexpr(CInstanceOfTemplate<TRemoveConstRef<L>, RStride>) return list.Stride(step);
		else return RStride<TRangeOf<L>>(RangeOf(INTRA_FWD(list)), step);
	};
};

template<CRange RangeOfLists> class RJoin
{
	using R = TRangeOf<TRangeValueRef<RangeOfLists>>;

	RangeOfLists mLists;
	R mCurrentRange;
	INTRA_NO_UNIQUE_ADDRESS TSelect<bool, EmptyType, CInfiniteRange<R>> mCreatedEmpty;
public:
	using TagAnyInstanceFinite = TTag<CFiniteRange<RangeOfLists> && CFiniteRange<R>>;
	using TagAnyInstanceInfinite = TTag<CInfiniteRange<RangeOfLists>>;

	template<CAccessibleList L> requires CAccessibleRange<TListValue<L>>
	constexpr RJoin(L&& listOfLists):
		mLists(RangeOf(INTRA_FWD(listOfLists))),
		mCreatedEmpty(mLists.Empty())
	{
		goToNearestNonEmptyElement();
	}

	[[nodiscard]] constexpr decltype(auto) First() const
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
	[[nodiscard]] INTRA_FORCEINLINE constexpr bool Empty() const
	{
		if constexpr(CInfiniteRange<R>) return mCreatedEmpty;
		else return mCurrentRange.Empty();
	}

	[[nodiscard]] constexpr decltype(auto) operator[](CNumber auto&& index) const
		requires CHasIndex<R> && (CInfiniteRange<R> || CHasLength<R> && CCopyConstructible<RangeOfLists>)
	{
		INTRA_PRECONDITION(!Empty());
		if constexpr(CInfiniteRange<R>) return mCurrentRange[INTRA_FWD(index)];
		else
		{
			INTRA_PRECONDITION(index < Length());
			auto curIndex = INTRA_FWD(index);
			auto curLen = mCurrentRange.Length();
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

	[[nodiscard]] constexpr auto Length() const
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
INTRA_DEFINE_FUNCTOR(Join)<CList ListOfLists>(ListOfLists&& lists) {
	if constexpr(CInfiniteList<ListOfLists> && CInfiniteList<TListValue<ListOfLists>>)
		return RangeOf(INTRA_FWD(lists)).First();
	else return RJoin<TRangeOf<ListOfLists>>(INTRA_FWD(lists));
};


// TODO: support readers via ReadSome method, including partial reads
template<CAccessibleRange R, CAssignableList B> class RBuffered
{
	static_assert(CSame<TRangeValueRef<R>, TListValueRef<B>>);
public:
	constexpr RBuffered(CAccessibleList auto&& list, CAssignableList auto&& buffer):
		mOriginalRange(RangeOf(INTRA_FWD(list))), mBuffer(INTRA_FWD(buffer)) {loadBuffer();}

	// External buffer referred by range cannot be copied
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

	constexpr auto PopFirstCount(CNumber auto&& n)
	{
		auto numElementsPopped = mBufferRange|Intra::PopFirstCount(n);
		const auto leftToPop = n - numElementsPopped;
		if(!mBufferRange.Empty()) return numElementsPopped;
		numElementsPopped += mOriginalRange|Intra::PopFirstCount(leftToPop);
		loadBuffer();
		return numElementsPopped;
	}

	template<COutputOf<TRangeValue<R>> Dst> constexpr void StreamTo(Dst&& dst)
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

	[[nodiscard]] constexpr decltype(auto) operator[](CNumber auto&& index) requires CHasIndex<R>
	{
		if(index < mBufferRange.Length()) return mBufferRange[index];
		return mOriginalRange[size_t(index) - size_t(mBufferRange.Length())];
	}

	auto GetBuffer() requires CConvertibleToSpan<B> {return Span(mBuffer);}
	auto GetBuffer() const requires CConvertibleToSpan<const B> {return Span(mBuffer);}

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

INTRA_DEFINE_FUNCTOR(Buffered)(CAssignableList auto&& buffer) {
	return [buffer = INTRA_FWD(buffer)]<CAccessibleList L>(L&& list) {
		return RBuffered(INTRA_FWD(list), buffer);
	};
};

INTRA_DEFINE_FUNCTOR(Iota)<CNumber T = NDebugOverflow<int64>>(T begin, T end, auto step = 1) {
	return IotaInf(begin, step)|Take((end - begin + step - 1) / step);
};

#if INTRA_CONSTEXPR_TEST
static_assert(CRandomAccessRange<decltype(IotaInf(1, 3))>);
static_assert(CRandomAccessRange<decltype(Iota(1, 2, 3))>);
#endif


template<class... Rs> class Chain
{
	Tuple<Rs...> mRanges;
	size_t mIndex = 0;
	INTRA_NO_UNIQUE_ADDRESS TConditionalField<size_t, (CBidirectionalRange<Rs> && ...)> mLastIndex;
public:
	using TagAnyInstanceFinite = TTag<(CFiniteRange<Rs> && ...)>;
	using TagAnyInstanceInfinite = TTag<(CInfiniteRange<Rs> || ...)>;

	template<class... Ls> requires (sizeof...(Ls) > 1 || !CSameUnqualRef<Chain, Ls...>) && (CAccessibleList<Ls> && ...)
	constexpr Chain(Ls&&... lists): mRanges(RangeOf(INTRA_FWD(lists))...) {}

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

	[[nodiscard]] constexpr auto Length() const requires (CHasLength<Rs> && ...)
	{
		auto acc = Accum(Add, Count, 0LL);
		ForEach(FRef(acc))(mRanges);
		return acc.Result;
	}

	[[nodiscard]] constexpr decltype(auto) operator[](CNumber auto&& index) const requires ((CHasIndex<Rs> && CHasLength<Rs>) && ...)
	{
		INTRA_PRECONDITION(index < Length());
		const auto getByIndex = [&]<size_t I>(auto& getByIndex, auto index, TIndex<I>) -> decltype(auto) {
			auto& range = At<I>(mRanges);
			if constexpr(I == sizeof...(Rs) - 1) return range[index];
			else
			{
				auto len = size_t(range.Length());
				if(index < len) return range[index];
				else return getByIndex(getByIndex, index - len, TIndex<I + 1>());
			}
		};
		return getByIndex(getByIndex, index, TIndex<0>());
	}

	constexpr auto PopFirstCount(CNumber auto&& maxElementsToPop) const requires (CHasPopFirstCount<Rs> && ...)
	{
		auto elemsLeftToPop = maxElementsToPop;
		ForEach([&](auto& range) {
			elemsLeftToPop -= range.PopFirstCount(elemsLeftToPop);
		})(mRanges);
		return maxElementsToPop - elemsLeftToPop;
	}

	constexpr auto PopLastCount(CNumber auto&& maxElementsToPop) const requires (CHasPopLastCount<Rs> && ...)
	{
		auto elemsLeftToPop = maxElementsToPop;
		ForEach([&](auto& range) {
			elemsLeftToPop -= range.PopLastCount(elemsLeftToPop);
		})(mRanges);
		return maxElementsToPop - elemsLeftToPop;
	}
};
template<CList... Ls> Chain(Ls&&...) -> Chain<TRangeOf<Ls&&>...>;

template<class... Rs> struct RoundRobin
{
	static_assert(sizeof...(Rs) != 0);
	using TagAnyInstanceInfinite = TTag<(CInfiniteRange<Rs> || ...)>;
	using TagAnyInstanceFinite = TTag<(CFiniteRange<Rs> && ...)>;

	RoundRobin() = default;
	template<class... Ls> requires (sizeof...(Ls) > 1 || !CSameUnqualRef<RoundRobin, Ls...>) && (CList<Ls> && ...)
	constexpr RoundRobin(Ls&&... lists): mRanges(RangeOf(INTRA_FWD(lists))...)
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

	[[nodiscard]] INTRA_FORCEINLINE constexpr bool Empty() const {return mCurrent == sizeof...(Rs);}

	[[nodiscard]] constexpr auto Length() const requires (CHasLength<Rs> && ...)
	{
		auto accum = Accum(Add, Count, 0LL);
		ForEachField(FRef(accum))(mRanges);
		return accum.Result;
	}

private:
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

template<auto LengthPolicy = Min, CRange... Rs>
class Zip: CopyableIf<!(CReference<Rs> || ...)>
{
	static_assert(VAnyOf(LengthPolicy,
		Min, //Length = Min(Rs::Length...)
		Equal, //Require all Rs::Length to be equal
		Max //Length = Max(Rs::Length...), empty range elements are default constructed
	));
	static_assert(!VSameTypes(LengthPolicy, Max) || (CConstructible<TRangeValueRef<Rs>> && ...));
	Tuple<Rs...> mRanges;
	static constexpr bool cacheLengthInConstructor = sizeof...(Rs) != 0 && !VSameTypes(LengthPolicy, Equal) && ((CHasLength<Rs> && !CReference<Rs>) && ...);
	INTRA_NO_UNIQUE_ADDRESS TConditionalField<size_t, cacheLengthInConstructor> mLen;

	static constexpr size_t IndexOfFirstRangeWithLength = IndexOfFirstTrueArg<0>(CHasLength<Rs>...);
public:
	using TagAnyInstanceFinite = TTag<(CFiniteRange<Rs> && ...) || VSameTypes(LengthPolicy, Min) && (CFiniteRange<Rs> || ...)>;
	using TagAnyInstanceInfinite = TTag<(CInfiniteRange<Rs> && ...) || VSameTypes(LengthPolicy, Max) && (CInfiniteRange<Rs> || ...)>;

	static_assert(!VSameTypes(LengthPolicy, Equal) || (CFiniteRange<Rs> || ...) && (CInfiniteRange<Rs> || ...),
		"All ranges are required to have the same length but Rs contains both finite and infinite ranges!");

	template<CAccessibleList... Ls> [[nodiscard]] constexpr Zip(Ls&&... lists): mRanges(RangeOf(INTRA_FWD(lists))...)
	{
		if constexpr(cacheLengthInConstructor)
		{
			mLen = mRanges|ForEachFieldToArray(Count)|ApplyPackedArgs(LengthPolicy);
		}

		//TODO: implement Min and Max policies
		if constexpr(VSameTypes(LengthPolicy, Equal))
		{
			if(IsConstantEvaluated() || Config::DebugCheckLevel >= 1)
			{
				//Try to check the requirement early, at construction
				if constexpr(((CFiniteRange<Rs> && !CReference<Rs>) || ...))
				{
					bool allRangeLengthsAreEqual = true;
					if constexpr((CHasLength<Rs> && ...))
					{
						int64 length = -1;
						mRanges|ForEach([&](auto& range) {
							auto len = int64(range.Length());
							if(length == -1) length = len;
							if(len != length) allRangeLengthsAreEqual = false;
						});
					}
					else if constexpr(Config::DebugCheckLevel >= 2 && CCopyConstructible<Zip>)
					{
						auto copy = *this;
						for(;; copy.PopFirst())
						{
							auto anyEmpty = AccumAny(IsEmpty);
							ForEachField(anyEmpty)(copy.mRanges);
							auto allEmpty = AccumAll(IsEmpty);
							ForEachField(allEmpty)(copy.mRanges);
							if(!anyEmpty.Result) continue;
							if(anyEmpty.Result != allEmpty.Result) allRangeLengthsAreEqual = false;
							break;
						}
					}
					INTRA_PRECONDITION(allRangeLengthsAreEqual);
				}
			}
		}
	}

	[[nodiscard]] constexpr auto First() const
	{
		if constexpr(VSameTypes(LengthPolicy, Max))
			return ForEachField([](auto& r) {TryFirst(r).Or({});})(mRanges);
		else return ForEachField(Intra::First)(mRanges);
	}

	constexpr void PopFirst()
	{
		INTRA_PRECONDITION(!Empty());
		if constexpr(VSameTypes(LengthPolicy, Max))
			ForEachField(Intra::PopFirstCount(1))(mRanges);
		else ForEachField(Intra::PopFirst)(mRanges);
	}

	[[nodiscard]] constexpr bool Empty() const
	{
		if constexpr(cacheLengthInConstructor)
			return mLen > 0;
		else if constexpr(VSameTypes(LengthPolicy, Max))
		{
			auto allEmpty = AccumAll(IsEmpty);
			ForEachField(allEmpty)(mRanges);
			return allEmpty.Result;
		}
		else return At<0>(mRanges).Empty();
	}


	[[nodiscard]] constexpr auto Last() const requires(CHasLast<Rs> && ...)
	{
		if constexpr(VSameTypes(LengthPolicy, Max))
			return ForEachField([](auto& r) {TryLast(r).Or({});})(mRanges);
		else return ForEachField(Intra::Last)(mRanges);
	}

	constexpr void PopLast() requires (CHasPopLast<Rs> && ...)
	{
		INTRA_PRECONDITION(!Empty());
		if constexpr(VSameTypes(LengthPolicy, Max))
			ForEachField(Intra::PopLastCount(1))(mRanges);
		else ForEachField(Intra::PopLast)(mRanges);
	}


	[[nodiscard]] constexpr auto Length() const requires CHasLength<TPackFirst<Rs...>>
	{
		if constexpr(cacheLengthInConstructor)
			return mLen;
		else if constexpr(VSameTypes(LengthPolicy, Equal))
			return At<0>(mRanges).Length();
		else ForEachFieldToArray(Count)(mRanges)|ApplyPackedArgs(LengthPolicy);
	}

	[[nodiscard]] constexpr auto operator[](CNumber auto&& index) const requires (CHasIndex<Rs> && ...)
	{return mRanges|ForEach(AtIndex(INTRA_FWD(index)));}

	constexpr auto PopFirstCount(CNumber auto&& maxElementsToPop) requires (CHasPopFirstCount<Rs> || ...)
	{return popCount(Intra::PopFirstCount(INTRA_FWD(maxElementsToPop)));}

	constexpr auto PopLastCount(CNumber auto&& maxElementsToPop) requires (CHasPopLast<Rs> && ...) && (CHasPopLastCount<Rs> || ...)
	{return popCount(Intra::PopLastCount(INTRA_FWD(maxElementsToPop)));}

private:
	constexpr auto popCount(auto op)
	{
		if constexpr(VSameTypes(LengthPolicy, Equal))
			return At<0>(ForEachField(op)(mRanges));
		else
		{
			auto accum = Accum(LengthPolicy, op, 0LL);
			ForEachField(FRef(accum))(mRanges);
			return accum.Result;
		}
	}
};

// Inherit from this class to globally register the object in a list using T as a key.
/* Useful for interface implementations, for example, codecs:
1. Each codec type inherits from GloballyRegistered<ICodec>;
2. Each codec type implements interface ICodec;
3. Each codec type has only one instance as a global variable/constant or static field (app initialization code must be able to call static initializers);
4. Use GloballyRegistered<ICodec>::Instances() to iterate over a list of registered codecs.
*/
template<typename T> class GloballyRegistered
{
	T* mNext;
	static T* lastInited;

	GloballyRegistered(const GloballyRegistered&) = delete;
	GloballyRegistered(GloballyRegistered&&) = delete;
	GloballyRegistered& operator=(const GloballyRegistered&) = delete;
	GloballyRegistered& operator=(GloballyRegistered&&) = delete;
protected:
	INTRA_FORCEINLINE GloballyRegistered(): mNext(lastInited) {lastInited = static_cast<T*>(this);}
public:
	[[nodiscard]] INTRA_FORCEINLINE T* NextListNode() const {return mNext;}
	[[nodiscard]] INTRA_FORCEINLINE static auto Instances() {return RangeOf(*lastInited);}
};
template<typename T> T* GloballyRegistered<T>::lastInited;

} INTRA_END
