#pragma once

#include "Cpp/Intrinsics.h"
#include "Cpp/Warnings.h"
#include "Cpp/Fundamental.h"
#include "Cpp/Features.h"

#include "Meta/Type.h"

#include "Utils/Debug.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Utils {

template<typename TFuncSignature> class Callback;
template<typename R, typename... Args> class Callback<R(Args...)>
{
public:
	typedef R(*TFunc)(void*, Args...);
	typedef R(*FreeFunc)(Args...);

	constexpr forceinline Callback(null_t=null) noexcept: mObj(null), mFunc(null) {}
	constexpr forceinline Callback(FreeFunc f) noexcept: mObj(null), mFreeFunc(f) {}
	constexpr forceinline Callback(void* o, TFunc f) noexcept: mObj(o), mFunc(f) {}

	template<typename T, typename = Meta::EnableIf<
		!Meta::TypeEquals<T, Callback>::_
	>> forceinline Callback(T& o) noexcept:
		mObj(&o), mFunc([](void* o, Args... args) {
		return (*static_cast<T*>(o))(Cpp::Forward<Args>(args)...);
	}) {}

	forceinline R operator()(Args... a) const
	{
		return mObj == null?
			mFunc(mObj, Cpp::Forward<Args>(a)...):
			mFreeFunc(Cpp::Forward<Args>(a)...);
	}

	constexpr forceinline bool operator==(null_t) const noexcept {return mFunc == null;}
	constexpr forceinline bool operator!=(null_t) const noexcept {return !operator==(null);}
	constexpr forceinline bool operator!() const noexcept {return operator==(null);}

	constexpr forceinline bool operator==(const Callback<R(Args...)>& rhs) const noexcept {return mObj == rhs.mObj && mFunc == rhs.mFunc;}
	constexpr forceinline bool operator!=(const Callback<R(Args...)>& rhs) const noexcept {return !operator==(rhs);}

private:
	void* mObj;
	union
	{
		TFunc mFunc;
		FreeFunc mFreeFunc;
	};
};

}}

INTRA_WARNING_POP
