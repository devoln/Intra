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
//INTRA_IGNORE_WARNING_COPY_MOVE_CONSTRUCT_IMPLICITLY_DELETED
//INTRA_IGNORE_WARNING_LOSING_CONVERSION
//INTRA_IGNORE_WARNING_SIGN_CONVERSION
//INTRA_IGNORE_WARNING_LANGUAGE_EXTENSION

template<typename T> T&& Val();
struct UniFunctor {template<typename... Args> void operator()(Args&&...);};

template<typename T> struct TConstructT {};
template<typename T> constexpr TConstructT<T> ConstructT;
struct EmptyType
{
	template<typename... Args> EmptyType(Args&&...) {}
};

template<typename T, T Value> struct TValueToType {static constexpr T _ = Value;};

struct NonCopyableType
{
	NonCopyableType() = default;
	NonCopyableType(NonCopyableType&&) = default;
	NonCopyableType& operator=(NonCopyableType&&) = default;
	NonCopyableType(const NonCopyableType&) = delete;
	NonCopyableType& operator=(const NonCopyableType&) = delete;
};

template<typename T> struct WrapperStruct {T value;};
template<typename T, size_t N = 0> struct TWrapper: T
{
	using T::T;
	constexpr TWrapper(const T& base): T(base) {}
	constexpr TWrapper(T&& base): T(Move(base)) {}
	TWrapper(const TWrapper&) = default;
	TWrapper(TWrapper&&) = default;
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

template<typename T> constexpr bool CUnknownBoundArrayType = false;
template<typename T> constexpr bool CUnknownBoundArrayType<T[]> = true;
template<typename T> constexpr bool CKnownBoundArrayType = false;
template<typename T, size_t N> constexpr bool CKnownBoundArrayType<T[N]> = true;
template<typename T> concept CArrayType = CUnknownBoundArrayType<T> || CKnownBoundArrayType<T>;

template<typename T> concept CLValueReference = z_D::CLValueReference_<T>;
template<typename T> constexpr bool CRValueReference = false;
template<typename T> constexpr bool CRValueReference<T&&> = true;

template<typename T> concept CReference = CLValueReference<T> || CRValueReference<T>;

template<typename T> constexpr bool CConst = false;
template<typename T> constexpr bool CConst<const T> = true;
template<typename T> constexpr bool CVolatile = false;
template<typename T> constexpr bool CVolatile<volatile T> = true;

template<typename T> concept CNCLValueReference = CLValueReference<T> && !CConst<TRemoveReference<T>>;
template<typename T> concept CNCRValueReference = CRValueReference<T> && !CConst<TRemoveReference<T>>;

namespace z_D {
template<typename IfTrue, typename IfFalse, bool COND>
struct TSelect_ {using _ = IfTrue;};
template<typename IfTrue, typename IfFalse>
struct TSelect_<IfTrue, IfFalse, false> {using _ = IfFalse;};
}
template<typename IfTrue, typename IfFalse, bool COND>
using TSelect = typename z_D::TSelect_<IfTrue, IfFalse, COND>::_;


template<bool COND> using CopyableIf = TSelect<EmptyType, NonCopyableType, COND>;

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
}

template<typename T> using TRemovePointer = typename z_D::TRemovePointer_<T>::_;
template<typename T> using TRemoveConst = typename z_D::TRemoveConst_<T>::_;
template<typename T> using TRemoveVolatile = typename z_D::TRemoveVolatile_<T>::_;
template<typename T> using TRemoveConstRef = TRemoveConst<TRemoveReference<T>>;
template<typename T> using TUnqual = TRemoveConst<TRemoveVolatile<T>>;
template<typename T> using TRemoveExtent = typename z_D::TRemoveExtent_<T>::_;

//TODO: doesn't work if T is a cv- or ref-qualified function type
template<typename T> using TAddPointer = TRemoveReference<T>*;


namespace z_D {
template<typename... Ts> constexpr bool CSame_ = false;
template<typename T> constexpr bool CSame_<T> = true;
template<typename T> constexpr bool CSame_<T, T> = true;
template<typename T1, typename T2, typename T3, typename... Ts>
constexpr bool CSame_<T1, T2, T3, Ts...> = CSame_<T1, T2> && CSame_<T2, T3> && (CSame_<T3, Ts> && ...);
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

template<typename T> constexpr bool CFunctionPointer = false;
#ifdef _MSC_VER
#ifdef __i386__
template<typename Ret, typename... Args>
constexpr bool CFunctionPointer<Ret(__cdecl*)(Args...)> = true;
template<typename Ret, typename... Args>
constexpr bool CFunctionPointer<Ret(__stdcall*)(Args...)> = true;
template<typename Ret, typename... Args>
constexpr bool CFunctionPointer<Ret(__fastcall*)(Args...)> = true;
#else
template<typename Ret, typename... Args>
constexpr bool CFunctionPointer<Ret(*)(Args...)> = true;
#if defined(__amd64__)
template<typename Ret, typename... Args>
constexpr bool CFunctionPointer<Ret(__vectorcall*)(Args...)> = true;
#endif
#endif
#else
template<typename Ret, typename... Args>
constexpr bool CFunctionPointer<Ret(*)(Args...)> = true;
#endif
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
template<typename T> concept CStandardLayout = __is_standard_layout(T);
template<typename T> concept CTriviallyCopyable = __is_trivially_copyable(T);
template<typename T, typename... Args> concept CTriviallyConstructible = __is_trivially_constructible(T, Args...);
template<typename T, typename Arg> concept CTriviallyAssignable = __is_trivially_assignable(T, Arg);
template<typename T> concept CTriviallyCopyAssignable =
	CTriviallyAssignable<TAddLValueReference<T>, TAddLValueReference<const T>>;
template<typename T> concept CTriviallyMoveAssignable =
	CTriviallyAssignable<TAddLValueReference<T>, TAddRValueReference<T>>;
#ifdef __clang__
#if !__is_identifier(__has_unique_object_representations) //since clang 6
#define INTRA_SUPPORT_CHasUniqueObjectRepresentations
#endif
#else
#define INTRA_SUPPORT_CHasUniqueObjectRepresentations
#endif
template<typename T> concept CHasUniqueObjectRepresentations =
#ifdef INTRA_SUPPORT_CHasUniqueObjectRepresentations
	__has_unique_object_representations(T);
#else
	CTriviallyCopyable<T>;
#endif
#undef INTRA_SUPPORT_CHasUniqueObjectRepresentations

template<typename T> concept CPlainVoid = CSame<T, void>;
template<typename T> concept CPlainUnsignedIntegral = CAnyOf<T,
	char16_t, char32_t, bool, byte, uint16, unsigned, unsigned long, uint64
#if defined(__CHAR_UNSIGNED__) || defined(_CHAR_UNSIGNED)
	, char
#endif
#if defined(__WCHAR_UNSIGNED__) || defined(_MSC_VER)
	, wchar_t
#endif
>;

template<typename T> concept CPlainSignedIntegral = CAnyOf<T,
	int8, short, int, long, int64
#if !defined(__CHAR_UNSIGNED__) && !defined(_CHAR_UNSIGNED)
	, char
#endif
#if !defined(__WCHAR_UNSIGNED__) && !defined(_MSC_VER)
	, wchar_t
#endif
>;

template<typename T> concept CPlainFloatingPoint = CAnyOf<T, float, double, long double>;
//TODO: compiler extensions: _fp16, _Float16, _float128

/** note: signed char and unsigned char are not treated as character types.
	They are separate types from char becasue Intra doesn't use them to store characters.
*/
template<typename T> concept CPlainChar = CAnyOf<T, char, char16_t, char32_t, wchar_t
#ifdef __cpp_char8_t
	, char8_t
#endif
>;

template<typename T> concept CPlainSigned = CPlainSignedIntegral<T> || CPlainFloatingPoint<T>;
template<typename T> concept CPlainIntegral = CPlainUnsignedIntegral<T> || CPlainSignedIntegral<T>;
template<typename T> concept CPlainArithmetic = CPlainIntegral<T> || CPlainFloatingPoint<T>;

template<typename T> constexpr bool CPlainPointer = false;
template<typename T> constexpr bool CPlainPointer<T*> = true;

template<typename T> constexpr bool CPlainMemberPointer = false;
template<typename T, class U> constexpr bool CPlainMemberPointer<T U::*> = true;

template<typename T> constexpr bool CPlainMethodPointer = false;
template<typename T, typename U> constexpr bool CPlainMethodPointer<T U::*> = CFunction<T>;

template<typename T> concept CVoid = CPlainVoid<TUnqual<T>>;
template<typename T> concept CSigned = CPlainSigned<TUnqual<T>>;
template<typename T> concept CIntegral = CPlainIntegral<TUnqual<T>>;
template<typename T> concept CSignedIntegral = CPlainSignedIntegral<TUnqual<T>>;
template<typename T> concept CUnsignedIntegral = CPlainUnsignedIntegral<TUnqual<T>>;
template<typename T> concept CFloatingPoint = CPlainFloatingPoint<TUnqual<T>>;
template<typename T> concept CChar = CPlainChar<TUnqual<T>>;
template<typename T> concept CArithmetic = CPlainArithmetic<TUnqual<T>>;
template<typename T> concept CPointer = CPlainPointer<TUnqual<T>>;
template<typename T> concept CMemberPointer = CPlainMemberPointer<TUnqual<T>>;
template<typename T> concept CMethodPointer = CPlainMethodPointer<TUnqual<T>>;

template<typename From, typename To> using TPropagateConst = TSelect<const To, To, CConst<From>>;
template<typename From, typename To> using TPropagateVolatile = TSelect<volatile To, To, CVolatile<From>>;
template<typename From, typename To> using TPropagateQualifiers = TPropagateConst<From, TPropagateVolatile<From, To>>;
template<typename From, typename To> using TPropagateLVRef = TSelect<To&, To, CLValueReference<From>>;
template<typename From, typename To> using TPropagateRVRef = TSelect<To&&, To, CRValueReference<From>>;
template<typename From, typename To> using TPropagateRef = TPropagateLVRef<From, TPropagateRVRef<From, To>>;
template<typename From, typename To> using TPropagateQualLVRef = TPropagateLVRef<From, TPropagateQualifiers<TRemoveReference<From>, To>>;

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
		TUnqual<TRemoveReference<T>>,
		CFunction<TRemoveReference<T>>>,
	CArrayType<TRemoveReference<T>>>;



namespace z_D {
template<typename T> struct TRemoveAllExtents_ {using _ = T;};
template<typename T> struct TRemoveAllExtents_<T[]>: TRemoveAllExtents_<T> {};
template<typename T, size_t N> struct TRemoveAllExtents_<T[N]>: TRemoveAllExtents_<T> {};
}
template<typename T> using TRemoveAllExtents = typename z_D::TRemoveAllExtents_<T>::_;

#ifdef __cpp_concepts
#define INTRA_DEFINE_CONCEPT_REQUIRES(checker_name, expr) \
	template<typename T> concept checker_name = requires(T) {expr;}
#else
#define INTRA_DEFINE_CONCEPT_REQUIRES(checker_name, expr) \
	struct z_D_ ## checker_name { \
		template<typename T> static auto test(int) -> decltype((expr), TBool<true>()); \
		template<typename T> static TBool<false> test(...); \
	}; \
	template<typename U> concept checker_name = decltype(z_D_ ## checker_name::test<U>(0))::_
#endif

#ifdef __cpp_concepts
#define INTRA_DEFINE_CONCEPT_REQUIRES2(checker_name, expr, default1, default2) \
	template<typename T1 default1, typename T2 default2> concept checker_name = requires(T1, T2) {expr;}
#else
#define INTRA_DEFINE_CONCEPT_REQUIRES2(checker_name, expr, default1, default2) \
	struct z_D_ ## checker_name {\
		template<typename T1, typename T2> static auto test(int) -> decltype((expr), TBool<true>());\
		template<typename T1, typename T2> static TBool<false> test(...); \
	};\
	template<typename U1 default1, typename U2 default2> concept checker_name = \
		decltype(z_D_ ## checker_name::test<U1, U2>(0))::_
#endif

#ifdef __cpp_concepts
#define INTRA_DEFINE_CONCEPT_REQUIRES_T_ARGS(checker_name, expr) \
	template<typename T, typename... Args> concept checker_name = requires(T val, Args... vals) {expr;}
#else
#define INTRA_DEFINE_CONCEPT_REQUIRES_T_ARGS(checker_name, expr) \
	struct z_D_ ## checker_name {\
		template<typename T, typename... Args> static auto test(int) -> decltype((expr), TBool<true>());\
		template<typename T, typename... Args> static TBool<false> test(...); \
	};\
	template<typename U, typename... UArgs> concept checker_name = \
		decltype(z_D_ ## checker_name::test<U, UArgs...>(0))::_
#endif

#define INTRA_DEFINE_SAFE_DECLTYPE(checker_name, expr) \
	struct z_D_ ## checker_name {\
		template<typename T> static auto test(int) -> decltype(expr);\
		template<typename T> static void test(...);\
	};\
	template<typename U> using checker_name = decltype(z_D_ ## checker_name::test<U>(0))

#define INTRA_DEFINE_CONCEPT_EXPR(checker_name, expr, resultOnError) \
	struct z_D_ ## checker_name {\
		template<typename T> static constexpr auto test(int) -> decltype((expr), bool()) {return expr;}\
		template<typename T> static constexpr bool test(...) {return resultOnError;}\
	};\
	template<typename U> concept checker_name = z_D_ ## checker_name::test<U>(0)

template<typename T> concept CScalar =
	CArithmetic<T> ||
	CPointer<T> ||
	CEnum<T> ||
	CMemberPointer<T> ||
	CSame<TUnqual<T>, decltype(null)>;

template<typename T> constexpr bool IsByteByByteLexicographicallyComparableCustomType = false;
template<typename T> constexpr bool IsBitwiseEqualityComparableCustomType =
	IsByteByByteLexicographicallyComparableCustomType<T>;

template<typename T> concept CByteByByteLexicographicallyComparable =
	(CHasUniqueObjectRepresentations<T> && IsByteByByteLexicographicallyComparableCustomType<TUnqual<T>> ||
		(CIntegral<T> || CPointer<T> || CEnum<T>) && (sizeof(T) == 1 || TargetIsBigEndian));

template<typename T> concept CTriviallyEqualComparable = CScalar<T> ||
	CByteByByteLexicographicallyComparable<T> ||
	IsBitwiseEqualityComparableCustomType<TUnqual<T>>;



#ifdef _MSC_VER
template<typename T> concept CDestructible = __is_destructible(T);
#else
namespace z_D {
INTRA_DEFINE_CONCEPT_REQUIRES(CDestructible_, Val<T>().~T());
}
template<typename T> concept CDestructible = CReference<T> ||
	!CUnknownBoundArrayType<T> &&
	z_D::CDestructible_<TRemoveAllExtents<T>>;
#endif

#if INTRA_CONSTEXPR_TEST
static_assert(!CDestructible<int[]>);
#endif

INTRA_DEFINE_CONCEPT_REQUIRES2(CStaticCastable, static_cast<T2>(Val<T1>()),,);

#if defined(_MSC_VER) || defined(__clang__) || defined(__GNUC__) && __GNUC__ >= 8
template<typename T, typename... Args> concept CConstructible = __is_constructible(T, Args...);
#else
namespace z_D {
INTRA_DEFINE_CONCEPT_EXPR(CDefaultConstructible_, (
	TRemoveAllExtents<T>(),
	(!CUnknownBoundArrayType<T> && !CSame<TRemoveAllExtents<T>, void>)
	), false);
INTRA_DEFINE_CONCEPT_REQUIRES2(CDirectConstructibleImpl, ::new T1(val2));
INTRA_DEFINE_CONCEPT_REQUIRES_T_ARGS(CNaryConstructible, T(Val<Args>()...));
template<typename T, typename Arg> concept CDirectConstructible =
		CReference<T>?
		CStaticCastable<Arg, T> &&
		!(!CSame<Arg, void> && !CFunction<Arg> &&
			!CSame<TUnqual<TRemoveReference<Arg>>, TUnqual<TRemoveReference<T>>> &&
			CDerived<TUnqual<TRemoveReference<T>>, TUnqual<TRemoveReference<Arg>>>
		) &&
		!(CLValueReference<Arg> && CRValueReference<DST> &&
			((!CFunction<TUnqual<TRemoveReference<Arg>>> &&
				CSame<TUnqual<TRemoveReference<Arg>>, TUnqual<TRemoveReference<T>>>) ||
			CDerived<TUnqual<TRemoveReference<Arg>>, TUnqual<TRemoveReference<T>>>)):
		CDestructible<T> && CDirectConstructibleImpl<T, Arg>;
template<typename T, typename... Args> constexpr bool CConstructible_ = CNaryConstructible<T, Args...>;
template<typename T> constexpr bool CConstructible_<T> = CDefaultConstructible_<T>;
template<typename T, typename Arg> constexpr bool CConstructible_<T, Arg> = CDirectConstructible<T>;
}
template<typename T, typename... Args> concept CConstructible = z_D::CConstructible_<T, Args...>;
#endif
template<typename T> concept CDefaultConstructible = CConstructible<T>;

#if defined(_MSC_VER) || defined(__clang__)
template<typename T, typename... Args> concept CNothrowConstructible = __is_nothrow_constructible(T, Args...);
#else
namespace z_D {
template<bool, typename T, typename... Args> constexpr bool isNothrowConstructible = false;
template<typename T, typename... Args>
constexpr bool isNothrowConstructible<true, T, Args...> = noexcept(T(Val<Args>()...));
template<typename T, typename Arg>
constexpr bool isNothrowConstructible<true, T, Arg> = noexcept(static_cast<T>(Val<Arg>()));
template<typename T> constexpr bool isNothrowConstructible<true, T> = noexcept(T());
template<typename T, size_t N>
constexpr bool isNothrowConstructible<true, T[N]> = noexcept(TRemoveExtent<T>());
}
template<typename T, typename... Args>
concept CNothrowConstructible = z_D::isNothrowConstructible<CConstructible<T, Args...>, T, Args...>;
#endif

template<typename T> concept CCopyConstructible = CConstructible<T, TAddLValueReference<const T>>;
template<typename T> concept CMoveConstructible = CConstructible<T, T>;
template<typename T> concept CTriviallyCopyConstructible = CTriviallyConstructible<T, TAddLValueReference<const T>>;
template<typename T> concept CTriviallyDefaultConstructible = CTriviallyConstructible<T>;
template<typename T> concept CTriviallyMoveConstructible = CTriviallyConstructible<T, T>;
template<typename T> concept CNothrowCopyConstructible = CNothrowConstructible<T, TAddLValueReference<const T>>;
template<typename T> concept CNothrowDefaultConstructible = CNothrowConstructible<T>;
template<typename T> concept CNothrowMoveConstructible = CNothrowConstructible<T, T>;

struct D_impl_CCallable {
	template<typename T, typename... Args> static decltype((Val<TRemoveReference<T>>()(Val<Args>()...)), TBool<true>()) func(int);
	template<typename T, typename... Args> static TBool<false> func(...);
};
template<typename T, typename... Args> concept CCallable = decltype(D_impl_CCallable::func<T, Args...>(0))::_;

template<typename T, typename... Args> using TResultOf = decltype(Val<TRemoveReference<T>>()(Val<Args>()...));
namespace z_D {
template<bool, typename T, typename... Args> struct TResultOfOrVoid {typedef TResultOf<T, Args...> _;};
template<typename T, typename... Args> struct TResultOfOrVoid<false, T, Args...> {typedef void _;};
}
template<typename T, typename... Args> using TResultOfOrVoid = typename z_D::TResultOfOrVoid<CCallable<T, Args...>, T, Args...>::_;


#if defined(_MSC_VER) || defined(__clang__) || defined(__GNUC__) && __GNUC__ >= 8
template<typename To, typename From> concept CAssignable = __is_assignable(To, From);
#else
INTRA_DEFINE_CONCEPT_REQUIRES2(CAssignable, val1 = val2,,);
#endif

template<typename T> concept CCopyAssignable = CAssignable<TAddLValueReference<T>, TAddLValueReference<const T>>;
template<typename T> concept CMoveAssignable = CAssignable<TAddLValueReference<T>, T>;

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
template<bool COND, class T=void> struct TRequires {};
template<class T> struct TRequires<true, T> {using _ = T;};
template<bool assertCondition, class T> struct TRequiresAssert {static_assert(assertCondition); using _ = T;};
}
template<bool COND, typename T = void> using Requires = typename z_D::TRequires<COND, T>::_;
template<bool COND, typename T = void> using RequiresAssert = typename z_D::TRequiresAssert<COND, T>::_;

#if defined(_MSC_VER) || defined(__clang__)
template<typename SRC, typename DST> concept CConvertibleTo = __is_convertible_to(SRC, DST);
#else
namespace z_D {
INTRA_DEFINE_CONCEPT_REQUIRES2(CConvertibleTo_, Val<void(*)(T2)>()(Val<T1>()),,);
}
template<typename From, typename To> concept CConvertibleTo = z_D::CConvertibleTo_<From, To> || CVoid<From> && CVoid<To>;
#endif

namespace z_D {
template<typename... Types>
struct TCommon_ {using _ = void;};
template<typename T>
struct TCommon_<T> {using _ = TDecay<T>;};
template<typename T, typename U>
struct TCommon_<T, U> {using _ = TDecay<decltype(bool()? Val<T>(): Val<U>())>;};
template<typename T, typename U, typename... V>
struct TCommon_<T, U, V...> {using _ = typename TCommon_<typename TCommon_<T, U>::_, V...>::_;};
}
//! Common type without const\volatile and references.
template<typename... Ts> using TCommon = typename z_D::TCommon_<Ts...>::_;

//! Common type preserving references.
template<typename... Ts> using TCommonRef = TSelect<
	TSelect<const TAddLValueReference<TCommon<Ts...>>, TAddLValueReference<TCommon<Ts...>>, (CConst<TDecay<Ts>> || ...)>,
	TCommon<Ts...>,
	CSame<TDecay<Ts>...> && (CReference<Ts> && ...)>;

/** Core function classes.
  @see http://ericniebler.com/2014/11/13/tiny-metaprogramming-library/ for more information.
*/

//! Wrap meta function into a metafunction class.
template<template<typename...> class MetaFunction> struct TMetaWrap {template<typename...Ts> using Apply = MetaFunction<Ts...>;};

//! Apply metafunction from metafunction class F
template<typename F, typename... As> using TMetaApply = typename F::template Apply<As...>;

//! Compose multiple metafunctions from classes F1, Fs... into a metafunction class performing a chain of transformations: F1(...(F[N-1](F[N](Arg))))
template<typename F1, typename... Fs> struct TMetaCompose {template<typename T> using Apply = TMetaApply<F1, TMetaApply<TMetaCompose<Fs...>, T>>;};
template<typename F1> struct TMetaCompose<F1> {template<typename T> using Apply = TMetaApply<F1, T>;};

//! Wrap integer sequence into a metafunction class
template<typename Int, template<Int...> class F> struct TMetaWrapI {template<typename... Ts> using Apply = F<Ts::_...>;};

//! Metafunction class containing a constant metafunction that always returns `T`.
template<typename T> struct TMetaAlways {template<typename...> using Apply = T;};

namespace z_D
{
	template<typename T> struct TMemberFieldType_;
	template<typename T, typename F> struct TMemberFieldType_<F T::*> {using _ = F;};
}
template<typename T> using TMemberFieldType = typename z_D::TMemberFieldType_<T>::_;

namespace z_D {
template<typename F> struct TFinally
{
	INTRA_CONSTEXPR_CPP20 ~TFinally() {OnDestruct();}
	F OnDestruct;
};

struct TFinallyMaker
{
	template<typename F> constexpr Requires<
		CCallable<F>,
	TFinally<F>> operator=(F&& f) const noexcept {return {Forward<F>(f)};}
};
}
/** Execute a statement block after leaving the current scope for any reason: normal or exception.
  usage: INTRA_FINALLY{<code block>};
*/
#define INTRA_FINALLY auto INTRA_CONCATENATE_TOKENS(z_D_finally_, __COUNTER__) = ::Intra::z_D::TFinallyMaker() = [&]()->void

template<class T, typename U> inline size_t MemberOffset(U T::* member) noexcept
{return reinterpret_cast<size_t>(&((static_cast<T*>(null))->*member));}

template<class T> inline T* AddressOf(T& arg) noexcept
{return reinterpret_cast<T*>(&const_cast<char&>(reinterpret_cast<const volatile char&>(arg)));}

#if INTRA_CONSTEXPR_TEST
static_assert(CArrayType<TRemoveReference<const char(&)[9]>>);
// TODO: add more tests
#endif

template<typename T, T... Ints> struct TIntSeq {using _ = TIntSeq;};
template<size_t... Ints> using TIndexSeq = TIntSeq<size_t, Ints...>;

#if defined(_MSC_VER) || defined(__clang__)
template<typename T, T Size> using TMakeIntSeq = __make_integer_seq<TIntSeq, T, Size>;
#elif defined(__GNUC__) && __GNUC__ >= 8
template<typename T, T Size> using TMakeIntSeq = TIntSeq<T, __integer_pack(Size)...>;
#else
// Generic O(log N) implementation
namespace z_D {
template<typename T, class Sequence1, class Sequence2> struct merge_and_renumber;
template<typename T, T... I1, T... I2>
struct merge_and_renumber<T, TIntSeq<T, I1...>, TIntSeq<T, I2...>>:
	TIntSeq<T, I1..., (sizeof...(I1)+I2)...> {};
}
template<typename T, T Size> struct TMakeIntSeq:
	z_D::merge_and_renumber<T, typename TMakeIntSeq<T, Size/2>::_,
		typename TMakeIntSeq<T, Size - Size/2>::_> {};
template<typename T> struct TMakeIntSeq<T, 0>: TIntSeq<T> {};
template<typename T> struct TMakeIntSeq<T, 1>: TIntSeq<T, 0> {};
#endif
template<size_t Size> using TMakeIndexSeq = TMakeIntSeq<size_t, Size>;
template<typename... Ts> using TSeqFor = TMakeIndexSeq<sizeof...(Ts)>;

template<typename T, T Add, class Seq> struct TIntSeqAdd;
template<typename T, T Add, T... Ints>
struct TIntSeqAdd<T, Add, TIntSeq<T, Ints...>>: TIntSeq<T, (Add+Ints)...> {};
template<size_t End, size_t Start = 0>
using TIndexRange = TIntSeqAdd<size_t, Start, TMakeIntSeq<size_t, End-Start>>;


template<typename... Args>
struct TList {static constexpr size_t Length = sizeof...(Args);};
namespace z_D {
template<typename T0, typename... Args>
struct TListTail_ {using _ = TList<Args...>;};
}
template<typename... Args> using TListTail = typename z_D::TListTail_<Args...>::_;

namespace z_D {
template<size_t N, typename TL> struct TListAt_;
template<size_t N, typename... Args>
struct TListAt_<N, TList<Args...>> {using _ = TPackAt<N, Args...>;};
}
template<unsigned N, typename TL> using TListAt = typename z_D::TListAt_<N, TL>::_;

namespace z_D {
template<typename TL1, typename T2> struct TListConcat_;
template<typename... Args1, typename... Args2>
struct TListConcat_<TList<Args1...>, TList<Args2...>> {using _ = TList<Args1..., Args2...>;};
template<typename T, typename... Args>
struct TListConcat_<T, TList<Args...>> {using _ = TList<Args..., T>;};
}
template<typename TL, typename... Args>
using TListConcat = typename z_D::TListConcat_<TL, Args...>::_;


namespace z_D {
template<typename TL, typename T> struct TListRemove_;
template<typename T, typename... Args> struct TListRemove_<TList<Args...>, T>
{
	using Removed = typename TListRemove_<TListTail<Args...>, T>::_;
	using First = TPackFirst<Args...>;
	using _ = TSelect<Removed, TListConcat<TList<First>, Removed>, CSame<First, T>>;
};

template<typename T, typename Head> struct TListRemove_<TList<Head>, T> {using _ = TList<Head>;};
template<typename T> struct TListRemove_<TList<T>, T> {using _ = TList<>;};
template<typename T> struct TListRemove_<TList<>, T> {using _ = TList<>;};
}
template<typename TL, typename T> using TListRemove = typename z_D::TListRemove_<TL, T>::_;


namespace z_D {
template<typename TL> struct TListRemoveDuplicates_ {};
template<> struct TListRemoveDuplicates_<TList<>> {using _ = TList<>;};
template<typename... Args> struct TListRemoveDuplicates_<TList<Args...>>
{
	using HeadRemovedFromTail = TListRemove<TPackFirst<Args...>, TListTail<Args...>>; //TODO: is the order correct?
	using TailWithoutDuplicates = typename TListRemoveDuplicates_<HeadRemovedFromTail>::_;
	using _ = TListConcat<TailWithoutDuplicates, TList<TPackFirst<Args...>>>;
};
}
template<typename TL> using TListRemoveDuplicates_ = typename z_D::TListRemoveDuplicates_<TL>::_;


template<typename TL, typename T, size_t IndexFrom = 0> constexpr size_t TListFind = 0;
template<typename T, size_t IndexFrom> constexpr size_t TListFind<TList<>, T, IndexFrom> = 0;
template<typename T, size_t IndexFrom, typename... Args>
constexpr size_t TListFind<TList<Args...>, T, IndexFrom> = IndexFrom +
	(CSame<TPackFirst<Args...>, T>? 0: 1 + TListFind<TListTail<Args...>, T, IndexFrom>);


namespace z_D {
template<typename TL, size_t Begin, size_t Last> struct TListSlice_ {using _ = void;};
template<size_t BeginAndEnd, typename... Args>
struct TListSlice_<TList<Args...>, BeginAndEnd, BeginAndEnd> {using _ = TList<>;};
template<size_t Begin, size_t End, typename... Args> struct TListSlice_<TList<Args...>, Begin, End>
{
	static_assert(End >= Begin, "Invalid range!");
	using _ = TListConcat<
		typename TListSlice_<TList<Args...>, Begin, End - 1>::_,
		TList<TPackAt<End-1, Args...>>
	>;
};
}
template<typename TL, size_t Begin, size_t End = TL::Length>
using TListSlice = typename z_D::TListSlice_<TL,
	Begin <= TL::Length? Begin: TL::Length,
	End <= TL::Length? End: TL::Length>::_;

#if INTRA_CONSTEXPR_TEST
static_assert(CSame<
	TListSlice<TList<int, float>, 0, 1>,
	TList<int>
>);
#endif

template<typename T0> constexpr T0 VMin(T0 t0) {return t0;}
template<typename T0, typename T1>
constexpr T0 VMin(T0 t0, T1 t1) {return t0 < t1? t0: t1;}
template<typename T0, typename T1, typename... Ts>
constexpr auto VMin(T0 t0, T1 t1, Ts... ts) {return VMin(t0, VMin(t1, ts...));}


template<template<typename> class Predicate, class TL>
constexpr bool CListAll = false;
template<template<typename> class Predicate, typename... Ts>
constexpr bool CListAll<Predicate, TList<Ts...>> = (Predicate<Ts>::_ && ...);

template<template<typename> class Predicate, class TL> constexpr bool CListAny = false;
template<template<typename> class Predicate, typename... Ts>
constexpr bool CListAny<Predicate, TList<Ts...>> = (Predicate<Ts>::_ || ...);


template<template<typename, typename> class Predicate, typename Arg0, class TL>
constexpr bool CListAll1 = false;
template<template<typename, typename> class Predicate, typename Arg0, typename... Ts>
constexpr bool CListAll1<Predicate, Arg0, TList<Ts...>> = (Predicate<Arg0, Ts>::_ && ...);

template<template<typename, typename> class Predicate, typename Arg0, class TL>
constexpr bool CListAny1 = false;
template<template<typename, typename> class Predicate, typename Arg0, typename... Ts>
constexpr bool CListAny1<Predicate, Arg0, TList<Ts...>> = (Predicate<Arg0, Ts>::_ || ...);


template<template<typename, typename> class Predicate, class TL1, class TL2>
constexpr bool CListAllPairs = false;
template<template<typename, typename> class Predicate, typename... Ts1, typename... Ts2>
constexpr bool CListAllPairs<Predicate, TList<Ts1...>, TList<Ts2...>> = (Predicate<Ts1, Ts2>::_ && ...);

template<template<typename, typename> class Predicate, class TL1, class TL2>
constexpr bool CListAnyPair = false;
template<template<typename, typename> class Predicate, typename... Ts1, typename... Ts2>
constexpr bool CListAnyPair<Predicate, TList<Ts1...>, TList<Ts2...>> = (Predicate<Ts1, Ts2>::_ || ...);

namespace z_D {
template<class TL, template<typename> class Transform>
struct TListTransform_ {using _ = void;};
template<template<typename> class Transform, typename... Ts>
struct TListTransform_<TList<Ts...>, Transform> {using _ = TList<Transform<Ts>...>;};

template<class TL, template<typename, typename> class Transform, typename Arg>
struct TListTransform1_ {using _ = void;};
template<template<typename, typename> class Transform, typename Arg, typename... Ts>
struct TListTransform1_<TList<Ts...>, Transform, Arg> {using _ = TList<Transform<Arg, Ts>...>;};

template<class TL, template<typename...> class Transform>
struct TListUnpackTo_ {using _ = void;};
template<template<typename...> class C, typename... Ts>
struct TListUnpackTo_<TList<Ts...>, C> {using _ = C<Ts...>;};
}
template<class TL, template<typename> class Transform>
using TListTransform = typename z_D::TListTransform_<TL, Transform>::_;
template<class TL, template<typename, typename> class Transform, typename Arg>
using TListTransform1 = typename z_D::TListTransform1_<TL, Transform, Arg>::_;
template<class TL, template<typename...> class C>
using TListUnpackTo = typename z_D::TListUnpackTo_<TL, C>::_;

#if INTRA_CONSTEXPR_TEST
static_assert(TListFind<TList<int, float, double>, int> == 0);
static_assert(TListFind<TList<int, float, double>, float> == 1);
static_assert(TListFind<TList<int, float, double>, void> == 3);
static_assert(CSame<TPackFirst<int, float, double>, int>);
static_assert(CSame<TListTail<int, float, double>, TList<float, double>>);
static_assert(CSame<TListRemove<TList<int, float, double>, float>, TList<int, double>>);
static_assert(CSame<TListSlice<TList<int, float, double, void>, 4, 4>, TList<>>);
static_assert(CSame<TListSlice<TList<int, float, double, void>, 1, 3>, TList<float, double>>);
static_assert(CSame<TListTransform<TList<float, const double, const void>, TRemoveConst>, TList<float, double, void>>);
#endif

template<class TL> using TListCommon = TListUnpackTo<TL, TCommon>;
namespace z_D {
template<typename To, typename From> using CAssignableT = TBool<CAssignable<To, From>>;
template<typename From, typename To> using CConvertibleToT = TBool<CConvertibleTo<From, To>>;
}
template<class From, class To> concept CTListConvertible = CListAllPairs<z_D::CConvertibleToT, From, To>;
template<class To, class From> concept CTListAssignable = CListAllPairs<z_D::CAssignableT, To, From>;

INTRA_END
