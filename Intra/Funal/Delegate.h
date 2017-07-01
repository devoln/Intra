#pragma once

#include "Cpp/Fundamental.h"
#include "Cpp/Features.h"
#include "Cpp/Warnings.h"

#include "Utils/Debug.h"
#include "Utils/Unique.h"

#include "Functor.h"
#include "Method.h"
#include "ObjectMethod.h"
#include "Bind.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Funal {

template<typename FuncSignature> class CopyableDelegate;
template<typename R, typename... Args> class CopyableDelegate<R(Args...)>
{
	Unique<ICopyableFunctor<R(Args...)>> mFunctor;
public:
	forceinline CopyableDelegate(null_t=null): mFunctor(null) {}

	template<typename T, typename = Meta::EnableIf<
		!Meta::IsFunction<T>::_
	>> CopyableDelegate(const T& obj):
		mFunctor(new CopyableFunctor<R(Args...), T>(obj)) {}

	CopyableDelegate(R(*freeFunction)(Args...))
	{if(freeFunction = new CopyableFunctor<R(Args...)>(freeFunction);)}

	template<typename T> CopyableDelegate(const T& obj, R(T::*method)(Args...))
	{if(method) mFunctor = new CopyableFunctor<R(Args...), T>(Bind(Method(method), obj));}

	template<typename T> CopyableDelegate(T&& obj, R(T::*method)(Args...))
	{if(method) mFunctor = new CopyableFunctor<R(Args...), T>(Bind(Method(method), Cpp::Move(obj)));}

	template<typename T> CopyableDelegate(T* obj, R(T::*method)(Args...))
	{if(method) mFunctor = new CopyableFunctor<R(Args...), T>(ObjectMethod(obj, method));}

	forceinline CopyableDelegate(Unique<ICopyableFunctor<R(Args...)>> functor):
		mFunctor(Cpp::Move(functor)) {}

	forceinline CopyableDelegate(const CopyableDelegate& rhs):
		mFunctor(rhs? null: rhs.mFunctor->Clone()) {}

	forceinline CopyableDelegate(CopyableDelegate&& rhs) = default;

	forceinline R operator()(Args... args) const
	{return (*mFunctor)(Cpp::Forward<Args>(args)...);}

	forceinline bool operator==(null_t) const {return mFunctor == null;}
	forceinline bool operator!=(null_t) const {return mFunctor != null;}
	forceinline bool operator!() const {return operator==(null);}


	CopyableDelegate& operator=(const CopyableDelegate& rhs)
	{
		if(!rhs.mFunctor) mFunctor = null;
		else mFunctor = rhs.mFunctor->Clone();
		return *this;
	}

	forceinline CopyableDelegate& operator=(CopyableDelegate&&) = default;

	Unique<ICopyableFunctor<R(Args...)>> TakeAwayFunctor() {return Cpp::Move(mFunctor);}
	ICopyableFunctor<R(Args...)>& MyFunctor() const {return *mFunctor;}
	ICopyableFunctor<R(Args...)>* ReleaseFunctor() {return mFunctor.Release();}

	explicit operator bool() const {return mFunctor != null;}
};


template<typename FuncSignature> class Delegate;
template<typename R, typename... Args> class Delegate<R(Args...)>
{
	Unique<IFunctor<R(Args...)>> mFunctor;
public:
	forceinline Delegate(null_t=null): mFunctor(null) {}

	template<typename T, typename NRT = Meta::RemoveConstRef<T>, typename = Meta::EnableIf<
		!Meta::IsFunction<NRT>::_
	>> Delegate(T&& obj): mFunctor(new Functor<R(Args...), NRT>(Cpp::Forward<T>(obj))) {}

	Delegate(R(*freeFunction)(Args...)) {if(freeFunction) mFunctor = new Functor<R(Args...)>(freeFunction);}

	template<typename T, typename NRT = Meta::RemoveConstRef<T>> Delegate(T&& obj, R(NRT::*method)(Args...))
	{if(method) mFunctor = new Functor<R(Args...), NRT>(Bind(Method(method), Cpp::Forward<T>(obj)));}

	template<typename T> Delegate(T* obj, R(T::*method)(Args...))
	{if(method) mFunctor = new Functor<R(Args...), T>(ObjectMethod(obj, method));}

	forceinline Delegate(Unique<IFunctor<R(Args...)>> functor): mFunctor(Cpp::Move(functor)) {}

	Delegate(Delegate&& rhs) = default;
	Delegate(CopyableDelegate<R(Args...)>&& rhs): mFunctor(rhs.TakeAwayFunctor()) {}
	Delegate(const Delegate& rhs) = delete;

	forceinline R operator()(Args... args) const
	{return (*mFunctor)(Cpp::Forward<Args>(args)...);}

	forceinline bool operator==(null_t) const {return mFunctor == null;}
	forceinline bool operator!=(null_t) const {return mFunctor != null;}
	forceinline bool operator!() const {return operator==(null);}

	Delegate& operator=(Delegate&&) = default;
	Delegate& operator=(const Delegate&) = delete;

	Unique<IFunctor<R(Args...)>> TakeAwayFunctor() {return Cpp::Move(mFunctor);}
	IFunctor<R(Args...)>& MyFunctor() const {return *mFunctor;}
	IFunctor<R(Args...)>* ReleaseFunctor() {return mFunctor.Release();}

	explicit operator bool() const {return mFunctor != null;}
};

}
using Funal::Delegate;
using Funal::CopyableDelegate;

}

INTRA_WARNING_POP
