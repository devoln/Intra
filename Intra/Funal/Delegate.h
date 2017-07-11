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

template<typename Func> struct DelegateTypes
{
	typedef IFunctor<Func> Interface;
	template<typename T> using Implementation = Functor<Func, T>;
	typedef Func Function;
};

template<typename Func> struct CopyableDelegateTypes
{
	typedef ICopyableFunctor<Func> Interface;
	template<typename T> using Implementation = CopyableFunctor<Func, T>;
	typedef Func Function;
};

template<typename Func> struct MutableDelegateTypes
{
	typedef IMutableFunctor<Func> Interface;
	template<typename T> using Implementation = MutableFunctor<Func, T>;
	typedef Func Function;
};

template<typename Func> struct CopyableMutableDelegateTypes
{
	typedef ICopyableMutableFunctor<Func> Interface;
	template<typename T> using Implementation = CopyableMutableFunctor<Func, T>;
	typedef Func Function;
};

template<typename Types, typename R, typename... Args> class BasicDelegate
{
protected:
	typedef typename Types::Interface Interface;
	template<typename T> using Implementation = typename Types::template Implementation<T>;
	typedef typename Types::Function Function;
	Unique<Interface> mFunctor;
public:
	forceinline BasicDelegate(null_t=null): mFunctor(null) {}

	template<typename T, typename = Meta::EnableIf<
		!Meta::IsFunction<Meta::RemovePointer<Meta::RemoveConstRef<T>>>::_ &&
		Meta::IsCallable<Meta::RemoveConstRef<T>, Args...>::_
	>> BasicDelegate(T&& obj):
		mFunctor(Implementation<Meta::RemoveConstRef<T>>(Cpp::Forward<T>(obj))) {}

	BasicDelegate(Function* freeFunction)
	{if(freeFunction) mFunctor = new Implementation<Function*>(freeFunction);}

	template<typename T> BasicDelegate(const T& obj, R(T::*method)(Args...))
	{if(method) mFunctor = new Implementation<decltype(Bind(Method(method), obj))>(Bind(Method(method), obj));}

	template<typename T> BasicDelegate(T&& obj, R(T::*method)(Args...))
	{if(method) mFunctor = new Implementation<decltype(Bind(Method(method), Cpp::Move(obj)))>(Bind(Method(method), Cpp::Move(obj)));}

	template<typename T> BasicDelegate(T* obj, R(T::*method)(Args...))
	{if(method) mFunctor = new Implementation<decltype(ObjectMethod(obj, method))>(ObjectMethod(obj, method));}

	forceinline BasicDelegate(Unique<Interface> functor):
		mFunctor(Cpp::Move(functor)) {}

	forceinline BasicDelegate(const BasicDelegate& rhs)
	{if(rhs) mFunctor = rhs.mFunctor->Clone();}

	forceinline BasicDelegate(BasicDelegate&& rhs) = default;

	forceinline bool operator==(null_t) const {return mFunctor == null;}
	forceinline bool operator!=(null_t) const {return mFunctor != null;}
	forceinline bool operator!() const {return operator==(null);}


	BasicDelegate& operator=(const BasicDelegate& rhs)
	{
		if(!rhs.mFunctor) mFunctor = null;
		else mFunctor = rhs.mFunctor->Clone();
		return *this;
	}

	forceinline BasicDelegate& operator=(BasicDelegate&&) = default;

	Unique<Interface> TakeAwayFunctor() {return Cpp::Move(mFunctor);}
	Interface& MyFunctor() const {return *mFunctor;}
	Interface* ReleaseFunctor() {return mFunctor.Release();}

	explicit operator bool() const {return mFunctor != null;}
};


template<typename Func> class CopyableDelegate;
template<typename R, typename... Args> class CopyableDelegate<R(Args...)>:
	public BasicDelegate<CopyableDelegateTypes<R(Args...)>, R, Args...>
{
	typedef BasicDelegate super;
public:
	using super::BasicDelegate;
	forceinline CopyableDelegate(const CopyableDelegate& rhs)
	{if(rhs) super::mFunctor = rhs.mFunctor->Clone();}

	forceinline CopyableDelegate(CopyableDelegate&& rhs) = default;

	forceinline R operator()(Args... args) const
	{return (*super::mFunctor)(Cpp::Forward<Args>(args)...);}

	CopyableDelegate& operator=(const CopyableDelegate& rhs)
	{
		if(!rhs.mFunctor) super::mFunctor = null;
		else super::mFunctor = rhs.mFunctor->Clone();
		return *this;
	}

	forceinline CopyableDelegate& operator=(CopyableDelegate&&) = default;
};


template<typename Func> class Delegate;
template<typename R, typename... Args> class Delegate<R(Args...)>:
	public BasicDelegate<DelegateTypes<R(Args...)>, R, Args...>
{
	typedef BasicDelegate super;
public:
	using super::BasicDelegate;

	forceinline R operator()(Args... args) const
	{return (*super::mFunctor)(Cpp::Forward<Args>(args)...);}
};

template<typename Func> class MutableDelegate;
template<typename R, typename... Args> class MutableDelegate<R(Args...)>:
	public BasicDelegate<MutableDelegateTypes<R(Args...)>, R, Args...>
{
	typedef BasicDelegate super;
public:
	using super::BasicDelegate;

	forceinline R operator()(Args... args)
	{return (*super::mFunctor)(Cpp::Forward<Args>(args)...);}
};

template<typename Func> class CopyableMutableDelegate;
template<typename R, typename... Args> class CopyableMutableDelegate<R(Args...)>:
	public BasicDelegate<CopyableMutableDelegateTypes<R(Args...)>, R, Args...>
{
	typedef BasicDelegate super;
public:
	using super::BasicDelegate;

	forceinline CopyableMutableDelegate(const CopyableMutableDelegate& rhs)
	{if(rhs) super::mFunctor = rhs.mFunctor->Clone();}

	forceinline CopyableMutableDelegate(CopyableMutableDelegate&& rhs) = default;

	CopyableMutableDelegate& operator=(const CopyableMutableDelegate& rhs)
	{
		if(!rhs.mFunctor) super::mFunctor = null;
		else super::mFunctor = rhs.mFunctor->Clone();
		return *this;
	}

	forceinline CopyableMutableDelegate& operator=(CopyableMutableDelegate&&) = default;

	forceinline R operator()(Args... args)
	{return (*super::mFunctor)(Cpp::Forward<Args>(args)...);}
};

}
using Funal::Delegate;
using Funal::CopyableDelegate;
using Funal::MutableDelegate;
using Funal::CopyableMutableDelegate;

}

INTRA_WARNING_POP
