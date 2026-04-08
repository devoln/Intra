#pragma once

#include <Intra/Core.h>
#include <Intra/Preprocessor.h>
#include <Intra/Numeric/Traits.h>
#include <Intra/Numeric/Integral.h>
#include <Intra/Platform/Toolchain.h>

namespace Intra {
template<typename T, size_t N> struct Array;
template<typename... Ts> struct Tuple;
}

namespace std { //NOLINT
//// For compatibility with STL and structured bindings
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
template<typename T> concept CTupleSizeDefined = requires(T) {::std::tuple_size<T>::value;};
template<typename T> concept CTupleElementDefined = requires(T) {Val<typename ::std::tuple_element<0, TUnqualRef<T>>::type>();};
}
template<typename T> constexpr index_t StaticLength = [] {
	using T1 = TUnqualRef<T>;
	if constexpr(z_D::CTupleSizeDefined<T1>) return index_t(::std::tuple_size<T1>::value);
	else if constexpr(CHasReflectFieldPointersOf<T1>) return StaticLength<decltype(ReflectFieldPointersOf(Type<T1>))>;
	else if constexpr(CKnownBoundArrayType<T1>) return index_t(ArrayExtent<T1>);
	else return -1;
}();
template<typename T> concept CStaticLengthContainer = StaticLength<T> != -1;
template<typename T> concept CVariant = CInstanceOfTemplate<TUnqualRef<T>, Variant> || CInstanceOfTemplate<TUnqualRef<T>, ::std::variant>;

#if INTRA_CONSTEXPR_TEST
static_assert(CStaticLengthContainer<const char(&)[5]>);
static_assert(CStaticLengthContainer<char[5]>);
static_assert(!CStaticLengthContainer<const char>);
static_assert(StaticLength<const char(&)[5]> == 5);

static_assert(::std::tuple_size<Tuple<int, float, char>>::value == 3);
static_assert(z_D::CTupleSizeDefined<Tuple<int, float, char>>);
static_assert(CSame<typename ::std::tuple_element<0, Intra::Tuple<int, float, char>>::type, int>);
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
	using _ = TList<typename ::std::tuple_element<Is, T>::type...>;
};
template<CTupleSizeDefined T> requires CTupleElementDefined<T>
struct TFieldTList_<T>: TupleElementsTList_<T, TMakeIndexSeq<::std::tuple_size<T>::value>> {};
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
template<typename T> concept CHasMethod_data = requires(T x, const void*& res) {{x.data()} -> CConvertibleTo<const void*>;};
template<typename T> concept CHasMethodData = requires(T x, const void*& res) {{x.Data()} -> CConvertibleTo<const void*>;};
}
INTRA_DEFINE_SAFE_DECLTYPE(TIteratorOf, begin(Val<T>()));


constexpr auto Length = []<class L>(L&& list)
	requires z_D::CHasMethodLength<L> || z_D::CHasMethod_size<L> || CKnownBoundArrayType<TRemoveReference<L>>
{
	if constexpr(CKnownBoundArrayType<TRemoveReference<L>>) return StaticLength<L>;
	else if constexpr(z_D::CHasMethodLength<L>) return list.Length();
	else return index_t(list.size());
};

constexpr auto Data = []<class L>(L&& list)
	requires z_D::CHasMethodData<L> || z_D::CHasMethod_data<L> || CArrayType<TRemoveReference<L>>
{
	if constexpr(CArrayType<TRemoveReference<L>>) return static_cast<TRemoveReference<decltype(list[0])>*>(list);
	else if constexpr(z_D::CHasMethodData<L>) return list.Data();
	else return list.data();
};

template<class L> using TArrayElementPtr = decltype(Data(Val<L>()));
template<class L> using TArrayElementKeepConst = TRemovePointer<TArrayElementPtr<L>>;
template<class L> using TArrayElementRef = TArrayElementKeepConst<L>&;
template<class L> using TArrayListValue = TRemoveConst<TArrayElementKeepConst<L>>;

template<typename T> concept CHasData = requires(T&& x) {{Data(x)} -> CConvertibleTo<const void*>;};
template<typename T> concept CHasLength = requires(T&& x) {{Length(x)} -> CConvertibleTo<index_t>;};
template<typename R> concept CConvertibleToSpan = CHasData<R> && CHasLength<R>;
template<typename... Rs> concept CSameArrays = (CConvertibleToSpan<Rs> && ...) && CSame<TArrayListValue<Rs>...>;

template<z_D::CHasMethodData L> [[nodiscard]] constexpr auto begin(L&& list) {return list.Data();}
template<z_D::CHasMethodData L> requires z_D::CHasMethodLength<L>
[[nodiscard]] constexpr auto end(L&& list) {return list.Data() + list.Length();}

#if INTRA_CONSTEXPR_TEST
static_assert(CHasData<const char(&)[5]>);
static_assert(!CHasData<int>);
static_assert(CHasLength<const char(&)[5]>);
static_assert(CConvertibleToSpan<const char(&)[5]>);

static_assert(CSame<TArrayElementPtr<const char(&)[5]>, const char*>);
static_assert(CSame<TArrayListValue<const char(&)[5]>, char>);
static_assert(CSame<TArrayElementKeepConst<const char(&)[5]>, const char>);
#endif


template<typename R> concept CAssignableArrayList = CConvertibleToSpan<R> && !CConst<TArrayElementKeepConst<R>>;


template<typename T> concept CHasPreIncrement = requires(T x) {++x;};
template<typename T> concept CHasPostIncrement = requires(T x) {x++;};
template<typename T> concept CHasPreDecrement = requires(T x) {--x;};
template<typename T> concept CHasPostDecrement = requires(T x) {x--;};
template<typename T> concept CHasDereference = requires(T x) {*x;};

template<class R> concept CHasFirst = requires(R&& r) {r.First();};
template<class R> concept CHasPopFirst = requires(R&& r) {r.PopFirst();};
template<class R> concept CHasEmpty = requires(R&& r, bool& res) {res = r.Empty();};

template<typename R> concept CRange = CHasFirst<R> && CHasPopFirst<TRemoveConstRef<R>> && CHasEmpty<R>;
template<CRange R> using TRangeValueRef = decltype(Val<R>().First());
template<CRange R> using TRangeValue = TUnqualRef<TRangeValueRef<R>>;
template<class R> concept CHasLast = requires(R&& r, TRangeValue<R>& res) {{r.Last()} -> CSame<decltype(r.First())>;};
template<class R> concept CHasPopLast = requires(R&& r) {r.PopLast();};

template<class M, class R> concept CMatcherInput = CRange<R> && requires(M&& m, R& r) {
	{INTRA_FWD(m)(r)} -> CConvertibleTo<bool>;
};

template<class M, class R> concept CMatcher = CMatcherInput<M, R>;

template<class M> concept CInputSafeMatcher = requires {TUnqualRef<M>::TagInputSafe::True;};

template<class R> concept CHasPopFirstCount = requires(R&& r, index_t& res) {res = r.PopFirstCount(size_t());};
template<class R> concept CHasPopLastCount = requires(R&& r, index_t& res) {res = r.PopLastCount(size_t());};
template<class R> concept CHasNext = requires(R&& r, TRangeValueRef<R>& res) {res = r.Next();};
template<class R> concept CHasGetBuffer = requires(R& r) {{r.GetBuffer()} -> CConvertibleToSpan;};

template<class RSrc, class RDst> concept CHasStreamToMethod =
	requires(RSrc&& src, RDst&& dst) {INTRA_FWD(src).StreamTo(INTRA_FWD(dst));};

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

template<class L> concept CHasToRange = requires(L&& list) {{list.ToRange()} -> CRange;};
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

template<CRange R> struct Advance
{
	R& RangeRef;
	INTRA_FORCEINLINE constexpr Advance(R& range) noexcept: RangeRef(range) {}

	[[nodiscard]] INTRA_FORCEINLINE constexpr decltype(auto) First() const {return RangeRef.First();}
	INTRA_FORCEINLINE constexpr void PopFirst() {RangeRef.PopFirst();}
	[[nodiscard]] INTRA_FORCEINLINE constexpr bool Empty() const {return RangeRef.Empty();}

	constexpr INTRA_FORCEINLINE decltype(auto) Last() const requires CHasLast<R> {return RangeRef.Last();}
	constexpr INTRA_FORCEINLINE void PopLast() requires CHasPopLast<R> {RangeRef.PopLast();}
	constexpr INTRA_FORCEINLINE auto operator[](CNumber auto&& index) const -> decltype(RangeRef[index]) {return RangeRef[index];}
	[[nodiscard]] INTRA_FORCEINLINE constexpr auto Length() const requires CHasLength<R> {return RangeRef.Length();}

	[[nodiscard]] INTRA_FORCEINLINE constexpr auto PopFirstCount(CNumber auto&& numElementsToPop) requires CHasPopFirstCount<R>
	{return RangeRef.PopFirstCount(INTRA_FWD(numElementsToPop));}
	[[nodiscard]] INTRA_FORCEINLINE constexpr auto PopLastCount(CNumber auto&& numElementsToPop) requires CHasPopLastCount<R>
	{return RangeRef.PopLastCount(INTRA_FWD(numElementsToPop));}
};
template<typename R> concept CAdvance = CInstanceOfTemplate<TUnqualRef<R>, Advance>;
namespace z_D {
template<typename R> struct TApplyAdvance_: TType<R> {};
template<CAdvance R> struct TApplyAdvance_<R>: TType<decltype(Val<R>().RangeRef)> {};
}
template<typename R> using TApplyAdvance = typename z_D::TApplyAdvance_<R>::_;

template<typename T> struct Span;
template<typename Node, typename T> struct LinkedRange;

namespace z_D {
// Internal RangeOfImpl - does NOT handle owning containers
// Used to break the dependency cycle: CList -> TRangeOfImplRef -> RangeOfImpl -> (no CList dependency)
template<typename L, template<typename> class ArrayRange = Span, template<typename, typename> class LinkedRange_ = LinkedRange>
[[nodiscard]] constexpr decltype(auto) RangeOfImpl(L&& list)
{
	if constexpr(CRange<L>)
	{
		if constexpr(CConst<TRemoveReference<L>> && CForwardRange<TUnqualRef<L>>) return TUnqualRef<L>(list);
		else return INTRA_FWD(list);
	}
	else if constexpr(CConvertibleToSpan<L> && !CRValueReference<L>) return ArrayRange(list);
	else if constexpr(CLinkedList<L>) return LinkedRange_(list);
	else if constexpr(z_D::CRangeForIterableEx<L>) return IteratorRange{begin(list), end(list)};
	else if constexpr(z_D::CRangeForIterableClass<L>) return IteratorRange{list.begin(), list.end()};
	else if constexpr(z_D::CHasToRange<L>) return INTRA_FWD(list).ToRange();
	//TODO: else if constexpr(CAnyReader<L>) return ReaderToRange(INTRA_FWD(list));
}
} // namespace z_D

// Type aliases using RangeOfImpl (for concepts that need to be defined before RangeOf)
template<typename T> using TBaseRangeOfImplRef = decltype(z_D::RangeOfImpl(Val<T>()));
template<typename T> using TRangeOfImplRef = TApplyAdvance<TBaseRangeOfImplRef<T>>;

// CList and related concepts use RangeOfImpl, breaking the cycle
template<typename T> concept CList = CRange<TRangeOfImplRef<T>>;
template<typename T> concept CFiniteList = CFiniteRange<TRangeOfImplRef<T>>;
template<typename T> concept CNonFiniteList = CNonInfiniteRange<TRangeOfImplRef<T>>;
template<typename T> concept CAssignableList = CAssignableRange<TRangeOfImplRef<T>>;
template<typename T> concept CForwardList = CForwardRange<TRangeOfImplRef<T>>;
template<typename T> concept CFiniteForwardList = CFiniteForwardRange<TRangeOfImplRef<T>>;
template<typename T> concept CNonInfiniteForwardList = CNonInfiniteForwardRange<TRangeOfImplRef<T>>;
template<typename T> concept CBidirectionalList = CBidirectionalRange<TRangeOfImplRef<T>>;
template<typename T> concept CRandomAccessList = CRandomAccessRange<TRangeOfImplRef<T>>;
template<typename T> concept CFiniteRandomAccessList = CFiniteRandomAccessRange<TRangeOfImplRef<T>>;
template<typename T> concept CInfiniteList = CInfiniteRange<TRangeOfImplRef<T>>;
template<typename T> concept CCharList = CCharRange<TRangeOfImplRef<T>>;
template<typename T> concept CForwardCharList = CForwardCharRange<TRangeOfImplRef<T>>;
template<typename T> using TListValueRef = TRangeValueRef<TRangeOfImplRef<T>>;
template<typename T> using TListValue = TRangeValue<TRangeOfImplRef<T>>;
template<typename R> concept CAccessibleList = CAccessibleRange<TRangeOfImplRef<R>>;
template<typename R> concept CConsumableList = CConsumableRange<TRangeOfImplRef<R>>;
template<typename R, typename T> concept CConsumableListOf = CConsumableRangeOf<TRangeOfImplRef<R>, T>;
template<typename R> using CAccessibleListT = TValue<CAccessibleList<R>>;

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
template<class C> concept CHasMethodSetLength = requires(C c) {c.SetLength(index_t());};
template<class C> concept CHasMethodSetLengthRaw = requires(C c) {c.SetLengthRaw(Unsafe, index_t());};

template<class C> concept CHasMethod_empty = requires(C c) {{c.empty()} -> CConvertibleTo<bool>;};
template<class C> concept CHasMethod_reserve = requires(C c) {c.reserve(size_t());};
template<class C> concept CHasMethodReserve = requires(C c) {c.Reserve(index_t());};
}

template<class L> concept CGrowingList = CList<L> &&
	z_D::CHasMethod_push_back<TRemoveConstRef<L>> &&
	CHasLength<L> && z_D::CHasMethod_empty<L>;

template<typename T> using TBaseRangeOfRef = decltype(RangeOf(Val<T>()));
template<typename T> using TRangeOfRef = TApplyAdvance<TBaseRangeOfRef<T>>;
template<typename T> using TRangeOf = TApplyAdvance<TUnqualRef<TBaseRangeOfRef<T>>>;
template<typename T> concept CHasRangeOf = !CVoid<TRangeOfRef<T>>;

template<class L> concept COwningList = CGrowingList<L> || CList<L> && (CStaticLengthContainer<L> || requires {L::TagOwningList::True;});

template<class L> concept CViewList = CList<L> && requires {TUnqualRef<L>::TagViewList::True;};
template<class R> concept CViewRange = CViewList<R> && CRange<R>;

// Forward declaration for OwningRange (defined after RangeOf since it uses RangeOf internally)
template<typename C> struct OwningRange;

INTRA_DEFINE_SAFE_DECLTYPE(TRawUnicodeUnit, RangeOf(Val<T>().RawUnicodeUnits()).First());
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
	CAssignableRange<O> && CAssignable<TListValue<O>, T>;

template<class O> concept CCharOutput = COutputOf<O, char8_t>;
template<class O> concept COutput = CCharOutput<O> || COutputOf<O, TListValue<O>> || COutputOf<O, typename O::value_type>;

template<typename R, typename T> concept COutputBufferOf = COutputOf<R, T> && CHasReset<R> && CHasWrittenRange<R>;

template<typename L> [[nodiscard]] constexpr decltype(auto) OutputOf(L&& listOrOutput)
{
	if constexpr(CAssignableList<L>) return RangeOf(INTRA_FWD(listOrOutput));
	else return INTRA_FWD(listOrOutput);
}

template<class C> concept CDynamicArrayContainer =
	CGrowingList<C> &&
	(z_D::CHasMethod_resize<TRemoveConst<C>> || z_D::CHasMethodSetLength<TRemoveConst<C>>) &&
	CHasData<C>;

template<class C> concept CResizableArrayContainer = CConvertibleToSpan<C> &&
	(z_D::CHasMethod_resize<TRemoveConst<C>> || z_D::CHasMethodSetLength<TRemoveConst<C>>);

template<class L> concept CStaticArrayContainer = CConvertibleToSpan<L> && CStaticLengthContainer<L>;

// TODO: split into INTRA_LIFETIMEBOUND and non-INTRA_LIFETIMEBOUND versions
template<typename L, template<typename> class ArrayRange = Span, template<typename, typename> class LinkedRange = LinkedRange>
[[nodiscard]] constexpr decltype(auto) RangeOf(INTRA_LIFETIMEBOUND L&& list)
{
	if constexpr(CRValueReference<L> && COwningList<L>)
		return OwningRange<TRemoveReference<L>>{INTRA_FWD(list)};
	else return z_D::RangeOfImpl<L, ArrayRange, LinkedRange>(INTRA_FWD(list));
}

// OwningRange: Stores container by value, provides range interface
// Used for rvalue owning containers (vector, ArrayList) to avoid dangling references
template<typename C> struct OwningRange
{
	C container;
	
	using ContainerRange = decltype(RangeOf(Val<C>()));
	using TagAnyInstanceFinite = TTag<CFiniteRange<ContainerRange>>;
	using TagAnyInstanceInfinite = TTag<CInfiniteRange<ContainerRange>>;
	using TagOwningList = TTag<true>;
	
	[[nodiscard]] constexpr decltype(auto) First() {return RangeOf(container).First();}
	[[nodiscard]] constexpr bool Empty() {return RangeOf(container).Empty();}
	constexpr void PopFirst() { RangeOf(container).PopFirst(); }
	
	[[nodiscard]] constexpr decltype(auto) Last() requires CHasLast<C> {return RangeOf(container).Last();}
	constexpr void PopLast() requires CHasPopLast<C> { RangeOf(container).PopLast(); }
	
	[[nodiscard]] constexpr auto Length() requires CHasLength<C> {return RangeOf(container).Length();}
	[[nodiscard]] constexpr decltype(auto) operator[](CNumber auto&& index) requires CHasIndex<C> {return RangeOf(container)[INTRA_FWD(index)];}
};

template<CRandomAccessList L> struct RIndexedRef
{
	using TagAnyInstanceFinite = TTag<CFiniteList<L>>;
	using TagAnyInstanceInfinite = TTag<CInfiniteList<L>>;
	using TagViewList = TTag<>;

	L* OriginalList = nullptr;
	size_t BeginIndex = 0;
	INTRA_NO_UNIQUE_ADDRESS TConditionalField<size_t, CHasLength<L>> EndIndex;

	constexpr RIndexedRef() = default;
	constexpr RIndexedRef(L& list): OriginalList(&list)
	{
		if constexpr(CHasLength<L>) EndIndex = size_t(Intra::Length(list));
	}

	[[nodiscard]] constexpr bool Empty() const
	{
		if constexpr(CHasLength<L>) return BeginIndex >= EndIndex;
		else return false;
	}

	[[nodiscard]] constexpr decltype(auto) First() const {INTRA_PRECONDITION(!Empty()); return (*OriginalList)[BeginIndex];}
	constexpr void PopFirst() {INTRA_PRECONDITION(!Empty()); BeginIndex++;}

	[[nodiscard]] constexpr decltype(auto) Last() const requires CHasLength<L>
	{
		INTRA_PRECONDITION(!Empty());
		return (*OriginalList)[EndIndex - 1];
	}

	constexpr void PopLast() requires CHasLength<L> {INTRA_PRECONDITION(!Empty()); EndIndex--;}

	[[nodiscard]] constexpr decltype(auto) operator[](CNumber auto&& index) const
	{
		INTRA_PRECONDITION(!Empty());
		return (*OriginalList)[BeginIndex + INTRA_FWD(index)];
	}

	[[nodiscard]] constexpr auto Length() const requires CHasLength<L> {return EndIndex - BeginIndex;}
};

template<CRandomAccessList L> RIndexedRef(L&) -> RIndexedRef<L>;

template<CList L> [[nodiscard]] constexpr auto ViewRangeOf(L&& list)
{
	using TL = TRemoveReference<L>;
	if constexpr(CRValueReference<L>) return RangeOf(INTRA_FWD(list));
	else if constexpr(CConvertibleToSpan<L>) return Span(list);
	else if constexpr(CRandomAccessList<TL>) return RIndexedRef<TL>(list);
	else return RangeOf(INTRA_FWD(list));
}

template<typename L, typename F> requires CCallable<F, L> && CList<L>
INTRA_FORCEINLINE constexpr decltype(auto) operator|(L&& list, F&& func) {return INTRA_FWD(func)(INTRA_FWD(list));}

template<CConsumableRange R> [[nodiscard]] constexpr auto begin(R&& range)
{
	return z_D::RangeForIterLike<TRangeOf<R>>{RangeOf(INTRA_FWD(range))};
}

template<CConsumableRange R> [[nodiscard]] constexpr auto end(R&&) noexcept {return nullptr;}

template<typename F> [[nodiscard]] constexpr decltype(auto) FunctorOf(F&& f)
{
	if constexpr(CClass<TRemoveReference<F>>)
	{
		if constexpr(CLValueReference<F> || !CMoveConstructible<TRemoveReference<F>>) return TRemoveReference<F>(f);
		else return TRemoveReference<F>(INTRA_FWD(f));
	}
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

template<typename P, typename... Ls> concept CElementPredicate = CSame<bool, TResultOfOrVoid<TFunctorOf<P>, TListValue<Ls>...>>;

template<typename F, typename... Ts> concept CAsCallable = CCallable<TFunctorOf<F>, Ts...>;

#define INTRAZ_D_REFLECTION_FIELD_NAME(class, field) #field
#define INTRAZ_D_REFLECTION_FIELD_NAMES(template, T, ...) \
	template constexpr auto ReflectFieldNamesOf(TType<T>) noexcept {return Array { \
		INTRA_MACRO2_FOR_EACH((,), INTRAZ_D_REFLECTION_FIELD_NAME, T, __VA_ARGS__) \
	};}

#define INTRAZ_D_REFLECTION_FIELD_POINTER(class, field) &class::field
#define INTRAZ_D_REFLECTION_FIELD_POINTERS(template, T, ...) \
	template constexpr auto ReflectFieldPointersOf(TType<T>) noexcept {return Tuple { \
		INTRA_MACRO2_FOR_EACH((,), INTRAZ_D_REFLECTION_FIELD_POINTER, T, __VA_ARGS__) \
	};}

/// @brief Add meta information about fields.
/// The first argument is the class/struct name, next are the fields in the order of their declaration.
#define INTRA_ADD_FIELD_REFLECTION(T, ...) \
	INTRAZ_D_REFLECTION_FIELD_NAMES(, T, __VA_ARGS__) \
    INTRAZ_D_REFLECTION_FIELD_POINTERS(, T, __VA_ARGS__)

template<class T> constexpr index_t ReflectSizeof = [] {
	if constexpr(CHasReflectFieldPointersOf<T>)
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
	if constexpr(CHasReflectFieldPointersOf<T>)
	{
		index_t res = 0;
		ForEachType<TFieldTList<T>>([&]<typename Field>(TType<Field>) mutable
		{
			res = Max(res, index_t(alignof(Field)));
		});
		return res;
	}
	else return -1;
}();

template<typename T> concept CReflectionMatchesSize = ReflectSizeof<T> == sizeof(T) && ReflectAlignof<T> == alignof(T);

/// @brief CTriviallySerializable checks if a type can be trivially binary serialized and deserialized.
/// It assumes that the type doesn't contain any pointers. It can be checked only if the type provides reflection information.
namespace z_D {
template<typename T> constexpr bool CTriviallySerializable_ = false;

template<CTriviallyCopyable T> requires CStandardLayout<T> && (!CBasicPointer<T>)
constexpr bool CTriviallySerializable_<T> = !CHasReflectFieldPointersOf<T>;

template<CTriviallyCopyable T> requires CStandardLayout<T> && (!CBasicPointer<T>) && CReflectionMatchesSize<T>
constexpr bool CTriviallySerializable_<T> = TListMapReduce<TFieldTList<T>>([]<typename T1>(TType<T1>) {return CTriviallySerializable_<T1>;}, And);
}
template<typename T> concept CTriviallySerializable = z_D::CTriviallySerializable_<T>;
static_assert(CTriviallySerializable<float>);
template<typename T> concept CSerializable = CSameUnqualRef<T, decltype(nullptr)> || CEnum<T> ||
	CNumber<T> || CStaticLengthContainer<T> || CConsumableList<T>;

template<class T> concept CIntraAware = CRange<T> || CList<T> && z_D::CHasMethodLength<T> || CInstanceOfTemplate<T, Tuple> || CInstanceOfTemplate<T, Variant>;

template<typename T, CCallable<T> F> requires CInstanceOfTemplate<T, Tuple> || CInstanceOfTemplate<T, Variant>
constexpr INTRA_FORCEINLINE decltype(auto) operator|(T&& obj, F&& func) {return INTRA_FWD(func)(INTRA_FWD(obj));}


template<CEnum T> constexpr index_t EnumLength = index_t(T::EnumLength);

template<CEnum Key, typename Value> struct EnumConvertArray: Array<Value, EnumLength<Key>>
{
	template<typename... Args> constexpr EnumConvertArray(Args&&... args):
		Array<Value, EnumLength<Key>>(INTRA_FWD(args)...) {
		static_assert(sizeof...(args) == EnumLength<Key>);
	};
};

template<CEnum T> requires requires {size_t(T::EnumLength); }
constexpr Array<const char*, size_t(T::EnumLength)> EnumNamesCStr;

enum class SeekOrigin: uint64 {Start, Current, End, EnumLength};
struct SeekParams
{
	int64 Offset: 61 = 0;
	SeekOrigin Origin: 2 = {};
	uint64 DiscardEffect: 1 = false; // don't modify position, only return result

	void ToAbsolute(int64 currentPosition, int64 length = -1);
};
static_assert(sizeof(SeekParams) == sizeof(int64));
template<class T> concept CSeekable = requires(T&& r, SeekParams pos) {{r.Seek(pos).Unwrap()} -> CIntegral;};


/// Non-owning reference to an array.
template<typename T> struct Span
{
	Span() = default;
	Span(const Span&) = default;

	template<CConvertibleToSpan L> requires (!CSameUnqualRef<L, Span>)
	INTRA_FORCEINLINE constexpr Span(INTRA_LIFETIMEBOUND L&& arr) noexcept: Span(Unsafe, Intra::Data(arr), Intra::Length(arr)) {}

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

INTRA_DEFINE_FUNCTOR(ConstSpanOf)<CConvertibleToSpan L>(INTRA_LIFETIMEBOUND L&& list) noexcept {return Span<const TArrayListValue<L>>(list);};

template<typename T> concept CSpan = CInstanceOfTemplate<TUnqualRef<T>, Span>;


/// Trivially relocatable is a less constrained concept than trivially copyable.
/// All trivially copyable types are also trivially relocatable.
/// However there may be types having move constructor and destructor that
///  are not trivial separately but combination of them may be trivial.
/// It is true for most containers. You can make a bitwise copy of a
///  container object without calling the move constructor and the destructor of source.
/// Specialize IsTriviallyRelocatable for such types after their definition.
template<typename T> constexpr bool IsTriviallyRelocatable = CTriviallyCopyable<T> || requires {T::TagTriviallyRelocatable::True;};
template<typename T> concept CTriviallyRelocatable = IsTriviallyRelocatable<T>;

template<class T> using TPostRelocateFixSignature = void(T* newBaseAddress, const T* oldFreedBaseAddress, size_t numElementsToPatch);

/// Non-trivially relocatable types that support post-relocation fixes must specialize this function pointer.
template<class T> constexpr auto FPostRelocateFix = Undefined;
template<CTriviallyRelocatable T> constexpr TPostRelocateFixSignature<T>* FPostRelocateFix<T> = nullptr; // nullptr means that patching is not required in this case

/// Raw allocators are realloc-like functions that also support memory zeroing and custom relocation for C++ classes.
/// Structure with parameters to pass to a type-safe allocation functions.
template<typename T> struct AllocParams
{
	/// The block previously allocated by this allocator to grow, shrink, free or to do the special command with.
	/// Length() may be smaller than the original NumElements, in this case all the elements beyond Length() are expected to be uninitialized or destructed by the caller.
	/// After the reallocation is done their values will be either undefined or value-initialized depending on ValueInitialize.
	Owner<Span<T>> ExistingMemoryBlock;

	/// Initialize newly allocated memory elements with default values by either
	///  calling default constructor or zero initialization. Otherwise the memory content will be undefined.
	size_t ValueInitialize: 1 = false;

	/// Minimum allocation size to request. Pass 0 to free the ptr.
	///  Note: Values above MaxValidNumElements are reserved and must not be used directly.
	///  Note: Largest possible 256 values above are invalid and are treated as an integer underflow bug.
	size_t NumElements: sizeof(size_t)*8 - 1 = 0;

	/// How many elements of ExistingMemoryBlock to keep.
	/// All elements having the index starting this number are expected to be already destructed by the caller.
	size_t NumPrevElementsToKeep = 0;
};

struct RawAllocParams
{
	AllocParams<char> ByteAllocParams;

	/// This function is used during relocation to copy. For most types this task can be trivially done with memcpy.
	/// But if a type saves or passes pointers to itself then using memcpy would result in dangling references.
	using TCustomRelocateSignature = void(char* dst, char* src, size_t numBytes);

	/// Function to use instead of memcpy to copy params.NumPrevElementsToKeep from previous allocation.
	/// It's recommended to pass a non-null pointer only when the result of memcpy would be incorrect.
	TCustomRelocateSignature* CustomRelocateFunc;

	/// CRawAllocator allows to request info about it or its allocations by using reserved values in NumElements field.
	/// If the operation is supported the result gets written back to numBytes. Otherwise numBytes isn't modified.
	enum class Cmd {
		Allocate,
		GetAllocatorInfo, // returns AllocatorInfo struct
		GetSize, // returns existing allocation size in bytes in RawValue[0]
		EnumLength
	};

	static RawAllocParams MakeCommand(Cmd cmd, void* existingMemoryBlockBegin = nullptr)
	{
		return {
			.ByteAllocParams = {
				.ExistingMemoryBlock = Span(Unsafe, static_cast<char*>(existingMemoryBlockBegin), Size(0)),
				.NumElements = MaxValidNumElements + size_t(cmd)
		}
		};
	}

	Cmd GetCommand() const
	{
		INTRA_PRECONDITION(ByteAllocParams.NumElements < MaxRepresentableNumElements - 255); // catch a size integer underflow
		if(ByteAllocParams.NumElements <= MaxValidNumElements) return Cmd::Allocate;
		return Cmd(ByteAllocParams.NumElements - MaxValidNumElements);
	}

private:
	static constexpr size_t MaxValidNumElements = (size_t() - (1519 << 1)) >> 1;
	static constexpr size_t MaxRepresentableNumElements = (size_t() - 1) >> 1;
};

struct AllocatorInfo
{
	bool HoldsAllocationSize: 1; // means that AllocatorCmd::GetSize will work
	bool SupportsFastMemZero: 1; // means that ValueInitialize may be faster than memset for large allocations
	bool SupportsFree: 1; // free is never a no-op, otherwise it's necessary to delete the allocator to free all the memory (false for linear or stack allocators)
	bool SupportsReallocInPlace: 1; // may sometimes grow or shrink allocations without copying the data
	bool CanHoldDebugSourceInfo: 1; // can hold file and line of allocation site
	bool NoDebugSentinels: 1; // can't be false when SmallAllocationOffsetBytes is non-zero, so this invalid combination means that the struct needs to be initialized
	bool FixedSizePool: 1; // if true, the only supported size can be calculated as (1 << MinimumGranularityShift) - SmallAllocationOffsetBytes
	bool ThreadSafe: 1; // can be used from multiple threads without any external synchronization
	uint8 GuaranteedAlignmentShift: 4; // 1 << GuaranteedAlignmentShift is the value of guaranteed alignment in bytes of all allocations
	uint8 MinimumGranularityShift: 4; // 1 << MinimumGranularityShift is the minimum step in bytes between possible allocation sizes
	uint8 SmallAllocationOffsetBytes; // total per-allocation overhead stored inside the same block next to the user data for smallest allocations

	explicit operator bool() const
	{
		static constexpr AllocatorInfo empty = {};
		return __builtin_memcmp(this, &empty, sizeof(*this)) == 0;
	}
};

INTRA_IGNORE_WARN_DEFAULT_CTOR_IMPLICITLY_DELETED
union RawAllocResult
{
	Owner<Span<char>> Allocation;
	size_t RawValue[2];
	AllocatorInfo Info;
};

using TRawAllocatorSignature = RawAllocResult(RawAllocParams params);
template<class A> concept CAllocator = CCallableWithSignature<A, TRawAllocatorSignature>;
template<class A> concept COptAllocator = CAllocator<A> || CSame<A, TUndefined>;

struct PlatformTunedByteAllocator {
	RawAllocResult operator()(RawAllocParams);
};
using DefaultAllocator = PlatformTunedByteAllocator;
static_assert(requires(RawAllocResult(*f)(RawAllocParams), RawAllocParams p) {f(p);});
static_assert(CCallable<RawAllocResult(*)(RawAllocParams), RawAllocParams>);
static_assert(CCallableWithSignature<DefaultAllocator, RawAllocResult(RawAllocParams params)>);
static_assert(CAllocator<DefaultAllocator>);

template<typename T> concept CPoolAllocator = requires(T allocator, SourceInfo allocatedAt)
{
	allocator.FreeOne(allocator.AllocateOne(allocatedAt));
};

template<typename T> union ReadResult;
template<typename T> struct ReadParams;
template<typename T> using TGenericReaderSignature = ReadResult<T>(ReadParams<T> params);
template<class R, typename T> concept CGenericReader = CCallableWithSignature<R, TGenericReaderSignature<T>>;
template<class R> concept CReader = CGenericReader<R, char>;
} INTRA_END
