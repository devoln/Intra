#pragma once

#include "Intra/Core.h"

/** This header file contains:
  1. Metafunctions for type manipulation
  2. Fundamental C++ concepts for checking type traits
  3. Tools for working with metafunction classes
  4. Checking a predicate on all type list elements.
  5. Type list manipulation (index, find, remove, slice).
  6. Type list transformations with metafunctions.
*/

INTRA_BEGIN
//INTRA_IGNORE_WARN_COPY_MOVE_CONSTRUCT_IMPLICITLY_DELETED
//INTRA_IGNORE_WARN_LOSING_CONVERSION
//INTRA_IGNORE_WARN_SIGN_CONVERSION
//INTRA_IGNORE_WARN_LANGUAGE_EXTENSION

template<typename T> T&& Val();

template<typename T> struct TConstructT {};
template<typename T> constexpr TConstructT<T> ConstructT;
template<size_t I> struct TConstructAt {};
template<size_t I> constexpr TConstructAt<I> ConstructAt;

template<typename> constexpr bool CConstructAt = false;
template<size_t I> constexpr bool CConstructAt<TConstructAt<I>> = true;

struct EmptyType
{
	EmptyType() = default;
	template<typename... Args> constexpr EmptyType(Args&&...) {}
};

template<auto Value> struct TValue {static constexpr auto _ = Value;};
template<size_t Value> using TIndex = TValue<Value>;

/// TTag is used by types to declare that they have some special trait:
/// ```using TagName = TTag<>;```
/// or with condition:
/// ```using TagName = TTag<constantBooleanExpression()>;```
/// that is easy to check with concepts: requires {T::TagName::True;}
template<bool V = true> struct TTag {};
template<> struct TTag<true> {enum {True};};

struct NonCopyableType
{
	NonCopyableType() = default;
	NonCopyableType(NonCopyableType&&) = default;
	NonCopyableType& operator=(NonCopyableType&&) = default;
	NonCopyableType(const NonCopyableType&) = delete;
	NonCopyableType& operator=(const NonCopyableType&) = delete;
};

namespace z_D {
template<typename T, typename... Ts> struct TPackFirst_ {using _ = T;};
}
template<typename... Ts> using TPackFirst = typename z_D::TPackFirst_<Ts...>::_;
#ifdef __clang__
template<size_t Index, typename... Ts>
using TPackAt = __type_pack_element<Index, Ts...>;
#else
namespace z_D {
template<size_t Index, typename T0, typename... Ts>
struct TPackAt_: TPackAt_<Index-1, Ts...> {};
template<typename T0, typename... Ts>
struct TPackAt_<0, T0, Ts...> {using _ = T0;};
}
template<size_t Index, typename... Ts>
using TPackAt = typename z_D::TPackAt_<Index, Ts...>::_;
#endif

template<size_t Index, auto... Values> constexpr auto VPackAt = TPackAt<Index, TValue<Values>...>::_;

template<typename T> constexpr bool CUnknownBoundArrayType = false;
template<typename T> constexpr bool CUnknownBoundArrayType<T[]> = true;
template<typename T> constexpr bool CKnownBoundArrayType = false;
template<typename T, size_t N> constexpr bool CKnownBoundArrayType<T[N]> = true;
template<typename T> concept CArrayType = CUnknownBoundArrayType<T> || CKnownBoundArrayType<T>;

namespace z_D {
template<typename T> struct TRemoveReference_ {using _ = T;};
template<typename T> struct TRemoveReference_<T&> {using _ = T;};
template<typename T> struct TRemoveReference_<T&&> {using _ = T;};
template<typename T> constexpr bool CLValueReference_ = false;
template<typename T> constexpr bool CLValueReference_<T&> = true;
template<typename T> constexpr bool CRValueReference_ = false;
template<typename T> constexpr bool CRValueReference_<T&&> = true;

template<typename T> constexpr bool CConst_ = false;
template<typename T> constexpr bool CConst_<const T> = true;
template<typename T> constexpr bool CVolatile_ = false;
template<typename T> constexpr bool CVolatile_<volatile T> = true;
}
template<typename T> using TRemoveReference = typename z_D::TRemoveReference_<T>::_;

template<typename T> concept CLValueReference = z_D::CLValueReference_<T>;
template<typename T> concept CRValueReference = z_D::CRValueReference_<T>;
template<typename T> concept CReference = CLValueReference<T> || CRValueReference<T>;

template<typename T> concept CConst = z_D::CConst_<T>;
template<typename T> concept CVolatile = z_D::CVolatile_<T>;
template<typename T> concept CNonConstLValueReference = CLValueReference<T> && !CConst<TRemoveReference<T>>;
template<typename T> concept CNonConstRValueReference = CRValueReference<T> && !CConst<TRemoveReference<T>>;

template<typename IfTrue, typename IfFalse, bool Condition> using TSelect = TPackAt<Condition, IfFalse, IfTrue>;


template<bool Condition> using CopyableIf = TSelect<EmptyType, NonCopyableType, Condition>;

namespace z_D {
template<typename T> struct TRemovePointer_ {using _ = T;};
template<typename T> struct TRemovePointer_<T*> {using _ = T;};
template<typename T> struct TRemovePointer_<T* const> {using _ = T;};
template<typename T> struct TRemovePointer_<T* volatile> {using _ = T;};
template<typename T> struct TRemovePointer_<T* const volatile> {using _ = T;};

template<typename T> struct TRemoveConst_ {using _ = T;};
template<typename T> struct TRemoveConst_<const T> {using _ = T;};

template<typename T> struct TRemoveVolatile_ {using _ = T;};
template<typename T> struct TRemoveVolatile_<volatile T> {using _ = T;};
template<typename T> struct TRemoveVolatile_<volatile T[]> {using _ = T[];};
template<typename T, size_t N> struct TRemoveVolatile_<volatile T[N]> {using _ = T[N];};

template<typename T> struct TRemoveExtent_ {using _ = T;};
template<typename T, size_t N> struct TRemoveExtent_<T[N]> {using _ = T;};
template<typename T> struct TRemoveExtent_<T[]> {using _ = T;};

template<typename T> struct TExplicitType_ {using _ = T;};
}

template<typename T> using TRemovePointer = typename z_D::TRemovePointer_<T>::_;
template<typename T> using TRemoveConst = typename z_D::TRemoveConst_<T>::_;
template<typename T> using TRemoveVolatile = typename z_D::TRemoveVolatile_<T>::_;
template<typename T> using TRemoveConstRef = TRemoveConst<TRemoveReference<T>>;
template<typename T> using TUnqual = TRemoveConst<TRemoveVolatile<T>>;
template<typename T> using TUnqualRef = TUnqual<TRemoveReference<T>>;
template<typename T> using TRemoveExtent = typename z_D::TRemoveExtent_<T>::_;

/// Use this as function argument type to avoid automatic template type inference
template<typename T> using TExplicitType = typename z_D::TExplicitType_<T>::_;

//TODO: doesn't work if T is a cv- or ref-qualified function type
template<typename T> using TAddPointer = TRemoveReference<T>*;


namespace z_D {
template<typename... Ts> constexpr bool CSame_ = false;
template<typename T> constexpr bool CSame_<T> = true;
template<typename T> constexpr bool CSame_<T, T> = true;
template<typename T1, typename T2, typename T3, typename... Ts>
constexpr bool CSame_<T1, T2, T3, Ts...> = CSame_<T1, T2> && CSame_<T1, T3> && (CSame_<T1, Ts> && ...);
}
template<typename... Ts> concept CSame = z_D::CSame_<Ts...>;
template<typename... Ts> concept CSameUnqual = CSame<TUnqual<Ts>...>;
template<typename... Ts> concept CSameIgnoreCVRef = CSameUnqual<TRemoveReference<Ts>...>;
template<typename... Ts> concept CSameIgnoreRef = CSame<TRemoveReference<Ts>...>;
template<typename... Ts> concept CSameNotVoid = CSame<Ts...> && !CSame<void, Ts...>;
template<typename T, typename... Ts> concept CAnyOf = (CSame<T, Ts> || ...);

namespace z_D {
template<typename T, bool = CSameUnqual<T, void>> struct TAddReference
{
	using LValue = T&;
	using RValue = T&&;
};
template<typename T> struct TAddReference<T, true>
{
	using LValue = void;
	using RValue = void;
};
}
template<typename T> using TAddLValueReference = typename z_D::TAddReference<T>::LValue;
template<typename T> using TAddRValueReference = typename z_D::TAddReference<T>::RValue;

namespace z_D {
template<typename T> constexpr bool CFunctionPointer_ = false;
#ifdef _MSC_VER
#ifdef __i386__
template<typename Ret, typename... Args> constexpr bool CFunctionPointer_<Ret(__cdecl*)(Args...)> = true;
template<typename Ret, typename... Args> constexpr bool CFunctionPointer_<Ret(__stdcall*)(Args...)> = true;
template<typename Ret, typename... Args> constexpr bool CFunctionPointer_<Ret(__fastcall*)(Args...)> = true;
#else
template<typename Ret, typename... Args> constexpr bool CFunctionPointer_<Ret(*)(Args...)> = true;
#if defined(__amd64__)
template<typename Ret, typename... Args> constexpr bool CFunctionPointer_<Ret(__vectorcall*)(Args...)> = true;
#endif
#endif
#else
template<typename Ret, typename... Args> constexpr bool CFunctionPointer_<Ret(*)(Args...)> = true;
#endif
}
template<typename T> concept CFunctionPointer = z_D::CFunctionPointer_<T>;
template<typename T> concept CAbstractClass = __is_abstract(T);
template<typename T> concept CUnion = __is_union(T);
template<typename T> concept CClass = __is_class(T);
template<typename T> concept CEnum = __is_enum(T);
template<typename T> concept CEmptyClass = __is_empty(T);
template<typename T> concept CFinalClass = __is_final(T);
template<typename T> concept CTriviallyDestructible = __has_trivial_destructor(T);
template<class T, class From> concept CDerived = __is_base_of(From, T);
template<typename T> concept CHasVirtualDestructor = __has_virtual_destructor(T);
template<typename T> concept CPolymorphic = __is_polymorphic(T);
template<typename T> concept CFunction = !CReference<T> && !CConst<const T>;
template<typename T> concept CObject = CConst<const T> && !CVoid<T>; //= CScalar<T> || CArray<T> || CUnion<T> || CClass<T> = !CFunction<T> && !CReference<T> && !CVoid<T>
template<typename T> concept CStandardLayout = __is_standard_layout(T);
template<typename T> concept CTriviallyCopyable = __is_trivially_copyable(T);
template<typename T, typename... Args> concept CTriviallyConstructible = __is_trivially_constructible(T, Args...);
template<typename T, typename Arg> concept CTriviallyAssignable = __is_trivially_assignable(T, Arg);
template<typename T> concept CTriviallyCopyAssignable = CTriviallyAssignable<TAddLValueReference<T>, TAddLValueReference<const T>>;
template<typename T> concept CTriviallyMoveAssignable = CTriviallyAssignable<TAddLValueReference<T>, TAddRValueReference<T>>;
template<typename T> concept CHasUniqueObjectRepresentations = __has_unique_object_representations(T);

namespace z_D {
template<typename T, template<typename...> class> constexpr bool CInstanceOfTemplate_ = false;
template<typename... Ts, template<typename...> class Template> constexpr bool CInstanceOfTemplate_<Template<Ts...>, Template> = true;
}
template<typename T, template<typename...> class Template> concept CInstanceOfTemplate = z_D::CInstanceOfTemplate_<T, Template>;


template<typename T> concept CUnqualedVoid = CSame<T, void>;
template<typename T> concept CUnqualedUnsignedIntegral = CAnyOf<T,
	char16_t, char32_t, bool, uint8, uint16, unsigned, unsigned long, uint64,
	TSelect<char, unsigned, (char(~0) > 0)>,
	TSelect<wchar_t, unsigned, (wchar_t(~0) > 0)>
>;

template<typename T> concept CUnqualedSignedIntegral = CAnyOf<T,
	int8, short, int, long, int64,
	TSelect<int, char, (char(~0) > 0)>,
	TSelect<int, wchar_t, (wchar_t(~0) > 0)>
>;

template<typename T> concept CUnqualedFloatingPoint = CAnyOf<T, float, double, long double>;

/** note: signed char and unsigned char are not treated as character types.
	They are separate types from char because Intra doesn't use them to store characters.
*/
template<typename T> concept CUnqualedChar = CAnyOf<T, char, char16_t, char32_t, wchar_t
#ifdef __cpp_char8_t
	, char8_t
#endif
>;

template<typename T> concept CUnqualedIntegral = CUnqualedUnsignedIntegral<T> || CUnqualedSignedIntegral<T>;
template<typename T> concept CUnqualedFixedPoint = false; // TODO: implement
template<typename T> concept CUnqualedSignedFixedPoint = false; // TODO: implement
template<typename T> concept CUnqualedSigned = CUnqualedSignedIntegral<T> || CUnqualedFloatingPoint<T> || CUnqualedSignedFixedPoint<T>;
template<typename T> concept CUnqualedReal = CUnqualedFloatingPoint<T> || CUnqualedFixedPoint<T>;
template<typename T> concept CUnqualedArithmetic = CUnqualedIntegral<T> || CUnqualedReal<T>;

template<typename T> constexpr bool CUnqualedPointer = false;
template<typename T> constexpr bool CUnqualedPointer<T*> = true;

template<typename T> constexpr bool CUnqualedMemberPointer = false;
template<typename T, class U> constexpr bool CUnqualedMemberPointer<T U::*> = true;

template<typename T> constexpr bool CUnqualedMethodPointer = false;
template<typename T, typename U> constexpr bool CUnqualedMethodPointer<T U::*> = CFunction<T>;

template<typename T> concept CVoid = CUnqualedVoid<TUnqual<T>>;
template<typename T> concept CChar = CUnqualedChar<TUnqual<T>>;
template<typename T> concept CSignedIntegral = CUnqualedSignedIntegral<TUnqual<T>>;
template<typename T> concept CUnsignedIntegral = CUnqualedUnsignedIntegral<TUnqual<T>>;
template<typename T> concept CIntegral = CUnqualedIntegral<TUnqual<T>>;
template<typename T> concept CFloatingPoint = CUnqualedFloatingPoint<TUnqual<T>>;
template<typename T> concept CFixedPoint = CUnqualedFixedPoint<TUnqual<T>>;
template<typename T> concept CSigned = CUnqualedSigned<TUnqual<T>>;
template<typename T> concept CReal = CUnqualedReal<TUnqual<T>>;
template<typename T> concept CArithmetic = CUnqualedArithmetic<TUnqual<T>>;
template<typename T> concept CPointer = CUnqualedPointer<TUnqual<T>>;
template<typename T> concept CMemberPointer = CUnqualedMemberPointer<TUnqual<T>>;
template<typename T> concept CMethodPointer = CUnqualedMethodPointer<TUnqual<T>>;

template<typename T> concept CAddable = requires(T x) {x + x;};
template<typename T> concept CSubtractable = requires(T x) {x - x;};
template<typename T> concept CMultipliable = requires(T x) {x * x;};
template<typename T> concept CDivisible = requires(T x) {x / x;};
template<typename T> concept CModDivisible = requires(T x) {x % x;};

template<typename T> concept CArithmeticType = CAddable<T> && CSubtractable<T> && CMultipliable<T> && CDivisible<T>;

template<typename From, typename To> using TPropagateConst = TSelect<const To, To, CConst<From>>;
template<typename From, typename To> using TPropagateVolatile = TSelect<volatile To, To, CVolatile<From>>;
template<typename From, typename To> using TPropagateQualifiers = TPropagateConst<From, TPropagateVolatile<From, To>>;
template<typename From, typename To> using TPropagateLVRef = TSelect<To&, To, CLValueReference<From>>;
template<typename From, typename To> using TPropagateRVRef = TSelect<To&&, To, CRValueReference<From>>;
template<typename From, typename To> using TPropagateRef = TPropagateLVRef<From, TPropagateRVRef<From, To>>;
template<typename From, typename To> using TPropagateQualLVRef = TPropagateLVRef<From, TPropagateQualifiers<TRemoveReference<From>, To>>;
template<typename From, typename To> using TPropagateQualRef = TPropagateRef<From, TPropagateQualifiers<TRemoveReference<From>, To>>;

#if INTRA_CONSTEXPR_TEST
static_assert(CSame<TPropagateConst<const int, float>, const float>);
static_assert(CSame<TPropagateRef<int&, float>, float&>);
static_assert(CSame<TPropagateRef<int&&, float>, float&&>);
static_assert(CSame<TPropagateQualLVRef<const int&, float>, const float&>);
#endif

template<class T, size_t N = 0> constexpr size_t ArrayExtent = 0;
template<class T> constexpr size_t ArrayExtent<T[], 0> = 0;
template<class T, size_t N> constexpr size_t ArrayExtent<T[], N> = ArrayExtent<T, N-1>;
template<class T, size_t N> constexpr size_t ArrayExtent<T[N], 0> = N;
template<class T, size_t I, size_t N> constexpr size_t ArrayExtent<T[I], N> = ArrayExtent<T, N-1>;

template<typename T> using TDecay = TSelect<
	TRemoveExtent<TRemoveReference<T>>*,
	TSelect<
		TAddPointer<TRemoveReference<T>>,
		TUnqualRef<T>,
		CFunction<TRemoveReference<T>>>,
	CArrayType<TRemoveReference<T>>>;



namespace z_D {
template<typename T> struct TRemoveAllExtents_ {using _ = T;};
template<typename T> struct TRemoveAllExtents_<T[]>: TRemoveAllExtents_<T> {};
template<typename T, size_t N> struct TRemoveAllExtents_<T[N]>: TRemoveAllExtents_<T> {};
}
template<typename T> using TRemoveAllExtents = typename z_D::TRemoveAllExtents_<T>::_;

#define INTRA_DEFINE_SAFE_DECLTYPE(checker_name, expr) \
	struct z_D_ ## checker_name {\
		template<typename T> static auto test(int) -> decltype(expr); \
		template<typename T> static void test(...); \
	};\
	template<typename U> using checker_name = decltype(z_D_ ## checker_name::test<U>(0))

#define INTRA_DEFINE_SAFE_DECLTYPE_T_ARGS(checker_name, expr) \
	struct z_D_ ## checker_name { \
		template<typename T, typename... Args> static auto test(int) -> decltype(expr); \
		static void test(...); \
	};\
	template<typename U, typename... UArgs> using checker_name = decltype(z_D_ ## checker_name::test<U, UArgs...>(0))

#define INTRA_DEFINE_CONCEPT_EXPR(checker_name, expr, resultOnError) \
	struct z_D_ ## checker_name {\
		template<typename T> static constexpr auto test(int) -> decltype((expr), bool()) {return expr;} \
		static constexpr bool test(...) {return resultOnError;} \
	}; \
	template<typename U> concept checker_name = z_D_ ## checker_name::test<U>(0)

template<typename T> concept CScalar =
	CArithmetic<T> ||
	CPointer<T> ||
	CEnum<T> ||
	CMemberPointer<T> ||
	CSame<TUnqual<T>, decltype(nullptr)>;

template<typename T> constexpr bool IsByteByByteLexicographicallyComparableCustomType = false;
template<typename T> constexpr bool IsBitwiseEqualityComparableCustomType =
	IsByteByByteLexicographicallyComparableCustomType<T>;

template<typename T> concept CByteByByteLexicographicallyComparable =
	CHasUniqueObjectRepresentations<T> && IsByteByByteLexicographicallyComparableCustomType<TUnqual<T>> ||
		(CIntegral<T> || CPointer<T> || CEnum<T>) && (sizeof(T) == 1 || Config::TargetIsBigEndian);

template<typename T> concept CTriviallyEqualComparable = CScalar<T> ||
	CByteByByteLexicographicallyComparable<T> ||
	IsBitwiseEqualityComparableCustomType<TUnqual<T>>;



template<typename T> concept CDestructible =
#if defined(_MSC_VER) || defined(__INTEL_COMPILER)
	__is_destructible(T);
#else
	CReference<T> || !CUnknownBoundArrayType<T> && requires(TRemoveAllExtents<T> x) {x.~T();};
#endif

#if INTRA_CONSTEXPR_TEST
static_assert(!CDestructible<int[]>);
static_assert(CDestructible<float&>);
static_assert(CDestructible<double[5]>);
#endif

template<typename T, typename To> concept CStaticCastable = requires(T x, To) {static_cast<To>(x);};

template<typename T, typename... Args> concept CConstructible = __is_constructible(T, Args...);

#if defined(_MSC_VER) || defined(__clang__) || defined(__INTEL_COMPILER) || defined(__GNUC__) && __GNUC__ >= 11
template<typename T, typename... Args> concept CNothrowConstructible = __is_nothrow_constructible(T, Args...);
template<typename To, typename From> concept CNothrowAssignable = __is_nothrow_assignable(To, From);
#else
template<typename T, typename... Args> concept CNothrowConstructible = []{
	if constexpr(!CConstructible<T, Args...>) return false;
	else if constexpr(sizeof...(Args) == 1) return noexcept(static_cast<T>(Val<Args...>()));
	else if constexpr(sizeof...(Args) == 0 && CKnownBoundArrayType<T>) return noexcept(TRemoveExtent<T>());
	else return noexcept(T(Val<Args>()...));
}();
template<typename To, typename From> concept CNothrowAssignable = []{
	if constexpr(!__is_assignable(To, From)) return false;
	else return noexcept(Val<To>() = Val<From>());
}();
#endif

template<typename T> concept CCopyConstructible = CConstructible<T, TAddLValueReference<const T>>;
template<typename T> concept CMoveConstructible = CConstructible<T, T>;
template<typename T> concept CTriviallyCopyConstructible = CTriviallyConstructible<T, TAddLValueReference<const T>>;
template<typename T> concept CTriviallyDefaultConstructible = CTriviallyConstructible<T>;
template<typename T> concept CTriviallyMoveConstructible = CTriviallyConstructible<T, T>;
template<typename T> concept CNothrowCopyConstructible = CNothrowConstructible<T, TAddLValueReference<const T>>;
template<typename T> concept CNothrowMoveConstructible = CNothrowConstructible<T, T>;

template<typename F, typename... Args> concept CCallable = requires(F&& f, Args&&... args) {INTRA_FWD(f)(INTRA_FWD(args)...);};

template<typename T, typename... Args> using TResultOf = decltype(Val<T>()(Val<Args>()...));
INTRA_DEFINE_SAFE_DECLTYPE_T_ARGS(TResultOfOrVoid, Val<T>()(Val<Args>()...));


template<typename To, typename From> concept CAssignable = __is_assignable(To, From);
template<typename T> concept CCopyAssignable = CAssignable<TAddLValueReference<T>, TAddLValueReference<const T>>;
template<typename T> concept CMoveAssignable = CAssignable<TAddLValueReference<T>, T>;
template<typename T> concept CNothrowCopyAssignable = CNothrowAssignable<TAddLValueReference<T>, TAddLValueReference<const T>>;
template<typename T> concept CNothrowMoveAssignable = CNothrowAssignable<TAddLValueReference<T>, T>;

/*!
  Trivially relocatable is a less constrained concept than trivially copyable.
  All trivially copyable types are also trivially relocatable.
  However there may be types having move constructor and destructor that are not trivial separately but combination of them may be trivial.
  It is true for most containers. You can make a bitwise copy of a container object without calling the move constructor and the destructor of source.
  Specialize IsTriviallyRelocatable for such types after their definition.
*/
template<typename T> constexpr bool IsTriviallyRelocatable = CTriviallyCopyable<T>;
template<typename T> concept CTriviallyRelocatable = IsTriviallyRelocatable<T>;

/** CTriviallySerializable is useful to check if a type can be trivially binary serialized and deserialized.
  However it cannot check if the type contains pointers.
  If it contains pointers it may be not suitable for serialization.
*/
template<typename T> concept CTriviallySerializable = CStandardLayout<T> && CTriviallyCopyable<T>;

namespace z_D {
template<bool Cond, class T=void> struct TRequires {};
template<class T> struct TRequires<true, T> {using _ = T;};
template<bool Cond, class T> struct TRequiresAssert {static_assert(Cond); using _ = T;};
}
template<bool Cond, typename T = void> using Requires = typename z_D::TRequires<Cond, T>::_;
template<typename TypeToCheckValidity, typename T = void> using RequiresT = T;
template<bool Cond, typename T = void> using RequiresAssert = typename z_D::TRequiresAssert<Cond, T>::_;

template<typename From, typename To> concept CConvertibleTo =
#if defined(_MSC_VER) || defined(__clang__) || defined(__INTEL_COMPILER)
	__is_convertible_to(From, To);
#else
	CVoid<From> && CVoid<To> || requires(void(*f)(To), From from) {f(from);};
#endif

namespace z_D {
template<typename... Ts> struct TCommon_ {using _ = void;};
template<typename T> struct TCommon_<T> {using _ = TDecay<T>;};
template<typename T, typename U> struct TCommon_<T, U> {using _ = TDecay<decltype(bool()? Val<T>(): Val<U>())>;};
template<typename T, typename U, typename... V> struct TCommon_<T, U, V...> {
	using _ = typename TCommon_<typename TCommon_<T, U>::_, V...>::_;
};
}
/// Common type without const\volatile and references.
template<typename... Ts> using TCommon = typename z_D::TCommon_<Ts...>::_;

/// Common type preserving references.
template<typename... Ts> using TCommonRef = TSelect<
	TSelect<const TAddLValueReference<TCommon<Ts...>>,
		TAddLValueReference<TCommon<Ts...>>,
		(CConst<TDecay<Ts>> || ...)>,
	TCommon<Ts...>,
	CSame<TDecay<Ts>...> && (CReference<Ts> && ...)>;

/** Meta function classes.
  @see http://ericniebler.com/2014/11/13/tiny-metaprogramming-library/ for more information.
*/

/// Wrap meta function into a metafunction class.
template<template<typename...> class MetaFunction> struct TMetaWrap {
	template<typename...Ts> using Apply = MetaFunction<Ts...>;
};

/// Apply metafunction from metafunction class F
template<typename F, typename... As> using TMetaApply = typename F::template Apply<As...>;

/// Compose multiple metafunctions from classes F1, Fs... into a metafunction class performing a chain of transformations: F1(...(F[N-1](F[N](Arg))))
template<typename F1, typename... Fs> struct TMetaCompose {
	template<typename T> using Apply = TMetaApply<F1, TMetaApply<TMetaCompose<Fs...>, T>>;
};
template<typename F1> struct TMetaCompose<F1> {
	template<typename T> using Apply = TMetaApply<F1, T>;
};

/// Wrap integer sequence into a metafunction class
template<typename Int, template<Int...> class F> struct TMetaWrapI {
	template<typename... Ts> using Apply = F<Ts::_...>;
};

/// Metafunction class containing a constant metafunction that always returns `T`.
template<typename T> struct TMetaAlways {template<typename...> using Apply = T;};

namespace z_D {
template<typename T> struct TMemberFieldType_;
template<typename T, typename F> struct TMemberFieldType_<F T::*> {using _ = F;};
}
template<typename T> using TMemberFieldType = typename z_D::TMemberFieldType_<T>::_;

template<typename T> constexpr INTRA_UTIL_INLINE T&& Forward(TRemoveReference<T>& t) noexcept {return static_cast<T&&>(t);}
template<typename T> constexpr INTRA_UTIL_INLINE T&& Forward(TRemoveReference<T>&& t) noexcept
{
	static_assert(!CLValueReference<T>, "Bad Forward call!");
	return static_cast<T&&>(t);
}

constexpr auto MoveNoexcept = []<typename T>(T&& t) -> decltype(auto) noexcept {
	if constexpr(CNothrowMoveConstructible<T> || !CCopyConstructible<T>)
		return static_cast<TRemoveReference<T>&&>(t);
	else return static_cast<TRemoveReference<T>&>(t);
};

namespace z_D {
template<typename F> struct TFinally
{
	INTRA_CONSTEXPR_DESTRUCTOR ~TFinally() {OnDestruct();}
	F OnDestruct;
};

struct TFinallyMaker
{
	template<CCallable F> constexpr	TFinally<F> operator=(F&& f) const noexcept {return {INTRA_FWD(f)};}
};
}
/** Execute a statement block after leaving the current scope for any reason: normal or exception.
  usage: INTRA_FINALLY{<code block>};
*/
#define INTRA_FINALLY auto INTRA_CONCATENATE_TOKENS(z_D_finally_, __COUNTER__) = ::Intra::z_D::TFinallyMaker() = [&]()->void

template<class T> [[nodiscard]] constexpr T* AddressOf(T& arg) noexcept {return __builtin_addressof(arg);}

template<class T, typename U> inline size_t MemberOffset(U T::* member) noexcept
{return reinterpret_cast<size_t>(AddressOf((static_cast<T*>(nullptr))->*member));}

#if INTRA_CONSTEXPR_TEST
static_assert(CArrayType<TRemoveReference<const char(&)[9]>>);
// TODO: add more tests
#endif

template<typename T, T... Ints> struct TIntSeq {using _ = TIntSeq;};
template<size_t... Ints> using TIndexSeq = TIntSeq<size_t, Ints...>;

#if defined(_MSC_VER) || defined(__clang__)
template<typename T, T Size> using TMakeIntSeq = __make_integer_seq<TIntSeq, T, Size>;
#elif defined(__GNUC__)
template<typename T, T Size> using TMakeIntSeq = TIntSeq<T, __integer_pack(Size)...>;
#endif
template<size_t Size> using TMakeIndexSeq = TMakeIntSeq<size_t, Size>;
template<typename... Ts> using TSeqFor = TMakeIndexSeq<sizeof...(Ts)>;

template<typename T, T Add, class Seq> struct TIntSeqAdd;
template<typename T, T Add, T... Ints>
struct TIntSeqAdd<T, Add, TIntSeq<T, Ints...>>: TIntSeq<T, (Add+Ints)...> {};
template<size_t End, size_t Start = 0>
using TIndexRange = TIntSeqAdd<size_t, Start, TMakeIntSeq<size_t, End-Start>>;


template<typename... Ts> struct TList;
namespace z_D {
template<template<typename...> class TL, typename TFirst, typename... TRest>
struct TListTail_ {using _ = TL<TRest...>;};
}
template<template<typename...> class TL, typename... Ts>
using TListTail = typename z_D::TListTail_<TL, Ts...>::_;

template<class TL>
constexpr size_t TListLength = 0;
template<template<typename...> class TL, typename... Ts>
constexpr size_t TListLength<TL<Ts...>> = sizeof...(Ts);

namespace z_D {
template<size_t N, class TL> struct TListAt_;
template<size_t N, template<typename...> class TL, typename... Ts>
struct TListAt_<N, TL<Ts...>> {using _ = TPackAt<N, Ts...>;};
}
template<unsigned N, typename TL> using TListAt = typename z_D::TListAt_<N, TL>::_;

namespace z_D {
template<class TL, template<typename...> class DstTL> struct TListConvert;
template<template<typename...> class TL, template<typename...> class DstTL, typename... Ts>
struct TListConvert<TL<Ts...>, DstTL> {using _ = DstTL<Ts...>;};
}
template<class TL, template<typename...> class DstTL = TList>
using TListConvert = typename z_D::TListConvert<TL, DstTL>::_;

namespace z_D {
template<class TL1, typename T2> struct TListConcat_;
template<template<typename...> class TL, typename... Ts1, typename... Ts2>
struct TListConcat_<TL<Ts1...>, TL<Ts2...>> {using _ = TL<Ts1..., Ts2...>;};
template<template<typename...> class TL, typename T, typename... Ts>
struct TListConcat_<T, TL<Ts...>> {using _ = TL<Ts..., T>;};
}
template<class TL, typename... Ts>
using TListConcat = typename z_D::TListConcat_<TL, Ts...>::_;


namespace z_D {
template<class TL, typename T> struct TListRemove_;
template<template<typename...> class TL, typename T, typename... Args> struct TListRemove_<TL<Args...>, T>
{
	using Removed = typename TListRemove_<TListTail<TL, Args...>, T>::_;
	using First = TPackFirst<Args...>;
	using _ = TSelect<Removed, TListConcat<TL<First>, Removed>, CSame<First, T>>;
};

template<template<typename...> class TL, typename T, typename Head>
struct TListRemove_<TL<Head>, T> {using _ = TL<Head>;};
template<template<typename...> class TL, typename T>
struct TListRemove_<TL<T>, T> {using _ = TL<>;};
template<template<typename...> class TL, typename T>
struct TListRemove_<TL<>, T> {using _ = TL<>;};
}
template<class TL, typename T> using TListRemove = typename z_D::TListRemove_<TL, T>::_;


namespace z_D {
template<class TL> struct TListRemoveDuplicates_ {};
template<template<typename...> class TL> struct TListRemoveDuplicates_<TL<>> {using _ = TL<>;};
template<template<typename...> class TL, typename... Ts> struct TListRemoveDuplicates_<TL<Ts...>>
{
	using HeadRemovedFromTail = TListRemove<TPackFirst<Ts...>, TListTail<TL, Ts...>>; //TODO: is the order correct?
	using TailWithoutDuplicates = typename TListRemoveDuplicates_<HeadRemovedFromTail>::_;
	using _ = TListConcat<TailWithoutDuplicates, TL<TPackFirst<Ts...>>>;
};
}
template<class TL> using TListRemoveDuplicates_ = typename z_D::TListRemoveDuplicates_<TL>::_;


template<class TL, typename T, size_t IndexFrom = 0>
constexpr size_t TListFind = 0;

template<template<typename...> class TL, typename T, size_t IndexFrom>
constexpr size_t TListFind<TL<>, T, IndexFrom> = 0;

template<template<typename...> class TL, typename T, size_t IndexFrom, typename... Ts>
constexpr size_t TListFind<TL<Ts...>, T, IndexFrom> = IndexFrom +
	(CSame<TPackFirst<Ts...>, T>? 0: 1 + TListFind<TListTail<TL, Ts...>, T, IndexFrom>);

template<class TL, typename T, size_t IndexFrom = 0>
constexpr size_t TListFindUnique = TListFind<TL, T, IndexFrom>; //TODO: check for unique to make the commented test below pass

#if INTRA_CONSTEXPR_TEST
static_assert(TListFind<TList<int, float, float>, float> == 1);
static_assert(TListFind<TList<int, float, float>, double> == 3);
//static_assert(TListFindUnique<TList<int, float, float>, float> == 3);
#endif

namespace z_D {
template<class TL, size_t Begin, size_t Last>
struct TListSlice_ {using _ = void;};

template<template<typename...> class TL, size_t BeginAndEnd, typename... Ts>
struct TListSlice_<TL<Ts...>, BeginAndEnd, BeginAndEnd> {using _ = TL<>;};

template<template<typename...> class TL, size_t Begin, size_t End, typename... Ts>
struct TListSlice_<TL<Ts...>, Begin, End>
{
	static_assert(End >= Begin, "Invalid range!");
	using _ = TListConcat<
		typename TListSlice_<TL<Ts...>, Begin, End - 1>::_,
		TL<TPackAt<End-1, Ts...>>
	>;
};
}
template<class TL, size_t Begin, size_t End = TListLength<TL>>
using TListSlice = typename z_D::TListSlice_<TL,
	Begin <= TListLength<TL>? Begin: TListLength<TL>,
	End <= TListLength<TL>? End: TListLength<TL>>::_;

#if INTRA_CONSTEXPR_TEST
static_assert(CSame<
	TListSlice<TList<int, float>, 0, 1>,
	TList<int>
>);
#endif


template<template<typename> class Predicate, class TL>
constexpr bool CListAll = false;
template<template<typename> class Predicate, template<typename...> class TL, typename... Ts>
constexpr bool CListAll<Predicate, TL<Ts...>> = (Predicate<Ts>::_ && ...);

template<template<typename> class Predicate, class TL> constexpr bool CListAny = false;
template<template<typename> class Predicate, template<typename...> class TL, typename... Ts>
constexpr bool CListAny<Predicate, TL<Ts...>> = (Predicate<Ts>::_ || ...);


template<template<typename, typename> class Predicate, typename Arg0, class TL>
constexpr bool CListAll1 = false;
template<template<typename, typename> class Predicate, template<typename...> class TL, typename Arg0, typename... Ts>
constexpr bool CListAll1<Predicate, Arg0, TL<Ts...>> = (Predicate<Arg0, Ts>::_ && ...);

template<template<typename, typename> class Predicate, typename Arg0, class TL>
constexpr bool CListAny1 = false;
template<template<typename, typename> class Predicate, template<typename...> class TL, typename Arg0, typename... Ts>
constexpr bool CListAny1<Predicate, Arg0, TL<Ts...>> = (Predicate<Arg0, Ts>::_ || ...);


template<template<typename, typename> class Predicate, class TL1, class TL2>
constexpr bool CListAllPairs = false;
template<template<typename, typename> class Predicate, template<typename...> class TL, typename... Ts1, typename... Ts2>
constexpr bool CListAllPairs<Predicate, TL<Ts1...>, TL<Ts2...>> = (Predicate<Ts1, Ts2>::_ && ...);

template<template<typename, typename> class Predicate, class TL1, class TL2>
constexpr bool CListAnyPair = false;
template<template<typename, typename> class Predicate, template<typename...> class TL, typename... Ts1, typename... Ts2>
constexpr bool CListAnyPair<Predicate, TL<Ts1...>, TL<Ts2...>> = (Predicate<Ts1, Ts2>::_ || ...);

namespace z_D {
template<class TL, template<typename...> class Transform, typename... BindArgs>
struct TListTransform_ {using _ = void;};
template<template<typename...> class Transform, template<typename...> class TL, typename... BindArgs, typename... Ts>
struct TListTransform_<TL<Ts...>, Transform, BindArgs...> {using _ = TL<Transform<BindArgs..., Ts>...>;};

template<class TL, template<typename...> class Transform>
struct TListUnpackTo_ {using _ = void;};
template<template<typename...> class C, template<typename...> class TL, typename... Ts>
struct TListUnpackTo_<TL<Ts...>, C> {using _ = C<Ts...>;};
}
template<class TL, template<typename...> class Transform, typename... BindArgs>
using TListTransform = typename z_D::TListTransform_<TL, Transform, BindArgs...>::_;
template<class TL, template<typename...> class C>
using TListUnpackTo = typename z_D::TListUnpackTo_<TL, C>::_;

template<typename T, auto... Values> constexpr auto VMapByType = TListAt<TListFind<TList<decltype(Values)...>, T>, TList<TValue<Values>...>>::_;

#if INTRA_CONSTEXPR_TEST
static_assert(TListFind<TList<int, float, double>, int> == 0);
static_assert(TListFind<TList<int, float, double>, float> == 1);
static_assert(TListFind<TList<int, float, double>, void> == 3);
static_assert(CSame<TPackFirst<int, float, double>, int>);
static_assert(CSame<TListTail<TList, int, float, double>, TList<float, double>>);
static_assert(CSame<TListRemove<TList<int, float, double>, float>, TList<int, double>>);
static_assert(CSame<TListSlice<TList<int, float, double, void>, 4, 4>, TList<>>);
static_assert(CSame<TListSlice<TList<int, float, double, void>, 1, 3>, TList<float, double>>);
static_assert(CSame<TListTransform<TList<float, const double, const void>, TRemoveConst>, TList<float, double, void>>);

static_assert(VMapByType<int32,
	uint32(4294967295),
	int16(32767),
	int32(2147483647),
	float(3.14f)
> == 2147483647);
#endif

template<class TL> using TListCommon = TListUnpackTo<TL, TCommon>;
namespace z_D {
template<typename To, typename From> using CAssignableT = TValue<CAssignable<To, From>>;
template<typename From, typename To> using CConvertibleToT = TValue<CConvertibleTo<From, To>>;
}
template<class From, class To> concept CTListConvertible = CListAllPairs<z_D::CConvertibleToT, From, To>;
template<class To, class From> concept CTListAssignable = CListAllPairs<z_D::CAssignableT, To, From>;
INTRA_END
