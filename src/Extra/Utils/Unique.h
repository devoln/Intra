#pragma once

#include "Intra/Type.h"
#include "Intra/Assert.h"

INTRA_BEGIN
template<typename T> struct Unique
{
	Unique() = default;

	/**
	  This constructor assumes, that ``ptrFromNew`` is null or points directly to an object allocated with new T(...).
	  It is not allowed to pass other pointer because it will be freed in destructor with delete.
	*/
	constexpr Unique(Owner<T*> ptrFromNew) noexcept: mPtr(ptrFromNew) {}

	INTRA_FORCEINLINE ~Unique() {delete mPtr;}

	INTRA_FORCEINLINE Unique(const Unique& rhs) = delete;
	Unique& operator=(const Unique& rhs) = delete;

	INTRA_FORCEINLINE Unique(Unique&& rhs) noexcept: mPtr(rhs.mPtr) {rhs.mPtr = null;}

	template<typename... Args> [[nodiscard]] INTRA_FORCEINLINE static Unique New(Args&&... args) noexcept
	{return new T(Forward<Args>(args)...);}

	constexpr Unique& operator=(Unique&& rhs) noexcept
	{
		Swap(mPtr, rhs.mPtr);
		return *this;
	}

	INTRA_FORCEINLINE Unique& operator=(Owner<T*> ptrFromNew) noexcept
	{
		Unique temp(ptrFromNew);
		Swap(mPtr, temp.mPtr);
		return *this;
	}

	INTRA_FORCEINLINE Unique& operator=(decltype(null)) noexcept
	{
		Unique temp;
		Swap(mPtr, temp.mPtr);
		return *this;
	}

	constexpr T* Ptr() const noexcept {return mPtr;}
	constexpr T* get() const noexcept {return mPtr;}

	INTRA_FORCEINLINE Owner<T*> Release() noexcept
	{
		const Owner<T*> result = mPtr;
		mPtr = null;
		return result;
	}

	[[nodiscard]] constexpr T& operator*() const {INTRA_DEBUG_ASSERT(mPtr != null); return *mPtr;}
	[[nodiscard]] constexpr T* operator->() const {INTRA_DEBUG_ASSERT(mPtr != null); return mPtr;}

	[[nodiscard]] constexpr bool operator==(decltype(null)) const noexcept {return mPtr == null;}
	[[nodiscard]] constexpr bool operator!=(decltype(null)) const noexcept {return !operator==(null);}

	[[nodiscard]] constexpr explicit operator bool() const noexcept {return mPtr != null;}

private:
	Owner<T*> mPtr = null;
};

template<typename T> INTRA_FORCEINLINE Unique<T> UniqueMove(T& rhs) {return new T(Move(rhs));}

template<typename T> constexpr bool IsTriviallyRelocatable<Unique<T>> = true;
INTRA_END
