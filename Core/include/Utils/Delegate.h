#pragma once

#include "Platform/FundamentalTypes.h"
#include "Platform/CppFeatures.h"
#include "Platform/CppWarnings.h"
#include "Callable.h"
#include "Platform/Debug.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Utils {

template<typename FuncSignature> class Delegate;
template<typename R, typename... Args> class Delegate<R(Args...)>
{
	ICallable<R(Args...)>* mCallback;
public:
	forceinline Delegate(null_t=null): mCallback(null) {}

	template<typename T> Delegate(R(*func)(const T&, Args...), const T& params):
		mCallback(new FreeFuncCallable<R(Args...), T>(func, params)) {}

	template<typename T, typename=Meta::EnableIf<!Meta::IsFunction<T>::_>> Delegate(const T& obj):
		mCallback(new FunctorCallable<R(Args...), T>(obj)) {}

	Delegate(R(*freeFunction)(Args...)):
		mCallback(freeFunction==null? null: new FunctorCallable<R(Args...)>(freeFunction)) {}

	template<typename T> Delegate(const T& obj, R(T::*method)(Args...)):
		mCallback(method==null? null: new ObjectRefMethodCallable<R(Args...), T>(obj, method)) {}

	Delegate(const Delegate& rhs):
		mCallback(rhs==null? null: rhs.mCallback->Clone()) {}

	Delegate(Delegate&& rhs):
		mCallback(rhs.mCallback) {rhs.mCallback=null;}

	R operator()(Args... a) const
	{
		INTRA_ASSERT(mCallback!=null);
		return mCallback->Call(Meta::Forward<Args>(a)...);
	}

	bool operator==(null_t) const {return mCallback==null;}
	bool operator!=(null_t) const {return !operator==(null);}
	bool operator!() const {return operator==(null);}


	Delegate& operator=(const Delegate& rhs)
	{
		if(mCallback!=null) delete mCallback;
		if(rhs.mCallback==null) mCallback = null;
		else mCallback = rhs.mCallback->Clone();
		return *this;
	}

	Delegate& operator=(Delegate&& rhs)
	{
		if(mCallback!=null) delete mCallback;
		mCallback = rhs.mCallback;
		rhs.mCallback = null;
		return *this;
	}
};



}}

INTRA_WARNING_POP
