#pragma once

#include "Intra/Type.h"

INTRA_BEGIN
template<typename T> concept CHasGetAllocationSize = requires(T x) {static_cast<size_t>(x.GetAllocationSize(static_cast<void*>(null)));};

class AnyPtr
{
	void* mPtr = null;
public:
	AnyPtr() = default;
	constexpr AnyPtr(decltype(null)) {}

	template<typename T> constexpr AnyPtr(T* p): mPtr(p) {}

	template<typename T> operator T*() const {return static_cast<T*>(mPtr);}

	constexpr bool operator==(decltype(null)) const {return mPtr == null;}
	constexpr bool operator!=(decltype(null)) const {return mPtr != null;}
};

#if INTRA_CONSTEXPR_TEST
static_assert(CTriviallySerializable<AnyPtr>);
#endif
INTRA_END
