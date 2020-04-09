#pragma once

#include "Intra/Type.h"

namespace Intra {
template<typename... Ts> struct Tuple;
template<typename T, size_t N> struct SArray;
}

namespace std { //NOLINT
//For compatibility with STL and structured bindings
INTRA_WARNING_PUSH
INTRA_IGNORE_WARNINGS_MSVC(4643);
template<class T> struct tuple_size;
template<size_t I, class T> struct tuple_element;
INTRA_WARNING_POP
template<typename... Ts> struct tuple_size<Intra::Tuple<Ts...>> {static constexpr size_t value = sizeof...(Ts);};
template<typename... Ts> struct tuple_size<const Intra::Tuple<Ts...>>: tuple_size<Intra::Tuple<Ts...>> {};
template<size_t I, typename... Ts>
struct tuple_element<I, Intra::Tuple<Ts...>> {using type = Intra::TPackAt<I, Ts...>;};
template<size_t I, typename... Ts>
struct tuple_element<I, const Intra::Tuple<Ts...>> {using type = const Intra::TPackAt<I, Ts...>;};

template<typename T, size_t N> struct tuple_size<Intra::SArray<T, N>> {static constexpr size_t value = N;};
template<typename T, size_t N> struct tuple_size<const Intra::SArray<T, N>>: tuple_size<Intra::SArray<T, N>> {};
template<size_t I, typename T, size_t N> struct tuple_element<I, Intra::SArray<T, N>> {using type = T;};
template<size_t I, typename T, size_t N> struct tuple_element<I, const Intra::SArray<T, N>> {using type = const T;};
}

INTRA_BEGIN
constexpr void FieldPointersOf(const void*);
constexpr void FieldNamesOf(const void*);
template<class T> concept CHasFieldPointersOf = !CVoid<decltype(FieldPointersOf(Val<const T*>()))>;
template<class T> concept CHasFieldNamesOf = !CVoid<decltype(FieldNamesOf(Val<const T*>()))>;

namespace z_D {
INTRA_DEFINE_CONCEPT_REQUIRES(CTupleSizeDefined, std::tuple_size<T>::value);
INTRA_DEFINE_CONCEPT_REQUIRES(CTupleElementDefined, (Val<typename std::tuple_element<0, T>::type>()));
template<typename T, int Category = CTupleSizeDefined<T>? 1: CHasFieldPointersOf<T>? 2: 0>
constexpr index_t StaticLength_ = -1;
template<typename T> constexpr index_t StaticLength_<T, 1> = index_t(std::tuple_size<T>::value);
template<typename T> constexpr index_t StaticLength_<T, 2> = StaticLength_<decltype(FieldPointersOf(Val<const T*>()))>;
template<typename T, size_t N> constexpr index_t StaticLength_<T[N], 0> = N;
}
template<typename T> constexpr index_t StaticLength = z_D::StaticLength_<TUnqual<TRemoveReference<T>>>;
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
    using FieldPointersTuple = decltype(FieldPointersOf(Val<const T*>()));
    using FieldPointersTList = typename TFieldTList_<TUnqual<TRemoveReference<FieldPointersTuple>>>::_;
    using _ = TListTransform<FieldPointersTList, TMemberFieldType>;
};
}
template<class Struct> using TFieldTList = typename z_D::TFieldTList_<TUnqual<TRemoveReference<Struct>>>::_;
template<class Struct> using TQualRefFieldTList = TListTransform1<TFieldTList<Struct>, TPropagateQualLVRef, Struct>;

template<class StaticLengthContainer, size_t End = StaticLength<StaticLengthContainer>, size_t Start = 0>
using TQualRefFieldTListSlice = TListSlice<TQualRefFieldTList<StaticLengthContainer>, Start, End>;

#if INTRA_CONSTEXPR_TEST
static_assert(CSame<TQualRefFieldTList<const Tuple<int, float>&>, TList<const int&, const float&>>);
static_assert(CSame<TQualRefFieldTListSlice<const Tuple<int, float>&, 1>, TList<const int&>>);
static_assert(CTListConvertible<TList<int>, TList<float>>);
static_assert(!CTListConvertible<int, float>);
#endif

template<class StaticLengthContainer, class MapFunc>
using TMappedFieldTList = TListTransform1<TQualRefFieldTList<StaticLengthContainer>, TResultOf, MapFunc>;
template<class StaticLengthContainer, class MapFunc>
using TMappedFieldCommon = TListCommon<TMappedFieldTList<StaticLengthContainer, MapFunc>>;


INTRA_DEFINE_CONCEPT_REQUIRES2(CEqualityComparable, Val<bool&>() = (Val<T1>() == Val<T2>()),, = U1);
INTRA_DEFINE_CONCEPT_REQUIRES2(CNonEqualityComparable, Val<bool&>() = (Val<T1>() != Val<T2>()),, = U1);

template<typename T> concept CObject =
	CScalar<T> ||
	CArrayType<T> ||
	CUnion<T> ||
	CClass<T>;

template<typename T> concept CMovable =
	CObject<T> &&
	CMoveConstructible<T> &&
	CMoveAssignable<T>;
	//&& CSwappable<T>;

template<typename T> concept CCopyable =
	CCopyConstructible<T> &&
	CMovable<T> &&
	CCopyAssignable<T>;

template<typename T> concept CSemiregular =
	CCopyable<T> &&
	CDefaultConstructible<T>;

template<typename T> concept CRegular =
	CSemiregular<T> &&
	CEqualityComparable<T>;

INTRA_DEFINE_CONCEPT_REQUIRES(CHas_size, Val<size_t&>() = Val<T>().size());
INTRA_DEFINE_CONCEPT_REQUIRES(CHasLength, Val<index_t&>() = Val<T>().Length());
INTRA_DEFINE_CONCEPT_REQUIRES(CHas_data, Val<const void*&>() = Val<T>().data());
INTRA_DEFINE_CONCEPT_REQUIRES(CHasData, Val<const void*&>() = Val<T>().Data());
INTRA_DEFINE_SAFE_DECLTYPE(TIteratorOf, begin(Val<T>()));


template<class T, class = Requires<CHasLength<T> || CHas_size<T> || CStaticLengthContainer<T>>>
constexpr index_t LengthOf(T&& list)
{
	if constexpr(CStaticLengthContainer<T>) return StaticLength<T>;
	else if constexpr(CHasLength<T>) return list.Length();
	else return index_t(list.size());
}

template<class R, class = Requires<CHasData<R> || CHas_data<R> || CPointer<TIteratorOf<R>>>>
constexpr auto DataOf(R&& r)
{
	if constexpr(CHasData<R>) return r.Data();
	else if constexpr(CHas_data<R>) return r.data();
	else return r.begin();
}
template<typename T, size_t N> constexpr T* DataOf(T(&arr)[N]) {return arr;}

INTRA_DEFINE_CONCEPT_REQUIRES(CHasDataOf, Val<const void*&>() = DataOf(Val<T>()));
INTRA_DEFINE_CONCEPT_REQUIRES(CHasLengthOf, Val<index_t&>() = LengthOf(Val<T>()));
template<typename R> concept CArrayClass = CHasDataOf<R> && CHasLengthOf<R>;

INTRA_DEFINE_SAFE_DECLTYPE(TArrayElementPtr, DataOf(Val<T>()));
template<typename T> using TArrayElementKeepConst = TRemovePointer<TArrayElementPtr<T>>;
template<typename T> using TArrayElementRef = TArrayElementKeepConst<T>&;
template<typename T> using TArrayElement = TRemoveConst<TArrayElementKeepConst<T>>;

#if INTRA_CONSTEXPR_TEST
static_assert(CHasDataOf<const char(&)[5]>);
static_assert(!CHasDataOf<int>);
static_assert(CHasLengthOf<const char(&)[5]>);
static_assert(CArrayClass<const char(&)[5]>);

static_assert(CSame<TArrayElementPtr<const char(&)[5]>, const char*>);
static_assert(CSame<TArrayElement<const char(&)[5]>, char>);
static_assert(CSame<TArrayElementKeepConst<const char(&)[5]>, const char>);
#endif


//! Check if Rhs elements can be assigned to Lhs elements. false, if at least one of them is not an CArrayClass.
template<typename Lhs, typename Rhs> concept CAssignableArrays =
	CSameNotVoid<TArrayElementKeepConst<Lhs>, TArrayElementKeepConst<Rhs>> ||
	CSameNotVoid<TArrayElementKeepConst<Lhs>, TArrayElementKeepConst<Rhs>>;

template<typename Lhs, typename T> concept CAssignableToArrayOf =
	CSame<T, TArrayElementKeepConst<Lhs>> ||
	CSame<TRemoveConst<T>, TArrayElementKeepConst<Lhs>>;

template<typename R> concept CAssignableArrayClass =
	CArrayClass<R> &&
	!CConst<TArrayElementKeepConst<R>>;


INTRA_DEFINE_CONCEPT_REQUIRES(CHasPreIncrement, ++Val<T>());
INTRA_DEFINE_CONCEPT_REQUIRES(CHasPostIncrement, Val<T>()++);
INTRA_DEFINE_CONCEPT_REQUIRES(CHasPreDecrement, --Val<T>());
INTRA_DEFINE_CONCEPT_REQUIRES(CHasPostDecrement, Val<T>()--);
INTRA_DEFINE_CONCEPT_REQUIRES(CHasDereference, *Val<T>());

INTRA_END
