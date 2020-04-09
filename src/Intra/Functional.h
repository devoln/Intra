#pragma once

#include "Assert.h"
#include "Type.h"

/** This header file contains the definitions of the folowing types:
  // TODO: list all types
  1) I[Copyable][Mutable]Functor - interfaces for functors: (non-copyable / copyable) x (const / mutable).
  2) [Copyable][Mutable]Functor - templated implementations of these interfaces as wrappers of non-polymorphic functors or function pointers.
*/

INTRA_BEGIN
INTRA_IGNORE_WARNING_COPY_MOVE_CONSTRUCT_IMPLICITLY_DELETED
INTRA_IGNORE_WARNING_LOSING_CONVERSION
INTRA_IGNORE_WARNING_SIGN_CONVERSION
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

//! Wrap by moving or copying ``val`` into a functor returning it by reference.
template<typename T> struct Value
{
	T Val;
	template<typename U, typename = Requires<CConstructible<T, U&&>>>
	explicit constexpr Value(U&& val) noexcept: Val(Forward<U>(val)) {}
	constexpr const T& operator()() const noexcept {return Val;}
	constexpr T& operator()() noexcept {return Val;}
};
template<typename T> Value(T) -> Value<T>;

#if INTRA_CONSTEXPR_TEST
static_assert(Value(5)() == 5);
#endif

//! Wrap reference ``val`` into a functor returning it.
template<typename T> struct Ref
{
	T& Ref;
	constexpr Ref(T& ref) noexcept: Ref(ref) {}
	constexpr T& operator()() const noexcept {return Ref;}
};


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
template<typename F, typename = Requires<CClass<TRemoveReference<F>>>>
[[nodiscard]] constexpr decltype(auto) FunctorOf(F&& f) {return Forward<F>(f);}

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
	template<typename... Args, typename = Requires<CCallable<F, A1, Args...>>> constexpr decltype(auto) operator()(Args&&... args)
	{return F::operator()(Arg1, Forward<Args>(args)...);}

	template<typename... Args, typename = Requires<CCallable<const F, A1, Args...>>> constexpr decltype(auto) operator()(Args&&... args) const
	{return F::operator()(Arg1, Forward<Args>(args)...);}

	A1 Arg1;
};

template<typename F, typename Arg1> [[nodiscard]] constexpr
TBind1<TRemoveConstRef<TFunctorOf<F>>, TRemoveConstRef<Arg1>>
Bind(F&& f, Arg1&& arg1)
{return {ForwardAsFunc<F>(f), Forward<Arg1>(arg1)};}

//! Arithmetic operations
///@{

namespace Tags {
struct TArith {};
struct TPred1 {};
struct TPred2 {};
struct TUnary {};
struct TBinary {};
}

#define INTRA_DEFINE_ARITH_OP(name, op) struct T ## name: Tags::TArith {\
	template<typename T1, typename T2> struct Typed {[[nodiscard]] constexpr auto operator()(T1 a, T2 b) const {return op;}};\
	template<typename T1, typename T2> [[nodiscard]] constexpr auto operator()(const T1& a, const T2& b) const {return Typed<const T1&, const T2&>()(a, b);}\
}; constexpr T ## name name{}

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

INTRA_DEFINE_ARITH_OP(FBitAnd, a & b);
INTRA_DEFINE_ARITH_OP(FBitOr, a | b);
INTRA_DEFINE_ARITH_OP(FBitXor, a ^ b);
INTRA_DEFINE_ARITH_OP(FAnd, a && b);
INTRA_DEFINE_ARITH_OP(FOr, a || b);

INTRA_DEFINE_ARITH_OP(FLShift, a << b);
INTRA_DEFINE_ARITH_OP(FRShift, a >> b);

INTRA_DEFINE_ARITH_OP(FCmp, (a > b) - (a < b));

#undef INTRA_DEFINE_ARITH_OP
///@}

//! Comparison operations

#define INTRA_DEFINE_PREDICATE2(name, op) struct T ## name: Tags::TPred2 {\
	template<typename T1, typename T2 = T1> constexpr bool operator()(const T1& a, const T2& b) const {return op;}\
}; constexpr T ## name name{}

INTRA_DEFINE_PREDICATE2(FLess, a < b);
INTRA_DEFINE_PREDICATE2(FLEqual, a <= b);
INTRA_DEFINE_PREDICATE2(FGreater, a > b);
INTRA_DEFINE_PREDICATE2(FGEqual, a >= b);
INTRA_DEFINE_PREDICATE2(FEqual, a == b);
INTRA_DEFINE_PREDICATE2(FNotEqual, a != b);

template<typename P> struct TNot: P
{
private:
	typedef P WrappedPredicate;
public:
	constexpr TNot(P pred): P(Move(pred)) {}
	template<typename... Args, typename = Requires<CCallable<P, Args&&...>>>
	[[nodiscard]] constexpr bool operator()(Args&&... args) const {return !P::operator()(Forward<Args>(args)...);}
};

template<typename P> constexpr P operator!(TNot<P> p) {return p;}

template<typename P> constexpr Requires<
	CDerived<P, Tags::TPred1>,
TNot<TRemoveConstRef<P>>> operator!(P&& p) {return Forward<P>(p);}

template<typename P> constexpr Requires<
	CDerived<P, Tags::TPred2>,
TNot<TRemoveConstRef<P>>> operator!(P&& p) {return Forward<P>(p);}

template<typename Reduce, typename Map, typename T> struct Accum: Reduce, Map
{
	T Result;
	constexpr Accum(Reduce reduce, Map map, T initialValue):
		Reduce(Move(reduce)), Map(Move(map)), Result(Move(initialValue)) {}
	template<typename... Args> constexpr T operator()(Args&&... args)
	{
		return Result = Reduce::operator()(Result, Map::operator()(Forward<Args>(args)...));
	}
};
template<typename Reduce, typename Map, typename T>
Accum(Reduce&& reduce, Map&& map, T initialValue) ->
	Accum<TRemoveConstRef<TFunctorOf<Reduce>>, TRemoveConstRef<TFunctorOf<Map>>, T>;

template<typename P> constexpr auto AccumAll(P&& pred) {return Accum(FAnd, Forward<P>(pred), true);}
template<typename P> constexpr auto AccumAny(P&& pred) {return Accum(FOr, Forward<P>(pred), false);}


//! Various unary predicates
#define INTRA_DEFINE_PREDICATE1(name, op) struct T ## name: Tags::TPred1 {\
	constexpr T ## name() noexcept {}\
	template<typename T> constexpr bool operator()(const T& a) const {return op;}\
}; constexpr T ## name name{}

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
INTRA_DEFINE_PREDICATE1(IsAsciiChar, unsigned(a) <= 127);
INTRA_DEFINE_PREDICATE1(IsAsciiControlChar, unsigned(a) <= 31 || a == 127);

#undef INTRA_DEFINE_PREDICATE1

struct INTRA_EMPTY_BASES TAlways: Tags::TPred1, Tags::TPred2
{
	template<typename... Args> constexpr bool operator()(Args&&...) const noexcept {return true;}
};
constexpr TAlways Always{};
using TNever = TNot<TAlways>;
constexpr TNever Never{Always};

template<class P> class CountPred: P
{
	index_t FalseInvocations = 0;
	index_t TrueInvocations = 0;
	constexpr CountPred(P pred) noexcept: P(Move(pred)) {}
	template<typename... Args, typename = Requires<CCallable<P, Args&&...>>>
	constexpr bool operator()(Args&&... args)
	{
		const bool res = P::operator()(Forward<Args>(args)...);
		FalseInvocations += index_t(!res);
		TrueInvocations += index_t(!!res);
		return res;
	}
};

constexpr struct {template<typename... Ts> bool operator()(Ts... ts) {return (ts && ...);}} All;
constexpr struct {template<typename... Ts> bool operator()(Ts... ts) {return (ts || ...);}} Any;


#define INTRA_DEFINE_UNARY_OP(name, op) struct T ## name: Tags::TUnary {\
	constexpr T ## name() noexcept {}\
	template<typename T> constexpr T operator()(const T& a) const {return op;}\
}; constexpr T ## name name{}

INTRA_DEFINE_UNARY_OP(FAbs, a < 0? -a: a);
INTRA_DEFINE_UNARY_OP(ToLowerAscii, T(IsUpperLatin(a)? a + ('a' - 'A'): a));
INTRA_DEFINE_UNARY_OP(ToUpperAscii, T(IsLowerLatin(a)? a - ('a' - 'A'): a));

#define INTRA_DEFINE_BINARY_OP(name, op) struct T ## name: Tags::TBinary {\
	constexpr T ## name() noexcept {}\
	template<typename T1, typename T2> constexpr decltype(auto) operator()(const T1& a, const T2& b) const {return op;}\
}; constexpr T ## name name{}

// Useful with ranges, for example:
// Map(indexRange, Bind(FIndex, valueRange))
INTRA_DEFINE_BINARY_OP(FIndex, a[b]);


template<typename T> struct TCastTo
{
	template<typename U> constexpr T operator()(U&& value) const {return T(Forward<U>(value));}
};
template<typename T> constexpr TCastTo<T> CastTo;

constexpr auto Identity = [](auto&& x) -> decltype(auto) {return Forward<decltype(x)>(x);};


///@{
//! Interface for polymorhic functor implementation.
template<typename FuncSignature> class IFunctor;
template<typename R, typename... Args> class IFunctor<R(Args...)>
{
public:
	virtual ~IFunctor() {}
	virtual R operator()(Args... args) const = 0;
};

template<typename FuncSignature> class ICopyableFunctor: public IFunctor<FuncSignature>
{
public:
	virtual ICopyableFunctor* Clone() const = 0;
};


template<typename FuncSignature> class IMutableFunctor;
template<typename R, typename... Args> class IMutableFunctor<R(Args...)>
{
public:
	virtual ~IMutableFunctor() {}
	virtual R operator()(Args... args) = 0;
};

template<typename FuncSignature> class ICopyableMutableFunctor: public IMutableFunctor<FuncSignature>
{
public:
	virtual ICopyableMutableFunctor* Clone() const = 0;
};
///@}

///@{
//! Polymorphic functor
template<typename FuncSignature, typename T = FuncSignature*> class Functor;
template<typename T, typename R, typename... Args>
class Functor<R(Args...), T>: public IFunctor<R(Args...)>
{
public:
	constexpr Functor(T&& obj): Obj(Move(obj)) {}
	constexpr Functor(const T& obj): Obj(obj) {}
	R operator()(Args... args) const final {return static_cast<R>(Obj(Forward<Args>(args)...));}
	T Obj;
};

template<typename FuncSignature, typename T = FuncSignature*> class CopyableFunctor;
template<typename T, typename R, typename... Args>
class CopyableFunctor<R(Args...), T>: public ICopyableFunctor<R(Args...)>
{
public:
	constexpr CopyableFunctor(T&& obj): Obj(Move(obj)) {}
	constexpr CopyableFunctor(const T& obj): Obj(obj) {}
	ICopyableFunctor<R(Args...)>* Clone() const final {return new CopyableFunctor(Obj);}
	R operator()(Args... args) const final {return static_cast<R>(Obj(Forward<Args>(args)...));}
	T Obj;
};


template<typename FuncSignature, typename T = FuncSignature*> class MutableFunctor;
template<typename T, typename R, typename... Args>
class MutableFunctor<R(Args...), T>: public IMutableFunctor<R(Args...)>
{
public:
	constexpr MutableFunctor(T&& obj): Obj(Move(obj)) {}
	constexpr MutableFunctor(const T& obj): Obj(obj) {}
	R operator()(Args... args) final {return static_cast<R>(Obj(Forward<Args>(args)...));}
	T Obj;
};

template<typename FuncSignature, typename T = FuncSignature*> class CopyableMutableFunctor;
template<typename T, typename R, typename... Args>
class CopyableMutableFunctor<R(Args...), T>: public ICopyableMutableFunctor<R(Args...)>
{
public:
	constexpr CopyableMutableFunctor(T&& obj): Obj(Move(obj)) {}
	constexpr CopyableMutableFunctor(const T& obj): Obj(obj) {}
	ICopyableMutableFunctor<R(Args...)>* Clone() const final {return new CopyableMutableFunctor(Obj);}
	R operator()(Args... args) final {return static_cast<R>(Obj(Forward<Args>(args)...));}
	T Obj;
};
///@}

INTRA_END
