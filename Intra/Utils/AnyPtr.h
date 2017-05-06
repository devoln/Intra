#pragma once

#include "Cpp/Features.h"
#include "Cpp/Fundamental.h"

namespace Intra {

struct AnyPtr
{
	void* ptr;

	forceinline AnyPtr(void* p): ptr(p) {}
	explicit forceinline AnyPtr(const void* p): ptr(const_cast<void*>(p)) {}
	template<typename T> forceinline operator T*() const {return reinterpret_cast<T*>(ptr);}
	forceinline bool operator==(null_t) const {return ptr==null;}
	forceinline bool operator!=(null_t) const {return ptr!=null;}
};

}
