#pragma once

#include "Core/Core.h"

/** This header file contains:
  1. Metafunctions for type manipulation
  2. Fundamental C++ concepts for checking type traits
  3. Tools for working with metafunction classes
*/

INTRA_CORE_BEGIN
INTRA_WARNING_DISABLE_COPY_MOVE_CONSTRUCT_IMPLICITLY_DELETED
INTRA_WARNING_DISABLE_LOSING_CONVERSION
INTRA_WARNING_DISABLE_SIGN_CONVERSION

#ifdef _MSC_VER
#if _MSC_VER >= 1900
#pragma warning(disable: 4647) //__is_pod(...) in older versions
#endif
#endif

template<typename T> T&& Val();
struct UniFunctor {template<typename... Args> void operator()(Args&&...);};

/** Dummy type used to expand template packs with side effects:
  Usage: TExpand{(void(f(args)), '\0')...}
  It is not necessary when fold expressions are available:
  (void(f(args)), ...)
*/
using TExpand = char[];

using Core::TypeFromValue;
using Core::FalseType;
using Core::TrueType;

template<typename T> struct AddReference
{
	typedef T& LValue;
	typedef T&& RValue;
};

template<> struct AddReference<void>
{
	typedef void LValue;
	typedef void RValue;
};

template<> struct AddReference<const void>
{
	typedef void LValue;
	typedef void RValue;
};

struct EmptyType
{
	template<typename... Args> EmptyType(Args&&...) {}
};

struct NonCopyableType
{
	NonCopyableType() = default;
	NonCopyableType(NonCopyableType&&) = default;
	NonCopyableType& operator=(NonCopyableType&&) = default;
	NonCopyableType(const NonCopyableType&) = delete;
	NonCopyableType& operator=(const NonCopyableType&) = delete;
};

template<typename T> struct WrapperStruct {T value;};

template<typename T> using AddLValueReference = typename AddReference<T>::LValue;
template<typename T> using AddRValueReference = typename AddReference<T>::RValue;

//TODO: concept
template<typename T1, typename T2> constexpr bool CSame = false;
template<typename T> constexpr bool CSame<T, T> = true;

template<typename T1, typename T2> constexpr bool CSameNotVoid = false;
template<> constexpr bool CSameNotVoid<void, void> = false;
template<typename T> constexpr bool CSameNotVoid<T, T> = true;

template<typename T> constexpr bool CArrayType = false;
template<typename T, size_t N> constexpr bool CArrayType<T[N]> = true;
template<typename T> constexpr bool CArrayType<T[]> = true;

template<typename T> constexpr bool CLValueReference = false;
template<typename T> constexpr bool CLValueReference<T&> = true;
template<typename T> constexpr bool CRValueReference = false;
template<typename T> constexpr bool CRValueReference<T&&> = true;

template<typename T> constexpr bool CReference = false;
template<typename T> constexpr bool CReference<T&> = true;
template<typename T> constexpr bool CReference<T&&> = true;

template<typename T> constexpr bool CNCLValueReference = false;
template<typename T> constexpr bool CNCLValueReference<T&> = true;
template<typename T> constexpr bool CNCLValueReference<const T&> = false;
template<typename T> constexpr bool CNCRValueReference = false;
template<typename T> constexpr bool CNCRValueReference<T&&> = true;
template<typename T> constexpr bool CNCRValueReference<const T&&> = false;

template<typename T> constexpr bool CConst = false;
template<typename T> constexpr bool CConst<const T> = true;

template<typename T> constexpr bool CVolatile = false;
template<typename T> constexpr bool CVolatile<volatile T> = true;

namespace D {
template<typename IfTrue, typename IfFalse, bool COND> struct TSelect_ {typedef IfTrue _;};
template<typename IfTrue, typename IfFalse> struct TSelect_<IfTrue, IfFalse, false> {typedef IfFalse _;};
}
template<typename IfTrue, typename IfFalse, bool COND>
using TSelect = typename D::TSelect_<IfTrue, IfFalse, COND>::_;


template<bool COND> using CopyableIf = TSelect<EmptyType, NonCopyableType, COND>;

namespace D {

template<typename T> struct TRemovePointer_ {typedef T _;};
template<typename T> struct TRemovePointer_<T*> {typedef T _;};
template<typename T> struct TRemovePointer_<T* const> {typedef T _;};
template<typename T> struct TRemovePointer_<T* volatile> {typedef T _;};
template<typename T> struct TRemovePointer_<T* const volatile> {typedef T _;};

template<typename T> struct TRemoveConst_ {typedef T _;};
template<typename T> struct TRemoveConst_<const T> {typedef T _;};

template<typename T> struct TRemoveConstOrDisable_;
template<typename T> struct TRemoveConstOrDisable_<const T> {typedef T _;};

template<typename T> struct TRemoveVolatile_ {typedef T _;};
template<typename T> struct TRemoveVolatile_<volatile T> {typedef T _;};
template<typename T> struct TRemoveVolatile_<volatile T[]> {typedef T _[];};
template<typename T, size_t N> struct TRemoveVolatile_<volatile T[N]> {typedef T _[N];};

template<typename T> struct TRemoveExtent_ {typedef T _;};
template<typename T, size_t N> struct TRemoveExtent_<T[N]> {typedef T _;};
template<typename T> struct TRemoveExtent_<T[]> {typedef T _;};
}

template<typename T> using TRemovePointer = typename D::TRemovePointer_<T>::_;
template<typename T> using TRemoveConst = typename D::TRemoveConst_<T>::_;
template<typename T> using TRemoveConstOrDisable = typename D::TRemoveConstOrDisable_<T>::_;
template<typename T> using TRemoveVolatile = typename D::TRemoveVolatile_<T>::_;
template<typename T> using TRemoveConstRef = TRemoveConst<TRemoveReference<T>>;
template<typename T> using TRemoveConstPointer = TRemoveConst<TRemovePointer<T>>;
template<typename T> using TUnqual = TRemoveConst<TRemoveVolatile<T>>;

template<typename T> using TRemoveExtent = typename D::TRemoveExtent_<T>::_;
template<typename T> using TAddPointer = TRemoveReference<T>*;

template<typename From, typename To> using TPropagateConst = TSelect<const To, To, CConst<From>>;
template<typename From, typename To> using TPropagateVolatile = TSelect<volatile To, To, CVolatile<From>>;
template<typename From, typename To> using TPropagateQualifiers = TPropagateConst<From, TPropagateVolatile<From, To>>;
template<typename From, typename To> using TPropagateLVRef = TSelect<To&, To, CLValueReference<From>>;
template<typename From, typename To> using TPropagateRVRef = TSelect<To&&, To, CRValueReference<From>>;
template<typename From, typename To> using TPropagateRef = TPropagateLVRef<From, TPropagateRVRef<From, To>>;
template<typename From, typename To> using TPropagateQualLVRef = TPropagateLVRef<From, TPropagateQualifiers<From, To>>;

template<typename T> constexpr bool CFunctionPointer = false;
#ifdef _MSC_VER
#if INTRA_PLATFORM_ARCH == INTRA_PLATFORM_X86
template<typename Ret, typename... Args> constexpr bool CFunctionPointer<Ret(__cdecl*)(Args...)> = true;
template<typename Ret, typename... Args> constexpr bool CFunctionPointer<Ret(__stdcall*)(Args...)> = true;
template<typename Ret, typename... Args> constexpr bool CFunctionPointer<Ret(__fastcall*)(Args...)> = true;
#elif INTRA_PLATFORM_ARCH == INTRA_PLATFORM_X86_64
template<typename Ret, typename... Args> constexpr bool CFunctionPointer<Ret(*)(Args...)> = true;
template<typename Ret, typename... Args> constexpr bool CFunctionPointer<Ret(__vectorcall*)(Args...)> = true;
#endif
#else
template<typename Ret, typename... Args> constexpr bool CFunctionPointer<Ret(*)(Args...)> = true;
#endif
template<typename T> concept CPod = __is_pod(T);
template<typename T> concept CAbstractClass = __is_abstract(T);
template<typename T> concept CUnion = __is_union(T);
template<typename T> concept CClass = __is_class(T);
template<typename T> concept CEnum = __is_enum(T);
template<typename T> concept CEmptyClass = __is_empty(T);
template<typename T> concept CFinalClass = __is_final(T);
template<typename T> concept CTriviallyDestructible = __has_trivial_destructor(T);
template<typename T, typename From> concept CDerived = __is_base_of(From, T);
template<typename T> concept CHasVirtualDestructor = __has_virtual_destructor(T);
template<typename T> concept CPolymorphic = __is_polymorphic(T);

namespace D {
template<typename T> char IsFunction__test(T*);
template<typename T> char IsFunction__test(EmptyType);
template<typename T> short IsFunction__test(...);
template<typename T> T& IsFunction__source(int);
template<typename T> EmptyType IsFunction__source(...);
}


namespace D {
template <class T, bool = CClass<T> ||
                          CUnion<T> ||
                          CSame<T, void> ||
                          CReference<T> ||
	                      CSame<TUnqual<T>, null_t>>
constexpr bool CFunction = sizeof(D::IsFunction__test<T>(D::IsFunction__source<T>(0))) == 1;
template <class T> constexpr bool CFunction<T, true> = false;
}
template <class T> concept CFunction = D::CFunction<T>;

template<class T, size_t N=0> constexpr size_t ArrayExtent = 0;
template<class T> constexpr size_t ArrayExtent<T[], 0> = 0;
template<class T, size_t N> constexpr size_t ArrayExtent<T[], N> = ArrayExtent<T, N-1>;
template<class T, size_t N> constexpr size_t ArrayExtent<T[N], 0> = N;
template<class T, size_t I, size_t N> constexpr size_t ArrayExtent<T[I], N> = ArrayExtent<T, N-1>;

namespace D {
template<typename T> struct TDecay_
{
	typedef TRemoveReference<T> T1;
	typedef TSelect<
		TRemoveExtent<T1>*,
		TSelect<
		    TAddPointer<T1>,
		    TUnqual<T1>,
		    CFunction<T1>>,
		CArrayType<T1>> _;
};
}
template<typename T> using TDecay = typename D::TDecay_<T>::_;


template<typename T> constexpr bool CPlainUnsignedIntegral = false;
#if defined(__CHAR_UNSIGNED__) || defined(_CHAR_UNSIGNED)
template<> constexpr bool CPlainUnsignedIntegral<char> = true;
#endif
#if defined(__WCHAR_UNSIGNED__) || defined(_MSC_VER)
template<> constexpr bool CPlainUnsignedIntegral<wchar_t> = true;
#endif
template<> constexpr bool CPlainUnsignedIntegral<bool> = true;
template<> constexpr bool CPlainUnsignedIntegral<byte> = true;
template<> constexpr bool CPlainUnsignedIntegral<ushort> = true;
template<> constexpr bool CPlainUnsignedIntegral<unsigned> = true;
template<> constexpr bool CPlainUnsignedIntegral<unsigned long> = true;
template<> constexpr bool CPlainUnsignedIntegral<uint64> = true;
#ifdef __cpp_unicode_characters
template<> constexpr bool CPlainUnsignedIntegral<char16_t> = true;
template<> constexpr bool CPlainUnsignedIntegral<char32_t> = true;
#endif

template<typename T> constexpr bool CPlainSignedIntegral = false;
#if !defined(__CHAR_UNSIGNED__) && !defined(_CHAR_UNSIGNED)
template<> constexpr bool CPlainSignedIntegral<char> = true;
#endif
#if !defined(__WCHAR_UNSIGNED__) && !defined(_MSC_VER)
template<> constexpr bool CPlainSignedIntegral<wchar_t> = true;
#endif
template<> constexpr bool CPlainSignedIntegral<sbyte> = true;
template<> constexpr bool CPlainSignedIntegral<short> = true;
template<> constexpr bool CPlainSignedIntegral<int> = true;
template<> constexpr bool CPlainSignedIntegral<long> = true;
template<> constexpr bool CPlainSignedIntegral<int64> = true;

template<typename T> constexpr bool CPlainFloatingPoint = false;
template<> constexpr bool CPlainFloatingPoint<float> = true;
template<> constexpr bool CPlainFloatingPoint<double> = true;
template<> constexpr bool CPlainFloatingPoint<long double> = true;
//TODO: compiler extensions: _fp16, _Float16, _float128

template<typename T> constexpr bool CPlainChar = false;
template<> constexpr bool CPlainChar<char> = true;
template<> constexpr bool CPlainChar<char16_t> = true;
template<> constexpr bool CPlainChar<char32_t> = true;
template<> constexpr bool CPlainChar<wchar_t> = true;
// Signed char and unsigned char are separate types from char and Intra doesn't use them to store characters. So it doesn't treat them as chars

template<typename T> concept CPlainSigned = CPlainSignedIntegral<T> || CPlainFloatingPoint<T>;
template<typename T> concept CPlainIntegral = CPlainUnsignedIntegral<T> || CPlainSignedIntegral<T>;
template<typename T> concept CPlainArithmetic = CPlainIntegral<T> || CPlainFloatingPoint<T>;

template<typename T> constexpr bool CPlainPointer = false;
template<typename T> constexpr bool CPlainPointer<T*> = true;

template<typename T> constexpr bool CPlainMemberPointer = false;
template<typename T, class U> constexpr bool CPlainMemberPointer<T U::*> = true;

template<typename T> concept CSigned = CPlainSigned<TUnqual<T>>;
template<typename T> concept CIntegral = CPlainIntegral<TUnqual<T>>;
template<typename T> concept CSignedIntegral = CPlainSignedIntegral<TUnqual<T>>;
template<typename T> concept CUnsignedIntegral = CPlainUnsignedIntegral<TUnqual<T>>;
template<typename T> concept CFloatingPoint = CPlainFloatingPoint<TUnqual<T>>;
template<typename T> concept CChar = CPlainChar<TUnqual<T>>;
template<typename T> concept CArithmetic = CPlainArithmetic<TUnqual<T>>;


template<typename T> concept CPointer = CPlainPointer<TUnqual<T>>;
template<typename T> concept CMemberPointer = CPlainMemberPointer<TUnqual<T>>;


namespace D {
template<typename T> struct TRemoveAllExtents_ {typedef T _;};
template<typename T> struct TRemoveAllExtents_<T[]>: TRemoveAllExtents_<T> {};
template<typename T, size_t N> struct TRemoveAllExtents_<T[N]>: TRemoveAllExtents_<T> {};
}
template<typename T> using TRemoveAllExtents = typename D::TRemoveAllExtents_<T>::_;

#define INTRA_DEFINE_CONCEPT_REQUIRES(checker_name, expr) \
	struct D_impl_ ## checker_name {\
		template<typename T> static decltype((expr), short()) func(::Intra::TRemoveReference<T>*);\
		template<typename T> static char func(...);\
	};\
	template<typename U> concept checker_name = sizeof(D_impl_ ## checker_name::func<U>(null)) == sizeof(short);

#define INTRA_DEFINE_CONCEPT_REQUIRES2(checker_name, expr, default1, default2) \
	struct D_impl_ ## checker_name {\
		template<typename T1, typename T2> static decltype((expr), short()) func(\
			::Intra::TRemoveReference<T1>*, ::Intra::TRemoveReference<T2>*);\
		template<typename T1, typename T2> static char func(...); \
	};\
	template<typename U1 default1, typename U2 default2> concept checker_name = \
		sizeof(D_impl_ ## checker_name::func<U1, U2>(null, null)) == sizeof(short);

INTRA_DEFINE_CONCEPT_REQUIRES2(CHasUnambigousTernary, true? Val<T1>(): Val<T2>(),,);
INTRA_DEFINE_CONCEPT_REQUIRES2(CHasUnambigousSum, Val<T1>()+Val<T2>(),,);
INTRA_DEFINE_CONCEPT_REQUIRES2(CEqualityComparable, Val<bool&>() = (Val<T1>() == Val<T2>()),, = U1);

template<typename T> concept CScalar =
	CArithmetic<T> ||
	CPointer<T> ||
	CEnum<T> ||
	CMemberPointer<T> ||
	CSame<TUnqual<T>, null_t>;

// This template can be specialized for user-defined types. To work properly they must not contain any
// unitialized private or padding bytes because their values are not guaranteed to be zeros.
template<typename T> constexpr bool IsTriviallyComparable = CScalar<T>;
template<typename T> concept CTriviallyComparable = IsTriviallyComparable<T>;


#ifdef _MSC_VER
template<typename T> concept CDestructible = __is_destructible(T);
#else
namespace D {
struct TIsDestructibleImpl
{
	template<typename T, typename = decltype(declval<T&>().~T())> static short test(int);
	template<typename> static char test(...);
};
template<typename T,
	bool = CSame<T, void> || (Extent<T> > 0) || IsFunction<T>,
	bool = IsReference<T> || CScalar<T>
> struct TIsDestructible: TrueType {};
template<typename T> struct TIsDestructible<T, false, false>: TBool<sizeof(TIsDestructibleImpl::test<RemoveAllExtents<T>>(0)) == sizeof(short)> {};
template<typename T> struct TIsDestructible<T, true, false>: FalseType {};
}
template<typename T> concept CDestructible = D::TIsDestructible<T>::_;
#endif


INTRA_DEFINE_CONCEPT_REQUIRES2(CStaticCastable, static_cast<T2>(Val<T1>()),,);

#ifdef _MSC_VER
template<typename T, typename... Args> concept CConstructible = __is_constructible(T, Args...);
template<typename T> concept CDefaultConstructible = CConstructible<T>;
#else

namespace D {

struct do_is_default_constructible_impl
{
	template<typename _Tp, typename = decltype(_Tp())>
	static short test(int);
	template<typename> static char test(...);
};

INTRA_DEFINE_CONCEPT_REQUIRES(is_default_constructible_impl, T());

template<typename T> concept is_default_constructible_atom = !CSame<T, void> && is_default_constructible_impl<T>;

template<typename T, bool = IsArrayType<T>> constexpr bool is_default_constructible_safe =
	(Extent<T> > 0) && is_default_constructible_atom<RemoveAllExtents<T>>;

template<typename T> constexpr bool is_default_constructible_safe<T, false> = is_default_constructible_atom<T>;

}

template<typename T> concept CDefaultConstructible = D::is_default_constructible_safe<T>;

namespace D {

struct do_is_direct_constructible_impl
{
	template<typename T, typename Arg, typename = decltype(::new T(Val<Arg>()))> static TrueType test(int);
	template<typename, typename> static FalseType test(...);
};

template<typename T, typename Arg> struct is_direct_constructible_impl: do_is_direct_constructible_impl
{
	typedef decltype(test<T, Arg>(0)) type;
};

template<typename T, typename Arg> struct is_direct_constructible_new_safe: TypeFromValue<bool,
	IsDestructible<T> && is_direct_constructible_impl<T, Arg>::type::_
> {};

template<typename SRC, typename DST, bool = !CSame<SRC, void> && !IsFunction<SRC>> struct is_base_to_derived_ref {};

template<typename SRC, typename DST> struct is_base_to_derived_ref<SRC, DST, true>
{
	typedef TUnqual<TRemoveReference<SRC>> src_t;
	typedef TUnqual<TRemoveReference<DST>> dst_t;
	enum: bool {_ = !CSame<src_t, dst_t> && IsInherited<dst_t, src_t>};
};

template<typename SRC, typename DST> struct is_base_to_derived_ref<SRC, DST, false>: FalseType {};

template<typename SRC, typename DST, bool = IsLValueReference<SRC> && IsRValueReference<DST>> struct is_lvalue_to_rvalue_ref;

template<typename SRC, typename DST> struct is_lvalue_to_rvalue_ref<SRC, DST, true>
{
	typedef TUnqual<TRemoveReference<SRC>> src_t;
	typedef TUnqual<TRemoveReference<DST>> dst_t;
	enum: bool {_ = (!IsFunction<src_t> && CSame<src_t, dst_t>) || IsInherited<src_t, dst_t>};
};

template<typename SRC, typename DST> struct is_lvalue_to_rvalue_ref<SRC, DST, false>: FalseType {};



template<typename T, typename Arg> struct is_direct_constructible_ref_cast: TypeFromValue<bool,
	IsStaticCastable<Arg, T> && !is_base_to_derived_ref<Arg, T>::_ && !is_lvalue_to_rvalue_ref<Arg, T>::_
> {};

template<typename T, typename Arg> struct is_direct_constructible:
	TSelect<
		is_direct_constructible_ref_cast<T, Arg>,
		is_direct_constructible_new_safe<T, Arg>,
		IsReference<T>
> {};


struct do_is_nary_constructible_impl
{
	template<typename T, typename... Args, typename = decltype(T(Val<Args>()...))>
	static TrueType test(int);

	template<typename, typename...> static FalseType test(...);
};

template<typename T, typename... Args> struct is_nary_constructible_impl: do_is_nary_constructible_impl
{
	typedef decltype(test<T, Args...>(0)) type;
};

template<typename T, typename... Args> struct is_nary_constructible: is_nary_constructible_impl<T, Args...>::type
{
	static_assert(sizeof...(Args) > 1, "Only useful for > 1 arguments");
};

template<typename T, typename... Args> struct is_constructible_impl: is_nary_constructible<T, Args...> {};

template<typename T, typename Arg> struct is_constructible_impl<T, Arg>: is_direct_constructible<T, Arg> {};
template<typename T> struct is_constructible_impl<T>: TBool<CDefaultConstructible<T>> {};

}

template<typename T, typename... Args> concept CConstructible = D::is_constructible_impl<T, Args...>::_;

#endif




#if defined(_MSC_VER)
template<typename T, typename... Args> concept CTriviallyConstructible = __is_trivially_constructible(T, Args...);
#else
template<typename T, typename... Args> concept CTriviallyConstructible = __has_trivial_copy(T);
#endif

#if defined(_MSC_VER) && _MSC_VER >= 1900
template<typename T> constexpr bool CCopyConstructible = __is_constructible(T, const T&);
template<> constexpr bool CCopyConstructible<void> = false;
#else
INTRA_DEFINE_CONCEPT_REQUIRES(CCopyConstructible, T(Val<const T&>()));
#endif


template<typename R, typename... Args> constexpr bool CCopyConstructible<R(Args...)> = false;
template<typename T> constexpr bool CMoveConstructible = CConstructible<T, AddRValueReference<T>>;
template<typename R, typename... Args> constexpr bool CMoveConstructible<R(Args...)> = false;

template<typename T> concept CTriviallyCopyConstructible = CTriviallyConstructible<T, AddLValueReference<const T>>;
template<typename T> concept CTriviallyDefaultConstructible = CTriviallyConstructible<T>;
template<typename T> concept CTriviallyMoveConstructible = CTriviallyConstructible<T, AddRValueReference<T>>;

namespace D {
struct is_callable_base {
	template<typename T, typename... Args> static decltype((Val<TRemoveReference<T>>()(Val<Args>()...)), short()) func(::Intra::TRemoveReference<T>*);
	template<typename T, typename... Args> static char func(...);
};
}
template<typename T, typename... Args> concept CCallable = sizeof(D::is_callable_base::func<T, Args...>(null)) == sizeof(short);


template<typename T, typename... Args> using TResultOf = decltype(Val<TRemoveReference<T>>()(Val<Args>()...));

namespace D {
template<bool, typename T, typename... Args> struct TResultOfOrVoid {typedef TResultOf<T, Args...> _;};
template<typename T, typename... Args> struct TResultOfOrVoid<false, T, Args...> {typedef void _;};
}
template<typename T, typename... Args> using TResultOfOrVoid = typename D::TResultOfOrVoid<CCallable<T, Args...>, T, Args...>::_;


#if defined(__clang__) || defined(__GNUC__) || defined(_MSC_VER) && _MSC_VER < 1900
INTRA_DEFINE_CONCEPT_REQUIRES(CAssignable, Val<T1>() = Val<T2>());
#else
template<typename To, typename From> concept CAssignable = __is_assignable(To, From);
#endif
template<typename To, typename From> struct CAssignableT: TBool<__is_assignable(To, From)> {};

#ifdef _MSC_VER
template<typename To, typename From> concept CTriviallyAssignable = __is_trivially_assignable(To, From);
template<typename T> concept CTriviallyCopyAssignable = CTriviallyAssignable<AddLValueReference<T>, AddLValueReference<const T>>;
#else
template<typename T> concept CTriviallyCopyAssignable = __has_trivial_assign(T);
#endif

template<typename T> concept CCopyAssignable = CAssignable<AddLValueReference<T>, AddLValueReference<const T>>;
template<typename T> concept CMoveAssignable= CAssignable<AddLValueReference<T>, AddRValueReference<T>>;
template<typename T> concept CTriviallyCopyable = CTriviallyCopyConstructible<T> || CTriviallyCopyAssignable<T>;

//template<template<typename> F, typename T, typename... Args> struct ForeachType;
//template<template<typename> F, typename T, typename H, typename... Args> struct ForeachType;


template<typename T> concept CTriviallyMovable =
	CTriviallyCopyable<T> ||
	CTriviallyMoveConstructible<T> ||
	CTriviallyCopyAssignable<T>;

/*!
  Trivially relocatable is a less constrained concept than trivially movable.
  All trivially movable types are also trivially relocatable.
  But there may be types having move constructor and destructor that are not trivial separately but combination of them may be trivial.
  It is true for most containers. You can make a bitwise copy of a container object without calling the move constructor and the destructor of source.
  Specialize IsTriviallyRelocatable for such types after their definition.
*/
template<typename T> constexpr bool IsTriviallyRelocatable = IsTriviallyMovable<T>;
template<typename T> concept CTriviallyRelocatable = IsTriviallyRelocatable<T>;

/** CAlmostPod is useful to check if a type can be trivially binary serialized and deserialized. It includes:
  1) CPod
  2) Any trivial type with default initialized fields
  3) Lambdas without not-trivial captures

  However like CPod it cannot check if the type contains pointers.
  If it contains pointers it may be not suitable for serialization.
*/
template<typename T> concept CAlmostPod = CPod<T> || CTriviallyDestructible<T> && CTriviallyCopyable<T>;



namespace D {
template<bool COND, class T=void> struct TRequires {};
template<class T> struct TRequires<true, T> {typedef T _;};
}
template<bool COND, typename T = void> using Requires = typename D::TRequires<COND, T>::_;

template<typename T1, typename T2> concept CSameUnqual = CSame<TUnqual<T1>, TUnqual<T2>>;
template<typename T1, typename T2> concept CSameIgnoreCVRef = CSameUnqual<TRemoveReference<T1>, TRemoveReference<T2>> {};
template<typename T1, typename T2> concept CSameIgnoreRef = CSame<TRemoveReference<T1>, TRemoveReference<T2>> {};


#ifdef _MSC_VER
template<typename SRC, typename DST> concept CConvertible = __is_convertible_to(SRC, DST);
template<typename SRC, typename DST> struct CConvertibleT: TBool<__is_convertible_to(SRC, DST)> {};
#else

template<typename SRC, typename DST,
	bool = CSame<SRC, void> || CFunction<DST> || CArrayType<DST>
> struct CConvertibleT: TBool<CSame<DST, void>> {};


template<typename SRC, typename DST> struct CConvertibleT<SRC, DST, false>
{
	template<typename DST1> static void test_aux(DST1);

	template<typename SRC1, typename DST1>
	static decltype(test_aux<DST1>(Val<SRC1>()), char()) test(int);

	template<typename, typename> static short test(...);

public:
	enum: bool {_=sizeof(test<SRC, DST>(0))==1};
};
template<typename SRC, typename DST> concept CConvertible = CConvertibleT<SRC, DST>::_;

#endif



namespace D {
template<typename... Types> struct TCommon_;
template<typename T> struct TCommon_<T> {typedef TDecay<T> _;};

template<typename T, typename U> struct TCommon_<T, U>
{
	typedef TDecay<decltype(true? Val<T>(): Val<U>())> _;
};

template<typename T, typename U, typename... V> struct TCommon_<T, U, V...>
{
	typedef typename TCommon_<typename TCommon_<T, U>::_, V...>::_ _;
};


template<typename... Types> struct TCommonRef_;
template<typename T> struct TCommonRef_<T> {typedef T _;};

template<typename T, typename U,
	bool = CHasUnambigousTernary<T, U>,
	bool = CHasUnambigousSum<T, U>> struct TCommonBaseRef_
{typedef decltype(true? Val<T>(): Val<U>()) _;};

template<typename T, typename U> struct TCommonBaseRef_<T, U, false, true>
{typedef decltype(Val<T>()+Val<U>()) _;};

template<typename T, typename U> struct TCommonBaseRef_<T, U, false, false>
{};

template<typename T, typename U> struct TCommonRef_<T, U>
{
private:
	typedef typename TCommonBaseRef_<T, U>::_ common_type_base;
public:
	typedef TSelect<
		common_type_base,
		TRemoveReference<common_type_base>,
		CReference<T> && CReference<U>> _;
};

template<typename T, typename U, typename... V> struct TCommonRef_<T, U, V...>
{
	typedef typename TCommonRef_<typename TCommonRef_<T, U>::_, V...>::_ _;
};

}

//! Common type without const\volatile and references.
template<typename... Types> using TCommon = typename Core::D::TCommon_<Types...>::_;

//! Common type preserving references and const.
template<typename... Types> using TCommonRef = typename Core::D::TCommonRef_<Types...>::_;

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


#define INTRA_DEFINE_HAS_MEMBER_TYPE(checker_name, Type) \
	template<typename T, typename U=void> struct checker_name\
	{\
    private:\
		struct Fallback {struct Type {};}; \
		struct Derived: T, Fallback {}; \
		template<typename C> static char& test(typename C::Type*); \
		template<typename C> static short& test(C*);\
	public: enum: bool {_ = sizeof(test<Derived>(null)) == sizeof(short)}; \
	};\
	template<typename T> class checker_name<T, ::Intra::Core::Requires<!CClass<T>>>\
	{\
		public: enum: bool {_ = false}; \
	};

namespace D
{
	template<typename T> struct TMemberFieldType_;
	template<typename T, typename F> struct TMemberFieldType_<F T::*> {typedef F _;};
}
template<typename T> using TMemberFieldType = typename D::TMemberFieldType_<T>::_;


template<typename T> concept CObject =
	CScalar<T> ||
	CArrayType<T> ||
	CUnion<T> ||
	CClass<T>;

template<typename T> concept CMovable =
	CObject<T> &&
	CMoveConstructible<T> &&
	CAssignable<T&, T>;
	//&& CSwappable<T>;

template<typename T> concept CCopyable =
	CCopyConstructible<T> &&
	CMovable<T> &&
	CCopyAssignable<T>;

template<typename T> concept CSemiregular =
	CCopyable<T> &&
	CDefaultConstructible<T>;

template<typename T> concept CRegular =
	CSemiregular<T>;
	//&& CEqualityComparable<T>;

INTRA_DEFINE_CONCEPT_REQUIRES(CHasPolymorphicCloneMethod, Val<T>().PolymorphicClone(&Val<T>(), index_t()));

namespace D {
template<typename F> struct TFinally
{
	forceinline ~TFinally() {OnDestruct();}
	F OnDestruct;
};

struct TFinallyMaker
{
	template<typename F> constexpr forceinline Requires<
		CCallable<F>,
	TFinally<F>> operator=(F&& f) const noexcept {return {Forward<F>(f)};}
};
}
/** Execute a statement block after leaving the current scope for any reason: normal or exception.
  usage: INTRA_FINALLY{<code block>};
*/
#define INTRA_FINALLY auto INTRA_CONCATENATE_TOKENS(finally__, INTRA_UNIQUE_NUMBER) = ::Intra::Utils::D::TFinallyMaker() = [&]()->void

class AnyPtr
{
	void* mPtr = null;
public:
	forceinline AnyPtr() = default;
	forceinline AnyPtr(null_t=null) {}

	template<typename T> forceinline AnyPtr(T* p)
	{
		typedef void* pvoid;
		mPtr = pvoid(p);
	}

	template<typename T> forceinline operator T*() const
	{
		typedef T* Tptr;
		return Tptr(mPtr);
	}

	forceinline bool operator==(null_t) const {return mPtr == null;}
	forceinline bool operator!=(null_t) const {return mPtr != null;}
};

static_assert(CAlmostPod<AnyPtr>, "TEST FAILED!");

template<class T, typename U> forceinline size_t MemberOffset(U T::* member) noexcept
{return reinterpret_cast<size_t>(&((static_cast<T*>(null))->*member));}

template<class T> forceinline T* AddressOf(T& arg) noexcept
{return reinterpret_cast<T*>(&const_cast<char&>(reinterpret_cast<const volatile char&>(arg)));}

#if INTRA_CONSTEXPR_TEST
static_assert(CArrayType<TRemoveReference<const char(&)[9]>>, "TEST FAILED!");
// TODO: add more tests
#endif
INTRA_CORE_END
