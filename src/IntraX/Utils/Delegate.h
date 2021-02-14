#pragma once

#include "Intra/Functional.h"
#include "Intra/Assert.h"
#include "Unique.h"

/** This header file contains the definitions of [Copyable][Mutable]Delegate type-erased functors wrapping corresponding I[Copyable][Mutable]Functor and managing their lifetime.
*/

namespace Intra { INTRA_BEGIN
INTRA_IGNORE_WARN_COPY_MOVE_IMPLICITLY_DELETED

///@{
/// Type erased functor
template<typename FuncSignature> class CopyableDelegate;
template<typename R, typename... Args> class CopyableDelegate<R(Args...)>
{
	typedef ICopyableFunctor<R(Args...)> Interface;
	template<typename T> using Impl = CopyableFunctor<R(Args...), T>;

	Unique<Interface> mFunctor;

public:
	CopyableDelegate() = default;
	constexpr CopyableDelegate(decltype(nullptr)) {}

	template<typename T, typename NRT = TRemoveConstRef<T>, typename = Requires<
		CCallable<NRT, Args...>
	>> INTRA_FORCEINLINE CopyableDelegate(T&& obj):
		mFunctor(new Impl<NRT>(Forward<T>(obj))) {}

	INTRA_FORCEINLINE CopyableDelegate(R(*freeFunction)(Args...))
	{if(freeFunction) mFunctor = new Impl<R(*)(Args...)>(freeFunction);}

	template<typename T> INTRA_FORCEINLINE CopyableDelegate(T&& obj, R(TRemoveConstRef<T>::*method)(Args...))
	{if(method) mFunctor = new Impl<decltype(Bind(Method(method), Forward<T>(obj)))>(Bind(Method(method), Forward<T>(obj)));}

	template<typename T> INTRA_FORCEINLINE CopyableDelegate(T* obj, R(T::*method)(Args...))
	{if(method) mFunctor = new Impl<decltype(ObjectMethod(obj, method))>(ObjectMethod(obj, method));}

	constexpr CopyableDelegate(Unique<Interface> functor): mFunctor(Move(functor)) {}

	INTRA_FORCEINLINE CopyableDelegate(const CopyableDelegate& rhs):
		mFunctor(rhs? rhs.mFunctor->Clone(): nullptr) {}

	INTRA_FORCEINLINE CopyableDelegate(CopyableDelegate&& rhs) = default;

	INTRA_FORCEINLINE R operator()(Args... args) const
	{return (*mFunctor)(Forward<Args>(args)...);}

	[[nodiscard]] constexpr bool operator==(decltype(nullptr)) const {return mFunctor == nullptr;}
	[[nodiscard]] constexpr bool operator!=(decltype(nullptr)) const {return mFunctor != nullptr;}
	[[nodiscard]] constexpr bool operator!() const {return operator==(nullptr);}


	CopyableDelegate& operator=(const CopyableDelegate& rhs)
	{
		if(!rhs.mFunctor) mFunctor = nullptr;
		else mFunctor = rhs.mFunctor->Clone();
		return *this;
	}

	INTRA_FORCEINLINE CopyableDelegate& operator=(CopyableDelegate&&) = default;

	[[nodiscard]] constexpr Unique<Interface> TakeAwayFunctor() {return Move(mFunctor);}
	[[nodiscard]] constexpr Interface& MyFunctor() const {return *mFunctor;}
	[[nodiscard]] constexpr Interface* ReleaseFunctor() {return mFunctor.Release();}

	[[nodiscard]] constexpr explicit operator bool() const {return mFunctor != nullptr;}
};

template<typename FuncSignature> class CopyableMutableDelegate;
template<typename R, typename... Args> class CopyableMutableDelegate<R(Args...)>
{
	typedef ICopyableMutableFunctor<R(Args...)> Interface;
	template<typename T> using Impl = CopyableMutableFunctor<R(Args...), T>;
	typedef R(*FunctionPtr)(Args...);

	Unique<Interface> mFunctor;

public:
	INTRA_FORCEINLINE CopyableMutableDelegate(decltype(nullptr)=nullptr) noexcept {}

	template<typename T, typename NRT = TRemoveConstRef<T>, typename = Requires<
		///CFunction<NRT>::_ &&
		CCallable<NRT, Args...>
	>> INTRA_FORCEINLINE CopyableMutableDelegate(T&& obj):
		mFunctor(new Impl<NRT>(Forward<T>(obj))) {}

	INTRA_FORCEINLINE CopyableMutableDelegate(FunctionPtr freeFunction)
	{if(freeFunction) mFunctor = new Impl<FunctionPtr>(freeFunction);}

	template<typename T> INTRA_FORCEINLINE CopyableMutableDelegate(T&& obj, R(TRemoveConstRef<T>::*method)(Args...))
	{if(method) mFunctor = new Impl<decltype(Bind(Method(method), Forward<T>(obj)))>(Bind(Method(method), Forward<T>(obj)));}

	template<typename T> INTRA_FORCEINLINE CopyableMutableDelegate(T* obj, R(T::*method)(Args...))
	{if(method) mFunctor = new Impl<decltype(ObjectMethod(obj, method))>(ObjectMethod(obj, method));}

	constexpr CopyableMutableDelegate(Unique<Interface> functor):
		mFunctor(Move(functor)) {}

	INTRA_FORCEINLINE CopyableMutableDelegate(const CopyableMutableDelegate& rhs):
		mFunctor(rhs? rhs.mFunctor->Clone(): nullptr) {}

	INTRA_FORCEINLINE CopyableMutableDelegate(CopyableMutableDelegate&& rhs) = default;

	INTRA_FORCEINLINE R operator()(Args... args)
	{return (*mFunctor)(Forward<Args>(args)...);}

	[[nodiscard]] constexpr bool operator==(decltype(nullptr)) const {return mFunctor == nullptr;}
	[[nodiscard]] constexpr bool operator!=(decltype(nullptr)) const {return mFunctor != nullptr;}
	[[nodiscard]] constexpr bool operator!() const {return operator==(nullptr);}


	CopyableMutableDelegate& operator=(const CopyableMutableDelegate& rhs)
	{
		if(!rhs.mFunctor) mFunctor = nullptr;
		else mFunctor = rhs.mFunctor->Clone();
		return *this;
	}

	INTRA_FORCEINLINE CopyableMutableDelegate& operator=(CopyableMutableDelegate&&) = default;

	[[nodiscard]] constexpr Unique<Interface> TakeAwayFunctor() {return Move(mFunctor);}
	[[nodiscard]] constexpr Interface& MyFunctor() const {return *mFunctor;}
	[[nodiscard]] constexpr Interface* ReleaseFunctor() {return mFunctor.Release();}

	[[nodiscard]] constexpr explicit operator bool() const {return mFunctor != nullptr;}
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
	constexpr Delegate(decltype(nullptr)) noexcept {}

	template<typename T, typename = Requires<
		CCallable<TRemoveConstRef<T>, Args...>
	>> INTRA_FORCEINLINE Delegate(T&& obj): mFunctor(new Impl<TRemoveConstRef<T>>(Forward<T>(obj))) {}

	INTRA_FORCEINLINE Delegate(FunctionPtr freeFunction) {if(freeFunction) mFunctor = new Impl<FunctionPtr>(freeFunction);}

	template<typename T> INTRA_FORCEINLINE Delegate(T&& obj, R(TRemoveConstRef<T>::*method)(Args...))
	{if(method) mFunctor = new Impl<decltype(Bind(Method(method), Forward<T>(obj)))>(Bind(Method(method), Forward<T>(obj)));}

	template<typename T> INTRA_FORCEINLINE Delegate(T* obj, R(T::*method)(Args...))
	{if(method) mFunctor = new Impl<decltype(ObjectMethod(obj, method))>(ObjectMethod(obj, method));}

	INTRA_FORCEINLINE Delegate(Unique<Interface> functor): mFunctor(Move(functor)) {}

	Delegate(Delegate&& rhs) = default;
	explicit INTRA_FORCEINLINE Delegate(CopyableDelegate<R(Args...)>&& rhs) noexcept: mFunctor(rhs.TakeAwayFunctor()) {}
	Delegate(const Delegate& rhs) = delete;

	INTRA_FORCEINLINE R operator()(Args... args) const
	{return (*mFunctor)(Forward<Args>(args)...);}

	[[nodiscard]] constexpr bool operator==(decltype(nullptr)) const {return mFunctor == nullptr;}
	[[nodiscard]] constexpr bool operator!=(decltype(nullptr)) const {return mFunctor != nullptr;}
	[[nodiscard]] constexpr bool operator!() const {return operator==(nullptr);}

	Delegate& operator=(Delegate&&) = default;
	Delegate& operator=(const Delegate&) = delete;

	[[nodiscard]] constexpr Unique<Interface> TakeAwayFunctor() {return Move(mFunctor);}
	[[nodiscard]] constexpr Interface& MyFunctor() const {return *mFunctor;}
	[[nodiscard]] constexpr Owner<Interface*> ReleaseFunctor() {return mFunctor.Release();}

	[[nodiscard]] constexpr explicit operator bool() const {return mFunctor != nullptr;}
};
///@}
} INTRA_END
