#pragma once

#include "Core/FundamentalTypes.h"
#include "Platform/PlatformInfo.h"
#include "Platform/CppWarnings.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

#ifdef _MSC_VER

#pragma warning(disable: 4310) //Не ругаться на приведение констант с усечением значения
#pragma warning(disable: 4510 4512 4610 4623 4626) //Не ругаться на то, что конструктор копирования, по умолчанию или оператор присваивания неявно удалён
#if _MSC_VER>=1900
#pragma warning(disable: 4647) //__is_pod(...) имеет другое значение в предыдущих версиях
#pragma warning(disable: 5027) //Не ругаться на то, что оператор перемещения неявно удалён
#endif

#endif

namespace Intra { namespace Meta {

template<typename T> T&& Val();
struct UniFunctor {template<typename T> void operator()(T);};

template<typename T, T value> struct TypeFromValue {enum {_=value};};

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

template<typename T> struct WrapperStruct {T value;};

template<typename T> using AddLValueReference = typename AddReference<T>::LValue;
template<typename T> using AddRValueReference = typename AddReference<T>::RValue;

template<typename T> using AddConst = const T;

template<typename T1, typename T2> struct TypeEquals: TypeFromValue<bool, false> {};
template<typename T> struct TypeEquals<T, T>: TypeFromValue<bool, true> {};

template<typename T> struct IsArrayType: TypeFromValue<bool, false> {};
template<typename T, size_t N> struct IsArrayType<T[N]>: TypeFromValue<bool, true> {};
template<typename T> struct IsArrayType<T[]>: TypeFromValue<bool, true> {};

template<typename T> struct IsLValueReference: TypeFromValue<bool, false> {};
template<typename T> struct IsLValueReference<T&>: TypeFromValue<bool, true> {};
template<typename T> struct IsRValueReference: TypeFromValue<bool, false> {};
template<typename T> struct IsRValueReference<T&&>: TypeFromValue<bool, true> {};
template<typename T> struct IsReference: TypeFromValue<bool, IsLValueReference<T>::_ || IsRValueReference<T>::_> {};

template<typename T> struct IsConst: TypeFromValue<bool, false> {};
template<typename T> struct IsConst<const T>: TypeFromValue<bool, true> {};
template<typename T> struct IsConst<const T&>: TypeFromValue<bool, true> {};
template<typename T> struct IsConst<const T&&>: TypeFromValue<bool, true> {};

namespace D
{
template<typename IfTrue, typename IfFalse, bool COND> struct SelectType {typedef IfTrue _;};
template<typename IfTrue, typename IfFalse> struct SelectType<IfTrue, IfFalse, false> {typedef IfFalse _;};
}

template<typename IfTrue, typename IfFalse, bool COND>
using SelectType = typename Meta::D::SelectType<IfTrue, IfFalse, COND>::_;

namespace D {
template<typename T> struct RemoveReference {typedef T _;};
template<typename T> struct RemoveReference<T&> {typedef T _;};
template<typename T> struct RemoveReference<T&&> {typedef T _;};

template<typename T> struct RemoveConst {typedef T _;};
template<typename T> struct RemoveConst<const T> {typedef T _;};

template<typename T> struct RemoveVolatile {typedef T _;};
template<typename T> struct RemoveVolatile<volatile T> {typedef T _;};
template<typename T> struct RemoveVolatile<volatile T[]> {typedef T _[];};
template<typename T, size_t N> struct RemoveVolatile<volatile T[N]> {typedef T _[N];};
}
template<typename T> using RemoveReference = typename D::RemoveReference<T>::_;
template<typename T> using RemoveConst = typename D::RemoveConst<T>::_;
template<typename T> using RemoveVolatile = typename D::RemoveVolatile<T>::_;
template<typename T> using RemoveConstRef = RemoveConst<RemoveReference<T>>;
template<typename T> using RemoveConstVolatile = RemoveConst<RemoveVolatile<T>>;

namespace D {
template<typename T> struct RemoveExtent {typedef T _;};
template<typename T, size_t N> struct RemoveExtent<T[N]> {typedef T _;};
template<typename T> struct RemoveExtent<T[]> {typedef T _;};
}
template<typename T> using RemoveExtent = typename D::RemoveExtent<T>::_;
template<typename T> using AddPointer = RemoveReference<T>*;

namespace D {
template<typename T> struct IsFunctionPointer: TypeFromValue<bool, false> {};
#if(defined(_MSC_VER) && INTRA_PLATFORM_ARCH==INTRA_PLATFORM_X86)
template<typename Ret, typename... Args> struct IsFunctionPointer<Ret(__cdecl*)(Args...)>: TypeFromValue<bool, true> {};
template<typename Ret, typename... Args> struct IsFunctionPointer<Ret(__stdcall*)(Args...)>: TypeFromValue<bool, true> {};
template<typename Ret, typename... Args> struct IsFunctionPointer<Ret(__fastcall*)(Args...)>: TypeFromValue<bool, true> {};
#else
template<typename Ret, typename... Args> struct IsFunctionPointer<Ret(*)(Args...)>: TypeFromValue<bool, true> {};
#endif
}
template<typename T> struct IsFunction: Meta::D::IsFunctionPointer<RemoveConstVolatile<T>*> {};
template<typename T> struct IsFunction<T&>: TypeFromValue<bool, false> {};
template<typename T> struct IsFunction<T&&>: TypeFromValue<bool, false> {};

template<class T, size_t N=0> struct Extent: TypeFromValue<size_t, 0> {};
template<class T> struct Extent<T[], 0>: TypeFromValue<size_t, 0> {};
template<class T, size_t N> struct Extent<T[], N>: Extent<T, N-1> {};
template<class T, size_t N> struct Extent<T[N], 0>: TypeFromValue<size_t, N> {};
template<class T, size_t I, size_t N> struct Extent<T[I], N>: Extent<T, N-1> {};

namespace D {
template<typename T> struct Decay
{
	typedef Meta::RemoveReference<T> T1;
	typedef Meta::SelectType<
		Meta::RemoveExtent<T1>*,
		Meta::SelectType<
		    Meta::AddPointer<T1>,
		    Meta::RemoveConstVolatile<T1>,
		    Meta::IsFunction<T1>::_>,
		Meta::IsArrayType<T1>::_> _;
};
}
template<typename T> using Decay = typename Meta::D::Decay<T>::_;




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

namespace D {

template<typename T> struct is_pointer_helper: TypeFromValue<bool, false> {};
template<typename T> struct is_pointer_helper<T*>: TypeFromValue<bool, true> {};

}
template<typename T> struct IsPointerType: D::is_pointer_helper<RemoveConstVolatile<T>> {};

namespace D {

template<typename T> struct is_member_pointer_helper: TypeFromValue<bool, false> {};
template<class T, class U> struct is_member_pointer_helper<T U::*>: TypeFromValue<bool, true> {};

}
template<typename T> struct IsMemberPointerType: D::is_member_pointer_helper<RemoveConstVolatile<T>> {};

template<typename T> struct IsArithmeticType: TypeFromValue<bool,
	TypeEquals<RemoveConstRef<T>, bool>::_ ||
	IsCharType<RemoveConstRef<T>>::_ ||
	IsIntegralType<RemoveConstRef<T>>::_ ||
	IsFloatType<RemoveConstRef<T>>::_
> {};






namespace D {

template<typename T> struct RemoveAllExtents {typedef T type;};
template<typename T> struct RemoveAllExtents<T[]>
{
	typedef typename RemoveAllExtents<T>::type type;
};
 
template<typename T, size_t N> struct RemoveAllExtents<T[N]>
{
	typedef typename RemoveAllExtents<T>::type type;
};

}

template<typename T> using RemoveAllExtents = typename D::RemoveAllExtents<T>::type;


#define INTRA_DEFINE_EXPRESSION_CHECKER_WITH_CONDITION(checker_name, expr, condition) \
	template<typename U> struct checker_name \
	{\
		template<typename T> static decltype((expr), ::Intra::Meta::TypeFromValue<bool, \
			(condition)>()) func(::Intra::Meta::RemoveReference<T>*);\
		template<typename T> static ::Intra::Meta::TypeFromValue<bool, false> func(...);\
		using type = decltype(func<U>(nullptr));\
		enum {_=type::_};\
	}

#define INTRA_DEFINE_EXPRESSION_CHECKER(checker_name, expr) \
	INTRA_DEFINE_EXPRESSION_CHECKER_WITH_CONDITION(checker_name, expr, true)

#define INTRA_DEFINE_EXPRESSION_CHECKER2_WITH_CONDITION(checker_name, expr, condition, default1, default2) \
	 \
	template<typename U1 default1, typename U2 default2> struct checker_name \
	{\
		template<typename T1, typename T2> static \
			decltype((expr), Meta::TypeFromValue<bool, (condition)>()) func(T1*, T2*);\
		template<typename T1, typename T2> static \
			Meta::TypeFromValue<bool, false> func(...); \
		using type = decltype(func<U1, U2>(null, null)); \
		enum {_=type::_}; \
	}

#define INTRA_DEFINE_EXPRESSION_CHECKER2(checker_name, expr, default1, default2) \
	INTRA_DEFINE_EXPRESSION_CHECKER2_WITH_CONDITION(checker_name, expr, true, default1, default2)



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
	static TypeFromValue<bool, true> test(int);

	template<typename> static TypeFromValue<bool, false> test(...);
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
template<typename T> struct is_destructible_safe<T, true, false>: TypeFromValue<bool, false> { };

}

template<typename T> struct IsDestructible: D::is_destructible_safe<T> {};


#endif


namespace D {

struct do_is_static_castable_impl
{
	template<typename SRC, typename DST, typename = decltype(static_cast<DST>(Val<SRC>()))> static TypeFromValue<bool, true> test(int);
    template<typename, typename> static Meta::TypeFromValue<bool, false> test(...);
};

template<typename SRC, typename DST> struct is_static_castable_impl: do_is_static_castable_impl
{
      typedef decltype(test<SRC, DST>(0)) type;
};

}

template<typename SRC, typename DST> struct IsStaticCastable: D::is_static_castable_impl<SRC, DST>::type {};

#ifdef _MSC_VER
template<typename T, typename... Args> struct IsConstructible:
	TypeFromValue<bool, __is_constructible(T, Args...)> {};
template<typename T> struct IsDefaultConstructible: IsConstructible<T> {};
#else

namespace D {

struct do_is_default_constructible_impl
{
	template<typename _Tp, typename = decltype(_Tp())>
	static TypeFromValue<bool, true> test(int);
	template<typename> static TypeFromValue<bool, false> test(...);
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
	template<typename T, typename Arg, typename = decltype(::new T(Val<Arg>()))> static TypeFromValue<bool, true> test(int);
	template<typename, typename> static TypeFromValue<bool, false> test(...);
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

template<typename SRC, typename DST> struct is_base_to_derived_ref<SRC, DST, false>: TypeFromValue<bool, false> {};

template<typename SRC, typename DST, bool = IsLValueReference<SRC>::_ && IsRValueReference<DST>::_> struct is_lvalue_to_rvalue_ref;

template<typename SRC, typename DST> struct is_lvalue_to_rvalue_ref<SRC, DST, true>
{
	typedef RemoveConstVolatile<RemoveReference<SRC>> src_t;
	typedef RemoveConstVolatile<RemoveReference<DST>> dst_t;
	enum: bool {_ = (!IsFunction<src_t>::_ && TypeEquals<src_t, dst_t>::_) || IsInherited<src_t, dst_t>::_};
};

template<typename SRC, typename DST> struct is_lvalue_to_rvalue_ref<SRC, DST, false>: TypeFromValue<bool, false> {};



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
	static TypeFromValue<bool, true> test(int);

	template<typename, typename...> static TypeFromValue<bool, false> test(...);
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

#ifdef _MSC_VER
template<typename T> struct IsCopyConstructible: IsConstructible<T, const T&> {};
#else
INTRA_DEFINE_EXPRESSION_CHECKER(IsCopyConstructible, T(Val<const T&>()));
#endif


template<typename R, typename... Args> struct IsCopyConstructible<R(Args...)>: TypeFromValue<bool, false> {};
template<typename T> struct IsMoveConstructible: IsConstructible<T, AddRValueReference<T>> {};
template<typename R, typename... Args> struct IsMoveConstructible<R(Args...)>: TypeFromValue<bool, false> {};

template<typename T> struct IsTriviallyCopyConstructible: IsTriviallyConstructible<T, AddLValueReference<AddConst<T>>> {};
template<typename T> struct IsTriviallyDefaultConstructible: IsTriviallyConstructible<T> {};
template<typename T> struct IsTriviallyMoveConstructible: IsTriviallyConstructible<T, AddRValueReference<T>> {};

template<typename T, typename... Args> struct IsCallable
{
	template<typename T1> struct dummy;

	template<typename CheckType> static short check(dummy<decltype(
		Val<Meta::RemoveReference<CheckType>>()(Val<Args>()...))>*);
	template<typename CheckType> static char check(...);

	enum: bool {_ = sizeof(check<T>(0))==sizeof(short)};
};

namespace D {

template<bool, typename T, typename... Args> struct ResultOf
{typedef decltype(Val<Meta::RemoveReference<T>>()(Val<Args>()...)) _;};

template<typename T, typename... Args> struct ResultOf<false, T, Args...>
{typedef void _;};

}

template<typename T, typename... Args> using ResultOf = typename D::ResultOf<IsCallable<T, Args...>::_, T, Args...>::_;


#if defined(__clang__) || defined(__GNUC__) || defined(_MSC_VER) && _MSC_VER<=1800
namespace D
{

	struct _Wrap_int {_Wrap_int(int) {}};

	template<class To, class From> struct _Is_assignable
	{
		template<class Dest, class Src> static auto _Fn(int) ->
			decltype((void)(Val<Dest>() = Val<Src>()), TypeFromValue<bool, true>());
		template<class Dest, class Src> static auto _Fn(_Wrap_int) -> TypeFromValue<bool, false>;
		typedef decltype(_Fn<To, From>(0)) _;
	};
}

template<typename To, typename From> struct IsAssignable: Meta::D::_Is_assignable<To, From>::_ {};
#else
template<typename To, typename From> struct IsAssignable: TypeFromValue<bool, __is_assignable(To, From)> {};
#endif

#ifdef _MSC_VER
template<typename To, typename From> struct IsTriviallyAssignable: TypeFromValue<bool, __is_trivially_assignable(To, From)> {};
template<typename T> struct IsTriviallyCopyAssignable: IsTriviallyAssignable<AddLValueReference<T>, AddLValueReference<AddConst<T>>> {};
#else
template<typename T> struct IsTriviallyCopyAssignable: TypeFromValue<bool, __has_trivial_assign(T)> {};
#endif

template<typename T> struct IsCopyAssignable: IsAssignable<AddLValueReference<T>, AddLValueReference<AddConst<T>>> {};
template<typename T> struct IsMoveAssignable: IsAssignable<AddLValueReference<T>, AddRValueReference<T>> {};


template<typename T> struct IsTriviallyCopyable: TypeFromValue<bool,
	IsTriviallyCopyConstructible<T>::_ || IsTriviallyCopyAssignable<T>::_> {};

//! Отличается от IsPod тем, что не требует тривиального конструктора по умолчанию.
template<typename T> struct IsAlmostPod: TypeFromValue<bool,
	IsTriviallyCopyable<T>::_ &&
	IsTriviallyDestructible<T>::_
> {};

//! Отличается от IsAlmostPod тем, что для классов и структур, содержащих указатели, он будет переопределяться на false.
template<typename T> struct IsTriviallySerializable: IsAlmostPod<T> {};

//template<template<typename> F, typename T, typename... Args> struct ForeachType;
//template<template<typename> F, typename T, typename H, typename... Args> struct ForeachType;


template<typename T> struct IsTriviallyMovable: TypeFromValue<bool,
	IsTriviallyCopyable<T>::_ || IsTriviallyMoveConstructible<T>::_ || IsTriviallyCopyAssignable<T>::_> {};

template<typename T> struct IsTriviallyRelocatable: IsTriviallyMovable<T> {};


namespace D
{
template<bool COND, class T = void> struct EnableIf {};
template<class T> struct EnableIf<true, T> {typedef T _;};
}
template<bool COND, typename T = void> using EnableIf = typename Meta::D::EnableIf<COND, T>::_;


template<typename T, typename R=void> using EnableIfPod = EnableIf<IsPod<T>::_, R>;
template<typename T, typename R=void> using EnableIfNotPod = EnableIf<!IsPod<T>::_, R>;

template<typename T, typename R=void> using EnableIfTrivConstructible = EnableIf<IsTriviallyConstructible<T>::_, R>;
template<typename T, typename R=void> using EnableIfNotTrivConstructible = EnableIf<!IsTriviallyConstructible<T>::_, R>;

template<typename T, typename R=void> using EnableIfTrivDestructible = EnableIf<IsTriviallyDestructible<T>::_, R>;
template<typename T, typename R=void> using EnableIfNotTrivDestructible = EnableIf<!IsTriviallyDestructible<T>::_, R>;

template<typename T, typename R=void> using EnableIfTrivMovable = EnableIf<IsTriviallyMovable<T>::_, R>;
template<typename T, typename R=void> using EnableIfNotTrivMovable = EnableIf<!IsTriviallyMovable<T>::_, R>;

template<typename T, typename R=void> using EnableIfTrivRelocatable = EnableIf<IsTriviallyRelocatable<T>::_, R>;
template<typename T, typename R=void> using EnableIfNotTrivRelocatable = EnableIf<!IsTriviallyRelocatable<T>::_, R>;

template<typename T, typename R=void> using EnableIfTrivCopyable = EnableIf<IsTriviallyCopyable<T>::_, R>;
template<typename T, typename R=void> using EnableIfNotTrivCopyable = EnableIf<!IsTriviallyCopyable<T>::_, R>;

template<typename T, typename R=void> using EnableIfTrivCopyAssignable = EnableIf<IsTriviallyCopyAssignable<T>::_, R>;
template<typename T, typename R=void> using EnableIfNotTrivCopyAssignable = EnableIf<!IsTriviallyCopyAssignable<T>::_, R>;


template<typename... Expressions> using void_t = void;
template<typename ReturnType, typename... Expressions> using EnableIfCompiles = ReturnType;
#define ENABLE_IF_COMPILES(returnType, expr) EnableIfCompiles<returnType, decltype(expr)>


template<typename T1, typename T2> struct TypeEqualsIgnoreCV: TypeEquals<RemoveConstVolatile<T1>, RemoveConstVolatile<T2>> {};
template<typename T1, typename T2> struct TypeEqualsIgnoreCVRef: TypeEqualsIgnoreCV<RemoveReference<T1>, RemoveReference<T2>> {};


#ifdef _MSC_VER
template<typename SRC, typename DST> struct IsConvertible: TypeFromValue<bool, __is_convertible_to(SRC, DST)> {};
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

template<typename T, typename U> struct CommonTypeRef<T, U>
{
private:
	typedef decltype(true? Val<T>(): Val<U>()) common_type_base;
public:
	typedef Meta::SelectType<
		common_type_base,
		Meta::RemoveReference<common_type_base>,
		Meta::IsReference<T>::_ && Meta::IsReference<U>::_> _;
};

template<typename T, typename U, typename... V> struct CommonTypeRef<T, U, V...>
{
	typedef typename CommonTypeRef<typename CommonTypeRef<T, U>::_, V...>::_ _;
};

}

//! Общий тип без const\volatile и ссылок.
template<typename... Types> using CommonType = typename Meta::D::CommonType<Types...>::_;

//! Общий тип по правилам тернарного оператора с сохранением ссылок и const.
template<typename... Types> using CommonTypeRef = typename Meta::D::CommonTypeRef<Types...>::_;






//#if(!defined(_MSC_VER) || defined(__clang__))
/*#define INTRA_DEFINE_HAS_METHOD_CHECKER(checker_name, method_name)\
template<typename T> struct checker_name\
	{\
private:\
		template<typename C> static char test(decltype(&C::method_name));\
		template<typename C> static short test(...);\
	    template<typename C, typename P> static auto test1(P* p) ->\
            decltype(Meta::Val<C>().method_name(*p), char(0));\
        template<typename, typename> static short test1(...); \
public:\
		enum: byte {_ = sizeof(test<T>(0))==1 || Meta::TypeEquals<char, decltype(test1<T, int>(nullptr))>::_};\
	};*/

#define INTRA_DEFINE_HAS_METHOD_CHECKER(checker_name, method_name) \
	INTRA_DEFINE_EXPRESSION_CHECKER(checker_name, &T::method_name)

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


template<typename T> constexpr forceinline Meta::RemoveReference<T>&& Move(T&& t)
{return static_cast<Meta::RemoveReference<T>&&>(t);}

template<typename T> constexpr forceinline T&& Forward(Meta::RemoveReference<T>& t)
{return static_cast<T&&>(t);}

template<typename T> constexpr forceinline T&& Forward(Meta::RemoveReference<T>&& t)
{
	static_assert(!Meta::IsLValueReference<T>::_, "bad forward call");
	return static_cast<T&&>(t);
}

template<typename T> forceinline EnableIf<!IsConst<T>::_> Swap(T&& a, T&& b)
{
	auto temp = Move(a);
	a = Move(b);
	b = Move(temp);
}

template<typename T, size_t N> constexpr forceinline size_t NumOf(const T(&)[N]) {return N;}

template<class T, typename U> constexpr forceinline size_t MemberOffset(U T::* member)
{return reinterpret_cast<size_t>(&((static_cast<T*>(nullptr))->*member));}

template<class T> forceinline T* AddressOf(T& arg)
{return reinterpret_cast<T*>(&const_cast<char&>(reinterpret_cast<const volatile char &>(arg)));}

template<typename T1, typename T2> struct Pair
{
	T1 first;
	T2 second;
};

#define INTRA_CHECK_TABLE_SIZE(table, expectedSize) static_assert(\
	sizeof(table)/sizeof(table[0])==size_t(expectedSize), "Table is outdated!")

}}

INTRA_WARNING_POP
