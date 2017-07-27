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
	typedef ICopyableFunctor<R(Args...)> Interface;
	template<typename T> using Impl = CopyableFunctor<R(Args...), T>;

	Unique<Interface> mFunctor;

public:
	forceinline CopyableDelegate(null_t=null): mFunctor(null) {}

	template<typename T, typename NRT = Meta::RemoveConstRef<T>, typename = Meta::EnableIf<
		Meta::IsCallable<NRT, Args...>::_
	>> CopyableDelegate(T&& obj):
		mFunctor(new Impl<NRT>(Cpp::Forward<T>(obj))) {}

	CopyableDelegate(R(*freeFunction)(Args...))
	{if(freeFunction) mFunctor = new Impl<R(*)(Args...)>(freeFunction);}

	template<typename T> CopyableDelegate(T&& obj, R(Meta::RemoveConstRef<T>::*method)(Args...))
	{if(method) mFunctor = new Impl<decltype(Bind(Method(method), Cpp::Forward<T>(obj)))>(Bind(Method(method), Cpp::Forward<T>(obj)));}

	template<typename T> CopyableDelegate(T* obj, R(T::*method)(Args...))
	{if(method) mFunctor = new Impl<decltype(ObjectMethod(obj, method))>(ObjectMethod(obj, method));}

	forceinline CopyableDelegate(Unique<Interface> functor):
		mFunctor(Cpp::Move(functor)) {}

	forceinline CopyableDelegate(const CopyableDelegate& rhs):
		mFunctor(rhs? rhs.mFunctor->Clone(): null) {}

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

	Unique<Interface> TakeAwayFunctor() {return Cpp::Move(mFunctor);}
	Interface& MyFunctor() const {return *mFunctor;}
	Interface* ReleaseFunctor() {return mFunctor.Release();}

	explicit operator bool() const {return mFunctor != null;}
};

template<typename FuncSignature> class CopyableMutableDelegate;
template<typename R, typename... Args> class CopyableMutableDelegate<R(Args...)>
{
	typedef ICopyableMutableFunctor<R(Args...)> Interface;
	template<typename T> using Impl = CopyableMutableFunctor<R(Args...), T>;
	typedef R(*FunctionPtr)(Args...);

	Unique<Interface> mFunctor;

public:
	forceinline CopyableMutableDelegate(null_t=null) noexcept {}

	template<typename T, typename NRT = Meta::RemoveConstRef<T>, typename = Meta::EnableIf<
		//!Meta::IsFunction<NRT>::_ &&
		Meta::IsCallable<NRT, Args...>::_
	>> CopyableMutableDelegate(T&& obj):
		mFunctor(new Impl<NRT>(Cpp::Forward<T>(obj))) {}

	CopyableMutableDelegate(FunctionPtr freeFunction)
	{if(freeFunction) mFunctor = new Impl<FunctionPtr>(freeFunction);}

	template<typename T> CopyableMutableDelegate(T&& obj, R(Meta::RemoveConstRef<T>::*method)(Args...))
	{if(method) mFunctor = new Impl<decltype(Bind(Method(method), Cpp::Forward<T>(obj)))>(Bind(Method(method), Cpp::Forward<T>(obj)));}

	template<typename T> CopyableMutableDelegate(T* obj, R(T::*method)(Args...))
	{if(method) mFunctor = new Impl<decltype(ObjectMethod(obj, method))>(ObjectMethod(obj, method));}

	forceinline CopyableMutableDelegate(Unique<Interface> functor):
		mFunctor(Cpp::Move(functor)) {}

	forceinline CopyableMutableDelegate(const CopyableMutableDelegate& rhs):
		mFunctor(rhs? rhs.mFunctor->Clone(): null) {}

	forceinline CopyableMutableDelegate(CopyableMutableDelegate&& rhs) = default;

	forceinline R operator()(Args... args)
	{return (*mFunctor)(Cpp::Forward<Args>(args)...);}

	forceinline bool operator==(null_t) const {return mFunctor == null;}
	forceinline bool operator!=(null_t) const {return mFunctor != null;}
	forceinline bool operator!() const {return operator==(null);}


	CopyableMutableDelegate& operator=(const CopyableMutableDelegate& rhs)
	{
		if(!rhs.mFunctor) mFunctor = null;
		else mFunctor = rhs.mFunctor->Clone();
		return *this;
	}

	forceinline CopyableMutableDelegate& operator=(CopyableMutableDelegate&&) = default;

	Unique<Interface> TakeAwayFunctor() {return Cpp::Move(mFunctor);}
	Interface& MyFunctor() const {return *mFunctor;}
	Interface* ReleaseFunctor() {return mFunctor.Release();}

	explicit operator bool() const {return mFunctor != null;}
};


template<typename FuncSignature> class Delegate;
template<typename R, typename... Args> class Delegate<R(Args...)>
{
	typedef IFunctor<R(Args...)> Interface;
	template<typename T> using Impl = Functor<R(Args...), T>;
	typedef R(*FunctionPtr)(Args...);
	
	Unique<Interface> mFunctor;
public:
	forceinline Delegate(null_t=null) noexcept {}

	template<typename T, typename = Meta::EnableIf<
		Meta::IsCallable<Meta::RemoveConstRef<T>, Args...>::_
	>> Delegate(T&& obj): mFunctor(new Impl<Meta::RemoveConstRef<T>>(Cpp::Forward<T>(obj))) {}

	Delegate(FunctionPtr freeFunction) {if(freeFunction) mFunctor = new Impl<FunctionPtr>(freeFunction);}

	template<typename T> Delegate(T&& obj, R(Meta::RemoveConstRef<T>::*method)(Args...))
	{if(method) mFunctor = new Impl<decltype(Bind(Method(method), Cpp::Forward<T>(obj)))>(Bind(Method(method), Cpp::Forward<T>(obj)));}

	template<typename T> Delegate(T* obj, R(T::*method)(Args...))
	{if(method) mFunctor = new Impl<decltype(ObjectMethod(obj, method))>(ObjectMethod(obj, method));}

	forceinline Delegate(Unique<Interface> functor): mFunctor(Cpp::Move(functor)) {}

	Delegate(Delegate&& rhs) = default;
	explicit forceinline Delegate(CopyableDelegate<R(Args...)>&& rhs) noexcept: mFunctor(rhs.TakeAwayFunctor()) {}
	Delegate(const Delegate& rhs) = delete;

	forceinline R operator()(Args... args) const
	{return (*mFunctor)(Cpp::Forward<Args>(args)...);}

	forceinline bool operator==(null_t) const {return mFunctor == null;}
	forceinline bool operator!=(null_t) const {return mFunctor != null;}
	forceinline bool operator!() const {return operator==(null);}

	Delegate& operator=(Delegate&&) = default;
	Delegate& operator=(const Delegate&) = delete;

	Unique<Interface> TakeAwayFunctor() {return Cpp::Move(mFunctor);}
	Interface& MyFunctor() const {return *mFunctor;}
	Interface* ReleaseFunctor() {return mFunctor.Release();}

	explicit operator bool() const {return mFunctor != null;}
};

}
using Funal::Delegate;
using Funal::CopyableDelegate;
using Funal::CopyableMutableDelegate;

}

INTRA_WARNING_POP
