#pragma once

#include "Assert.h"
#include "Type.h"

/** This header file contains the definitions of the folowing types:
  // TODO: list all the types
  1) I[Copyable][Mutable]Functor - interfaces for functors: (non-copyable / copyable) x (const / mutable).
  2) [Copyable][Mutable]Functor - templated implementations of these interfaces as wrappers of non-polymorphic functors or function pointers.
*/

INTRA_BEGIN
INTRA_IGNORE_WARN_COPY_MOVE_CONSTRUCT_IMPLICITLY_DELETED
INTRA_IGNORE_WARN_LOSING_CONVERSION
INTRA_IGNORE_WARN_SIGN_CONVERSION
template<class T, typename R, typename... Args> struct ObjectMethodWrapper
{
	T* ObjectRef;
	R(T::*Method)(Args...);

	constexpr R operator()(Args... args)
	{return (ObjectRef->*Method)(args...);}
};

template<class T, typename R, typename... Args> struct ObjectConstMethodWrapper
{
	const T* ObjectRef;
	R(T::*Method)(Args...);

	constexpr R operator()(Args... args) const
	{return (ObjectRef->*Method)(args...);}
};

///@{
/** Wrap ``method`` and ``object`` pointer associated with it into a functor.

  ATTENTION: Avoid dangling references. Returned functor cannot be called after destruction of ``object``.
*/
template<typename T, typename R, typename... Args>
constexpr auto ObjectMethod(T* object, R(T::* method)(Args...))
{
	INTRA_PRECONDITION(object != null);
	INTRA_PRECONDITION(method != null);
	return ObjectMethodWrapper<T, R, Args...>{object, method};
}

template<typename T, typename R, typename... Args>
constexpr auto ObjectMethod(T* object, R (T::*method)(Args...) const)
{
	INTRA_PRECONDITION(object != null);
	INTRA_PRECONDITION(method != null);
	return ObjectConstMethodWrapper<T, R, Args...>{object, method};
}
///@}


/** Combine multiple functors into a single overloaded functor.
	May be used to create compile time visitors.
*/
template<class... Callbacks> struct CombineOverloads: Callbacks... {using Callbacks::operator()...;};
template<class... Callbacks> CombineOverloads(Callbacks...) -> CombineOverloads<Callbacks...>;

#if INTRA_CONSTEXPR_TEST
static_assert(CombineOverloads{
	[](int x) {return x+1;},
	[](float x) {return x*2;}
}(3.1f) == 6.2f);
#endif


namespace z_D {
template<typename T> struct TFunctionPtrFunctor;

// Forward all classes, structs and lambdas and references to them as there are
// Ideally, type should be undefined if operator() doesn't exist.
// However it is not possible to do in common case in C++ without knowing signature
template<typename F> requires CClass<TRemoveReference<F>>
[[nodiscard]] constexpr decltype(auto) FunctorOf(F&& f) {return INTRA_FWD(f);}

#define INTRA_FUNC_WRAPPER0(callType, NOEXCEPT) template<typename R, typename... Args> \
	struct TFunctionPtrFunctor<R(callType*)(Args...) NOEXCEPT>\
	{\
		R(callType *Ptr)(Args...) NOEXCEPT;\
		constexpr R operator()(Args... args) const {return Ptr(args...);}\
	};\
	template<typename R, typename... Args> \
	constexpr auto FunctorOf(R(callType *ptr)(Args...) NOEXCEPT) {return TFunctionPtrFunctor<R(callType*)(Args...) NOEXCEPT>{ptr};}

#define INTRA_FUNC_WRAPPER(callType) INTRA_FUNC_WRAPPER0(callType,) INTRA_FUNC_WRAPPER0(callType, noexcept)

#ifdef _MSC_VER
#ifdef __i386__
INTRA_FUNC_WRAPPER(__cdecl)
INTRA_FUNC_WRAPPER(__stdcall)
INTRA_FUNC_WRAPPER(__fastcall)
#else
INTRA_FUNC_WRAPPER()
#ifdef __amd64__
INTRA_FUNC_WRAPPER(__vectorcall)
#endif
#endif
#else
INTRA_FUNC_WRAPPER()
#endif
#undef INTRA_FUNC_WRAPPER

template<class T, typename R, typename... Args> struct TMethodPtrFunctor
{
	R(T::*Ptr)(Args...);
	constexpr R operator()(T& object, Args... args) const {return (object.*Ptr)(args...);}
};
template<typename R, class T, typename... Args>
constexpr TMethodPtrFunctor<T, R, Args...> FunctorOf(R(T::*ptr)(Args...)) {return {ptr};}

template<class T, typename R, typename... Args> struct TConstMethodPtrFunctor
{
	R(T::*Ptr)(Args...) const;
	constexpr R operator()(const T& object, Args... args) const {return (object.*Ptr)(args...);}
};
template<typename R, class T, typename... Args>
constexpr TConstMethodPtrFunctor<T, R, Args...> FunctorOf(R(T::*ptr)(Args...) const) {return {ptr};}

template<class T, typename R> struct TFieldPtrFunctor
{
	R T::*Ptr;
	constexpr R operator()(T& object) const {return object.*Ptr;}
};
template<typename R, class T> constexpr TFieldPtrFunctor<T, R> FunctorOf(R T::*ptr) {return {ptr};}

template<class T, typename R> struct TConstFieldPtrFunctor
{
	R T::* const Ptr;
	constexpr R operator()(T& object) const {return object.*Ptr;}
};
template<typename R, class T> constexpr TConstFieldPtrFunctor<T, R> FunctorOf(R T::* const ptr) {return {ptr};}
}


// If type is not a class or struct or lambda, it must be something that can be wrapped to a functor:
// function pointer, method pointer or a field pointer. Otherwise TFunctorOf is void
INTRA_DEFINE_SAFE_DECLTYPE(TFunctorOf, z_D::FunctorOf(Val<T>()));

template<typename T> constexpr TFunctorOf<T&&> ForwardAsFunc(TRemoveReference<T>& arg)
{
	return z_D::FunctorOf(static_cast<T&&>(arg));
}

template<typename T> constexpr TFunctorOf<T&&> ForwardAsFunc(TRemoveReference<T>&& arg)
{
	static_assert(!CLValueReference<T>, "Bad ForwardAsFunc call!");
	return z_D::FunctorOf(static_cast<T&&>(arg));
}

template<typename F, typename... Ts> concept CAsCallable = CCallable<TFunctorOf<F>, Ts...>;

#if INTRA_CONSTEXPR_TEST
namespace z_TestFunctional {
static_assert(CSame<TFunctorOf<int(*)(int)>, z_D::TFunctionPtrFunctor<int(*)(int)>>);
static_assert(CSame<TRemoveReference<decltype(ForwardAsFunc<int(*)(int)>(Val<int(*)(int)>()))>, z_D::TFunctionPtrFunctor<int(*)(int)>>);
static_assert(CSame<TFunctorOf<int(*)(int)>, z_D::TFunctionPtrFunctor<int(*)(int)>>);

inline int func(int) {return 0;}
template<typename T> inline auto forwardTest(T&& t) {return ForwardAsFunc<T>(t);}
static_assert(CSame<TRemoveReference<decltype(forwardTest(&func))>, z_D::TFunctionPtrFunctor<int(*)(int)>>);
// TODO: test class methods and fields
}
#endif


template<typename F, typename A1> struct TBind1: F
{
	template<typename... Args> requires CCallable<F, A1, Args...>
	constexpr decltype(auto) operator()(Args&&... args)
	{return F::operator()(Arg1, INTRA_FWD(args)...);}

	template<typename... Args> requires CCallable<const F, A1, Args...>
	constexpr decltype(auto) operator()(Args&&... args) const
	{return F::operator()(Arg1, INTRA_FWD(args)...);}

	A1 Arg1;
};

template<typename F, typename Arg1> [[nodiscard]] constexpr
TBind1<TRemoveConstRef<TFunctorOf<F>>, TRemoveConstRef<Arg1>>
Bind(F&& f, Arg1&& arg1)
{return {ForwardAsFunc<F>(f), INTRA_FWD(arg1)};}

template<class P> class FCount: P
{
	index_t InvocationCounter = 0;
	constexpr FCount(P pred) noexcept: P(Move(pred)) {}
	template<typename... Args> requires CCallable<P, Args&&...>
	constexpr bool operator()(Args&&... args)
	{
		InvocationCounter++;
		return P::operator()(INTRA_FWD(args)...);
	}
};

template<class P> class CountPred: P
{
	index_t FalseInvocations = 0;
	index_t TrueInvocations = 0;
	constexpr CountPred(P pred) noexcept: P(Move(pred)) {}
	template<typename... Args> requires CCallable<P, Args&&...>
	constexpr bool operator()(Args&&... args)
	{
		const bool res = P::operator()(INTRA_FWD(args)...);
		FalseInvocations += index_t(!res);
		TrueInvocations += index_t(!!res);
		return res;
	}
};

constexpr auto FNot = []<typename P>(P&& f) {
	return [f = ForwardAsFunc<P>(f)]<typename... Args>(Args&&... args) requires(CCallable<P, Args&&...>) {
		return !f(INTRA_FWD(args)...);
	};
};

/// Wrap by moving or copying ``val`` into a functor returning it by reference.
template<typename T> struct Value
{
	T Val;
	template<typename U> requires CConstructible<T, U&&>
	explicit constexpr Value(U&& val) noexcept: Val(INTRA_FWD(val)) {}
	constexpr const T& operator()() const noexcept {return Val;}
	constexpr T& operator()() noexcept {return Val;}
};
template<typename T> Value(T) -> Value<T>;

template<typename T> struct FRef
{
	T& FunctorReference;
	constexpr FRef(T& functorReference) noexcept: FunctorReference(functorReference) {}
	template<typename... Args> requires CCallable<const T&, Args&&>
	constexpr decltype(auto) operator()(Args&&... args) const {return FunctorReference(INTRA_FWD(args)...);}
	template<typename... Args> requires CCallable<T&, Args&&>
	constexpr decltype(auto) operator()(Args&&... args) {return FunctorReference(INTRA_FWD(args)...);}
};

template<auto Value> constexpr auto StaticConst = [](auto&&...) {return Value;};
constexpr auto Always = StaticConst<true>;
constexpr auto Never = StaticConst<false>;

#if INTRA_CONSTEXPR_TEST
static_assert(Value(5)() == 5);
static_assert(Always(7, 5, "qwerty", 1));
static_assert(!Never(43, null, 65));
#endif

/// Comparison operations
constexpr auto Less = [](const auto& a, const auto& b) {return a < b;};
constexpr auto LEqual = [](const auto& a, const auto& b) {return a <= b;};
constexpr auto Greater = [](const auto& a, const auto& b) {return a > b;};
constexpr auto GEqual = [](const auto& a, const auto& b) {return a >= b;};
constexpr auto Equal = [](const auto& a, const auto& b) {return a == b;};
constexpr auto NotEqual = [](const auto& a, const auto& b) {return a != b;};

constexpr auto EqualsTo = [](auto&& x) {
	return Bind(Equal, INTRA_FWD(x));
};

/// Various unary predicates
constexpr auto IsEven = [](auto&& a) {return (a & 1) == 0;};
constexpr auto IsOdd = FNot(IsEven);

constexpr auto IsHorSpace = [](auto&& a) {return a == ' ' || a == '\t';};
constexpr auto IsLineSeparator = [](auto&& a) {return a == '\r' || a == '\n';};
constexpr auto IsSpace = [](auto&& a) {return IsHorSpace(a) || IsLineSeparator(a);};
constexpr auto IsAnySlash = [](auto&& a) {return a == '\\' || a == '/';};
constexpr auto IsDigit = [](auto&& a) {return '0' <= a && a <= '9';};
constexpr auto IsUpperLatin = [](auto&& a) {return 'A' <= a && a <= 'Z';};
constexpr auto IsLowerLatin = [](auto&& a) {return 'a' <= a && a <= 'z';};
constexpr auto IsLatin = [](auto&& a) {return IsUpperLatin(a) || IsLowerLatin(a);};
constexpr auto IsAsciiChar = [](auto&& a) {return unsigned(a) <= 127;};
constexpr auto IsAsciiControlChar = [](auto& a) {return unsigned(a) <= 31 || a == 127;};

constexpr auto Add = [](const auto& a, const auto& b) {return a + b;};
constexpr auto Sub = [](const auto& a, const auto& b) {return a - b;};
constexpr auto RSub = [](const auto& a, const auto& b) {return b - a;};
constexpr auto Mul = [](const auto& a, const auto& b) {return a * b;};
constexpr auto Div = [](const auto& a, const auto& b) {return a / b;};
constexpr auto RDiv = [](const auto& a, const auto& b) {return b / a;};
constexpr auto Mod = [](const auto& a, const auto& b) {return a % b;};
constexpr auto RMod = [](const auto& a, const auto& b) {return b % a;};

namespace z_D {
template<typename T1, typename T2> requires(requires(T1 a, T2 b) {a < b? a: b;})
constexpr auto Min_(T1 a, T2 b)
{
	using T = TCommon<T1, T2>;
#if defined(__clang__) || defined(__GNUC__) //better codegen without -ffast-math
	if constexpr(CUnqualedFloatingPoint<T>) if(!IsConstantEvaluated(a, b))
	{
		if constexpr(CSame<T, float>) return __builtin_fminf(x, y);
		else if constexpr(CSame<T, double>) return __builtin_fmin(x, y);
		else return __builtin_fminl(x, y);
	}
#endif
	return a < b? a: b;
}

template<typename T1, typename T2> requires(requires(T1 a, T2 b) {a > b? a: b;})
constexpr auto Max_(T1 a, T2 b)
{
	using T = TCommon<T1, T2>;
#if defined(__clang__) || defined(__GNUC__) //better codegen without -ffast-math
	if constexpr(CUnqualedFloatingPoint<T>) if(!IsConstantEvaluated(a, b))
	{
		if constexpr(CSame<T, float>) return __builtin_fmaxf(x, y);
		else if constexpr(CSame<T, double>) return __builtin_fmax(x, y);
		else return __builtin_fmaxl(x, y);
	}
#endif
	return a > b? a: b;
}
}

constexpr auto Min = [](const auto& a, const auto&... args)
{
	if constexpr(sizeof...(args) == 0) return INTRA_FWD(a);
	else if constexpr(sizeof...(args) == 1) return z_D::Min_(a, args...);
	else return operator()(a, operator()(args...));
};
constexpr auto Max = [](const auto& a, const auto&... args)
{
	if constexpr(sizeof...(args) == 0) return a;
	else if constexpr(sizeof...(args) == 1) return z_D::Max_(a, args...);
	else return operator()(a, operator()(args...));
};

constexpr auto Cmp = [](const auto& a, const auto& b) {return (a > b) - (a < b);};
constexpr auto ISign = [](const auto& a) {return (a > 0) - (a < 0);};
constexpr auto Sign = [](const auto& a) {return TRemoveConstRef<decltype(a)>(ISign(a));};

constexpr auto BitAnd = [](const auto& a, const auto& b) {return a & b;};
constexpr auto BitOr = [](const auto& a, const auto& b) {return a | b;};
constexpr auto BitXor = [](const auto& a, const auto& b) {return a ^ b;};
constexpr auto LShift = [](const auto& a, const auto& b) {return a << b;};
constexpr auto RShift = [](const auto& a, const auto& b) {return a >> b;};

constexpr auto And = [](const auto& a, const auto& b) {return a && b;};
constexpr auto Or = [](const auto& a, const auto& b) {return a || b;};

constexpr auto Swap = []<typename T>(T&& a, T&& b) {
	if(&a == &b) return;
	auto temp = INTRA_MOVE(a);
	a = INTRA_MOVE(b);
	b = INTRA_MOVE(temp);
};

constexpr auto Exchange = [](auto& dst, auto&& newValue) {
	auto oldValue = INTRA_MOVE(dst);
	dst = INTRA_FWD(newValue);
	return oldValue;
};

constexpr auto Move = [](auto&& x) noexcept -> decltype(auto) {return INTRA_MOVE(x);};

constexpr auto Dup = []<typename T>(T&& x) noexcept {
	if constexpr(CLValueReference<T>) return x;
	else return INTRA_MOVE(x);
};

template<typename Reduce, typename Map, typename T> struct Accum: Reduce, Map
{
	T Result;
	constexpr Accum(Reduce reduce, Map map, T initialValue):
		Reduce(INTRA_MOVE(reduce)), Map(INTRA_MOVE(map)), Result(INTRA_MOVE(initialValue)) {}
	template<typename... Args> constexpr T operator()(Args&&... args)
	{
		return Result = Reduce::operator()(Result, Map::operator()(INTRA_FWD(args)...));
	}
};
template<typename Reduce, typename Map, typename T>
Accum(Reduce&& reduce, Map&& map, T initialValue) ->
	Accum<TRemoveConstRef<TFunctorOf<Reduce>>, TRemoveConstRef<TFunctorOf<Map>>, T>;

template<typename P> constexpr auto AccumAll(P&& pred) {return Accum(And, INTRA_FWD(pred), true);}
template<typename P> constexpr auto AccumAny(P&& pred) {return Accum(Or, INTRA_FWD(pred), false);}

constexpr auto All = [](auto&&... ts) {return (ts && ...);};
constexpr auto Any = [](auto&&... ts) {return (ts || ...);};

constexpr auto ToLowerAscii = [](auto a) {return decltype(a)(IsUpperLatin(a)? a + ('a' - 'A'): a);};
constexpr auto ToUpperAscii = [](auto a) {return decltype(a)(IsLowerLatin(a)? a - ('a' - 'A'): a);};

/// Useful with ranges, for example:
/// Map(indexRange, Bind(IndexOp, valueRange))
constexpr auto IndexOp = [](auto&& from, auto&& index) {return from[index];};


template<typename T> constexpr auto CastTo = [](auto&& value) {return T(INTRA_FWD(value));};
constexpr auto Identity = [](auto&& x) -> decltype(auto) {return INTRA_FWD(x);};

///@{
/// Interface for polymorhic functor implementation.
template<typename FuncSignature> class IFunctor;
template<typename R, typename... Args> class IFunctor<R(Args...)>
{
public:
	INTRA_CONSTEXPR_DESTRUCTOR virtual ~IFunctor() {}
	constexpr virtual R operator()(Args... args) const = 0;
};

template<typename FuncSignature> class ICopyableFunctor: public IFunctor<FuncSignature>
{
public:
	constexpr virtual ICopyableFunctor* Clone() const = 0;
};


template<typename FuncSignature> class IMutableFunctor;
template<typename R, typename... Args> class IMutableFunctor<R(Args...)>
{
public:
	INTRA_CONSTEXPR_DESTRUCTOR virtual ~IMutableFunctor() {}
	constexpr virtual R operator()(Args... args) = 0;
};

template<typename FuncSignature> class ICopyableMutableFunctor: public IMutableFunctor<FuncSignature>
{
public:
	constexpr virtual ICopyableMutableFunctor* Clone() const = 0;
};
///@}

///@{
/// Polymorphic functor
template<typename FuncSignature, typename T = FuncSignature*> class Functor;
template<typename T, typename R, typename... Args>
class Functor<R(Args...), T>: public IFunctor<R(Args...)>
{
public:
	constexpr Functor(T&& obj): Obj(Move(obj)) {}
	constexpr Functor(const T& obj): Obj(obj) {}
	constexpr R operator()(Args... args) const final {return static_cast<R>(Obj(INTRA_FWD(args)...));}
	T Obj;
};

template<typename FuncSignature, typename T = FuncSignature*> class CopyableFunctor;
template<typename T, typename R, typename... Args>
class CopyableFunctor<R(Args...), T>: public ICopyableFunctor<R(Args...)>
{
public:
	constexpr CopyableFunctor(T&& obj): Obj(Move(obj)) {}
	constexpr CopyableFunctor(const T& obj): Obj(obj) {}
	constexpr ICopyableFunctor<R(Args...)>* Clone() const final {return new CopyableFunctor(Obj);}
	constexpr R operator()(Args... args) const final {return static_cast<R>(Obj(INTRA_FWD(args)...));}
	T Obj;
};


template<typename FuncSignature, typename T = FuncSignature*> class MutableFunctor;
template<typename T, typename R, typename... Args>
class MutableFunctor<R(Args...), T>: public IMutableFunctor<R(Args...)>
{
public:
	constexpr MutableFunctor(T&& obj): Obj(Move(obj)) {}
	constexpr MutableFunctor(const T& obj): Obj(obj) {}
	constexpr R operator()(Args... args) final {return static_cast<R>(Obj(INTRA_FWD(args)...));}
	T Obj;
};

template<typename FuncSignature, typename T = FuncSignature*> class CopyableMutableFunctor;
template<typename T, typename R, typename... Args>
class CopyableMutableFunctor<R(Args...), T>: public ICopyableMutableFunctor<R(Args...)>
{
public:
	constexpr CopyableMutableFunctor(T&& obj): Obj(Move(obj)) {}
	constexpr CopyableMutableFunctor(const T& obj): Obj(obj) {}
	constexpr ICopyableMutableFunctor<R(Args...)>* Clone() const final {return new CopyableMutableFunctor(Obj);}
	constexpr R operator()(Args... args) final {return static_cast<R>(Obj(INTRA_FWD(args)...));}
	T Obj;
};
///@}

INTRA_END
