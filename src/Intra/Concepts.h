#pragma once

#include "Intra/Core.h"
#include "Intra/Preprocessor.h"

namespace Intra {
template<typename... Ts> struct Tuple;
}

namespace std { //NOLINT
//For compatibility with STL and structured bindings
INTRA_WARNING_PUSH
INTRA_IGNORE_WARNS_MSVC(4643);
template<class T> struct tuple_size;
template<size_t I, class T> struct tuple_element;
INTRA_WARNING_POP
template<typename... Ts> struct tuple_size<Intra::Tuple<Ts...>> {static constexpr size_t value = sizeof...(Ts);};
template<typename... Ts> struct tuple_size<const Intra::Tuple<Ts...>>: tuple_size<Intra::Tuple<Ts...>> {};
template<size_t I, typename T0, typename... Ts> struct tuple_element<I, Intra::Tuple<T0, Ts...>> {using type = Intra::TPackAt<I, T0, Ts...>;};
template<size_t I> struct tuple_element<I, Intra::Tuple<>> {};
template<size_t I, typename... Ts> struct tuple_element<I, const Intra::Tuple<Ts...>> {using type = const Intra::TPackAt<I, Ts...>;};
template<size_t I> struct tuple_element<I, const Intra::Tuple<>> {};

template<typename T, size_t N> struct tuple_size<Intra::Array<T, N>> {static constexpr size_t value = N;};
template<typename T, size_t N> struct tuple_size<const Intra::Array<T, N>>: tuple_size<Intra::Array<T, N>> {};
template<size_t I, typename T, size_t N> struct tuple_element<I, Intra::Array<T, N>> {using type = T;};
template<size_t I, typename T, size_t N> struct tuple_element<I, const Intra::Array<T, N>> {using type = const T;};
template<typename... Ts> class variant;
}

namespace Intra { INTRA_BEGIN
template<typename... Ts> class Variant;
constexpr void ReflectFieldPointersOf(...);
constexpr void ReflectFieldNamesOf(...);
template<class T> concept CHasReflectFieldPointersOf = !CVoid<decltype(ReflectFieldPointersOf(Type<T>))>;
template<class T> concept CHasReflectFieldNamesOf = !CVoid<decltype(ReflectFieldNamesOf(Type<T>))>;

namespace z_D {
template<typename T> concept CTupleSizeDefined = requires(T) {std::tuple_size<T>::value;};
template<typename T> concept CTupleElementDefined = requires(T) {Val<typename std::tuple_element<0, TRemoveReference<T>>::type>();};
}
template<typename T> constexpr index_t StaticLength = [] {
	using T1 = TUnqualRef<T>;
	if constexpr(z_D::CTupleSizeDefined<T1>) return index_t(std::tuple_size<T1>::value);
	else if constexpr(CHasFieldPointersOf<T1>) return StaticLength<decltype(FieldPointersOf<T1>)>;
	else if constexpr(CKnownBoundArrayType<T1>) return ArrayExtent<T1>;
	else return -1;
}();
template<typename T> concept CStaticLengthContainer = StaticLength<T> != -1;
template<typename T> concept CVariant = CInstanceOfTemplate<TUnqualRef<T>, Variant> || CInstanceOfTemplate<TUnqualRef<T>, std::variant>;

#if INTRA_CONSTEXPR_TEST
static_assert(CStaticLengthContainer<const char(&)[5]>);
static_assert(CStaticLengthContainer<char[5]>);
static_assert(!CStaticLengthContainer<const char>);
static_assert(StaticLength<const char(&)[5]> == 5);

static_assert(std::tuple_size<Tuple<int, float, char>>::value == 3);
static_assert(z_D::CTupleSizeDefined<Tuple<int, float, char>>);
static_assert(CSame<typename std::tuple_element<0, Intra::Tuple<int, float, char>>::type, int>);
static_assert(z_D::CTupleElementDefined<Tuple<int, float, char>>);

static_assert(CStaticLengthContainer<Tuple<int, float, char>>);
static_assert(StaticLength<Tuple<int, float, char>> == 3);
static_assert(StaticLength<Array<int, 7>> == 7);
#endif

namespace z_D {
template<class T> struct TFieldTList_;
template<class T, class Seq> struct TupleElementsTList_;
template<class T, size_t... Is> struct TupleElementsTList_<T, TIndexSeq<Is...>>
{
	using _ = TList<typename std::tuple_element<Is, T>::type...>;
};
template<CTupleSizeDefined T> requires CTupleElementDefined<T> struct TFieldTList_<T>: TupleElementsTList_<T, TMakeIndexSeq<std::tuple_size<T>::value>> {};
template<CHasReflectFieldPointersOf T> struct TFieldTList_<T>
{
    using FieldPointersTuple = decltype(ReflectFieldPointersOf(Type<T>));
    using FieldPointersTList = typename TFieldTList_<TUnqualRef<FieldPointersTuple>>::_;
    using _ = TListTransform<FieldPointersTList, TMemberFieldType>;
};
}
template<CStaticLengthContainer C> using TFieldTList = typename z_D::TFieldTList_<TUnqualRef<C>>::_;
template<CStaticLengthContainer C> using TQualRefFieldTList = TListTransform<TFieldTList<C>, TPropagateQualLVRef, C>;

template<CStaticLengthContainer C, size_t End = StaticLength<C>, size_t Start = 0>
using TQualRefFieldTListSlice = TListSlice<TQualRefFieldTList<C>, Start, End>;

#if INTRA_CONSTEXPR_TEST
static_assert(CSame<TQualRefFieldTList<const Tuple<int, float>&>, TList<const int&, const float&>>);
static_assert(CSame<TQualRefFieldTListSlice<const Tuple<int, float>&, 1>, TList<const int&>>);
static_assert(CTListConvertible<TList<int>, TList<float>>);
static_assert(!CTListConvertible<int, float>);
#endif

template<CStaticLengthContainer C, class MapFunc>
using TMappedFieldTList = TListTransform<TQualRefFieldTList<C>, TResultOfOrVoid, MapFunc>;
template<CStaticLengthContainer C, class MapFunc>
using TMappedFieldCommon = TListCommon<TMappedFieldTList<C, MapFunc>>;
template<CStaticLengthContainer C, class MapFunc>
using TMappedFieldCommonRef = TListCommonRef<TMappedFieldTList<C, MapFunc>>;


template<typename L, typename R = L> concept CEqualityComparable = requires(L lhs, R rhs) {lhs == rhs;};
template<typename L, typename R = L> concept CNonEqualityComparable = requires(L lhs, R rhs) {lhs != rhs;};

template<typename T> concept CMovable = CObject<T> && CMoveConstructible<T> && CMoveAssignable<T>;
template<typename T> concept CCopyable = CCopyConstructible<T> && CMovable<T> && CCopyAssignable<T>;
template<typename T> concept CSemiregular = CCopyable<T> && CConstructible<T>;
template<typename T> concept CRegular = CSemiregular<T> && CEqualityComparable<T>;

namespace z_D {
template<typename T> concept CHasMethod_size = requires(T x) {{x.size()} -> CConvertibleTo<size_t>;};
template<typename T> concept CHasMethodLength = requires(T x) {{x.Length()} -> CConvertibleTo<index_t>;};
template<typename T> concept CHasMethod_data = requires(T x, const void*& res) {{x.data()} -> CConvertible<const void*>;};
template<typename T> concept CHasMethodData = requires(T x, const void*& res) {{x.Data()} -> CConvertible<const void*>;};
}
INTRA_DEFINE_SAFE_DECLTYPE(TIteratorOf, begin(Val<T>()));


constexpr auto Length = []<class L>(L&& list)
	requires z_D::CHasMethodLength<L> || z_D::CHasMethod_size<L> || CKnownBoundArrayType<TRemoveReference<L>>
{
	if constexpr(CKnownBoundArrayType<L>) return StaticLength<L>;
	else if constexpr(z_D::CHasLength<L>) return list.Length();
	else return index_t(list.size());
};

constexpr auto Data = []<class L>(L&& list)
	requires z_D::CHasMethodData<L> || z_D::CHasMethod_data<L> || CArrayType<L>
{
	if constexpr(CArrayType<L>) return static_cast<decltype(list[0])*>(list);
	else if constexpr(z_D::CHasData<L>) return list.Data();
	else return list.data();
};

template<typename T> concept CHasData = requires(T&& x) {{Data(x)} -> CConvertibleTo<const void*>;};
template<typename T> concept CHasLength = requires(T&& x) {{Length(x)} -> CConvertibleTo<index_t>;};
template<typename R> concept CArrayList = CHasData<R> && CHasLength<R>;
template<typename... Rs> concept CSameArrays = (CArrayList<Rs> && ...) && CSame<TArrayListValue<Rs>...>;

template<z_D::CHasMethodData L> [[nodiscard]] constexpr auto begin(L&& list) {return list.Data();}
template<z_D::CHasMethodData L> requires z_D::CHasMethodLength<L>
[[nodiscard]] constexpr auto end(L&& list) {return list.Data() + list.Length();}

template<class L> using TArrayElementPtr = decltype(Data(Val<T>()));
template<typename T> using TArrayElementKeepConst = TRemovePointer<TArrayElementPtr<T>>;
template<typename T> using TArrayElementRef = TArrayElementKeepConst<T>&;
template<typename T> using TArrayListValue = TRemoveConst<TArrayElementKeepConst<T>>;

#if INTRA_CONSTEXPR_TEST
static_assert(CHasData<const char(&)[5]>);
static_assert(!CHasData<int>);
static_assert(CHasLength<const char(&)[5]>);
static_assert(CArrayList<const char(&)[5]>);

static_assert(CSame<TArrayElementPtr<const char(&)[5]>, const char*>);
static_assert(CSame<TArrayListValue<const char(&)[5]>, char>);
static_assert(CSame<TArrayElementKeepConst<const char(&)[5]>, const char>);
#endif


template<typename R> concept CAssignableArrayList = CArrayList<R> && !CConst<TArrayElementKeepConst<R>>;


template<typename T> concept CHasPreIncrement = requires(T x) {++x;};
template<typename T> concept CHasPostIncrement = requires(T x) {x++;};
template<typename T> concept CHasPreDecrement = requires(T x) {--x;};
template<typename T> concept CHasPostDecrement = requires(T x) {x--;};
template<typename T> concept CHasDereference = requires(T x) {*x;};


template<class R> concept CHasFirst = requires(R&& r) {r.First();};
template<class R> concept CHasPopFirst = requires(R&& r) {r.PopFirst();};
template<class R> concept CHasEmpty = requires(R&& r, bool& res) {res = r.Empty();};
template<class R> concept CHasLast = requires(R&& r, TRangeValue<R>& res) {{r.Last()} -> CSame<decltype(r.First())>;};
template<class R> concept CHasPopLast = requires(R&& r) {r.PopLast();};

template<class R> concept CHasPopFirstCount = requires(R&& r, index_t& res) {res = r.PopFirstCount(size_t());};
template<class R> concept CHasPopLastCount = requires(R&& r, index_t& res) {res = r.PopLastCount(size_t());};
template<class R> concept CHasNext = requires(R&& r, TRangeValueRef<R>& res) {res = r.Next();};

template<class RSrc, class RDst> concept CHasReadWriteMethod =
	requires(RSrc&& src, RDst&& dst) {INTRA_FWD(src).ReadWrite(INTRA_FWD(dst));};

template<typename R> concept CRange = /*CConstructible<R> &&*/
	CHasFirst<R> && CHasPopFirst<TRemoveConstRef<R>> && CHasEmpty<R>;

template<CRange R> using TRangeValueRef = decltype(Val<R>().First());
template<CRange R> using TRangeValue = TUnqualRef<TRangeValueRef<R>>;

template<typename R> concept CForwardRange = CRange<R> && CCopyConstructible<TRemoveReference<R>>;
template<typename R> concept CBidirectionalRange = CForwardRange<R> && CHasLast<R> && CHasPopLast<TRemoveConstRef<R>>;
template<typename R> concept CRandomAccessRange = CForwardRange<R> && CHasIndex<R>;
template<typename T> concept CCharRange = CRange<T> && CChar<TRangeValue<T>>;
template<typename T> concept CForwardCharRange = CForwardRange<T> && CChar<TRangeValue<T>>;
template<typename T> concept CAssignableRange = CRange<T> && CNonConstLValueReference<TRangeValueRef<T>>;

template<typename R> concept CFiniteRange = CRange<R> &&
	(requires {R::TagAnyInstanceFinite::True;} || CHasLength<R> || CBidirectionalRange<R>);
template<typename R> concept CInfiniteRange = CRange<R> && requires {R::TagAnyInstanceInfinite::True;};
template<typename R> concept CNonInfiniteRange = !CInfiniteRange<R> && CRange<R>;
template<typename R> concept CFiniteForwardRange = CFiniteRange<R> && CForwardRange<R>;
template<typename R> concept CNonInfiniteForwardRange = CNonInfiniteRange<R> && CForwardRange<R>;
template<typename R> concept CFiniteRandomAccessRange = CRandomAccessRange<R> && CHasLength<R>;
template<typename R> concept CInfiniteRandomAccessRange = CRandomAccessRange<R> && !CHasLength<R>;
template<typename R1, typename R2> concept CSameValueType = CSame<TRangeValue<R1>, TRangeValue<R2>>;
template<typename R, typename T> concept CFiniteInputRangeOfExactly = CFiniteRange<R> && CSame<TRangeValue<R>, T>;

template<typename R, typename T> concept CFiniteForwardRangeOfExactly =
	CFiniteForwardRange<R> && CSame<TRangeValue<R>, T>;

template<typename R> concept CAccessibleRange = CRange<R> &&
	(CNonConstRValueReference<R> || CCopyConstructible<TRemoveReference<R>>);

template<typename R> concept CConsumableRange = CAccessibleRange<R> && !CInfiniteRange<R>;
template<typename R, typename T> concept CConsumableRangeOf = CConsumableRange<R> && CConvertibleTo<TRangeValue<R>, T>;

template<typename P, typename... R> concept CElementPredicate = CSame<bool, TResultOf<P, TRangeValue<R>...>>;
template<typename P, typename... R> concept CElementAsPredicate = CElementPredicate<TFunctorOf<P>>;


template<typename T1, typename T2 = T1> concept CHasIntegralDifference = requires(T1 x, T2 y) {{x - y} -> CIntegralNumber;};

namespace z_D {
template<typename R> struct RangeForIterLike
{
	constexpr RangeForIterLike& operator++() {Range.PopFirst(); return *this;}
	constexpr TRangeValueRef<R> operator*() const {return Range.First();}
	constexpr bool operator!=(decltype(nullptr)) const {return !Range.Empty();}
	R Range;
};

template<class R> concept CRangeForIterableClass = requires(R&& r, bool& res)
{
	res = r.begin() != r.end();
    *++Val<decltype(r.begin())&>();
};

template<class R> concept CRangeForIterableEx = requires(R&& r, bool& res)
{
	res = begin(r) != end(r);
	*++Val<decltype(begin(r))&>();
};
}
template<typename T> concept CRangeForIterable =
	CArrayType<TRemoveReference<T>> ||
	z_D::CRangeForIterableClass<T> || z_D::CRangeForIterableEx<T>;

#if INTRA_CONSTEXPR_TEST
static_assert(CRangeForIterable<int(&)[5]>);
#endif

INTRA_DEFINE_SAFE_DECLTYPE(TIteratorReturnValueTypeOf, *Val<TRemoveConstRef<T>>());
template<typename R> using TIteratorValueTypeOf = TRemoveConstRef<TIteratorReturnValueTypeOf<R>>;

template<typename T> concept CMinimalInputIterator =
	CHasPreIncrement<T> && CHasDereference<T> && CNonEqualityComparable<T, T> &&
	CMoveConstructible<T> && CMoveAssignable<T> && CDestructible<T>;

template<typename T> concept CInputIterator = CMinimalInputIterator<T> &&
	CHasPostIncrement<T> && CEqualityComparable<T, T> &&
	CCopyConstructible<T> && CCopyAssignable<T>;

template<typename T> concept CMinimalBidirectionalIterator = CMinimalInputIterator<T> &&
	CHasPreDecrement<T> &&
	CCopyConstructible<T> && CCopyAssignable<T>;

template<CMinimalInputIterator I1, typename I2> struct IteratorRange
{
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

	constexpr void PopLast() requires CMinimalBidirectionalIterator<I2>
	{
	    INTRA_PRECONDITION(!Empty());
	    --End;
	}

	constexpr decltype(auto) Last() const requires CMinimalBidirectionalIterator<I2>
	{
	    INTRA_PRECONDITION(!Empty());
	    return *--I2(End);
	}

	[[nodiscard]] constexpr index_t Length() const requires CHasIntegralDifference<I2, I1>
	{
	    INTRA_PRECONDITION(!Empty());
	    return index_t(End - Begin);
	}
};

template<typename T> struct LinkedNode;
template<typename T> struct BidirectionalLinkedNode;
template<typename T, typename Node = LinkedNode<T>> struct LinkedRange;
template<typename T, typename Node = BidirectionalLinkedNode<T>> struct BidirectionalLinkedRange;

template<class L> concept CLinkedList = requires(L&& r) {r.NextListNode();};
template<class L> concept CBidirectionalLinkedList = CLinkedList<L> && requires(L&& r) {r.PrevListNode();};

template<typename T> struct Span;
template<typename L, template<typename> class ArrayRange = Span, template<typename, typename> class LinkedRange = LinkedRange>
[[nodiscard]] constexpr decltype(auto) RangeOf(L&& list)
{
	if constexpr(CRange<L>)
	{
		if constexpr(CConst<TRemoveReference<L>> && CForwardRange<TUnqualRef<L>>) return TUnqualRef<L>(list);
		else return INTRA_FWD(list);
	}
	else if constexpr(CArrayList<L> && !CRValueReference<L>) return ArrayRange(list);
	else if constexpr(CLinkedList<L>) return LinkedRange(list);
	else if constexpr(z_D::CRangeForIterableEx<L>) return IteratorRange{begin(list), end(list)};
	else if constexpr(z_D::CRangeForIterableClass<L>) return IteratorRange{list.begin(), list.end()};
}

template<typename T> using TRangeOfRef = decltype(RangeOf(Val<T>()));
template<typename T> using TRangeOf = TUnqualRef<TRangeOfRef<T>>;
template<typename T> concept CHasRangeOf = !CVoid<TRangeOfRef<T>>;


template<typename T> concept CList = CRange<TRangeOfRef<T>>;
template<typename T> concept CFiniteList = CFiniteRange<TRangeOfRef<T>>;
template<typename T> concept CNonFiniteList = CNonInfiniteRange<TRangeOfRef<T>>;
template<typename T> concept CAssignableList = CAssignableRange<TRangeOfRef<T>>;

template<typename T> concept CForwardList = CForwardRange<TRangeOfRef<T>>;
template<typename T> concept CFiniteForwardList = CFiniteForwardRange<TRangeOfRef<T>>;
template<typename T> concept CNonInfiniteForwardList = CNonInfiniteForwardRange<TRangeOfRef<T>>;

template<typename T> concept CBidirectionalList = CBidirectionalRange<TRangeOfRef<T>>;

template<typename T> concept CRandomAccessList = CRandomAccessRange<TRangeOfRef<T>>;
template<typename T> concept CFiniteRandomAccessList = CFiniteRandomAccessRange<TRangeOfRef<T>>;

template<typename T> concept CInfiniteList = CInfiniteRange<TRangeOfRef<T>>;
template<typename T> concept CCharList = CCharRange<TRangeOfRef<T>>;
template<typename T> concept CForwardCharList = CForwardCharRange<TRangeOfRef<T>>;

template<typename T> using TListValueRef = TRangeValueRef<TRangeOfRef<T>>;
template<typename T> using TListValue = TRangeValue<TRangeOfRef<T>>;

template<typename R> concept CAccessibleList = CAccessibleRange<TRangeOfRef<R>>;
template<typename R> concept CConsumableList = CConsumableRange<TRangeOfRef<R>>;
template<typename R, typename T> concept CConsumableListOf = CConsumableRangeOf<TRangeOfRef<R>, T>;

template<typename R> using CAccessibleListT = TValue<CAccessibleList<R>>;

template<CList L> using TRawUnicodeUnit = decltype(RangeOf(Val<T>().RawUnicodeUnits()).First());
template<class L> concept CUnicodeList = CChar<TRemoveReference<TRawUnicodeUnit<L>>>;



template<class R> concept CHasFull = requires(R&& r) {{r.Full()} -> CConvertibleTo<bool>;};
template<class R, typename T> concept CHasTryPut = requires(R&& r, T&& val) {{r.TryPut(val)} -> CConvertibleTo<bool>;};
template<class R> concept CHasWrittenRange = requires(R&& r) {{r.WrittenRange()} -> CRange;};
template<class R> using TWrittenRangeType = decltype(Val<R>().WrittenRange());

template<class R> concept CHasReset = requires(R&& r) {r.Reset();};

namespace z_D {
template<class R, typename T> concept CHasMethodPut = requires(R&& r, T&& val) {r.Put(val);};
template<class R, class RSrc> concept CHasMethodPutAll =
	requires(R&& dst, RSrc&& src) {INTRA_FWD(dst).PutAll(INTRA_FWD(src));};
}

template<class O, typename T> concept COutputOf = z_D::CHasMethodPut<O, T> ||
	CAssignableRange<O> && CMoveAssignable<TListValue<O>, T>;

template<class O> concept CCharOutput = COutputOf<O, char8_t>;
template<class O> concept COutput = CCharOutput<O> || COutputOf<O, TListValue<O>> || COutputOf<O, typename O::value_type>;

template<typename R, typename T> concept COutputBufferOf = COutputOf<R, T> && CHasReset<R> && CHasWrittenRange<R>;

template<typename L> [[nodiscard]] constexpr decltype(auto) OutputOf(L&& listOrOutput)
{
	if constexpr(CAssignableList<L>) return RangeOf(INTRA_FWD(listOrOutput));
	else return INTRA_FWD(listOrOutput);
}

namespace z_D {
template<class C, typename T = TListValue<C>> concept CHasMethod_push_back = requires(C c, T&& x) {c.push_back(INTRA_FWD(x));};
template<class C, typename T = TListValue<C>> concept CHasMethodAddLast = requires(C c, T&& x) {c.AddLast(INTRA_FWD(x));};
template<class C, typename T = TListValue<C>> concept CHasMethod_push_front = requires(C c, T&& x) {c.push_front(INTRA_FWD(x));};
template<class C, typename T = TListValue<C>> concept CHasMethodAddFirst = requires(C c, T&& x) {c.AddFirst(INTRA_FWD(x));};
template<class C, typename... Args> concept CHasMethod_emplace_back = requires(C c, Args&&... args) {c.emplace_back(INTRA_FWD(args)...);};
template<class C, typename... Args> concept CHasMethodEmplaceLast = requires(C c, Args&&... args) {c.EmplaceLast(INTRA_FWD(args)...);};
template<class C, typename... Args> concept CHasMethod_emplace_front = requires(C c, Args&&... args) {c.emplace_front(INTRA_FWD(args)...);};
template<class C, typename... Args> concept CHasMethodEmplaceFirst = requires(C c, Args&&... args) {c.EmplaceFirst(INTRA_FWD(args)...);};

template<class C> concept CHasMethod_clear = requires(C c) {c.clear();};
template<class C> concept CHasMethodClear = requires(C c) {c.Clear();};
template<class C> concept CHasMethod_resize = requires(C c) {c.resize(size_t());};
template<class C> concept CHasMethodSetLength = requires(C c) {c.CHasSetLength(index_t());};

template<class C> concept CHasMethod_empty = requires(C c) {{c.empty()} -> CConvertibleTo<bool>;};
template<class C> concept CHasMethod_reserve = requires(C c) {c.reserve(size_t());};
template<class C> concept CHasMethodReserve = requires(C c) {c.Reserve(index_t());};
}

constexpr auto AddLast = [](auto&& v)
{
	return [v = INTRA_FWD(v)]<CList C>(C& container) requires z_D::CHasMethod_push_back<C> || z_D::CHasMethodAddLast<C> {
		if constexpr(z_D::CHasMethodAddLast<C>) container.AddLast(v);
		else container.push_back(v);
	};
};

constexpr auto AddLastRef = [](auto&& v)
{
	return [&]<CList C>(C& container) requires z_D::CHasMethod_push_back<C> || z_D::CHasMethodAddLast<C> {
		if constexpr(z_D::CHasMethodAddLast<C>) container.AddLast(INTRA_FWD(v));
		else container.push_back(INTRA_FWD(v));
	};
};

constexpr auto SetLength = [](Size newLength)
{
	return [newLength]<class C>(C& container) requires z_D::CHasMethod_resize<C> || z_D::CHasMethodSetLength<C> {
		if constexpr(z_D::CHasMethodSetLength<C>) container.SetLength(newLength);
		else container.resize(size_t(newLength));
	};
};

constexpr auto Reserve = [](Size capacity)
{
	return [capacity]<class C>(C& container) requires z_D::CHasMethod_reserve<C> || z_D::CHasMethodReserve<C> {
		if constexpr(z_D::CHasMethodReserve<C>) container.Reserve(capacity);
		else if constexpr(z_D::CHasMethod_reserve<C>) container.reserve(size_t(capacity));
	};
};


template<class L> concept CGrowingList = CList<L> &&
	(z_D::CHasMethod_push_back<TRemoveConstRef<L>> || z_D::CHasMethodAddLast<TRemoveConstRef<L>>) &&
	CHasLength<L> && (z_D::CHasMethod_empty<L> || z_D::CHasMethodEmpty<L>);

template<class C> concept CDynamicArrayContainer =
	CGrowingList<C> &&
	(z_D::CHasMethod_resize<TRemoveConst<C>> || z_D::CHasMethodSetLength<TRemoveConst<C>>) &&
	CHasData<C>;

template<class L> concept CStaticArrayContainer = CArrayList<L> && CStaticLengthContainer<L>;

template<class L> concept COwningList = CGrowingList<L> || CList<L> && (CStaticLengthContainer<L> || requires {L::TagOwningList::True;});



template<typename P, typename... Rs> concept CAsElementPredicate = CElementPredicate<P, TRangeOfRef<Rs>...>;
template<typename P, typename... Rs> concept CAsElementAsPredicate = CElementAsPredicate<P, TRangeOfRef<Rs>...>;

template<CRange R> struct Advance
{
	R& RangeRef;
	Advance(R& range) noexcept: RangeRef(range) {}
};
template<typename R> concept CAdvance = CInstanceOfTemplate<TUnqualRef<R>, Advance>;

template<typename L, typename F> requires CCallable<F, L> && (CList<L> || CAdvance<L>)
constexpr INTRA_FORCEINLINE decltype(auto) operator|(L&& list, F&& func) {return INTRA_FWD(func)(INTRA_FWD(list));}

template<CConsumableRange R> [[nodiscard]] constexpr auto begin(R&& range)
{
	return z_D::RangeForIterLike<TRangeOf<R>>{RangeOf(INTRA_FWD(range))};
}

template<CConsumableRange R> [[nodiscard]] constexpr auto end(R&& range) noexcept {return nullptr;}


template<typename F> [[nodiscard]] constexpr decltype(auto) FunctorOf(F&& f)
{
	if constexpr(CClass<TRemoveReference<F>>) return INTRA_FWD(f);
	else if constexpr(CFunctionPointer<F>)
	{
		return [f](auto&&... args)
			noexcept(noexcept(f(INTRA_FWD(args)...)))
			-> decltype(f(INTRA_FWD(args)...))
		{return f(INTRA_FWD(args)...);};
	}
	else if constexpr(CFieldPointer<F>)
	{
		return [f](auto&& object) noexcept -> decltype(INTRA_FWD(object).*f) {return INTRA_FWD(object).*f;};
	}
	else if constexpr(CMethodPointer<F>)
	{
		return [f](auto&& object, auto&&... args)
			noexcept(noexcept((INTRA_FWD(object).*f)(INTRA_FWD(args)...)))
			-> decltype((INTRA_FWD(object).*f)(INTRA_FWD(args)...))
		{return (INTRA_FWD(object).*f)(INTRA_FWD(args)...);};
	}
}

// If a T is a class or a struct or a lambda, the result is T.
// If it is a function pointer, a method pointer or a field pointer wraps them in a functor.
// Otherwise TFunctorOf is void
template<typename T> using TFunctorOfRef = decltype(FunctorOf(Val<T>()));
template<typename T> using TFunctorOf = TFunctorOfRef<TUnqualRef<T>>;

template<typename F, typename... Ts> concept CAsCallable = CCallable<TFunctorOf<F>, Ts...>;


#define INTRAZ_D_REFLECTION_FIELD_NAME(class, field) #field
#define INTRAZ_D_REFLECTION_FIELD_NAMES(template, T, ...) \
	template constexpr auto ReflectFieldNamesOf(TType<T>) noexcept {return Array { \
		INTRA_MACRO2_FOR_EACH((,), INTRAZ_D_REFLECTION_FIELD_NAME, T, __VA_ARGS__) \
	};}

#define INTRAZ_D_REFLECTION_FIELD_POINTER(class, field) &class::field
#define INTRAZ_D_REFLECTION_FIELD_POINTERS(template, T, ...) \
	template constexpr auto ReflectFieldPointersOf(TType<T>) noexcept {return Tuple { \
		INTRA_MACRO2_FOR_EACH((,), INTRAZ_D_REFLECTION_FIELD_POINTER, T, __VA_ARGS__)\
	};}

/** Add meta information about fields.
  The first argument is the class/struct name, next are the fields in the order of their declaration.
*/
#define INTRA_ADD_FIELD_REFLECTION(T, ...) \
	INTRAZ_D_REFLECTION_FIELD_NAMES(, T, __VA_ARGS__) \
    INTRAZ_D_REFLECTION_FIELD_POINTERS(, T, __VA_ARGS__)

template<class T> constexpr index_t ReflectSizeof = [] {
	if constexpr(CHasFieldPointersOf<T>)
	{
		index_t res = 0;
		ForEachType<TFieldTList<T>>([&]<typename Field>(TType<Field>) mutable
		{
			res += sizeof(Field);
			res = (res + alignof(Field) - 1) & (~alignof(Field) + 1);
		});
		return res;
	}
	else return -1;
}();

template<class T> constexpr index_t ReflectAlignof = [] {
	if constexpr(CHasFieldPointersOf<T>)
	{
		index_t res = 0;
		ForEachType<decltype(TFieldTList<T>)>([&]<typename Field>(TType<Field>) mutable
		{
			res = Max(res, alignof(Field));
		});
		return res;
	}
	else return -1;
}();

template<typename T> concept CReflectionMatchesSize = ReflectSizeof<T> == sizeof(T) && ReflectAlignof<T> == alignof(T);

/** CTriviallySerializable checks if a type can be trivially binary serialized and deserialized.
It assumes that the type doesn't contain any pointers. It can be checked only if the type provides reflection information.
*/
namespace z_D {
template<typename T> constexpr bool CTriviallySerializable_ = false;
template<CTriviallyCopyable T> requires(CStandardLayout<T> && (!CBasicPointer<T>))
constexpr bool CTriviallySerializable_<T> = !CHasFieldPointersOf<T> ||
	CReflectionMatchesSize<T> && TListMapReduce<TFieldTList<T>>([]<typename T>(TType<T>){return CTriviallySerializable_<T>;}, And);
}
template<typename T> concept CTriviallySerializable = z_D::CTriviallySerializable_<T>;
template<typename T> concept CSerializable = CSameUnqualRef<T, decltype(nullptr)> || CEnum<T> ||
	CNumber<T> || CStaticLengthContainer<T> || CConsumableList<T>;

template<class T> concept CIntraAware = CRange<T> || CList<T> && z_D::CHasMethodLength<T> || CInstanceOfTemplate<Tuple> || CInstanceOfTemplate<Variant>;

//template<typename T, CCallable<T> F>
//constexpr INTRA_FORCEINLINE decltype(auto) operator|(T&& obj, F&& func) {return INTRA_FWD(func)(INTRA_FWD(obj));}

} INTRA_END
