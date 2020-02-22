#pragma once

#include "Type.h"

/** This header file contains common utilities working with variadic templates.
  1. Checking a predicate on all elements.
  2. Type list manipulation (index, find, remove, slice).
  3. Type list transformations with metafunctions.
*/

INTRA_BEGIN
#ifdef __clang__
#if __has_builtin(__type_pack_element) //Clang 3.9+
#define INTRA_FAST_TUPLE_PACK_ELEMENT_SUPPORT
template<size_t Index, typename... Ts> using TAtIndex = __type_pack_element<Index, Ts...>;
#endif
#endif

#ifndef INTRA_FAST_TUPLE_PACK_ELEMENT_SUPPORT
namespace D {
template<size_t Index, typename T0, typename... Ts>
struct TAtIndex_: TAtIndex_<Index-1, Ts...> {};
template<typename T0, typename... Ts>
struct TAtIndex_<0, T0, Ts...> {typedef T0 _;};
}
template<size_t Index, typename... Ts> using TAtIndex = typename D::TAtIndex_<Index, Ts...>::_;
#endif
namespace D {
template<typename T0, typename... Ts> struct TFirst_ {typedef T0 _;};
}
template<typename... Ts> using TFirst = typename D::TFirst_<Ts...>::_;

template<typename T, T... Ints> struct TIntSeq {typedef TIntSeq _;};
template<size_t... Ints> using TIndexSeq = TIntSeq<size_t, Ints...>;

#ifdef __clang__
#if __has_builtin(__make_integer_seq)
#define INTRA_MAKE_INTEGER_SEQ_BUILTIN
#endif
#endif

#if defined(_MSC_VER) || defined(INTRA_MAKE_INTEGER_SEQ_BUILTIN)
// Fast O(1) implementation using MSVC or Clang 3.9+ built-in
template<typename T, T Size> using TMakeIntSeq = __make_integer_seq<TIntSeq, T, Size>;
#elif defined(__GNUC__) && __GNUC__ >= 8
// Fast O(1) implementation using GCC 8+ built-in
template<typename T, T Size> using TMakeIntSeq = TIntSeq<T, __integer_pack(Size)...>;
#else
// Generic O(log N) implementation
namespace D {
template<typename T, class Sequence1, class Sequence2> struct merge_and_renumber;
template<typename T, T... I1, T... I2>
struct merge_and_renumber<T, TIntSeq<T, I1...>, TIntSeq<T, I2...>>:
	TIntSeq<T, I1..., (sizeof...(I1)+I2)...>
{};
}
template<typename T, T N> struct TMakeIntSeq:
	D::merge_and_renumber<T, typename TMakeIntSeq<T, N/2>::_,
	typename TMakeIntSeq<T, N - N/2>::_>
{};
template<> struct TMakeIntSeq<char, 0>: TIntSeq<char> {};
template<> struct TMakeIntSeq<char, 1>: TIntSeq<char, 0> {};
template<> struct TMakeIntSeq<signed char, 0>: TIntSeq<signed char> {};
template<> struct TMakeIntSeq<signed char, 1>: TIntSeq<signed char, 0> {};
template<> struct TMakeIntSeq<short, 0>: TIntSeq<short> {};
template<> struct TMakeIntSeq<short, 1>: TIntSeq<short, 0> {};
template<> struct TMakeIntSeq<int, 0>: TIntSeq<int> {};
template<> struct TMakeIntSeq<int, 1>: TIntSeq<int, 0> {};
template<> struct TMakeIntSeq<long, 0>: TIntSeq<long> {};
template<> struct TMakeIntSeq<long, 1>: TIntSeq<long, 0> {};
template<> struct TMakeIntSeq<long long, 0>: TIntSeq<long long> {};
template<> struct TMakeIntSeq<long long, 1>: TIntSeq<long long, 0> {};
template<> struct TMakeIntSeq<unsigned char, 0>: TIntSeq<unsigned char> {};
template<> struct TMakeIntSeq<unsigned char, 1>: TIntSeq<unsigned char, 0> {};
template<> struct TMakeIntSeq<unsigned short, 0>: TIntSeq<unsigned short> {};
template<> struct TMakeIntSeq<unsigned short, 1>: TIntSeq<unsigned short, 0> {};
template<> struct TMakeIntSeq<unsigned int, 0>: TIntSeq<unsigned int> {};
template<> struct TMakeIntSeq<unsigned int, 1>: TIntSeq<unsigned int, 0> {};
template<> struct TMakeIntSeq<unsigned long, 0>: TIntSeq<unsigned long> {};
template<> struct TMakeIntSeq<unsigned long, 1>: TIntSeq<unsigned long, 0> {};
template<> struct TMakeIntSeq<unsigned long long, 0>: TIntSeq<unsigned long long> {};
template<> struct TMakeIntSeq<unsigned long long, 1>: TIntSeq<unsigned long long, 0> {};
#endif
template<size_t Size> using TMakeIndexSeq = TMakeIntSeq<size_t, Size>;
template<typename... Ts> using TSeqFor = TMakeIndexSeq<sizeof...(Ts)>;

template<typename T, T Add, class Seq> struct TIntSeqAdd;
template<typename T, T Add, T... Ints> struct TIntSeqAdd<T, Add, TIntSeq<T, Ints...>>: TIntSeq<T, (Add+Ints)...> {};
template<size_t End, size_t Start = 0> using TIndexRange = TIntSeqAdd<size_t, Start, TMakeIntSeq<size_t, End-Start>>;


template<typename... Args> struct TList {
	enum: size_t {Length = sizeof...(Args)};
};
namespace D {
template<typename T0, typename... Args> struct TTail_ {typedef TList<Args...> _;};
}
template<typename... Args> using TTail = typename D::TTail_<Args...>::_;

namespace D {
template<size_t N, typename TL> struct TListAt_;
template<size_t N, typename... Args> struct TListAt_<N, TList<Args...>> {typedef TAtIndex<N, Args...> _;};
}
template<uint N, typename TL> using TListAt = typename D::TListAt_<N, TL>::_;

namespace D
{
template<typename TL1, typename T2> struct TListConcat_;
template<typename... Args1, typename... Args2> struct TListConcat_<TList<Args1...>, TList<Args2...>> {typedef TList<Args1..., Args2...> _;};
template<typename T, typename... Args> struct TListConcat_<T, TList<Args...>> {typedef TList<Args..., T> _;};
}
template<typename TL, typename... Args> using TListConcat = typename D::TListConcat_<TL, Args...>::_;


namespace D
{
template<typename TL, typename T> struct TListRemove_;
template<typename T, typename... Args> struct TListRemove_<TList<Args...>, T>
{
	typedef typename TListRemove_<TTail<Args...>, T>::_ Removed;
	typedef TFirst<Args...> First;
	typedef TSelect<Removed, TListConcat<TList<First>, Removed>, CSame<First, T>> _;
};

template<typename T, typename Head> struct TListRemove_<TList<Head>, T> {typedef TList<Head> _;};
template<typename T> struct TListRemove_<TList<T>, T> {typedef TList<> _;};
template<typename T> struct TListRemove_<TList<>, T> {typedef TList<> _;};
}
template<typename TL, typename T> using TListRemove = typename D::TListRemove_<TL, T>::_;


namespace D {
template<typename TL> struct TListRemoveDuplicates_ {};
template<> struct TListRemoveDuplicates_<TList<>> {typedef TList<> _;};
template<typename... Args> struct TListRemoveDuplicates_<TList<Args...>>
{
	typedef TListRemove<TFirst<Args...>, TTail<Args...>> HeadRemovedFromTail;
	typedef typename TListRemoveDuplicates_<HeadRemovedFromTail>::_ TailWithoutDuplicates;
	typedef TListConcat<TailWithoutDuplicates, TList<TFirst<Args...>>> _;
};
}
template<typename TL> using TListRemoveDuplicates_ = typename D::TListRemoveDuplicates_<TL>::_;


template<typename TL, typename T, size_t IndexFrom = 0> constexpr size_t TListFind = 0;
template<typename T, size_t IndexFrom> constexpr size_t TListFind<TList<>, T, IndexFrom> = 0;
template<typename T, size_t IndexFrom, typename... Args>
constexpr size_t TListFind<TList<Args...>, T, IndexFrom> = IndexFrom +
(CSame<TFirst<Args...>, T>? 0: 1 + TListFind<TTail<Args...>, T, IndexFrom>);


namespace D {
template<typename TL, size_t Begin, size_t Last> struct TListSlice_ {};
template<size_t Begin, size_t Last> struct TListSlice_<TList<>, Begin, Last> {typedef TList<> _;};
template<size_t BeginEqualsEnd, typename... Args> struct TListSlice_<TList<Args...>, BeginEqualsEnd, BeginEqualsEnd>
{typedef TList<> _;};

template<uint Begin, uint End, typename... Args> struct TListSlice_<TList<Args...>, Begin, End>
{
	static_assert(End >= Begin, "Invalid range!");
	typedef TListConcat<
		typename TListSlice_<TList<Args...>, Begin, End - 1>::_,
		TList<TAtIndex<End-1, Args...>>
	> _;
};
}
template<typename TL, size_t Begin, size_t End = TL::Length> using TListSlice = typename D::TListSlice_<TL, Begin, End>::_;


#if defined(__cpp_fold_expressions) && __cpp_fold_expressions >= 201603
template<typename... Ts> constexpr forceinline bool VAll(Ts... ts) {return (... && ts);}
template<typename... Ts> constexpr forceinline bool VAny(Ts... ts) {return (... || ts);}
#else
constexpr bool VAll() {return true;}
template<typename T0, typename... Ts> constexpr forceinline bool VAll(T0 t0, Ts... ts) {return t0 && VAll(ts...);}
constexpr bool VAny() {return false;}
template<typename T0, typename... Ts> constexpr forceinline bool VAny(T0 t0, Ts... ts) {return t0 || VAny(ts...);}
#endif
template<typename T0> constexpr forceinline T0 VMin(T0 t0) {return t0;}
template<typename T0, typename T1> constexpr forceinline T0 VMin(T0 t0, T1 t1) {return t0 < t1? t0: t1;}
template<typename T0, typename T1, typename... Ts> constexpr forceinline auto VMin(T0 t0, T1 t1, Ts... ts) {return VMin(t0, VMin(t1, ts...));}

template<template<typename> class Predicate, typename... Ts> constexpr bool CAll = VAll(Predicate<Ts>::_...);
template<template<typename> class Predicate, typename... Ts> constexpr bool CAny = VAny(Predicate<Ts>::_...);

template<template<typename> class Predicate, class TL> constexpr bool CListAll = false;
template<template<typename> class Predicate, typename... Ts>
constexpr bool CListAll<Predicate, TList<Ts...>> = VAll(Predicate<Ts>::_...);

template<template<typename> class Predicate, class TL> constexpr bool CListAny = false;
template<template<typename> class Predicate, typename... Ts>
constexpr bool CListAny<Predicate, TList<Ts...>> = VAny(Predicate<Ts>::_...);


template<template<typename, typename> class Predicate, typename Arg0, class TL> constexpr bool CListAll1 = false;
template<template<typename, typename> class Predicate, typename Arg0, typename... Ts>
constexpr bool CListAll1<Predicate, Arg0, TList<Ts...>> = VAll(Predicate<Arg0, Ts>::_...);

template<template<typename, typename> class Predicate, typename Arg0, class TL> constexpr bool CListAny1 = false;
template<template<typename, typename> class Predicate, typename Arg0, typename... Ts>
constexpr bool CListAny1<Predicate, Arg0, TList<Ts...>> = VAny(Predicate<Arg0, Ts>::_...);


template<template<typename, typename> class Predicate, class TL1, class TL2> constexpr bool CListAllPairs = false;
template<template<typename, typename> class Predicate, typename... Ts1, typename... Ts2>
constexpr bool CListAllPairs<Predicate, TList<Ts1...>, TList<Ts2...>> = VAll(Predicate<Ts1, Ts2>::_...);

template<template<typename, typename> class Predicate, class TL1, class TL2> constexpr bool CListAnyPair = false;
template<template<typename, typename> class Predicate, typename... Ts1, typename... Ts2>
constexpr bool CListAnyPair<Predicate, TList<Ts1...>, TList<Ts2...>> = VAny(Predicate<Ts1, Ts2>::_...);

namespace D {
template<class TL, template<typename> class Transform> struct TListTransform_;
template<template<typename> class Transform, typename... Ts> struct TListTransform_<TList<Ts...>, Transform> {typedef TList<Transform<Ts>...> _;};
template<class TL, template<typename, typename> class Transform, typename Arg> struct TListTransform1_;
template<template<typename, typename> class Transform, typename Arg, typename... Ts> struct TListTransform1_<TList<Ts...>, Transform, Arg> {typedef TList<Transform<Arg, Ts>...> _;};
template<class TL, template<typename...> class Transform> struct TListUnpackTo_;
template<template<typename...> class C, typename... Ts> struct TListUnpackTo_<TList<Ts...>, C> {typedef C<Ts...> _;};
}
template<class TL, template<typename> class Transform> using TListTransform = typename D::TListTransform_<TL, Transform>::_;
template<class TL, template<typename, typename> class Transform, typename Arg> using TListTransform1 = typename D::TListTransform1_<TL, Transform, Arg>::_;
template<class TL, template<typename...> class C> using TListUnpackTo = typename D::TListUnpackTo_<TL, C>::_;

#if INTRA_CONSTEXPR_TEST
static_assert(TListFind<TList<int, float, double>, int> == 0, "TEST FAILED!");
static_assert(TListFind<TList<int, float, double>, float> == 1, "TEST FAILED!");
static_assert(TListFind<TList<int, float, double>, void> == 3, "TEST FAILED!");
static_assert(CSame<TFirst<int, float, double>, int>, "TEST FAILED!");
static_assert(CSame<TTail<int, float, double>, TList<float, double>>, "TEST FAILED!");
static_assert(CSame<TListRemove<TList<int, float, double>, float>, TList<int, double>>, "TEST FAILED!");
static_assert(CSame<TListSlice<TList<int, float, double, void>, 4, 4>, TList<>>, "TEST FAILED!");
static_assert(CSame<TListSlice<TList<int, float, double, void>, 1, 3>, TList<float, double>>, "TEST FAILED!");
static_assert(CSame<TListTransform<TList<int, const float, const double, void>, TRemoveConst>, TList<int, float, double, void>>, "TEST FAILED!");
#endif

INTRA_END
