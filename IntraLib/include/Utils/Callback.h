#pragma once

#include "Core/Core.h"
#include "Meta/Type.h"

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
		return obj? func(obj, core::forward<Args>(a)...):
			freefunc(core::forward<Args>(a)...);
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
		func((TFunc)f), destructor(d), copy_constructor(cc)
	{
		static_assert(sizeof(T)<=MaxDataSize, "Too big struct of delegate parameters!");
		new(data) T(callData);
		core::memset(data+sizeof(callData), 0, MaxDataSize-sizeof(callData));
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
		//core::memcpy(data, rhs.data, sizeof(rhs.data)>sizeof(data)? sizeof(data): sizeof(rhs.data));
	}

	~FixedDelegate()
	{
		if(destructor) destructor(data);
	}

	R operator()(Args... a) const
	{
		return func(data, core::forward<Args>(a)...);
	}

	bool operator==(null_t) const {return func==null;}
	bool operator!=(null_t) const {return !operator==(null);}
	bool operator!() const {return operator==(null);}

	bool operator==(const FixedDelegate<R(Args...), MaxDataSize>& rhs) const
	{
		return func==rhs.func && destructor==rhs.destructor && copy_constructor==rhs.copy_constructor &&
			core::memcmp(data, rhs.data, MaxDataSize)==0;
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
