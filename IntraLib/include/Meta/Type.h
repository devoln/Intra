#pragma once

#include "Core/FundamentalTypes.h"

namespace Intra { namespace Meta {

template<typename T, T value> struct TypeFromValue {enum {_=value};};

template<typename T> struct AddReference
{
	typedef T& LValue;
	typedef T&& RValue;
};

struct EmptyType
{
	template<typename... Args> EmptyType(Args&&...) {}
};

template<typename T> struct WrapperStruct {T value;};

template<typename T> using AddLValueReference = typename AddReference<T>::LValue;
template<typename T> using AddRValueReference = typename AddReference<T>::RValue;

template<typename T> using AddConst = const T;

template<typename T> struct IsPod: TypeFromValue<bool, __is_pod(T)> {};
template<typename T> struct IsAbstractClass: TypeFromValue<bool, __is_abstract(T)> {};
template<typename T> struct IsUnion: TypeFromValue<bool, __is_union(T)> {};
template<typename T> struct IsClass: TypeFromValue<bool, __is_class(T)> {};
template<typename T> struct IsEmptyClass: TypeFromValue<bool, __is_empty(T)> {};

template<typename T> struct IsTriviallyCopyable: TypeFromValue<bool, IsPod<T>::_ || __is_trivially_copyable(T)> {};
template<typename T> struct IsTriviallyDestructible: TypeFromValue<bool, __has_trivial_destructor(T)> {};

template<typename SRC, typename DST> struct IsConvertible: TypeFromValue<bool, __is_convertible_to(SRC, DST)> {};

template<typename T, typename... Args> struct IsConstructible: TypeFromValue<bool, __is_constructible(T, Args...)> {};
template<typename T, typename... Args> struct IsTriviallyConstructible: TypeFromValue<bool, __is_trivially_constructible(T, Args...)> {};

template<typename T> struct IsCopyConstructible: IsConstructible<T, AddLValueReference<AddConst<T>>> {};
template<typename T> struct IsDefaultConstructible: IsConstructible<T> {};
template<typename T> struct IsMoveConstructible: IsConstructible<T, AddRValueReference<T>> {};

template<typename T> struct IsTriviallyCopyConstructible: IsTriviallyConstructible<T, AddLValueReference<AddConst<T>>> {};
template<typename T> struct IsTriviallyDefaultConstructible: IsTriviallyConstructible<T> {};
template<typename T> struct IsTriviallyMoveConstructible: IsTriviallyConstructible<T, AddRValueReference<T>> {};

template<typename T> struct IsArray: TypeFromValue<bool, false> {};
template<typename T, size_t N> struct IsArray<T[N]>: TypeFromValue<bool, true> {};
template<typename T> struct IsArray<T[]>: TypeFromValue<bool, true> {};

template<typename T> struct IsLValueReference: TypeFromValue<bool, false> {};
template<typename T> struct IsLValueReference<T&>: TypeFromValue<bool, true> {};
template<typename T> struct IsRValueReference: TypeFromValue<bool, false> {};
template<typename T> struct IsRValueReference<T&&>: TypeFromValue<bool, true> {};
template<typename T> struct IsReference: TypeFromValue<bool, IsLValueReference<T>::_ || IsRValueReference<T>::_> {};

namespace detail {
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
template<typename T> using RemoveReference = typename detail::RemoveReference<T>::_;
template<typename T> using RemoveConst = typename detail::RemoveConst<T>::_;
template<typename T> using RemoveVolatile = typename detail::RemoveVolatile<T>::_;
template<typename T> using RemoveConstRef = RemoveConst<RemoveReference<T>>;
template<typename T> using RemoveConstVolatile = RemoveConst<RemoveVolatile<T>>;

namespace detail {
template<typename T> struct RemoveExtent {typedef T _;};
template<typename T, size_t N> struct RemoveExtent<T[N]> {typedef T _;};
template<typename T> struct RemoveExtent<T[]> {typedef T _;};
}
template<typename T> using RemoveExtent = typename detail::RemoveExtent<T>::_;
template<typename T> using AddPointer = RemoveReference<T>*;

namespace detail {
template<typename T> struct IsFunctionPointer: TypeFromValue<bool, false> {};
#if(defined(_MSC_VER) && INTRA_PLATFORM_ARCH==INTRA_PLATFORM_X86)
template<typename Ret, typename... Args> struct IsFunctionPointer<Ret(__cdecl*)(Args...)>: TypeFromValue<bool, true> {};
template<typename Ret, typename... Args> struct IsFunctionPointer<Ret(__stdcall*)(Args...)>: TypeFromValue<bool, true> {};
template<typename Ret, typename... Args> struct IsFunctionPointer<Ret(__fastcall*)(Args...)>: TypeFromValue<bool, true> {};
#else
template<typename Ret, typename... Args> struct IsFunctionPointer<Ret(*)(Args...)>: TypeFromValue<bool, true> {};
#endif
}
template<typename T> struct IsFunction: Meta::detail::IsFunctionPointer<RemoveConstVolatile<T>*> {};
template<typename T> struct IsFunction<T&>: TypeFromValue<bool, false> {};
template<typename T> struct IsFunction<T&&>: TypeFromValue<bool, false> {};

template<typename T> AddRValueReference<T> Val();

template<typename T, typename... Args> struct IsCallable
{
	template<typename T> struct dummy;

	template<typename CheckType> static short check(dummy<decltype(
		Val<CheckType>()(Val<Args>()...))>*);
	template<typename CheckType> static char check(...);

	enum: bool {_ = sizeof(check<T>(0))==sizeof(short)};
};

template<typename T, typename... Args> using ResultOf = decltype(Val<T>()(Meta::Val<Args>()...));


#if defined(__clang__) || defined(_MSC_VER) && _MSC_VER<=1800
namespace detail
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

template<typename To, typename From> struct IsAssignable: Meta::detail::_Is_assignable<To, From>::_ {};
#else
template<typename To, typename From> struct IsAssignable: TypeFromValue<bool, __is_assignable(To, From)> {};
#endif

template<typename To, typename From> struct IsTriviallyAssignable: TypeFromValue<bool, __is_trivially_assignable(To, From)> {};

template<typename T> struct IsCopyAssignable: IsAssignable<AddLValueReference<T>, AddLValueReference<AddConst<T>>> {};
template<typename T> struct IsMoveAssignable: IsAssignable<AddLValueReference<T>, AddRValueReference<T>> {};

template<typename T> struct IsTriviallyCopyAssignable: IsTriviallyAssignable<AddLValueReference<T>, AddLValueReference<AddConst<T>>> {};
template<typename T> struct IsTriviallyMoveAssignable: IsTriviallyAssignable<AddLValueReference<T>, AddRValueReference<T>> {};

//! Отличается от IsPod тем, что не требует тривиального конструктора по умолчанию.
template<typename T> struct IsTrivial: TypeFromValue<bool,
	IsTriviallyCopyable<T>::_ &&
	IsTriviallyDestructible<T>::_
> {};

//! Отличается от IsTrivial тем, что для классов и структур, содержащих указатели, он будет переопределяться на false.
template<typename T> struct IsTriviallySerializable: IsTrivial<T> {};


template<class T, class From> struct IsInherited: TypeFromValue<bool, IsClass<T>::_ && __is_base_of(From, T)> {};

//template<template<typename> F, typename T, typename... Args> struct ForeachType;
//template<template<typename> F, typename T, typename H, typename... Args> struct ForeachType;


template<typename T> struct IsTriviallyMovable: TypeFromValue<bool,
	IsTriviallyCopyable<T>::_ || IsTriviallyMoveConstructible<T>::_ || IsTriviallyMoveAssignable<T>::_> {};

template<typename T> struct IsTriviallyRelocatable: IsTriviallyMovable<T> {};


namespace detail
{
template<bool COND, class T = void> struct EnableIf {};
template<class T> struct EnableIf<true, T> {typedef T _;};
}
template<bool COND, typename T = void> using EnableIf = typename Meta::detail::EnableIf<COND, T>::_;


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


#define INTRA_DEFINE_EXPRESSION_CHECKER_WITH_CONDITION(checker_name, expr, condition) template<typename U> struct checker_name\
{\
	template<typename T> static decltype((expr), Meta::TypeFromValue<bool, (condition)>()) func(Meta::RemoveReference<T>*);\
	template<typename T> static Meta::TypeFromValue<bool, false> func(...);\
	using type = decltype(func<U>(null));\
	enum {_=type::_};\
};

#define INTRA_DEFINE_EXPRESSION_CHECKER(checker_name, expr) \
	INTRA_DEFINE_EXPRESSION_CHECKER_WITH_CONDITION(checker_name, expr, true)

#define INTRA_DEFINE_EXPRESSION_CHECKER2_WITH_CONDITION(checker_name, expr, condition, default1, default2) \
template<typename U1 default1, typename U2 default2> struct checker_name\
{\
	template<typename T1, typename T2> static decltype((expr), Meta::TypeFromValue<bool, (condition)>()) func(T1*, T2*);\
	template<typename T1, typename T2> static Meta::TypeFromValue<bool, false> func(...);\
	using type = decltype(func<U1, U2>(null, null));\
	enum {_=type::_};\
};

#define DEFINE_EXPRESSION_CHECKER2(checker_name, expr, default1, default2) \
	INTRA_DEFINE_EXPRESSION_CHECKER2_WITH_CONDITION(checker_name, expr, true, default1, default2)

DEFINE_EXPRESSION_CHECKER2(HasIndexOperator, Val<T1>()[Val<T2>()], , = size_t);


template<typename T1, typename T2> struct TypeEquals: TypeFromValue<bool, false> {};
template<typename T> struct TypeEquals<T, T>: TypeFromValue<bool, true> {};

template<typename T1, typename T2> struct TypeEqualsIgnoreCV: TypeEquals<RemoveConstVolatile<T1>, RemoveConstVolatile<T2>> {};
template<typename T1, typename T2> struct TypeEqualsIgnoreCVRef: TypeEqualsIgnoreCV<RemoveReference<T1>, RemoveReference<T2>> {};

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

template<typename T> struct IsPointerType: TypeFromValue<bool, false> {};
template<typename T> struct IsPointerType<T*>: TypeFromValue<bool, true> {};

template<typename T> struct IsFundamentalType: TypeFromValue<bool,
	TypeEquals<RemoveConstRef<T>, bool>::_ ||
	IsCharType<RemoveConstRef<T>>::_ ||
	IsIntegralType<RemoveConstRef<T>>::_ ||
	IsFloatType<RemoveConstRef<T>>::_ ||
	IsPointerType<RemoveConstRef<T>>::_
> {};

namespace detail
{
template<typename IfTrue, typename IfFalse, bool COND> struct SelectType {typedef IfTrue _;};
template<typename IfTrue, typename IfFalse> struct SelectType<IfTrue, IfFalse, false> {typedef IfFalse _;};
}

template<typename IfTrue, typename IfFalse, bool COND>
using SelectType = typename Meta::detail::SelectType<IfTrue, IfFalse, COND>::_;


namespace detail {
template<typename T> struct Decay
{
	typedef Meta::RemoveReference<T> T1;
	typedef Meta::SelectType<
		Meta::RemoveExtent<T1>*,
		Meta::SelectType<
		    Meta::AddPointer<T1>,
		    Meta::RemoveConstVolatile<T1>,
		    Meta::IsFunction<T1>::_>,
		Meta::IsArray<T1>::_> _;
};
}
template<typename T> using Decay = typename Meta::detail::Decay<T>::_;



namespace detail {
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
	typedef decltype(true? Val<T>(): Val<U>()) _;
};

template<typename T, typename U, typename... V> struct CommonTypeRef<T, U, V...>
{
	typedef typename CommonTypeRef<typename CommonTypeRef<T, U>::_, V...>::_ _;
};

}

//! Общий тип без const\volatile и ссылок.
template<typename... Types> using CommonType = typename Meta::detail::CommonType<Types...>::_;

//! Общий тип по правилам тернарного оператора с сохранением ссылок и const.
template<typename... Types> using CommonTypeRef = typename Meta::detail::CommonTypeRef<Types...>::_;






//#if(!defined(_MSC_VER) || defined(__clang__))
/*#define DEFINE_HAS_METHOD_CHECKER(checker_name, method_name)\
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

#ifdef _MSC_VER

#define DEFINE_HAS_METHOD_CHECKER(checker_name, method_name)\
	template<typename T> class checker_name { struct Yes {char unused[1];}; struct No {char unused[2];};\
		template <class C> static Yes test(char (*)[(&C::method_name == 0) + 1]);\
		template<class C> static No test(...);\
		public: enum {_ = (sizeof(test<T>(0)) == sizeof(Yes))};\
	};

#else
#define DEFINE_HAS_METHOD_CHECKER(checker_name, method_name) INTRA_DEFINE_EXPRESSION_CHECKER(checker_name, &T::method_name)
/*#define DEFINE_HAS_METHOD_CHECKER(checker_name, method_name)\
	template<typename T> class checker_name { struct Yes {char unused[1];}; struct No {char unused[2];};\
		template <class C> static Yes test(char (*)[sizeof(&C::method_name) + 1]);\
		template<class C> static No test(...);\
		public: enum {_ = (sizeof(test<T>(0)) == sizeof(Yes))};\
	};*/

#endif

/*#else
#define DEFINE_HAS_METHOD_CHECKER(checker_name, method_name)\
template<typename T> struct checker_name\
{\
    __if_exists(T::method_name) {enum: bool {_=true};}\
	__if_not_exists(T::method_name) {enum: bool {_=false};}\
};
#endif*/

#define DEFINE_HAS_MEMBER_TYPE(checker_name, Type) \
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



namespace detail
{
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
template<typename T> using LargerIntType = typename Meta::detail::LargerIntType<T>::_;

namespace detail
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
template<typename T> using MakeUnsignedType = typename Meta::detail::MakeUnsignedType<T>::_;


template<typename T> struct IsRValueRef: TypeFromValue<bool, false> {};
template<typename T> struct IsRValueRef<T&&>: TypeFromValue<bool, true> {};

namespace detail
{
	template<typename T> struct GetMemberFieldType;
	template<typename T, typename F> struct GetMemberFieldType<F T::*> {typedef F _;};
}
template<typename T> using GetMemberFieldType = typename Meta::detail::GetMemberFieldType<T>::_;

template<typename T> struct IsArrayType: TypeFromValue<bool, false> {};
template<typename T, size_t N> struct IsArrayType<T[N]>: TypeFromValue<bool, true> {};


namespace detail
{
	template<typename... Args> struct GetFirstType;
	template<typename T, typename... Args> struct GetFirstType<T, Args...> {typedef T _;};
}
template<typename... Args> using GetFirstType = typename detail::GetFirstType<Args...>::_;



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

#pragma warning(push)
#pragma warning (disable: 4310) //Не ругаться на приведение констант с усечением значения
template<typename T> struct NumericLimits<T, EnableIf<IsUnsignedIntegralType<T>::_>>
{
	constexpr static T Min() {return 0;}
	constexpr static T Max() {return T(~0ull);}
	constexpr static T Eps() {return 1;}
	enum: bool {Signed = false};
	constexpr static NumericType Type() {return NumericType::Integral;}
};
#pragma warning(pop)

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



namespace detail
{
	template<size_t SIZE> struct TypeFromSize;
	template<> struct TypeFromSize<1> {typedef byte type;};
	template<> struct TypeFromSize<2> {typedef ushort type;};
	template<> struct TypeFromSize<3> {typedef uint type;};
	template<> struct TypeFromSize<4> {typedef uint type;};
	template<> struct TypeFromSize<5> {typedef ulong64 type;};
	template<> struct TypeFromSize<6> {typedef ulong64 type;};
	template<> struct TypeFromSize<7> {typedef ulong64 type;};
	template<> struct TypeFromSize<8> {typedef ulong64 type;};
}
template<size_t SIZE> using TypeFromSize = typename Meta::detail::TypeFromSize<SIZE>::type;

}}
