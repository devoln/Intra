#pragma once

#include "Core/Functional.h"
#include "Core/Assert.h"
#include "Unique.h"

/** This header file contains the definitions of [Copyable][Mutable]Delegate type-erased functors wrapping corresponding I[Copyable][Mutable]Functor and managing their lifetime.
*/

INTRA_BEGIN
INTRA_WARNING_DISABLE_COPY_MOVE_IMPLICITLY_DELETED

///@{
//! Type erased functor
template<typename FuncSignature> class CopyableDelegate;
template<typename R, typename... Args> class CopyableDelegate<R(Args...)>
{
	typedef ICopyableFunctor<R(Args...)> Interface;
	template<typename T> using Impl = CopyableFunctor<R(Args...), T>;

	Unique<Interface> mFunctor;

public:
	CopyableDelegate() = default;
	constexpr forceinline CopyableDelegate(null_t) {}

	template<typename T, typename NRT = TRemoveConstRef<T>, typename = Requires<
		CCallable<NRT, Args...>
	>> forceinline CopyableDelegate(T&& obj):
		mFunctor(new Impl<NRT>(Forward<T>(obj))) {}

	forceinline CopyableDelegate(R(*freeFunction)(Args...))
	{if(freeFunction) mFunctor = new Impl<R(*)(Args...)>(freeFunction);}

	template<typename T> forceinline CopyableDelegate(T&& obj, R(TRemoveConstRef<T>::*method)(Args...))
	{if(method) mFunctor = new Impl<decltype(Bind(Method(method), Forward<T>(obj)))>(Bind(Method(method), Forward<T>(obj)));}

	template<typename T> forceinline CopyableDelegate(T* obj, R(T::*method)(Args...))
	{if(method) mFunctor = new Impl<decltype(ObjectMethod(obj, method))>(ObjectMethod(obj, method));}

	constexpr forceinline CopyableDelegate(Unique<Interface> functor): mFunctor(Move(functor)) {}

	forceinline CopyableDelegate(const CopyableDelegate& rhs):
		mFunctor(rhs? rhs.mFunctor->Clone(): null) {}

	forceinline CopyableDelegate(CopyableDelegate&& rhs) = default;

	forceinline R operator()(Args... args) const
	{return (*mFunctor)(Forward<Args>(args)...);}

	INTRA_NODISCARD constexpr forceinline bool operator==(null_t) const {return mFunctor == null;}
	INTRA_NODISCARD constexpr forceinline bool operator!=(null_t) const {return mFunctor != null;}
	INTRA_NODISCARD constexpr forceinline bool operator!() const {return operator==(null);}


	CopyableDelegate& operator=(const CopyableDelegate& rhs)
	{
		if(!rhs.mFunctor) mFunctor = null;
		else mFunctor = rhs.mFunctor->Clone();
		return *this;
	}

	forceinline CopyableDelegate& operator=(CopyableDelegate&&) = default;

	INTRA_NODISCARD constexpr forceinline Unique<Interface> TakeAwayFunctor() {return Move(mFunctor);}
	INTRA_NODISCARD constexpr forceinline Interface& MyFunctor() const {return *mFunctor;}
	INTRA_NODISCARD constexpr forceinline Interface* ReleaseFunctor() {return mFunctor.Release();}

	INTRA_NODISCARD constexpr forceinline explicit operator bool() const {return mFunctor != null;}
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

	template<typename T, typename NRT = TRemoveConstRef<T>, typename = Requires<
		//!CFunction<NRT>::_ &&
		CCallable<NRT, Args...>
	>> forceinline CopyableMutableDelegate(T&& obj):
		mFunctor(new Impl<NRT>(Forward<T>(obj))) {}

	forceinline CopyableMutableDelegate(FunctionPtr freeFunction)
	{if(freeFunction) mFunctor = new Impl<FunctionPtr>(freeFunction);}

	template<typename T> forceinline CopyableMutableDelegate(T&& obj, R(TRemoveConstRef<T>::*method)(Args...))
	{if(method) mFunctor = new Impl<decltype(Bind(Method(method), Forward<T>(obj)))>(Bind(Method(method), Forward<T>(obj)));}

	template<typename T> forceinline CopyableMutableDelegate(T* obj, R(T::*method)(Args...))
	{if(method) mFunctor = new Impl<decltype(ObjectMethod(obj, method))>(ObjectMethod(obj, method));}

	constexpr forceinline CopyableMutableDelegate(Unique<Interface> functor):
		mFunctor(Move(functor)) {}

	forceinline CopyableMutableDelegate(const CopyableMutableDelegate& rhs):
		mFunctor(rhs? rhs.mFunctor->Clone(): null) {}

	forceinline CopyableMutableDelegate(CopyableMutableDelegate&& rhs) = default;

	forceinline R operator()(Args... args)
	{return (*mFunctor)(Forward<Args>(args)...);}

	INTRA_NODISCARD constexpr forceinline bool operator==(null_t) const {return mFunctor == null;}
	INTRA_NODISCARD constexpr forceinline bool operator!=(null_t) const {return mFunctor != null;}
	INTRA_NODISCARD constexpr forceinline bool operator!() const {return operator==(null);}


	CopyableMutableDelegate& operator=(const CopyableMutableDelegate& rhs)
	{
		if(!rhs.mFunctor) mFunctor = null;
		else mFunctor = rhs.mFunctor->Clone();
		return *this;
	}

	forceinline CopyableMutableDelegate& operator=(CopyableMutableDelegate&&) = default;

	INTRA_NODISCARD constexpr forceinline Unique<Interface> TakeAwayFunctor() {return Move(mFunctor);}
	INTRA_NODISCARD constexpr forceinline Interface& MyFunctor() const {return *mFunctor;}
	INTRA_NODISCARD constexpr forceinline Interface* ReleaseFunctor() {return mFunctor.Release();}

	INTRA_NODISCARD constexpr forceinline explicit operator bool() const {return mFunctor != null;}
};


template<typename FuncSignature> class Delegate;
template<typename R, typename... Args> class Delegate<R(Args...)>
{
	typedef IFunctor<R(Args...)> Interface;
	template<typename T> using Impl = Functor<R(Args...), T>;
	typedef R(*FunctionPtr)(Args...);
	
	Unique<Interface> mFunctor;
public:
	Delegate() = default;
	constexpr forceinline Delegate(null_t) noexcept {}

	template<typename T, typename = Requires<
		CCallable<TRemoveConstRef<T>, Args...>
	>> forceinline Delegate(T&& obj): mFunctor(new Impl<TRemoveConstRef<T>>(Forward<T>(obj))) {}

	forceinline Delegate(FunctionPtr freeFunction) {if(freeFunction) mFunctor = new Impl<FunctionPtr>(freeFunction);}

	template<typename T> forceinline Delegate(T&& obj, R(TRemoveConstRef<T>::*method)(Args...))
	{if(method) mFunctor = new Impl<decltype(Bind(Method(method), Forward<T>(obj)))>(Bind(Method(method), Forward<T>(obj)));}

	template<typename T> forceinline Delegate(T* obj, R(T::*method)(Args...))
	{if(method) mFunctor = new Impl<decltype(ObjectMethod(obj, method))>(ObjectMethod(obj, method));}

	forceinline Delegate(Unique<Interface> functor): mFunctor(Move(functor)) {}

	Delegate(Delegate&& rhs) = default;
	explicit forceinline Delegate(CopyableDelegate<R(Args...)>&& rhs) noexcept: mFunctor(rhs.TakeAwayFunctor()) {}
	Delegate(const Delegate& rhs) = delete;

	forceinline R operator()(Args... args) const
	{return (*mFunctor)(Forward<Args>(args)...);}

	INTRA_NODISCARD constexpr forceinline bool operator==(null_t) const {return mFunctor == null;}
	INTRA_NODISCARD constexpr forceinline bool operator!=(null_t) const {return mFunctor != null;}
	INTRA_NODISCARD constexpr forceinline bool operator!() const {return operator==(null);}

	Delegate& operator=(Delegate&&) = default;
	Delegate& operator=(const Delegate&) = delete;

	INTRA_NODISCARD constexpr forceinline Unique<Interface> TakeAwayFunctor() {return Move(mFunctor);}
	INTRA_NODISCARD constexpr forceinline Interface& MyFunctor() const {return *mFunctor;}
	INTRA_NODISCARD constexpr forceinline Owner<Interface*> ReleaseFunctor() {return mFunctor.Release();}

	INTRA_NODISCARD constexpr forceinline explicit operator bool() const {return mFunctor != null;}
};
///@}
INTRA_END
