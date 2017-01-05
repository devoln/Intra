#pragma once

#include "Platform/Intrinsics.h"
#include "Platform/CppWarnings.h"
#include "Core/FundamentalTypes.h"
#include "Core/Debug.h"
#include "Meta/Type.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS
#ifdef _MSC_VER
#pragma warning(disable: 4191 4626)
#if(_MSC_VER>=1900)
#pragma warning(disable: 5027)
#endif
#endif

namespace Intra { namespace Utils {

template<typename TFuncSignature> class Callback;
template<typename R, typename... Args> class Callback<R(Args...)>
{
public:
	typedef R(*TFunc)(void*, Args...);
	typedef R(*FreeFunc)(Args...);

	Callback(null_t=null): obj(null), func(null) {}
	Callback(void* o, TFunc f): obj(o), func(f) {}

	R operator()(Args... a) const
	{
		return obj? func(obj, Meta::Forward<Args>(a)...):
			freefunc(Meta::Forward<Args>(a)...);
	}

	bool operator==(null_t) const {return func==null;}
	bool operator!=(null_t) const {return !operator==(null);}
	bool operator!() const {return operator==(null);}

	bool operator==(const Callback<R(Args...)>& rhs) const {return obj==rhs.obj && func==rhs.func;}
	bool operator!= (const Callback<R(Args...)>& rhs) const {return !operator==(rhs);}

private:
	void* obj;
	union
	{
		TFunc func;
		FreeFunc freefunc;
	};
};

template<typename FuncSignature> class ICallback;
template<typename R, typename... Args> class ICallback<R(Args...)>
{
public:
	virtual ICallback* Clone() = 0;
	virtual ~ICallback() {}
	virtual R Call(Args&&... args) = 0;
};

template<typename FuncSignature, typename T=FuncSignature*> class FunctorCallback;
template<typename T, typename R, typename... Args> class FunctorCallback<R(Args...), T>: public ICallback<R(Args...)>
{
public:
	FunctorCallback(T&& functor): Functor(Meta::Move(functor)) {}
	FunctorCallback(const T& functor): Functor(functor) {}
	ICallback<R(Args...)>* Clone() override final {return new FunctorCallback(Functor);}
	R Call(Args&&... args) override final {return Functor(Meta::Forward<Args>(args)...);}

	T Functor;
};

template<typename FuncSignature, typename T> class ObjectRefMethodCallback;
template<typename T, typename R, typename... Args> class ObjectRefMethodCallback<R(Args...), T>: public ICallback<R(Args...)>
{
	typedef R(T::*MethodSignature)(Args...);
public:
	ObjectRefMethodCallback(const T& obj, MethodSignature method): ObjectRef(&obj), Method(method) {}
	ICallback<R(Args...)>* Clone() override final {return new ObjectRefMethodCallback(*ObjectRef, Method);}
	R Call(Args&&... args) override final {return ObjectRef->*Method(Meta::Forward<Args>(args)...);}

	const T* ObjectRef;
	MethodSignature Method;
};


template<typename FuncSignature> class Delegate;
template<typename R, typename... Args> class Delegate<R(Args...)>
{
	typedef R(*FreeFunc)(Args...);
	template<typename T> class FreeFuncDataWrapper: public ICallback<R(Args...)>
	{
		typedef R(*FreeDataFunc)(const T&, Args...);
	public:
		FreeFuncDataWrapper(FreeDataFunc func, T&& params): Func(func), Params(Meta::Move(params)) {}
		FreeFuncDataWrapper(FreeDataFunc func, const T& params): Func(func), Params(params) {}
		ICallback<R(Args...)>* Clone() override final {return new FreeFuncDataWrapper(Func, Params);}
		R Call(Args&&... args) override final {return Func(Params, Meta::Forward<Args>(args)...);}
		
		FreeDataFunc Func;
		T Params;
	};

	ICallback<R(Args...)>* callback;

public:
	forceinline Delegate(null_t=null): callback(null) {}

	template<typename T> Delegate(R(*func)(const T&, Args...), const T& params):
		callback(new FreeFuncDataWrapper<T>(func, params)) {}

	template<typename T> Delegate(const T& obj):
		callback(new FunctorCallback<R(Args...), T>(obj)) {}

	Delegate(FreeFunc f):
		callback(f==null? null: new FunctorCallback<R(Args...)>(f)) {}

	template<typename T> Delegate(const T& obj, R(T::*method)(Args...)):
		callback(method==null? null: new ObjectRefMethodCallback<R(Args...), T>(obj, method)) {}

	Delegate(const Delegate& rhs):
		callback(rhs==null? null: rhs.callback->Clone()) {}

	Delegate(Delegate&& rhs):
		callback(rhs.callback) {rhs.callback=null;}

	R operator()(Args... a) const
	{
		INTRA_ASSERT(callback!=null);
		return callback->Call(Meta::Forward<Args>(a)...);
	}

	bool operator==(null_t) const {return callback==null;}
	bool operator!=(null_t) const {return !operator==(null);}
	bool operator!() const {return operator==(null);}


	Delegate& operator=(const Delegate& rhs)
	{
		if(callback!=null) delete callback;
		callback = rhs.callback==null? null: rhs.callback->Clone();
		return *this;
	}

	Delegate& operator=(Delegate&& rhs)
	{
		if(callback!=null) delete callback;
		callback = rhs.callback;
		rhs.callback = null;
		return *this;
	}
};




template<typename TFuncSignature, uint MaxDataSize=40> class FixedDelegate;
template<uint MaxDataSize, typename R, typename... Args> class FixedDelegate<R(Args...), MaxDataSize>
{
	typedef R(*TFunc)(const void*, Args...);
	typedef void(*DestructorFunc)(void*);
	typedef void(*CopyConstructorFunc)(void*, const void*);
	typedef R(*FreeFunc)(Args...);

	static R fun_call_helper(FreeFunc f, Args... args) {return f(args...);}
	template<typename T> static R call_helper(const T& data, Args... args) {return data(args...);}
	template<typename T> static void destructor_helper(void* data) {(void)data; (reinterpret_cast<T*>(data))->~T();}
	template<typename T> static void copy_helper(void* dst, const void* src) {new(dst) T(*reinterpret_cast<const T*>(src));}

public:
	FixedDelegate(null_t=null): func(null), destructor(null), copy_constructor(null) {}

	template<typename T> FixedDelegate(R(*f)(const T&, Args...), const T& callData,
		void(*d)(void*), void(*cc)(void*, const void*)):
		func(reinterpret_cast<TFunc>(f)), destructor(d), copy_constructor(cc)
	{
		static_assert(sizeof(T)<=MaxDataSize, "Too big struct of delegate parameters!");
		new(data) T(callData);
		C::memset(data+sizeof(callData), 0, MaxDataSize-sizeof(callData));
	}

	template<typename T> FixedDelegate(R(*f)(const T&, Args...), const T& callData):
		FixedDelegate(f, callData, destructor_helper<T>, copy_helper<T>) {}

	template<typename T> FixedDelegate(const T& obj):
		FixedDelegate(call_helper<T>, obj, destructor_helper<T>, copy_helper<T>) {}

	FixedDelegate(FreeFunc f):
		FixedDelegate(fun_call_helper, f) {}

	FixedDelegate(const FixedDelegate& rhs):
		func(rhs.func), destructor(rhs.destructor), copy_constructor(rhs.copy_constructor)
	{
		if(copy_constructor) copy_constructor(data, rhs.data);
		//memcpy(data, rhs.data, sizeof(rhs.data)>sizeof(data)? sizeof(data): sizeof(rhs.data));
	}

	~FixedDelegate()
	{
		if(destructor) destructor(data);
	}

	R operator()(Args... a) const
	{
		return func(data, Meta::Forward<Args>(a)...);
	}

	bool operator==(null_t) const {return func==null;}
	bool operator!=(null_t) const {return !operator==(null);}
	bool operator!() const {return operator==(null);}

	bool operator==(const FixedDelegate<R(Args...), MaxDataSize>& rhs) const
	{
		return func==rhs.func && destructor==rhs.destructor && copy_constructor==rhs.copy_constructor &&
			C::memcmp(data, rhs.data, MaxDataSize)==0;
	}
	bool operator!=(const FixedDelegate<R(Args...), MaxDataSize>& rhs) const {return !operator==(rhs);}

	FixedDelegate& operator=(const FixedDelegate& rhs)
	{
		if(destructor) destructor(data);
		func=rhs.func;
		destructor=rhs.destructor;
		copy_constructor=rhs.copy_constructor;
		if(copy_constructor) copy_constructor(data, rhs.data);
		return *this;
	}

private:
	TFunc func;
	DestructorFunc destructor;
	CopyConstructorFunc copy_constructor;
	byte data[MaxDataSize];
};

}}

INTRA_WARNING_POP
