#pragma once

#include "Cpp/Features.h"
#include "Cpp/Fundamental.h"

namespace Intra { namespace Utils {

class AnyPtr
{
	void* mPtr;
public:

	forceinline AnyPtr(null_t=null): mPtr(null) {}

	template<typename T> forceinline AnyPtr(T* p)
	{
		typedef void* pvoid;
		mPtr = pvoid(p);
	}

	template<typename T> forceinline operator T*() const
	{
		typedef T* Tptr;
		return Tptr(mPtr);
	}

	forceinline bool operator==(null_t) const {return mPtr == null;}
	forceinline bool operator!=(null_t) const {return mPtr != null;}
};

}
using Utils::AnyPtr;

}
