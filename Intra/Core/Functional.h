#pragma once

#include "Type.h"

INTRA_CORE_BEGIN
INTRA_WARNING_DISABLE_COPY_MOVE_CONSTRUCT_IMPLICITLY_DELETED
INTRA_WARNING_DISABLE_LOSING_CONVERSION
INTRA_WARNING_DISABLE_SIGN_CONVERSION
template<class T, typename R, typename... Args> struct ObjectMethodWrapper
{
	T* ObjectRef;
	R(T::*Method)(Args...);

	INTRA_CONSTEXPR2 forceinline R operator()(Args... args)
	{return (ObjectRef->*Method)(args...);}
};

template<class T, typename R, typename... Args> struct ObjectConstMethodWrapper
{
	const T* ObjectRef;
	R(T::*Method)(Args...);

	constexpr forceinline R operator()(Args... args) const
	{return (ObjectRef->*Method)(args...);}
};

///@{
/** Wrap ``method`` and ``object`` pointer associated with it into a functor.

  ATTENTION: Avoid dangling references. Returned functor cannot be called after destruction of ``object``.
*/
template<typename T, typename R, typename... Args>
constexpr forceinline auto ObjectMethod(NotNull<T*> object, NotNull<R(T::*)(Args...)> method)
{return ObjectMethodWrapper<T, R, Args...>{object, method};}

template<typename T, typename R, typename... Args>
constexpr forceinline auto ObjectMethod(NotNull<T*> obj, NotNull<R(T::*)(Args...) const> method)
{return ObjectConstMethodWrapper<T, R, Args...>{obj, method};}
///@}

template<typename T> struct TValue
{
	T Value;
	constexpr forceinline TValue(T&& value) noexcept: Value(Move(value)) {}
	constexpr forceinline TValue(const T& value): Value(value) {}
	constexpr forceinline const T& operator()() const noexcept {return Value;}
	INTRA_CONSTEXPR2 forceinline T& operator()() noexcept {return Value;}
};

//! Wrap by moving or copying ``val`` into a functor returning it by reference.
template<typename T> constexpr forceinline TValue<TRemoveConstRef<T>> Value(T&& val) {return Forward<T>(val);}

template<typename T> struct TRef
{
	T& Ref;
	constexpr forceinline TRef(T& ref) noexcept: Ref(ref) {}
	constexpr forceinline T& operator()() const noexcept {return Ref;}
};

//! Wrap reference ``val`` into a functor returning it.
template<typename T> constexpr forceinline TRef<T> Ref(T& ref) noexcept {return ref;}


//! Combine multiple functors into a single overloaded functor.
//! May be used to create compile time visitors.
#if defined(__cpp_variadic_using) && __cpp_variadic_using >= 201611
template<class... Callbacks> struct TCombineOverloads: Ts... {using Callbacks::operator()...;};
#else
template<typename... Callbacks> struct TCombineOverloads;
template<typename F0> struct TCombineOverloads<F0>: F0
{
	constexpr forceinline TCombineOverloads(F0 f0): F0(f0) {}
	using F0::operator();
};
template<typename Base, typename... Callbacks> struct TCombineOverloads<Base, Callbacks...>: Base, TCombineOverloads<Callbacks...>
{
	constexpr forceinline TCombineOverloads(Base&& f0, Callbacks... nextCallbacks):
		Base(Move(f0)), TCombineOverloads<Callbacks...>(Move(nextCallbacks)...) {}
	using Base::operator();
	using TCombineOverloads<Callbacks...>::operator();
};
#endif
#if defined(__cpp_deduction_guides) && __cpp_deduction_guides >= 201703
template<class... Callbacks> TCombineOverloads(Callbacks...) -> TCombineOverloads<Callbacks...>;
#endif

template<typename... Callbacks> constexpr forceinline TCombineOverloads<TDecay<Callbacks>...> CombineOverloads(Callbacks&&... callbacks)
{return {Forward<Callbacks>(callbacks)...};}



namespace D {
template<typename T> struct TFunctorOf_ {};

template<typename T> struct TFunctionPtrFunctor;

// Forward all classes, structs and lambdas and references to them as there are
// Ideally type should be undefined if operator() doesn't exist.
// However it is not possible to do in common case in C++ without knowing signature
template<typename F> constexpr forceinline Requires<
	CClass<TRemoveReference<F>>,
F&&> FunctorOf(F&& f) {return Forward<F>(f);}

#define INTRA_FUNC_WRAPPER(callType) template<typename R, typename... Args> struct TFunctionPtrFunctor<R(callType*)(Args...)>\
{\
	R(callType *Ptr)(Args...);\
	INTRA_CONSTEXPR2 forceinline R operator()(Args... args) const {return Ptr(args...);}\
};\
template<typename R, typename... Args> struct TFunctorOf_<R(callType*)(Args...)> {typedef TFunctionPtrFunctor<R(callType*)(Args...)> _;};\
template<typename R, typename... Args> constexpr forceinline TFunctionPtrFunctor<R(callType*)(Args...)> FunctorOf(R(callType *ptr)(Args...)) {return {ptr};}

#ifdef _MSC_VER
#if INTRA_PLATFORM_ARCH == INTRA_PLATFORM_X86
INTRA_FUNC_WRAPPER(__cdecl);
INTRA_FUNC_WRAPPER(__stdcall);
INTRA_FUNC_WRAPPER(__fastcall);
#elif INTRA_PLATFORM_ARCH == INTRA_PLATFORM_X86_64
INTRA_FUNC_WRAPPER();
INTRA_FUNC_WRAPPER(__vectorcall);
#endif
#else
INTRA_FUNC_WRAPPER();
#endif
#undef INTRA_FUNC_WRAPPER

template<class T, typename R, typename... Args> struct TMethodPtrFunctor
{
	R(T::*Ptr)(Args...);
	constexpr forceinline R operator()(T& object, Args... args) const {return (object.*Ptr)(args...);}
};
template<typename R, class T, typename... Args> struct TFunctorOf_<R(T::*)(Args...)> {typedef TMethodPtrFunctor<T, R, Args...> _;};
template<typename R, class T, typename... Args> constexpr forceinline TMethodPtrFunctor<T, R, Args...> FunctorOf(R(T::*ptr)(Args...)) {return {ptr};}

template<class T, typename R, typename... Args> struct TConstMethodPtrFunctor
{
	R(T::*Ptr)(Args...) const;
	constexpr forceinline R operator()(const T& object, Args... args) const {return (object.*Ptr)(args...);}
};
template<typename R, class T, typename... Args> struct TFunctorOf_<R(T::*)(Args...) const> {typedef TConstMethodPtrFunctor<T, R, Args...> _;};

template<typename T, bool IsClass = CClass<TRemoveReference<T>>> struct TFunctorOf1_ {typedef T _;};
template<typename T> struct TFunctorOf1_<T, false>: TFunctorOf_<TRemovePointer<TRemoveConstRef<T>>*> {};

}


// If type is not a class or struct or lambda, it must be something that can be wrapped to a functor:
// function pointer, method pointer or a field pointer. Otherwise TFunctorOf is undefined, so it is usable in SFINAE
template<typename T> using TFunctorOf = typename D::TFunctorOf1_<T>::_;

// TODO: wrap methods and member fields

template<typename T> constexpr forceinline TFunctorOf<T&&> ForwardAsFunc(TRemoveReference<T>& arg)
{
	return D::FunctorOf(static_cast<T&&>(arg));
}

template<typename T> constexpr forceinline TFunctorOf<T&&> ForwardAsFunc(TRemoveReference<T>&& arg)
{
	static_assert(!CLValueReference<T>, "Bad ForwardAsFunc call!");
	return D::FunctorOf(static_cast<T&&>(arg));
}

template<typename F, typename... Ts> concept CAsCallable = CCallable<TFunctorOf<F>, Ts...>;


template<typename F, typename A1> struct TBind1
{
	template<typename... Args> INTRA_CONSTEXPR2 forceinline decltype(auto) operator()(Args&&... args)
	{return Func(Arg1, Forward<Args>(args)...);}

	template<typename... Args> constexpr forceinline decltype(auto) operator()(Args&&... args) const
	{return Func(Arg1, Forward<Args>(args)...);}

	F Func;
	A1 Arg1;
};

template<typename F, typename Arg1> INTRA_NODISCARD constexpr forceinline
TBind1<F, TRemoveConstRef<Arg1>>
Bind(F&& f, Arg1&& arg1)
{return {Forward<F>(f), Forward<Arg1>(arg1)};}


template<typename P> struct TPAccumAll: P
{
	bool Result = true;
	constexpr forceinline TPAccumAll(P predicate): P(Move(predicate)) {}
	template<typename... Args> INTRA_CONSTEXPR2 void operator()(Args&&... args)
	{
		Result = Result && P::operator()(Forward<Args>(args)...);
	}
};

template<typename P> struct TPAccumAny: P
{
	bool Result = false;
	constexpr forceinline TPAccumAny(P predicate): P(Move(predicate)) {}
	template<typename... Args> INTRA_CONSTEXPR2 void operator()(Args&&... args)
	{
		Result = Result || P::operator()(Forward<Args>(args)...);
	}
};


//! Arithmetic operations
///@{

namespace D {
struct TArith {};
struct TPred1 {};
struct TPred2 {};
struct TUnary {};
}

#define INTRA_DEFINE_ARITH_OP(name, op) struct T ## name: D::TArith {\
	template<typename T1, typename T2> struct Typed {INTRA_NODISCARD constexpr forceinline auto operator()(T1 a, T2 b) const {return op;}};\
	template<typename T> INTRA_NODISCARD constexpr forceinline auto operator()(const T& a, const T& b) const {return Typed<const T&, const T&>()(a, b);}\
}; constexpr static const T ## name name{}

INTRA_DEFINE_ARITH_OP(FAdd, a + b);
INTRA_DEFINE_ARITH_OP(FSub, a - b);
INTRA_DEFINE_ARITH_OP(FRSub, b - a);
INTRA_DEFINE_ARITH_OP(FMul, a * b);
INTRA_DEFINE_ARITH_OP(FDiv, a / b);
INTRA_DEFINE_ARITH_OP(FRDiv, b / a);
INTRA_DEFINE_ARITH_OP(FMod, a % b);
INTRA_DEFINE_ARITH_OP(FRMod, b % a);

INTRA_DEFINE_ARITH_OP(FMin, a < b? a: b);
INTRA_DEFINE_ARITH_OP(FMax, b < a? a: b);

INTRA_DEFINE_ARITH_OP(FAnd, a & b);
INTRA_DEFINE_ARITH_OP(FOr, a | b);
INTRA_DEFINE_ARITH_OP(FXor, a ^ b);

INTRA_DEFINE_ARITH_OP(FLShift, a << b);
INTRA_DEFINE_ARITH_OP(FRShift, a >> b);

INTRA_DEFINE_ARITH_OP(FCmp, (a > b) - (a < b));

#undef INTRA_DEFINE_ARITH_OP
///@}

//! Comparison operations

#define INTRA_DEFINE_PREDICATE2(name, op) struct T ## name: D::TPred2 {\
	template<typename T1, typename T2 = T1> constexpr forceinline bool operator()(const T1& a, const T2& b) const {return op;}\
}; constexpr static const T ## name name{}

INTRA_DEFINE_PREDICATE2(FLess, a < b);
INTRA_DEFINE_PREDICATE2(FLEqual, a <= b);
INTRA_DEFINE_PREDICATE2(FGreater, a > b);
INTRA_DEFINE_PREDICATE2(FGEqual, a >= b);
INTRA_DEFINE_PREDICATE2(FEqual, a == b);
INTRA_DEFINE_PREDICATE2(FNotEqual, a != b);

template<typename P> struct TNot1: P
{
private:
	typedef P WrappedPredicate;
public:
	constexpr forceinline TNot1(P pred): P(Move(pred)) {}
	template<typename T> constexpr forceinline bool operator()(const T& a) const {return !P::operator()(a);}
};

template<typename P> struct TNot2: P
{
private:
	typedef P WrappedPredicate;
public:
	constexpr forceinline TNot2(P pred): P(Move(pred)) {}
	template<typename T1, typename T2> constexpr forceinline bool operator()(const T1& a, const T2& b) const {return !P::operator()(a, b);}
};

template<typename P> constexpr forceinline P operator!(TNot1<P> p) {return p;}
template<typename P> constexpr forceinline P operator!(TNot2<P> p) {return p;}

template<typename P> constexpr forceinline Requires<
	CDerived<P, D::TPred1>,
TNot1<TRemoveConstRef<P>>> operator!(P&& p) {return Forward<P>(p);}

template<typename P> constexpr forceinline Requires<
	CDerived<P, D::TPred2>,
TNot2<TRemoveConstRef<P>>> operator!(P&& p) {return Forward<P>(p);}


template<typename P1, typename P2> struct TAnd1: P1, P2
{
private:
	typedef P1 WrappedPredicate1;
	typedef P2 WrappedPredicate2;
public:
	constexpr forceinline TAnd1(P1 pred1, P2 pred2): P1(Move(pred1)), P2(Move(pred2)) {}
	template<typename T> constexpr forceinline bool operator()(const T& a) const {return P1::operator()(a) && P2::operator()(a);}
};

template<typename P1, typename P2> struct TAnd2: P1, P2
{
private:
	typedef P1 WrappedPredicate1;
	typedef P2 WrappedPredicate2;
public:
	constexpr forceinline TAnd2(P1 pred1, P2 pred2): P1(Move(pred1)), P2(Move(pred2)) {}
	template<typename T1, typename T2> constexpr forceinline bool operator()(const T1& a, const T2& b) const {return P1::operator()(a, b) && P2::operator()(a, b);}
};

template<typename P1, typename P2> constexpr forceinline Requires<
	CDerived<P1, D::TPred1> &&
	CDerived<P2, D::TPred1>,
TAnd1<TRemoveConstRef<P1>, TRemoveConstRef<P2>>> operator&&(P1&& p1, P2&& p2)
{return {Forward<P1>(p1), Forward<P2>(p2)};}

template<typename P1, typename P2> constexpr forceinline Requires<
	CDerived<P1, D::TPred2> &&
	CDerived<P2, D::TPred2>,
TAnd2<TRemoveConstRef<P1>, TRemoveConstRef<P2>>> operator&&(P1&& p1, P2&& p2)
{return {Forward<P1>(p1), Forward<P2>(p2)};}


template<typename P1, typename P2> struct TOr1: P1, P2
{
	constexpr forceinline TOr1(P1 pred1, P2 pred2): P1(Move(pred1)), P2(Move(pred2)) {}
	template<typename T> constexpr forceinline bool operator()(const T& a) const {return P1::operator()(a) && P2::operator()(a);}
};

template<typename P1, typename P2> struct TOr2: P1, P2
{
	constexpr forceinline TOr2(P1 pred1, P2 pred2): P1(Move(pred1)), P2(Move(pred2)) {}
	template<typename T1, typename T2> constexpr forceinline bool operator()(const T1& a, const T2& b) const {return P1::operator()(a, b) && P2::operator()(a, b);}
};

template<typename P1, typename P2> constexpr forceinline Requires<
	CDerived<TRemoveConstRef<P1>, D::TPred1> &&
	CDerived<TRemoveConstRef<P2>, D::TPred1>,
TOr1<TRemoveConstRef<P1>, TRemoveConstRef<P2>>> operator||(P1&& p1, P2&& p2)
{return {Forward<P1>(p1), Forward<P2>(p2)};}

template<typename P1, typename P2> constexpr forceinline Requires<
	CDerived<TRemoveConstRef<P1>, D::TPred2> &&
	CDerived<TRemoveConstRef<P2>, D::TPred2>,
TOr2<TRemoveConstRef<P1>, TRemoveConstRef<P2>>> operator||(P1&& p1, P2&& p2)
{return {Forward<P1>(p1), Forward<P2>(p2)};}


//! Various unary predicates
#define INTRA_DEFINE_PREDICATE1(name, op) struct T ## name: D::TPred1 {\
	constexpr forceinline T ## name() noexcept {}\
	template<typename T> constexpr forceinline bool operator()(const T& a) const {return op;}\
}; constexpr static const T ## name name{}

INTRA_DEFINE_PREDICATE1(IsEven, (a & 1) == 0);
INTRA_DEFINE_PREDICATE1(IsOdd, (a & 1) != 0);

INTRA_DEFINE_PREDICATE1(IsHorSpace, a == ' ' || a == '\t');
INTRA_DEFINE_PREDICATE1(IsLineSeparator, a == '\r' || a == '\n');
INTRA_DEFINE_PREDICATE1(IsSpace, a == ' ' || a == '\t' || a == '\r' || a == '\n');
INTRA_DEFINE_PREDICATE1(IsAnySlash, a == '\\' || a == '/');
INTRA_DEFINE_PREDICATE1(IsDigit, '0' <= a && a <= '9');
INTRA_DEFINE_PREDICATE1(IsUpperLatin, 'A' <= a && a <= 'Z');
INTRA_DEFINE_PREDICATE1(IsLowerLatin, 'a' <= a && a <= 'z');
INTRA_DEFINE_PREDICATE1(IsLatin, ('A' <= a && a <= 'Z') || ('a' <= a && a <= 'z'));
INTRA_DEFINE_PREDICATE1(IsAsciiChar, uint(a) <= 127);
INTRA_DEFINE_PREDICATE1(IsAsciiControlChar, uint(a) <= 31 || a == 127);

INTRA_DEFINE_PREDICATE1(IsEmpty, a.Empty());
INTRA_DEFINE_PREDICATE1(IsFull, a.Full());

#undef INTRA_DEFINE_PREDICATE1

struct TFTrue
{
	constexpr forceinline TFTrue() noexcept {}
	template<typename... Args> constexpr forceinline bool operator()(Args&&...) const noexcept {return true;}
};
constexpr static const TFTrue FTrue{};

struct TFFalse
{
	constexpr forceinline TFFalse() noexcept {}
	template<typename... Args> constexpr forceinline bool operator()(Args&&...) const noexcept {return false;}
};
constexpr static const TFFalse FFalse{};


#define INTRA_DEFINE_UNARY_OP(name, op) struct T ## name {\
	constexpr forceinline T ## name() noexcept {}\
	template<typename T> constexpr forceinline T operator()(const T& a) const {return op;}\
}; constexpr static const T ## name name{}

INTRA_DEFINE_UNARY_OP(FAbs, a < 0? - a: a);
INTRA_DEFINE_UNARY_OP(ToLowerAscii, T(IsUpperLatin(a)? a + ('a' - 'A'): a));
INTRA_DEFINE_UNARY_OP(ToUpperAscii, T(IsLowerLatin(a)? a - ('a' - 'A'): a));

#define INTRA_DEFINE_BINARY_OP(name, op) struct T ## name {\
	constexpr forceinline T ## name() noexcept {}\
	template<typename T1, typename T2> constexpr forceinline decltype(auto) operator()(const T1& a, const T2& b) const {return op;}\
}; constexpr static const T ## name name{}

// Useful with ranges, for example:
// Map(indexRange, Bind(FIndex, valueRange))
INTRA_DEFINE_BINARY_OP(FIndex, a[b]);

#if INTRA_CONSTEXPR_TEST >= 201304
static_assert(Value(5)() == 5, "TEST FAILED!");
#endif

#if INTRA_CONSTEXPR_TEST
namespace CompileTimeTest_Functional {
struct add1
{
	constexpr int operator()(int x) const { return x+1; }
};
struct mul2
{
	constexpr float operator()(float x) const { return x*2; }
};
constexpr auto addmul = CombineOverloads(add1(), mul2());
static_assert(addmul(3) == 4, "TEST FAILED!");
static_assert(addmul(3.0f) == 6.0f, "TEST FAILED!");

#if INTRA_CONSTEXPR_TEST >= 201603
constexpr auto addmul2 = CombineOverloads(
	[](int x) {return x+1; },
	[](float x) {return x*2; }
);
static_assert(addmul2(3) == 4, "TEST FAILED!");
static_assert(addmul2(3.0f) == 6.0f, "TEST FAILED!");
#endif
}
#endif

#if INTRA_CONSTEXPR_TEST
namespace CompileTimeTest_Functional {
inline int func(int) {return 0;}
template<typename T> inline auto forwardTest(T&& t) {return ForwardAsFunc<T>(t);}
static_assert(CSame<int(*)(int), TRemovePointer<TRemoveConstRef<int(*&&)(int)>>*>, "");
typedef TFunctorOf<int(*&&)(int)> TTF;
static_assert(CSame<TRemoveReference<decltype(ForwardAsFunc<int(*)(int)>(Val<int(*)(int)>()))>, Core::D::TFunctionPtrFunctor<int(*)(int)>>, "TEST FAILED!");
static_assert(CSame<TFunctorOf<int(*)(int)>, Core::D::TFunctionPtrFunctor<int(*)(int)>>, "TEST FAILED!");
static_assert(CFunctionPointer<TRemoveReference<decltype(Val<int(*)(int)>())>>, "TEST FAILED!");
static_assert(CSame<TRemoveReference<decltype(forwardTest(&func))>, Core::D::TFunctionPtrFunctor<int(*)(int)>>, "TEST FAILED");
// TODO: test class methods and fields
}
#endif

INTRA_CORE_END
