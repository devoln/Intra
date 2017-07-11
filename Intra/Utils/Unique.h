#pragma once

#include "Cpp/Fundamental.h"
#include "Cpp/Features.h"
#include "Cpp/Warnings.h"

#include "Utils/Debug.h"

namespace Intra { namespace Utils {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename T> struct Unique
{
	constexpr forceinline Unique(null_t=null) noexcept: mPtr(null) {}

	//! Этот конструктор предполагает, что объект выделен как new T(...).
	//! Передавать другие указатели запрещено.
	//! Объект по переданному указателю переходит во владение
	//! нового экземпляра Unique и освобождается в деструкторе, используя delete.
	constexpr forceinline Unique(T* ptrFromNew) noexcept: mPtr(ptrFromNew) {}

	forceinline ~Unique() noexcept {delete mPtr;}

	forceinline Unique(const Unique& rhs) = delete;
	Unique& operator=(const Unique& rhs) = delete;

	forceinline Unique(Unique&& rhs) noexcept:
		mPtr(rhs.mPtr) {rhs.mPtr = null;}

	template<typename... Args> forceinline static Unique New(Args&&... args) noexcept
	{return new T(Cpp::Forward<Args>(args)...);}

	Unique& operator=(Unique&& rhs) noexcept
	{
		Cpp::Swap(mPtr, rhs.mPtr);
		return *this;
	}

	Unique& operator=(T* ptrFromNew) noexcept
	{
		Unique temp(ptrFromNew);
		Cpp::Swap(mPtr, temp.mPtr);
		return *this;
	}

	forceinline Unique& operator=(null_t) noexcept
	{
		Unique temp;
		Cpp::Swap(mPtr, temp.mPtr);
		return *this;
	}

	constexpr forceinline T* Ptr() const noexcept {return mPtr;}
	constexpr forceinline T* get() const noexcept {return mPtr;}

	forceinline T* Release() noexcept
	{
		T* const result = mPtr;
		mPtr = null;
		return result;
	}

	forceinline T& operator*() const {INTRA_DEBUG_ASSERT(mPtr != null); return *mPtr;}
	forceinline T* operator->() const {INTRA_DEBUG_ASSERT(mPtr != null); return mPtr;}

	constexpr forceinline bool operator==(null_t) const noexcept {return mPtr == null;}
	constexpr forceinline bool operator!=(null_t) const noexcept {return !operator==(null);}

	constexpr forceinline explicit operator bool() const noexcept {return mPtr != null;}

private:
	T* mPtr;
};

INTRA_WARNING_POP

template<typename T> forceinline Unique<T> UniqueMove(T& rhs) {return new T(Cpp::Move(rhs));}

}
using Utils::Unique;
using Utils::UniqueMove;

}
