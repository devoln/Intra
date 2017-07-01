#pragma once

#include "Cpp/Intrinsics.h"
#include "Cpp/Warnings.h"
#include "Cpp/Fundamental.h"
#include "Cpp/Features.h"

#include "Meta/Type.h"

#include "Utils/Debug.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Funal {

//! Невладеющий функтор.
/*!
Может принимать:
1) Указатель на функцию.
2а) Ссылку на какой-либо функтор.
2б) Ссылку на какой-либо объект и его метод.
В случае 2 необходимо следить за временем жизни ссылаемого объекта.
Вызов после удаления объекта приведёт к неопределённому поведению.
*/
template<typename TFuncSignature> class FuncRef;
template<typename R, typename... Args> class FuncRef<R(Args...)>
{
public:
	typedef R(*TFunc)(void*, Args...);
	typedef R(*FreeFunc)(Args...);

	constexpr forceinline FuncRef(null_t=null) noexcept: mObjRef(null), mFunc(null) {}
	constexpr forceinline FuncRef(FreeFunc f) noexcept: mObjRef(null), mFreeFunc(f) {}
	constexpr forceinline FuncRef(void* o, TFunc f) noexcept: mObjRef(o), mFunc(f) {}

	template<typename T, typename = Meta::EnableIf<
		!Meta::TypeEquals<T, FuncRef>::_
	>> forceinline FuncRef(T* o) noexcept:
		mObjRef(o), mFunc([](void* o, Args... args) {
		return (*static_cast<T*>(o))(Cpp::Forward<Args>(args)...);
	}) {}

	template<typename T, typename = Meta::EnableIf<
		!Meta::TypeEquals<T, FuncRef>::_
	>> forceinline FuncRef(T* o, R(T::*method)(Args...)) noexcept:
		mObjRef(o), mFunc([](void* o, Args... args) {
		return static_cast<T*>(o)->*method(Cpp::Forward<Args>(args)...);
	}) {}

	

	forceinline R operator()(Args... a) const
	{
		return mObjRef == null?
			mFunc(mObjRef, Cpp::Forward<Args>(a)...):
			mFreeFunc(Cpp::Forward<Args>(a)...);
	}

	constexpr forceinline bool operator==(null_t) const noexcept {return mFunc == null;}
	constexpr forceinline bool operator!=(null_t) const noexcept {return !operator==(null);}
	constexpr forceinline bool operator!() const noexcept {return operator==(null);}

	constexpr forceinline bool operator==(const FuncRef<R(Args...)>& rhs) const noexcept {return mObjRef == rhs.mObjRef && mFunc == rhs.mFunc;}
	constexpr forceinline bool operator!=(const FuncRef<R(Args...)>& rhs) const noexcept {return !operator==(rhs);}

	explicit operator bool() {return mFreeFunc != null;}

private:
	void* mObjRef;
	union
	{
		TFunc mFunc;
		FreeFunc mFreeFunc;
	};
};

}
using Funal::FuncRef;

}

INTRA_WARNING_POP
