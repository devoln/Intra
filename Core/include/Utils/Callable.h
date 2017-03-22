#pragma once

#include "Platform/CppFeatures.h"
#include "Platform/CppWarnings.h"
#include "Meta/Type.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Utils {

template<typename FuncSignature> class ICallable;
template<typename R, typename... Args> class ICallable<R(Args...)>
{
public:
	virtual ICallable* Clone() = 0;
	virtual ~ICallable() {}
	virtual R Call(Args&&... args) = 0;
};

template<typename FuncSignature, typename T=FuncSignature*> class FunctorCallable;
template<typename T, typename R, typename... Args> class FunctorCallable<R(Args...), T>: public ICallable<R(Args...)>
{
public:
	FunctorCallable(T&& functor): Functor(Meta::Move(functor)) {}
	FunctorCallable(const T& functor): Functor(functor) {}
	ICallable<R(Args...)>* Clone() final {return new FunctorCallable(Functor);}
	R Call(Args&&... args) final {return Functor(Meta::Forward<Args>(args)...);}

	FunctorCallable& operator=(const FunctorCallable&) = delete;

	T Functor;
};

template<typename FuncSignature, typename T> class ObjectRefMethodCallable;
template<typename T, typename R, typename... Args> class ObjectRefMethodCallable<R(Args...), T>: public ICallable<R(Args...)>
{
	typedef R(T::*MethodSignature)(Args...);
public:
	ObjectRefMethodCallable(const T& obj, MethodSignature method): ObjectRef(&obj), Method(method) {}
	ICallable<R(Args...)>* Clone() final {return new ObjectRefMethodCallable(*ObjectRef, Method);}
	R Call(Args&&... args) final {return ObjectRef->*Method(Meta::Forward<Args>(args)...);}

	const T* ObjectRef;
	MethodSignature Method;
};

template<typename Signature, typename Param> class FreeFuncCallable;
template<typename R, typename T, typename... Args> class FreeFuncCallable<R(Args...), T>: public ICallable<R(Args...)>
{
	typedef R(*FreeDataFunc)(const T&, Args...);
public:
	FreeFuncCallable(FreeDataFunc func, T&& params): Func(func), Params(Meta::Move(params)) {}
	FreeFuncCallable(FreeDataFunc func, const T& params): Func(func), Params(params) {}
	ICallable<R(Args...)>* Clone() final {return new FreeFuncCallable(Func, Params);}
	R Call(Args&&... args) final {return Func(Params, Meta::Forward<Args>(args)...);}

	FreeFuncCallable& operator=(const FreeFuncCallable&) = delete;

	FreeDataFunc Func;
	T Params;
};

}}

INTRA_WARNING_POP
