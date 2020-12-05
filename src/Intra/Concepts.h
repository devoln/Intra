#pragma once

#include "Intra/Type.h"
#include "Intra/Preprocessor.h"
#include "Intra/Assert.h"

namespace Intra {
template<typename... Ts> struct Tuple;
template<typename T, size_t N> struct Array;
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
template<size_t I, typename... Ts>
struct tuple_element<I, Intra::Tuple<Ts...>> {using type = Intra::TPackAt<I, Ts...>;};
template<size_t I, typename... Ts>
struct tuple_element<I, const Intra::Tuple<Ts...>> {using type = const Intra::TPackAt<I, Ts...>;};

template<typename T, size_t N> struct tuple_size<Intra::Array<T, N>> {static constexpr size_t value = N;};
template<typename T, size_t N> struct tuple_size<const Intra::Array<T, N>>: tuple_size<Intra::Array<T, N>> {};
template<size_t I, typename T, size_t N> struct tuple_element<I, Intra::Array<T, N>> {using type = T;};
template<size_t I, typename T, size_t N> struct tuple_element<I, const Intra::Array<T, N>> {using type = const T;};
}

INTRA_BEGIN
constexpr void z_D_ADL_FieldPointersOf(const void*);
constexpr void z_D_ADL_FieldNamesOf(const void*);
template<class T> concept CHasFieldPointersOf = !CVoid<decltype(z_D_ADL_FieldPointersOf(Val<const T*>()))>;
template<class T> concept CHasFieldNamesOf = !CVoid<decltype(z_D_ADL_FieldNamesOf(Val<const T*>()))>;

#define INTRAZ_D_REFLECTION_FIELD_NAME(class, field) #field
#define INTRAZ_D_REFLECTION_FIELD_NAMES(implName, template, T, ...) \
	namespace {template constexpr const char* implName[] = { \
		INTRA_MACRO2_FOR_EACH((,), INTRAZ_D_REFLECTION_FIELD_NAME, T, __VA_ARGS__) \
	};} \
	template constexpr decltype(auto) z_D_ADL_FieldNamesOf(const T&) noexcept {return static_cast< \
		const char* const(&)[sizeof(implName)/sizeof(implName[0])]>(implName);\
	}

#define INTRAZ_D_REFLECTION_FIELD_POINTER(class, field) &class::field
#define INTRAZ_D_REFLECTION_FIELD_POINTERS(template, T, ...) \
	template constexpr auto z_D_ADL_FieldPointersOf(const T&) noexcept {return Tuple( \
		INTRA_MACRO2_FOR_EACH((,), INTRAZ_D_REFLECTION_FIELD_POINTER, T, __VA_ARGS__)\
	);}

/** Add meta information about fields.
  The first argument is the class/struct name, next are the fields in the order of their declaration.
*/
#define INTRA_ADD_FIELD_REFLECTION(T, ...) \
	INTRAZ_D_REFLECTION_FIELD_NAMES(INTRA_CONCATENATE_TOKENS(z_D_ADL_FieldNames_, __COUNTER__),, T, __VA_ARGS__) \
    INTRAZ_D_REFLECTION_FIELD_POINTERS(, T, __VA_ARGS__)

template<class T> constexpr decltype(auto) FieldPointersOf = z_D_ADL_FieldPointersOf(static_cast<const T*>(nullptr));
template<class T> constexpr decltype(auto) FieldNamesOf = z_D_ADL_FieldNamesOf(static_cast<const T*>(nullptr));

namespace z_D {
template<typename T> concept CTupleSizeDefined = requires(T) {std::tuple_size<T>::value;};
template<typename T> concept CTupleElementDefined = requires(T) {Val<typename std::tuple_element<0, T>::type>();};
template<typename T, int Category = CTupleSizeDefined<T>? 1: CHasFieldPointersOf<T>? 2: 0>
constexpr index_t StaticLength_ = -1;
template<typename T> constexpr index_t StaticLength_<T, 1> = index_t(std::tuple_size<T>::value);
template<typename T> constexpr index_t StaticLength_<T, 2> = StaticLength_<decltype(FieldPointersOf<T>)>;
template<typename T, size_t N> constexpr index_t StaticLength_<T[N], 0> = N;
}
template<typename T> constexpr index_t StaticLength = z_D::StaticLength_<TUnqualRef<T>>;
template<typename T> concept CStaticLengthContainer = StaticLength<T> != -1;

#if INTRA_CONSTEXPR_TEST
static_assert(CStaticLengthContainer<const char(&)[5]>);
static_assert(CStaticLengthContainer<char[5]>);
static_assert(!CStaticLengthContainer<const char>);
static_assert(StaticLength<const char(&)[5]> == 5);

static_assert(std::tuple_size<Tuple<int, float, char>>::value == 3);
static_assert(z_D::CTupleSizeDefined<Tuple<int, float, char>>);
static_assert(CStaticLengthContainer<Tuple<int, float, char>>);
static_assert(StaticLength<Tuple<int, float, char>> == 3);
#endif

namespace z_D {
template<class T, int = CTupleElementDefined<T> && CTupleSizeDefined<T>? 1: CHasFieldPointersOf<T>? 2: 0>
struct TFieldTList_ {using _ = void;};
template<typename... Ts> struct TFieldTList_<Tuple<Ts...>, 0> {using _ = TList<Ts...>;};
template<class T, class Seq> struct TupleElementsTList_;
template<class T, size_t... Is> struct TupleElementsTList_<T, TIndexSeq<Is...>>
{
using _ = TList<typename std::tuple_element<Is, T>::type...>;
};
template<class T> struct TFieldTList_<T, 1>: TupleElementsTList_<T, TMakeIndexSeq<std::tuple_size<T>::value>> {};
template<class T> struct TFieldTList_<T, 2>
{
    using FieldPointersTuple = decltype(FieldPointersOf<T>);
    using FieldPointersTList = typename TFieldTList_<TUnqualRef<FieldPointersTuple>>::_;
    using _ = TListTransform<FieldPointersTList, TMemberFieldType>;
};
}
template<class SLC> using TFieldTList = typename z_D::TFieldTList_<TUnqualRef<SLC>>::_;
template<class SLC> using TQualRefFieldTList = TListTransform<TFieldTList<SLC>, TPropagateQualLVRef, SLC>;

template<class StaticLengthContainer, size_t End = StaticLength<StaticLengthContainer>, size_t Start = 0>
using TQualRefFieldTListSlice = TListSlice<TQualRefFieldTList<StaticLengthContainer>, Start, End>;

#if INTRA_CONSTEXPR_TEST
static_assert(CSame<TQualRefFieldTList<const Tuple<int, float>&>, TList<const int&, const float&>>);
static_assert(CSame<TQualRefFieldTListSlice<const Tuple<int, float>&, 1>, TList<const int&>>);
static_assert(CTListConvertible<TList<int>, TList<float>>);
static_assert(!CTListConvertible<int, float>);
#endif

template<class SLC, class MapFunc>
using TMappedFieldTList = TListTransform<TQualRefFieldTList<SLC>, TResultOfOrVoid, MapFunc>;
template<class SLC, class MapFunc>
using TMappedFieldCommon = TListCommon<TMappedFieldTList<SLC, MapFunc>>;


template<typename L, typename R = L> concept CEqualityComparable = requires(L lhs, R rhs) {lhs == rhs;};
template<typename L, typename R = L> concept CNonEqualityComparable = requires(L lhs, R rhs) {lhs != rhs;};

template<typename T> concept CMovable = CObject<T> && CMoveConstructible<T> && CMoveAssignable<T>;
template<typename T> concept CCopyable = CCopyConstructible<T> && CMovable<T> && CCopyAssignable<T>;
template<typename T> concept CSemiregular = CCopyable<T> && CConstructible<T>;
template<typename T> concept CRegular = CSemiregular<T> && CEqualityComparable<T>;

template<typename T> concept CHas_size = requires(T x, size_t& res) {res = x.size();};
template<typename T> concept CHasLength = requires(T x, index_t& res) {res = x.Length();};
template<typename T> concept CHas_data = requires(T x, const void*& res) {res = x.data();};
template<typename T> concept CHasData = requires(T x, const void*& res) {res = x.Data();};
INTRA_DEFINE_SAFE_DECLTYPE(TIteratorOf, begin(Val<T>()));


template<class T> requires CHasLength<T> || CHas_size<T> || CStaticLengthContainer<T>
constexpr index_t LengthOf(T&& list)
{
	if constexpr(CStaticLengthContainer<T>) return StaticLength<T>;
	else if constexpr(CHasLength<T>) return list.Length();
	else return index_t(list.size());
}

template<class R> requires CHasData<R> || CHas_data<R> || CPointer<TIteratorOf<R>>
constexpr auto DataOf(R&& r)
{
	if constexpr(CHasData<R>) return r.Data();
	else if constexpr(CHas_data<R>) return r.data();
	else return r.begin();
}
template<typename T, size_t N> constexpr T* DataOf(T(&arr)[N]) {return arr;}

template<typename T> concept CHasDataOf = requires(T&& x, const void*& res) {res = DataOf(x);};
template<typename T> concept CHasLengthOf = requires(T&& x, index_t& res) {res = LengthOf(x);};
template<typename R> concept CArrayList = CHasDataOf<R> && CHasLengthOf<R>;
template<typename... Rs> concept CSameArrays = (CArrayList<Rs> && ...) && CSame<TArrayElement<Rs>...>;

template<CArrayList R> [[nodiscard]] constexpr auto begin(R&& range) {return DataOf(range);}
template<CArrayList R> [[nodiscard]] constexpr auto end(R&& range) {return DataOf(range) + LengthOf(range);}

INTRA_DEFINE_SAFE_DECLTYPE(TArrayElementPtr, DataOf(Val<T>()));
template<typename T> using TArrayElementKeepConst = TRemovePointer<TArrayElementPtr<T>>;
template<typename T> using TArrayElementRef = TArrayElementKeepConst<T>&;
template<typename T> using TArrayElement = TRemoveConst<TArrayElementKeepConst<T>>;

#if INTRA_CONSTEXPR_TEST
static_assert(CHasDataOf<const char(&)[5]>);
static_assert(!CHasDataOf<int>);
static_assert(CHasLengthOf<const char(&)[5]>);
static_assert(CArrayList<const char(&)[5]>);

static_assert(CSame<TArrayElementPtr<const char(&)[5]>, const char*>);
static_assert(CSame<TArrayElement<const char(&)[5]>, char>);
static_assert(CSame<TArrayElementKeepConst<const char(&)[5]>, const char>);
#endif


/// Check if Rhs elements can be assigned to Lhs elements. false, if at least one of them is not an CArrayList.
template<typename Lhs, typename Rhs> concept CAssignableArrays =
	CSameNotVoid<TArrayElementKeepConst<Lhs>, TArrayElementKeepConst<Rhs>> ||
	CSameNotVoid<TArrayElementKeepConst<Lhs>, TArrayElementKeepConst<Rhs>>;

template<typename Lhs, typename T> concept CAssignableToArrayOf =
	CSame<T, TArrayElementKeepConst<Lhs>> ||
	CSame<TRemoveConst<T>, TArrayElementKeepConst<Lhs>>;

template<typename R> concept CAssignableArrayList = CArrayList<R> && !CConst<TArrayElementKeepConst<R>>;


template<typename T> concept CHasPreIncrement = requires(T x) {++x;};
template<typename T> concept CHasPostIncrement = requires(T x) {x++;};
template<typename T> concept CHasPreDecrement = requires(T x) {--x;};
template<typename T> concept CHasPostDecrement = requires(T x) {x--;};
template<typename T> concept CHasDereference = requires(T x) {*x;};



INTRA_DEFINE_SAFE_DECLTYPE(TRangeValueRef, Val<T>().First());
template<typename R> using TRangeValue = TRemoveConstRef<TRangeValueRef<R>>;

template<class R> concept CHasFirst = requires(R&& r, auto& res) {res = r.First();};
template<class R> concept CHasPopFirst = requires(R&& r) {r.PopFirst();};
template<class R> concept CHasEmpty = requires(R&& r, bool& res) {res = r.Empty();};
template<class R> concept CHasLast = requires(R&& r, auto& res) {res = r.Last();};
template<class R> concept CHasPopLast = requires(R&& r) {r.PopLast();};
template<class R> concept CHasIndex = requires(R&& r) {r[index_t()];};

template<class R> concept CHasPopFirstCount = requires(R&& r, index_t& res) {res = r.PopFirstCount(size_t());};
template<class R> concept CHasPopLastCount = requires(R&& r, index_t& res) {res = r.PopLastCount(size_t());};
template<class R> concept CHasNext = requires(R&& r, TRangeValueRef<R>& res) {res = r.Next();};

template<class RSrc, class RDst> concept CHasReadWriteMethod =
	requires(RSrc&& src, RDst&& dst) {INTRA_FWD(src).ReadWrite(INTRA_FWD(dst));};

template<typename R> concept CRange = /*CConstructible<R> &&*/
	CHasFirst<R> && CHasPopFirst<TRemoveConst<R>> && CHasEmpty<R>;

template<typename R> concept CForwardRange = CRange<R> && CCopyConstructible<TRemoveReference<R>>;
template<typename R> concept CBidirectionalRange = CForwardRange<R> && CHasLast<R> && CHasPopLast<TRemoveConst<R>>;
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

template<class R, class RSrc> concept CHasPutAllAdvanceMethod =
	requires(R&& dst, RSrc&& src) {INTRA_FWD(dst).PutAllAdvance(INTRA_FWD(src));};
template<class R, typename T> concept CHasPut = requires(R&& r, T&& val) {r.Put(val);};
template<class R> concept CHasReset = requires(R&& r) {r.Reset();};
INTRA_DEFINE_SAFE_DECLTYPE(TWrittenRangeType, Val<T>().WrittenRange());

template<class R> concept CHasFull = requires(R&& r, bool& res) {res = r.Full();};
template<class R> concept CHasWrittenRange = CRange<TWrittenRangeType<R>>;

template<typename T> concept CCharOutput = CHasPut<T, char32_t>;
template<typename R, typename T> concept COutputOf = CHasPut<R, T> ||
	CAssignableRange<R> && CMoveAssignable<TRangeValue<R>, T>;

template<typename R, typename T> concept COutputBufferOf =
	COutputOf<R, T> && CHasReset<R> && CHasWrittenRange<R>;

template<typename P, typename... R> concept CElementPredicate =
	CSame<bool, TResultOfOrVoid<P, TRangeValue<R>...>>;

template<typename P, typename... R> concept CElementAsPredicate =
	CSame<bool, TResultOfOrVoid<TFunctorOf<P>, TRangeValue<R>...>>;


template<typename T1, typename T2 = T1> concept CHasIntegralDifference =
	requires(T1 x, T2 y, index_t& res) {res = x - y;};

namespace z_D {
template<typename R> struct RangeForIterLike
{
	constexpr RangeForIterLike& operator++() {Range.PopFirst(); return *this;}
	constexpr TRangeValueRef<R> operator*() const {return Range.First();}
	constexpr bool operator!=(decltype(null)) const {return !Range.Empty();}
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
template<typename R, template<typename> class ArrayRange = Span, template<typename, typename> class LinkedRange = LinkedRange>
[[nodiscard]] constexpr decltype(auto) RangeOf(R&& r)
{
	if constexpr(CRange<R> || COutput<R>)
	{
		if constexpr(CConst<TRemoveReference<R>> && CForwardRange<TUnqualRef<R>>) return Dup(r);
		else return INTRA_FWD(r);
	}
	else if constexpr(COutput<TUnqualRef<R>> && CCopyConstructible<R>) return r;
	else if constexpr(CArrayList<R>) return ArrayRange(r);
	else if constexpr(CLinkedList<R>) return LinkedRange(r);
	else if constexpr(z_D::CRangeForIterableEx<R>) return IteratorRange{begin(r), end(r)};
	else if constexpr(z_D::CRangeForIterableClass<R>) return IteratorRange{r.begin(), r.end()};
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

template<typename P, typename... Rs> concept CAsElementPredicate = CElementPredicate<P, TRangeOfRef<Rs>...>;
template<typename P, typename... Rs> concept CAsElementAsPredicate = CElementAsPredicate<P, TRangeOfRef<Rs>...>;


template<CList L> constexpr decltype(auto) ForwardAsRange(TRemoveReference<L>& t)
{
	return RangeOf(static_cast<L&&>(t));
}
template<CList L> constexpr decltype(auto) ForwardAsRange(TRemoveReference<L>&& t)
{
	static_assert(!CLValueReference<L>, "Bad ForwardAsRange call!");
	return RangeOf(static_cast<L&&>(t));
}

template<CRange R> struct Advance
{
	R& RangeRef;
	Advance(R& range) noexcept: RangeRef(range) {}
};
template<typename R> concept CAdvance = CInstanceOfTemplate<TUnqualRef<R>, Advance>;

template<typename L, typename F> requires CCallable<F, L> && (CList<L> || COutput<L> || CAdvance<L>)
constexpr INTRA_FORCEINLINE decltype(auto) operator|(L&& list, F&& func) {return INTRA_FWD(func)(INTRA_FWD(range));}

template<CConsumableRange R> [[nodiscard]] constexpr auto begin(R&& range)
{
	return z_D::RangeForIterLike<TRangeOf<R>>{ForwardAsRange<R>(range)};
}

template<CConsumableRange R> [[nodiscard]] constexpr auto end(R&& range) noexcept {return null;}

INTRA_END
