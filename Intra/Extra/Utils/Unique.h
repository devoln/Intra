#pragma once

#include "Core/Type.h"
#include "Core/Assert.h"

INTRA_BEGIN
template<typename T> struct Unique
{
	Unique() = default;

	/**
	  This constructor assumes, that ``ptrFromNew`` is null or points directly to an object allocated with new T(...).
	  It is not allowed to pass other pointer because it will be freed in destructor with delete.
	*/
	constexpr forceinline Unique(Owner<T*> ptrFromNew) noexcept: mPtr(ptrFromNew) {}

	forceinline ~Unique() {delete mPtr;}

	forceinline Unique(const Unique& rhs) = delete;
	Unique& operator=(const Unique& rhs) = delete;

	forceinline Unique(Unique&& rhs) noexcept: mPtr(rhs.mPtr) {rhs.mPtr = null;}

	template<typename... Args> INTRA_NODISCARD forceinline static Unique New(Args&&... args) noexcept
	{return new T(Forward<Args>(args)...);}

	constexpr forceinline Unique& operator=(Unique&& rhs) noexcept
	{
		Swap(mPtr, rhs.mPtr);
		return *this;
	}

	forceinline Unique& operator=(Owner<T*> ptrFromNew) noexcept
	{
		Unique temp(ptrFromNew);
		Swap(mPtr, temp.mPtr);
		return *this;
	}

	forceinline Unique& operator=(null_t) noexcept
	{
		Unique temp;
		Swap(mPtr, temp.mPtr);
		return *this;
	}

	constexpr forceinline T* Ptr() const noexcept {return mPtr;}
	constexpr forceinline T* get() const noexcept {return mPtr;}

	forceinline Owner<T*> Release() noexcept
	{
		const Owner<T*> result = mPtr;
		mPtr = null;
		return result;
	}

	INTRA_NODISCARD constexpr forceinline T& operator*() const {INTRA_DEBUG_ASSERT(mPtr != null); return *mPtr;}
	INTRA_NODISCARD constexpr forceinline T* operator->() const {INTRA_DEBUG_ASSERT(mPtr != null); return mPtr;}

	INTRA_NODISCARD constexpr forceinline bool operator==(null_t) const noexcept {return mPtr == null;}
	INTRA_NODISCARD constexpr forceinline bool operator!=(null_t) const noexcept {return !operator==(null);}

	INTRA_NODISCARD constexpr forceinline explicit operator bool() const noexcept {return mPtr != null;}

private:
	Owner<T*> mPtr = null;
};

template<typename T> forceinline Unique<T> UniqueMove(T& rhs) {return new T(Move(rhs));}

template<typename T> constexpr bool IsTriviallyRelocatable<Unique<T>> = true;
INTRA_END
