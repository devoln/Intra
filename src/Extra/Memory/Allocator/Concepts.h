#pragma once

#include "Intra/Type.h"

INTRA_BEGIN
INTRA_DEFINE_CONCEPT_REQUIRES(CHasGetAllocationSize,\
	static_cast<size_t>(Val<T>().GetAllocationSize(static_cast<void*>(null))));

class AnyPtr
{
	void* mPtr = null;
public:
	AnyPtr() = default;
	AnyPtr(decltype(null)=null) {}

	template<typename T> AnyPtr(T* p)
	{
		typedef void* pvoid;
		mPtr = pvoid(p);
	}

	template<typename T> operator T*() const
	{
		typedef T* Tptr;
		return Tptr(mPtr);
	}

	bool operator==(decltype(null)) const {return mPtr == null;}
	bool operator!=(decltype(null)) const {return mPtr != null;}
};

#if INTRA_CONSTEXPR_TEST
static_assert(CTriviallySerializable<AnyPtr>);
#endif
INTRA_END
