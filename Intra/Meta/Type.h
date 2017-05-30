#pragma once

#include "Cpp/Fundamental.h"
#include "Cpp/PlatformDetect.h"
#include "Cpp/Warnings.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_COPY_MOVE_CONSTRUCT_IMPLICITLY_DELETED
INTRA_WARNING_DISABLE_LOSING_CONVERSION
INTRA_WARNING_DISABLE_SIGN_CONVERSION

#ifdef _MSC_VER

#pragma warning(disable: 4310) //Не ругаться на приведение констант с усечением значения
#if _MSC_VER>=1900
#pragma warning(disable: 4647) //__is_pod(...) имеет другое значение в предыдущих версиях
#endif

#endif

namespace Intra { namespace Meta {

template<typename T> T&& Val();
struct UniFunctor {template<typename T> void operator()(T);};

using Cpp::TypeFromValue;
using Cpp::FalseType;
using Cpp::TrueType;

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
	NonCopyableType(NonCopyableType&&) {}
	NonCopyableType& operator=(NonCopyableType&&) {return *this;}
	NonCopyableType(const NonCopyableType&) = delete;
	NonCopyableType& operator=(const NonCopyableType&) = delete;
};

template<typename T> struct WrapperStruct {T value;};

template<typename T> using AddLValueReference = typename AddReference<T>::LValue;
template<typename T> using AddRValueReference = typename AddReference<T>::RValue;

template<typename T> using AddConst = const T;

template<typename T1, typename T2> struct TypeEquals: FalseType {};
template<typename T> struct TypeEquals<T, T>: TrueType {};

template<typename T1, typename T2> struct TypeEqualsNotVoid: FalseType {};
template<> struct TypeEqualsNotVoid<void, void>: FalseType {};
template<typename T> struct TypeEqualsNotVoid<T, T>: TrueType {};

template<typename T> struct IsArrayType: FalseType {};
template<typename T, size_t N> struct IsArrayType<T[N]>: TrueType {};
template<typename T> struct IsArrayType<T[]>: TrueType {};

template<typename T> struct IsLValueReference: FalseType {};
template<typename T> struct IsLValueReference<T&>: TrueType {};
template<typename T> struct IsRValueReference: FalseType {};
template<typename T> struct IsRValueReference<T&&>: TrueType {};

template<typename T> struct IsReference: TypeFromValue<bool,
	IsLValueReference<T>::_ || IsRValueReference<T>::_
> {};

template<typename T> struct IsNCLValueReference: FalseType {};
template<typename T> struct IsNCLValueReference<T&>: TrueType {};
template<typename T> struct IsNCLValueReference<const T&>: FalseType {};
template<typename T> struct IsNCRValueReference: FalseType {};
template<typename T> struct IsNCRValueReference<T&&>: TrueType {};
template<typename T> struct IsNCRValueReference<const T&&>: TrueType {};

template<typename T> struct IsConst: FalseType {};
template<typename T> struct IsConst<const T>: TrueType {};
template<typename T> struct IsConst<const T&>: TrueType {};
template<typename T> struct IsConst<const T&&>: TrueType {};

namespace D {
template<typename IfTrue, typename IfFalse, bool COND> struct TSelectType {typedef IfTrue _;};
template<typename IfTrue, typename IfFalse> struct TSelectType<IfTrue, IfFalse, false> {typedef IfFalse _;};
}
template<typename IfTrue, typename IfFalse, bool COND>
using SelectType = typename D::TSelectType<IfTrue, IfFalse, COND>::_;


template<bool COND> using CopyableIf = SelectType<EmptyType, NonCopyableType, COND>;

namespace D {

template<typename T> struct TRemovePointer {typedef T _;};
template<typename T> struct TRemovePointer<T*> {typedef T _;};
template<typename T> struct TRemovePointer<T* const> {typedef T _;};

template<typename T> struct TRemoveConst {typedef T _;};
template<typename T> struct TRemoveConst<const T> {typedef T _;};
template<typename T> struct TRemoveConst<const T&> {typedef T& _;};
template<typename T> struct TRemoveConst<const T&&> {typedef T&& _;};

template<typename T> struct TRemoveConstOrDisable;
template<typename T> struct TRemoveConstOrDisable<const T> {typedef T _;};
template<typename T> struct TRemoveConstOrDisable<const T&> {typedef T& _;};
template<typename T> struct TRemoveConstOrDisable<const T&&> {typedef T&& _;};

template<typename T> struct TRemoveVolatile {typedef T _;};
template<typename T> struct TRemoveVolatile<volatile T> {typedef T _;};
template<typename T> struct TRemoveVolatile<volatile T[]> {typedef T _[];};
template<typename T, size_t N> struct TRemoveVolatile<volatile T[N]> {typedef T _[N];};

template<typename T> struct TRemoveExtent {typedef T _;};
template<typename T, size_t N> struct TRemoveExtent<T[N]> {typedef T _;};
template<typename T> struct TRemoveExtent<T[]> {typedef T _;};
}

template<typename T> using RemoveReference = typename Cpp::D::TRemoveReference<T>::_;
template<typename T> using RemovePointer = typename D::TRemovePointer<T>::_;
template<typename T> using RemoveConst = typename D::TRemoveConst<T>::_;
template<typename T> using RemoveConstOrDisable = typename D::TRemoveConstOrDisable<T>::_;
template<typename T> using RemoveVolatile = typename D::TRemoveVolatile<T>::_;
template<typename T> using RemoveConstRef = RemoveConst<RemoveReference<T>>;
template<typename T> using RemoveConstPointer = RemoveConst<RemovePointer<T>>;
template<typename T> using RemoveConstVolatile = RemoveConst<RemoveVolatile<T>>;

template<typename T> using RemoveExtent = typename D::TRemoveExtent<T>::_;
template<typename T> using AddPointer = RemoveReference<T>*;

template<typename T> struct IsFunctionPointer: FalseType {};
#if(defined(_MSC_VER) && INTRA_PLATFORM_ARCH==INTRA_PLATFORM_X86)
template<typename Ret, typename... Args> struct IsFunctionPointer<Ret(__cdecl*)(Args...)>: TrueType {};
template<typename Ret, typename... Args> struct IsFunctionPointer<Ret(__stdcall*)(Args...)>: TrueType {};
template<typename Ret, typename... Args> struct IsFunctionPointer<Ret(__fastcall*)(Args...)>: TrueType {};
#else
template<typename Ret, typename... Args> struct IsFunctionPointer<Ret(*)(Args...)>: TrueType {};
#endif

template<typename T> struct IsFunction: IsFunctionPointer<RemoveConstVolatile<T>*> {};
template<typename T> struct IsFunction<T&>: FalseType {};
template<typename T> struct IsFunction<T&&>: FalseType {};

template<class T, size_t N=0> struct Extent: TypeFromValue<size_t, 0> {};
template<class T> struct Extent<T[], 0>: TypeFromValue<size_t, 0> {};
template<class T, size_t N> struct Extent<T[], N>: Extent<T, N-1> {};
template<class T, size_t N> struct Extent<T[N], 0>: TypeFromValue<size_t, N> {};
template<class T, size_t I, size_t N> struct Extent<T[I], N>: Extent<T, N-1> {};

namespace D {
template<typename T> struct TDecay
{
	typedef RemoveReference<T> T1;
	typedef SelectType<
		RemoveExtent<T1>*,
		SelectType<
		    AddPointer<T1>,
		    RemoveConstVolatile<T1>,
		    IsFunction<T1>::_>,
		IsArrayType<T1>::_> _;
};
}
template<typename T> using Decay = typename D::TDecay<T>::_;




template<typename T> struct IsUnsignedIntegralType: TypeFromValue<bool,
    TypeEquals<T, byte>::_ || TypeEquals<T, ushort>::_ || TypeEquals<T, uint>::_ ||
	TypeEquals<T, ulong64>::_ || TypeEquals<T, unsigned long>::_
> {};

template<typename T> struct IsSignedIntegralType: TypeFromValue<bool,
    TypeEquals<T, sbyte>::_ || TypeEquals<T, short>::_ ||
	TypeEquals<T, int>::_ || TypeEquals<T, long>::_ || TypeEquals<T, long64>::_
> {};

template<typename T> struct IsIntegralType: TypeFromValue<bool,
	IsUnsignedIntegralType<T>::_ || IsSignedIntegralType<T>::_
> {};

template<typename T> struct IsFloatType: TypeFromValue<bool,
	TypeEquals<T, float>::_ || TypeEquals<T, double>::_ || TypeEquals<T, real>::_
> {};

template<typename T> struct IsSignedType: TypeFromValue<bool,
	IsSignedIntegralType<T>::_ || IsFloatType<T>::_
> {};

template<typename T> struct IsCharTypeExactly: TypeFromValue<bool,
	TypeEquals<T, char>::_ || TypeEquals<T, wchar>::_ || TypeEquals<T, dchar>::_ || TypeEquals<T, wchar_t>::_
> {};

template<typename T> struct IsCharType: IsCharTypeExactly<RemoveConstVolatile<T>> {};


template<typename T> struct IsPointerType: FalseType {};
template<typename T> struct IsPointerType<T*>: TrueType {};
template<typename T> struct IsPointerType<T* const>: TrueType {};
template<typename T> struct IsPointerType<T* volatile>: TrueType {};
template<typename T> struct IsPointerType<T* const volatile>: TrueType {};


template<typename T> struct IsMemberPointerType: FalseType {};
template<class T, class U> struct IsMemberPointerType<T U::*>: TrueType {};
template<class T, class U> struct IsMemberPointerType<T U::* const>: TrueType {};
template<class T, class U> struct IsMemberPointerType<T U::* volatile>: TrueType {};
template<class T, class U> struct IsMemberPointerType<T U::* const volatile>: TrueType {};

template<typename T> struct IsNonCRefArithmeticType: TypeFromValue<bool,
	TypeEquals<T, bool>::_ ||
	IsCharType<T>::_ ||
	IsIntegralType<T>::_ ||
	IsFloatType<T>::_
> {};

template<typename T> struct IsArithmeticType: IsNonCRefArithmeticType<RemoveConstRef<T>> {};





namespace D {

template<typename T> struct TRemoveAllExtents {typedef T _;};
template<typename T> struct TRemoveAllExtents<T[]>: TRemoveAllExtents<T> {};
template<typename T, size_t N> struct TRemoveAllExtents<T[N]>: TRemoveAllExtents<T> {};

}
template<typename T> using RemoveAllExtents = typename D::TRemoveAllExtents<T>::_;


#define INTRA_DEFINE_EXPRESSION_CHECKER_WITH_CONDITION(checker_name, expr, condition) \
	template<typename U> struct checker_name \
	{\
		template<typename T> static decltype((expr), ::Intra::Meta::TypeFromValue<bool, \
			(condition)>()) func(::Intra::Meta::RemoveReference<T>*);\
		template<typename T> static ::Intra::Meta::FalseType func(...);\
		using type = decltype(func<U>(nullptr));\
		enum {_ = type::_};\
	}


#define INTRA_DEFINE_EXPRESSION_CHECKER(checker_name, expr) \
	struct D ## checker_name ## _base {\
		template<typename T> static decltype((expr), short()) func(::Intra::Meta::RemoveReference<T>*);\
		template<typename T> static char func(...);\
	};\
	template<typename U> struct checker_name: D ## checker_name ## _base \
	{enum {_ = sizeof(func<U>(nullptr))==sizeof(short)};}

#define INTRA_DEFINE_EXPRESSION_CHECKER2_WITH_CONDITION(checker_name, expr, condition, default1, default2) \
	template<typename U1 default1, typename U2 default2> struct checker_name \
	{\
		template<typename T1, typename T2> static \
			decltype((expr), ::Intra::Meta::TypeFromValue<bool, (condition)>()) func(T1*, T2*);\
		template<typename T1, typename T2> static \
			::Intra::Meta::FalseType func(...); \
		using type = decltype(func<U1, U2>(null, null)); \
		enum {_=type::_}; \
	}

#define INTRA_DEFINE_EXPRESSION_CHECKER2(checker_name, expr, default1, default2) \
	namespace D { struct checker_name ## _base {\
		template<typename T1, typename T2> static decltype((expr), short()) func(\
			::Intra::Meta::RemoveReference<T1>*, ::Intra::Meta::RemoveReference<T2>*);\
		template<typename T1, typename T2> static char func(...); \
	};}\
	template<typename U1 default1, typename U2 default2> struct checker_name: D::checker_name ## _base \
	{enum {_ = sizeof(func<U1, U2>(nullptr, nullptr))==sizeof(short)};}

INTRA_DEFINE_EXPRESSION_CHECKER2(HasUnambigousTernary, true? Val<T1>(): Val<T2>(),,);
INTRA_DEFINE_EXPRESSION_CHECKER2(HasUnambigousSum, Val<T1>()+Val<T2>(),,);


template<typename T> struct IsPod: TypeFromValue<bool, __is_pod(T)> {};
template<typename T> struct IsAbstractClass: TypeFromValue<bool, __is_abstract(T)> {};
template<typename T> struct IsUnion: TypeFromValue<bool, __is_union(T)> {};
template<typename T> struct IsClass: TypeFromValue<bool, __is_class(T)> {};
template<typename T> struct IsEnumType: TypeFromValue<bool, __is_enum(T)> {};
template<typename T> struct IsEmptyClass: TypeFromValue<bool, __is_empty(T)> {};
template<typename T> struct IsTriviallyDestructible: TypeFromValue<bool, __has_trivial_destructor(T)> {};
template<class T, class From> struct IsInherited: TypeFromValue<bool, IsClass<T>::_ && __is_base_of(From, T)> {};

//! Определить, является ли тип скалярным: арифметическим, указателем, перечислением или null_t.
template<typename T> struct IsScalarType: TypeFromValue<bool,
	IsArithmeticType<T>::_ ||
	IsPointerType<T>::_ ||
	IsEnumType<T>::_ ||
	IsMemberPointerType<T>::_ ||
	TypeEquals<RemoveConstRef<T>, null_t>::_
> {};


#ifdef _MSC_VER

template<typename T> struct IsDestructible: TypeFromValue<bool, __is_destructible(T)> {};

#else

namespace D {

struct do_is_destructible_impl
{
	template<typename T, typename = decltype(Val<T&>().~T())>
	static TrueType test(int);

	template<typename> static FalseType test(...);
};

template<typename T> struct is_destructible_impl: do_is_destructible_impl
{
	typedef decltype(test<T>(0)) type;
};

template<typename T,
	bool = TypeEquals<T, void>::_ || (Extent<T>::_>0) || IsFunction<T>::_,
	bool = IsReference<T>::_ || IsScalarType<T>::_>
struct is_destructible_safe;

template<typename T> struct is_destructible_safe<T, false, false>: is_destructible_impl<RemoveAllExtents<T>>::type {};
template<typename T> struct is_destructible_safe<T, true, false>: FalseType { };

}

template<typename T> struct IsDestructible: D::is_destructible_safe<T> {};


#endif


namespace D {

struct do_is_static_castable_impl
{
	template<typename SRC, typename DST, typename = decltype(static_cast<DST>(Val<SRC>()))> static short test(int);
    template<typename, typename> static char test(...);
};

}

template<typename SRC, typename DST> struct IsStaticCastable: D::do_is_static_castable_impl
{enum: bool {_ = sizeof(test<SRC, DST>(0))==sizeof(short)};};

#ifdef _MSC_VER
template<typename T, typename... Args> struct IsConstructible:
	TypeFromValue<bool, __is_constructible(T, Args...)> {};
template<typename... Args> struct IsConstructible<void, Args...>: FalseType {};
template<typename T> struct IsDefaultConstructible: IsConstructible<T> {};
#else

namespace D {

struct do_is_default_constructible_impl
{
	template<typename _Tp, typename = decltype(_Tp())>
	static TrueType test(int);
	template<typename> static FalseType test(...);
};

template<typename T> struct is_default_constructible_impl: do_is_default_constructible_impl
{
	typedef decltype(test<T>(0)) type;
};

template<typename T> struct is_default_constructible_atom: TypeFromValue<bool,
	!TypeEquals<T, void>::_ && is_default_constructible_impl<T>::type::_
> {};

template<typename T, bool = IsArrayType<T>::_> struct is_default_constructible_safe;

template<typename T> struct is_default_constructible_safe<T, true>: TypeFromValue<bool,
	(Extent<T>::_>0) && is_default_constructible_atom<RemoveAllExtents<T>>::_
> {};

template<typename T> struct is_default_constructible_safe<T, false>: is_default_constructible_atom<T> {};

}

template<typename T> struct IsDefaultConstructible: D::is_default_constructible_safe<T> {};

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

template<typename T, typename Arg> struct is_direct_constructible_new_safe: Meta::TypeFromValue<bool,
	IsDestructible<T>::_ && is_direct_constructible_impl<T, Arg>::type::_
> {};

template<typename SRC, typename DST, bool = !TypeEquals<SRC, void>::_ && !IsFunction<SRC>::_> struct is_base_to_derived_ref {};

template<typename SRC, typename DST> struct is_base_to_derived_ref<SRC, DST, true>
{
	typedef RemoveConstVolatile<RemoveReference<SRC>> src_t;
	typedef RemoveConstVolatile<RemoveReference<DST>> dst_t;
	enum: bool {_ = !TypeEquals<src_t, dst_t>::_ && IsInherited<dst_t, src_t>::_};
};

template<typename SRC, typename DST> struct is_base_to_derived_ref<SRC, DST, false>: FalseType {};

template<typename SRC, typename DST, bool = IsLValueReference<SRC>::_ && IsRValueReference<DST>::_> struct is_lvalue_to_rvalue_ref;

template<typename SRC, typename DST> struct is_lvalue_to_rvalue_ref<SRC, DST, true>
{
	typedef RemoveConstVolatile<RemoveReference<SRC>> src_t;
	typedef RemoveConstVolatile<RemoveReference<DST>> dst_t;
	enum: bool {_ = (!IsFunction<src_t>::_ && TypeEquals<src_t, dst_t>::_) || IsInherited<src_t, dst_t>::_};
};

template<typename SRC, typename DST> struct is_lvalue_to_rvalue_ref<SRC, DST, false>: FalseType {};



template<typename T, typename Arg> struct is_direct_constructible_ref_cast: TypeFromValue<bool,
	IsStaticCastable<Arg, T>::_ && !is_base_to_derived_ref<Arg, T>::_ && !is_lvalue_to_rvalue_ref<Arg, T>::_
> {};

template<typename T, typename Arg> struct is_direct_constructible:
	Meta::SelectType<
		is_direct_constructible_ref_cast<T, Arg>,
		is_direct_constructible_new_safe<T, Arg>,
		IsReference<T>::_
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
	static_assert(sizeof...(Args)>1, "Only useful for > 1 arguments");
};

template<typename T, typename... Args> struct is_constructible_impl: is_nary_constructible<T, Args...> {};

template<typename T, typename Arg> struct is_constructible_impl<T, Arg>: is_direct_constructible<T, Arg> {};
template<typename T> struct is_constructible_impl<T>: IsDefaultConstructible<T> {};



}

template<typename T, typename... Args> struct IsConstructible: D::is_constructible_impl<T, Args...> {};



#endif




#if defined(_MSC_VER)
template<typename T, typename... Args> struct IsTriviallyConstructible:
	TypeFromValue<bool, __is_trivially_constructible(T, Args...)> {};
#else
template<typename T, typename... Args> struct IsTriviallyConstructible:
	TypeFromValue<bool, __has_trivial_copy(T)> {};
#endif

#if defined(_MSC_VER) && _MSC_VER>=1900
template<typename T> struct IsCopyConstructible: IsConstructible<T, const T&> {};
template<> struct IsCopyConstructible<void>: FalseType {};
#else
INTRA_DEFINE_EXPRESSION_CHECKER(IsCopyConstructible, T(Val<const T&>()));
#endif


template<typename R, typename... Args> struct IsCopyConstructible<R(Args...)>: FalseType {};
template<typename T> struct IsMoveConstructible: IsConstructible<T, AddRValueReference<T>> {};
template<typename R, typename... Args> struct IsMoveConstructible<R(Args...)>: FalseType {};

template<typename T> struct IsTriviallyCopyConstructible: IsTriviallyConstructible<T, AddLValueReference<AddConst<T>>> {};
template<typename T> struct IsTriviallyDefaultConstructible: IsTriviallyConstructible<T> {};
template<typename T> struct IsTriviallyMoveConstructible: IsTriviallyConstructible<T, AddRValueReference<T>> {};

template<typename T, typename... Args> struct IsCallable
{
	template<typename T1> struct dummy;

	template<typename CheckType> static short check(dummy<decltype(
		Val<RemoveReference<CheckType>>()(Val<Args>()...))>*);
	template<typename CheckType> static char check(...);

	enum: bool {_ = sizeof(check<T>(nullptr))==sizeof(short)};
};


template<typename T, typename... Args> struct TResultOf
{typedef decltype(Val<RemoveReference<T>>()(Val<Args>()...)) _;};
template<typename T, typename... Args> using ResultOf = typename TResultOf<T, Args...>::_;

namespace D {
template<bool, typename T, typename... Args> struct ResultOfOrVoid: TResultOf<T, Args...> {};
template<typename T, typename... Args> struct ResultOfOrVoid<false, T, Args...> {typedef void _;};
}
template<typename T, typename... Args> using ResultOfOrVoid = typename D::ResultOfOrVoid<IsCallable<T, Args...>::_, T, Args...>::_;


#if defined(__clang__) || defined(__GNUC__) || defined(_MSC_VER) && _MSC_VER<=1800
namespace D
{

	struct _Wrap_int {_Wrap_int(int) {}};

	template<class To, class From> struct _Is_assignable
	{
		template<class Dest, class Src> static auto _Fn(int) ->
			decltype((void)(Val<Dest>() = Val<Src>()), TrueType());
		template<class Dest, class Src> static auto _Fn(_Wrap_int) -> FalseType;
		typedef decltype(_Fn<To, From>(0)) _;
	};
}

template<typename To, typename From> struct IsAssignable:
	Meta::D::_Is_assignable<To, From>::_ {};
#else
template<typename To, typename From> struct IsAssignable:
	TypeFromValue<bool, __is_assignable(To, From)> {};
#endif

#ifdef _MSC_VER
template<typename To, typename From> struct IsTriviallyAssignable:
	TypeFromValue<bool, __is_trivially_assignable(To, From)> {};
template<typename T> struct IsTriviallyCopyAssignable:
	IsTriviallyAssignable<AddLValueReference<T>, AddLValueReference<AddConst<T>>> {};
#else
template<typename T> struct IsTriviallyCopyAssignable:
	TypeFromValue<bool, __has_trivial_assign(T)> {};
#endif

template<typename T> struct IsCopyAssignable:
	IsAssignable<AddLValueReference<T>, AddLValueReference<AddConst<T>>> {};

template<typename T> struct IsMoveAssignable:
	IsAssignable<AddLValueReference<T>, AddRValueReference<T>> {};


template<typename T> struct IsTriviallyCopyable: TypeFromValue<bool,
	IsTriviallyCopyConstructible<T>::_ || IsTriviallyCopyAssignable<T>::_> {};

//! Отличается от IsPod тем, что не требует тривиального конструктора по умолчанию.
template<typename T> struct IsAlmostPod: TypeFromValue<bool,
	IsTriviallyCopyable<T>::_ &&
	IsTriviallyDestructible<T>::_
> {};

//! Отличается от IsAlmostPod тем, что для некоторых классов и структур он будет переопределяться на false.
template<typename T> struct IsTriviallySerializable: IsAlmostPod<T> {};

//template<template<typename> F, typename T, typename... Args> struct ForeachType;
//template<template<typename> F, typename T, typename H, typename... Args> struct ForeachType;


template<typename T> struct IsTriviallyMovable: TypeFromValue<bool,
	IsTriviallyCopyable<T>::_ ||
	IsTriviallyMoveConstructible<T>::_ ||
	IsTriviallyCopyAssignable<T>::_> {};

template<typename T> struct IsTriviallyRelocatable: IsTriviallyMovable<T> {};



namespace D {
template<bool COND, class T=void> struct TEnableIf {};
template<class T> struct TEnableIf<true, T> {typedef T _;};
}
template<bool COND, typename T=void> using EnableIf = typename D::TEnableIf<COND, T>::_;

template<typename T1, typename T2> struct TypeEqualsIgnoreCV: TypeEquals<RemoveConstVolatile<T1>, RemoveConstVolatile<T2>> {};
template<typename T1, typename T2> struct TypeEqualsIgnoreCVRef: TypeEqualsIgnoreCV<RemoveReference<T1>, RemoveReference<T2>> {};
template<typename T1, typename T2> struct TypeEqualsIgnoreRef: TypeEquals<RemoveReference<T1>, RemoveReference<T2>> {};


#ifdef _MSC_VER
template<typename SRC, typename DST> struct IsConvertible:
	TypeFromValue<bool, __is_convertible_to(SRC, DST)> {};
#else

namespace D {

template<typename SRC, typename DST,
	bool = TypeEquals<SRC, void>::_ || IsFunction<DST>::_ || IsArrayType<DST>::_>
	struct IsConvertible: Meta::TypeEquals<DST, void> {};


template<typename SRC, typename DST> struct IsConvertible<SRC, DST, false>
{
	template<typename DST1> static void test_aux(DST1);

	template<typename SRC1, typename DST1>
	static decltype(test_aux<DST1>(Val<SRC1>()), char()) test(int);

	template<typename, typename> static short test(...);

public:
	enum: bool {_=sizeof(test<SRC, DST>(0))==1};
};

}
template<typename SRC, typename DST> struct IsConvertible: D::IsConvertible<SRC, DST> {};


#endif



namespace D {
template<typename... Types> struct CommonType;
template<typename T> struct CommonType<T> {typedef Meta::Decay<T> _;};

template<typename T, typename U> struct CommonType<T, U>
{
	typedef Meta::Decay<decltype(true? Val<T>(): Val<U>())> _;
};

template<typename T, typename U, typename... V> struct CommonType<T, U, V...>
{
	typedef typename CommonType<typename CommonType<T, U>::_, V...>::_ _;
};


template<typename... Types> struct CommonTypeRef;
template<typename T> struct CommonTypeRef<T> {typedef T _;};

template<typename T, typename U, bool=HasUnambigousTernary<T, U>::_, bool=HasUnambigousSum<T, U>::_> struct CommonTypeBaseRef
{typedef decltype(true? Val<T>(): Val<U>()) _;};

template<typename T, typename U> struct CommonTypeBaseRef<T, U, false, true>
{typedef decltype(Val<T>()+Val<U>()) _;};

template<typename T, typename U> struct CommonTypeBaseRef<T, U, false, false>
{};

template<typename T, typename U> struct CommonTypeRef<T, U>
{
private:
	typedef typename CommonTypeBaseRef<T, U>::_ common_type_base;
public:
	typedef SelectType<
		common_type_base,
		RemoveReference<common_type_base>,
		IsReference<T>::_ && IsReference<U>::_> _;
};

template<typename T, typename U, typename... V> struct CommonTypeRef<T, U, V...>
{
	typedef typename CommonTypeRef<typename CommonTypeRef<T, U>::_, V...>::_ _;
};

}

//! Общий тип без const\volatile и ссылок.
template<typename... Types> using CommonType = typename Meta::D::CommonType<Types...>::_;

//! Общий тип с сохранением ссылок и const.
template<typename... Types> using CommonTypeRef = typename Meta::D::CommonTypeRef<Types...>::_;





#define INTRA_DEFINE_HAS_METHOD_CHECKER(checker_name, method_name) \
	INTRA_DEFINE_EXPRESSION_CHECKER(checker_name, ::Intra::Meta::Val<T>().method_name)

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
	template<typename T> class checker_name<T, Meta::EnableIf<!IsClass<T>::_>>\
	{\
		public: enum: bool {_ = false}; \
	};



namespace D {
	template<typename T> struct LargerIntType {};
#define DECLARE_LARGER_INT_TYPE(T, LARGER) template<> struct LargerIntType<T> {typedef LARGER _;}
	DECLARE_LARGER_INT_TYPE(sbyte, short);
	DECLARE_LARGER_INT_TYPE(byte, ushort);
	DECLARE_LARGER_INT_TYPE(short, int);
	DECLARE_LARGER_INT_TYPE(ushort, uint);
	DECLARE_LARGER_INT_TYPE(int, long64);
	DECLARE_LARGER_INT_TYPE(uint, ulong64);
#undef DECLARE_LARGER_INT_TYPE
}
template<typename T> using LargerIntType = typename Meta::D::LargerIntType<T>::_;

namespace D
{
template<typename T> struct MakeUnsignedType {typedef T _;};
#define DECLARE_UNSIGNED_TYPE(T, UNSIGNED) template<> struct MakeUnsignedType<T> {typedef UNSIGNED _;}
DECLARE_UNSIGNED_TYPE(sbyte, byte);
DECLARE_UNSIGNED_TYPE(short, ushort);
DECLARE_UNSIGNED_TYPE(int, uint);
DECLARE_UNSIGNED_TYPE(long, unsigned long);
DECLARE_UNSIGNED_TYPE(long64, ulong64);
#undef DECLARE_UNSIGNED_TYPE
}
template<typename T> using MakeUnsignedType = typename Meta::D::MakeUnsignedType<T>::_;

namespace D
{
	template<typename T> struct GetMemberFieldType;
	template<typename T, typename F> struct GetMemberFieldType<F T::*> {typedef F _;};
}
template<typename T> using GetMemberFieldType = typename Meta::D::GetMemberFieldType<T>::_;


namespace D
{
	template<typename... Args> struct GetFirstType;
	template<typename T, typename... Args> struct GetFirstType<T, Args...> {typedef T _;};
}
template<typename... Args> using GetFirstType = typename D::GetFirstType<Args...>::_;



enum class NumericType: byte {Integral, FloatingPoint, FixedPoint, Normalized};


template<typename T, typename Enable = void> struct NumericLimits;

template<typename T> struct NumericLimits<T, EnableIf<IsSignedIntegralType<T>::_>>
{
	constexpr static T Min() {return T(1ull << (sizeof(T)*8-1));}
	constexpr static T Max() {return T((1ull << (sizeof(T)*8-1)) - 1);}
	constexpr static T Eps() {return 1;}
	enum: bool {Signed = true};
	constexpr static NumericType Type() {return NumericType::Integral;}
};

template<typename T> struct NumericLimits<T, EnableIf<IsUnsignedIntegralType<T>::_>>
{
	constexpr static T Min() {return 0;}
	constexpr static T Max() {return T(~0ull);}
	constexpr static T Eps() {return 1;}
	enum: bool {Signed = false};
	constexpr static NumericType Type() {return NumericType::Integral;}
};

template<> struct NumericLimits<float>
{
	constexpr static float Min() {return 1.175494351e-38f;}
	constexpr static float Max() {return 3.402823466e+38f;}
	constexpr static float Eps() {return 1.192092896e-07f;}
	enum: bool {Signed = true};
	constexpr static NumericType Type() {return NumericType::FloatingPoint;}
};

template<> struct NumericLimits<double>
{
	constexpr static double Min() {return 2.2250738585072014e-308;}
	constexpr static double Max() {return 1.7976931348623158e+308;}
	constexpr static double Eps() {return 2.2204460492503131e-016;}
	enum: bool {Signed = true};
	constexpr static NumericType Type() {return NumericType::FloatingPoint;}
};

#ifdef _MSC_VER
template<> struct NumericLimits<real>: NumericLimits<double> {};
#else
template<> struct NumericLimits<real>
{
	//constexpr static const double Min = 2.2250738585072014e-308, Max=1.7976931348623158e+308, Eps=2.2204460492503131e-016;
	enum: bool {Signed = true};
	constexpr static NumericType Type() {return NumericType::FloatingPoint;}
};
#endif



namespace D {
	template<size_t SIZE> struct IntegralTypeFromMinSize;
	template<> struct IntegralTypeFromMinSize<1> {typedef byte type;};
	template<> struct IntegralTypeFromMinSize<2> {typedef ushort type;};
	template<> struct IntegralTypeFromMinSize<3> {typedef uint type;};
	template<> struct IntegralTypeFromMinSize<4> {typedef uint type;};
	template<> struct IntegralTypeFromMinSize<5> {typedef ulong64 type;};
	template<> struct IntegralTypeFromMinSize<6> {typedef ulong64 type;};
	template<> struct IntegralTypeFromMinSize<7> {typedef ulong64 type;};
	template<> struct IntegralTypeFromMinSize<8> {typedef ulong64 type;};
}
template<size_t SIZE> using IntegralTypeFromMinSize = typename Meta::D::IntegralTypeFromMinSize<SIZE>::type;

template<class T, typename U> constexpr forceinline size_t MemberOffset(U T::* member) noexcept
{return reinterpret_cast<size_t>(&((static_cast<T*>(nullptr))->*member));}

template<class T> forceinline T* AddressOf(T& arg) noexcept
{return reinterpret_cast<T*>(&const_cast<char&>(reinterpret_cast<const volatile char&>(arg)));}

#define INTRA_CHECK_TABLE_SIZE(table, expectedSize) static_assert(\
	sizeof(table)/sizeof(table[0])==size_t(expectedSize), "Table is outdated!")




static_assert(IsArrayType<RemoveReference<const char(&)[9]>>::_, "error");

}}


INTRA_WARNING_POP
