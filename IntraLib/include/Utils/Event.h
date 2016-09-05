#pragma once

#include "Utils/Callback.h"
#include "Containers/Array.h"

namespace Intra { namespace Utils {

namespace detail
{
	template<typename R, class T, typename... Args> struct DeduceConstMemCallback
	{
		template<R(T::*Func)(Args...) const> static Callback<R(Args...)> Bind(T* o)
		{
			struct _ { static R wrapper(void* o, Args... a) { return (static_cast<T*>(o)->*Func)(core::forward<Args>(a)...); } };
			return Callback<R(Args...)>(o, (R(*)(void*, Args...)) _::wrapper);
		}
	};

	template<typename R, class T, typename... Args> struct DeduceMemCallback
	{
		template<R(T::*Func)(Args...)> static Callback<R(Args...)> Bind(T* o)
		{
			struct _ {static R wrapper(void* o, Args... a) {return (static_cast<T*>(o)->*Func)(core::forward<Args>(a)...);}};
			return Callback<R(Args...)>(o, (R(*)(void*, Args...)) _::wrapper);
		}
	};

	template<typename R, typename... Args> struct DeduceStaticCallback
	{
		template<R(*Func)(Args...)> static Callback<R(Args...)> Bind()
		{
			struct _ {static R wrapper(void*, Args... a) {return (*Func)(core::forward<Args>(a)...);}};
			return Callback<R(Args...)>(0, (R(*)(void*, Args...)) _::wrapper);
		}
	};
}

template<typename R, class T, typename... Args> detail::DeduceConstMemCallback<R, T, Args...> DeduceCallback(R(T::*)(Args...) const)
{
	return detail::DeduceConstMemCallback<R, T, Args...>();
}

template<typename R, class T, typename... Args> detail::DeduceMemCallback<R, T, Args...> DeduceCallback(R(T::*)(Args...))
{
	return detail::DeduceMemCallback<R, T, Args...>();
}

template<typename R, typename... Args> detail::DeduceStaticCallback<R, Args...> DeduceCallback(R(*)(Args...))
{
	return detail::DeduceStaticCallback<R, Args...>();
}

template <typename... T1> class GenericEvent
{
	typedef void(*TSignature)(T1...);
	typedef Callback<void(T1...)> TCallback;
	typedef Array<TCallback> InvocationTable;

	typedef void(*CB)(T1...);
	static void WrapperCall(void* o, T1... args) {((CB)o)(core::forward<T1>(args)...);}

protected:
	InvocationTable invocations;

public:
	GenericEvent() {}

	template<void(*TFunc)(T1...)> void Subscribe()
	{
		TCallback c = DeduceCallback(TFunc).template Bind<TFunc>();
		invocations.AddLast(c);
	}

	void Subscribe(void(*callback)(T1...))
	{
		invocations.AddLast(TCallback(callback, &WrapperCall));
	}

	template<typename T, void (T::* TFunc)(T1...)> void Subscribe(T& object)
	{
		Subscribe<T, TFunc>(&object);
	}

	template <typename T, void (T::* TFunc)(T1...)> void Subscribe(T* object)
	{
		TCallback c = DeduceCallback(TFunc).template Bind<TFunc>(object);
		invocations.AddLast(c);
	}

	template<typename T, void (T::* TFunc)(T1...) const> void Subscribe(T& object)
	{
		Subscribe<T, TFunc>(&object);
	}

	template <typename T, void (T::* TFunc)(T1...) const> void Subscribe(T* object)
	{
		TCallback c = DeduceCallback(TFunc).template Bind<TFunc>(object);
		invocations.push_back(c);
	}

	void Invoke(T1... t1)
	{
		for(auto&& inv: invocations)
			inv(t1...);
	}

	void operator()(T1... t1)
	{
		Invoke(core::forward<T1>(t1)...);
	}

	size_t InvocationCount() const {return invocations.Count();}

	void Unsubscribe(void(*callback)(T1...))
	{
		Unsubscribe(TCallback(callback, &WrapperCall));
	}
	template<void(*TFunc)(T1...)> bool Unsubscribe()
	{
		return Unsubscribe(DeduceCallback(TFunc).template Bind<TFunc>());
	}
	template<typename T, void(T::* TFunc)(T1...)> bool Unsubscribe(T& object)
	{
		return Unsubscribe<T, TFunc>(&object);
	}
	template<typename T, void(T::* TFunc)(T1...)> bool Unsubscribe(T* object)
	{
		return Unsubscribe(DeduceCallback(TFunc).template Bind<TFunc>(object));
	}
	template<typename T, void(T::* TFunc)(T1...) const> bool Unsubscribe(T& object)
	{
		return Unsubscribe<T, TFunc>(&object);
	}
	template<typename T, void(T::* TFunc)(T1...) const> bool Unsubscribe(T* object)
	{
		return Unsubscribe(DeduceCallback(TFunc).template Bind<TFunc>(object));
	}

protected:
	bool Unsubscribe(const TCallback& target)
	{
		size_t index = invocations().CountUntil(target);
		if(index==invocations.Length()) return false;
		invocations.Remove(index);
		return true;
	}
};

typedef GenericEvent<> Event;

}}

