#pragma once

namespace Intra { namespace Utils {

template<class T, typename R, typename... Args> struct MethodWrapper
{
	typedef R(T::*Pointer)(Args...);
	Pointer Ptr;

	forceinline R operator()(T& object, Args... args) const {return (object.*Ptr)(args...);}
};

template<class T, typename R, typename... Args> struct ConstMethodWrapper
{
	typedef R(T::*Pointer)(Args...) const;
	Pointer Ptr;

	forceinline R operator()(const T& object, Args... args) const {return (object.*Ptr)(args...);}
};

template<typename T, typename R, typename... Args>
forceinline MethodWrapper<T, R, Args...> Method(R(T::*ptr)(Args...)) {return {ptr};}

template<typename T, typename R, typename... Args>
forceinline ConstMethodWrapper<T, R, Args...> Method(R(T::*ptr)(Args...) const) {return {ptr};}

}
using Utils::Method;

}

