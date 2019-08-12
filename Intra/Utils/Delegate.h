#pragma once

#include "Core/Functional.h"
#include "Core/Assert.h"
#include "Unique.h"

/** This header file contains the definitions of the folowing types:
  1) I[Copyable][Mutable]Functor - interfaces for functors: (non-copyable / copyable) x (const / mutable).
  2) [Copyable][Mutable]Functor - templated implementations of these interfaces as wrappers of non-polymorphic functors or function pointers.
  3) [Copyable][Mutable]Delegate - type erased functors - wrappers around corresponding I[Copyable][Mutable]Functor managing its lifetime.
*/

INTRA_BEGIN
INTRA_WARNING_DISABLE_COPY_MOVE_IMPLICITLY_DELETED
inline namespace Utils {

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
template<typename FuncSignature, typename T=FuncSignature*> class Functor;
template<typename T, typename R, typename... Args>
class Functor<R(Args...), T>: public IFunctor<R(Args...)>
{
public:
	INTRA_CONSTEXPR2 forceinline Functor(T&& obj): Obj(Move(obj)) {}
	constexpr forceinline Functor(const T& obj): Obj(obj) {}
	R operator()(Args... args) const final {return Obj(Forward<Args>(args)...);}
	T Obj;
};

template<typename FuncSignature, typename T = FuncSignature*> class CopyableFunctor;
template<typename T, typename R, typename... Args>
class CopyableFunctor<R(Args...), T>: public ICopyableFunctor<R(Args...)>
{
public:
	INTRA_CONSTEXPR2 forceinline CopyableFunctor(T&& obj): Obj(Move(obj)) {}
	constexpr forceinline CopyableFunctor(const T& obj): Obj(obj) {}
	ICopyableFunctor<R(Args...)>* Clone() const final {return new CopyableFunctor(Obj);}
	R operator()(Args... args) const final {return Obj(Forward<Args>(args)...);}
	T Obj;
};


template<typename FuncSignature, typename T=FuncSignature*> class MutableFunctor;
template<typename T, typename R, typename... Args>
class MutableFunctor<R(Args...), T>: public IMutableFunctor<R(Args...)>
{
public:
	INTRA_CONSTEXPR2 forceinline MutableFunctor(T&& obj): Obj(Move(obj)) {}
	constexpr forceinline MutableFunctor(const T& obj): Obj(obj) {}
	R operator()(Args... args) final {return Obj(Forward<Args>(args)...);}
	T Obj;
};

template<typename FuncSignature, typename T = FuncSignature*> class CopyableMutableFunctor;
template<typename T, typename R, typename... Args>
class CopyableMutableFunctor<R(Args...), T>: public ICopyableMutableFunctor<R(Args...)>
{
public:
	INTRA_CONSTEXPR2 forceinline CopyableMutableFunctor(T&& obj): Obj(Move(obj)) {}
	constexpr forceinline CopyableMutableFunctor(const T& obj): Obj(obj) {}
	ICopyableMutableFunctor<R(Args...)>* Clone() const final {return new CopyableMutableFunctor(Obj);}
	R operator()(Args... args) final {return Obj(Forward<Args>(args)...);}
	T Obj;
};
///@}


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

	INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline Unique<Interface> TakeAwayFunctor() {return Move(mFunctor);}
	INTRA_NODISCARD constexpr forceinline Interface& MyFunctor() const {return *mFunctor;}
	INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline Interface* ReleaseFunctor() {return mFunctor.Release();}

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

	INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline Unique<Interface> TakeAwayFunctor() {return Move(mFunctor);}
	INTRA_NODISCARD constexpr forceinline Interface& MyFunctor() const {return *mFunctor;}
	INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline Interface* ReleaseFunctor() {return mFunctor.Release();}

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

	INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline Unique<Interface> TakeAwayFunctor() {return Move(mFunctor);}
	INTRA_NODISCARD constexpr forceinline Interface& MyFunctor() const {return *mFunctor;}
	INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline Owner<Interface*> ReleaseFunctor() {return mFunctor.Release();}

	INTRA_NODISCARD constexpr forceinline explicit operator bool() const {return mFunctor != null;}
};
///@}

}
INTRA_END
