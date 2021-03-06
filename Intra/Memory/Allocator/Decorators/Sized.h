#pragma once

#include "Cpp/Fundamental.h"
#include "Cpp/Warnings.h"

#include "Utils/AnyPtr.h"
#include "Utils/Debug.h"

#include "Meta/Type.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_DEFAULT_CONSTRUCTOR_IMPLICITLY_DELETED
INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED

namespace Intra { namespace Memory {

template<class A> struct ASized: A
{
	size_t GetAlignment() const {return sizeof(size_t);}

	ASized() = default;
	ASized(A&& allocator): A(Cpp::Move(allocator)) {}

	AnyPtr Allocate(size_t& bytes, Utils::SourceInfo sourceInfo)
	{
		INTRA_DEBUG_ASSERT(bytes!=0);
		size_t totalBytes = bytes+sizeof(size_t);
		size_t* data = A::Allocate(totalBytes, sourceInfo);
		if(data!=null)
		{
			bytes = totalBytes-sizeof(size_t);
			*data++ = bytes;
		}
		return data;
	}
 
	/*AnyPtr Reallocate(void* ptr, size_t newBytes)
	{
		size_t* newData = A::Reallocate(ptr!=null? (size_t*)ptr-1: null, newBytes+sizeof(size_t));
		if(newData!=null) *newData = newBytes;
		return newData+1;
	}*/

	void Free(void* ptr, size_t size)
	{
		INTRA_DEBUG_ASSERT(GetAllocationSize(ptr)==size);
		size_t* originalPtr = reinterpret_cast<size_t*>(ptr)-1;
		A::Free(originalPtr, size+sizeof(size_t));
	}

	forceinline size_t GetAllocationSize(void* ptr) const
	{return *(reinterpret_cast<size_t*>(ptr)-1);}
};

}}

INTRA_WARNING_POP

