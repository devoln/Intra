#pragma once

#include "Cpp/Features.h"
#include "Utils/Debug.h"

namespace Intra { namespace Funal {

template<class T, typename R, typename... Args> struct ObjectMethodWrapper
{
	typedef R(T::*MethodPtr)(Args...);
	T* ObjectRef;
	MethodPtr Method;

	forceinline R operator()(T& object, Args... args) const
	{
		INTRA_DEBUG_ASSERT(ObjectRef != null);
		INTRA_DEBUG_ASSERT(Method != null);
		return (ObjectRef->*Method)(args...);
	}
};

template<class T, typename R, typename... Args> struct ObjectConstMethodWrapper
{
	typedef R(T::*MethodPtr)(Args...) const;
	const T* ObjectRef;
	MethodPtr Method;

	forceinline R operator()(Args... args) const
	{
		INTRA_DEBUG_ASSERT(ObjectRef != null);
		INTRA_DEBUG_ASSERT(Method != null);
		return (ObjectRef->*Method)(args...);
	}
};

template<typename T, typename R, typename... Args>
forceinline ObjectMethodWrapper<T, R, Args...> ObjectMethod(T* obj, R(T::*method)(Args...)) {return {obj, method};}

template<typename T, typename R, typename... Args>
forceinline ObjectConstMethodWrapper<T, R, Args...> ObjectMethod(T* obj, R(T::*method)(Args...) const) {return {obj, method};}

}
using Funal::ObjectMethod;

}

