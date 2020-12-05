#pragma once

#include "Intra/Assert.h"
#include "Intra/Type.h"

INTRA_BEGIN
INTRA_IGNORE_WARN_DEFAULT_CTOR_IMPLICITLY_DELETED
INTRA_IGNORE_WARN_COPY_IMPLICITLY_DELETED
template<class A> struct ASized: A
{
	size_t GetAlignment() const {return sizeof(size_t);}

	ASized() = default;
	ASized(A&& allocator): A(Move(allocator)) {}

	AnyPtr Allocate(size_t& bytes, SourceInfo sourceInfo = SourceInfo())
	{
		INTRA_PRECONDITION(bytes != 0);
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
		INTRA_PRECONDITION(GetAllocationSize(ptr) == size);
		size_t* originalPtr = reinterpret_cast<size_t*>(ptr)-1;
		A::Free(originalPtr, size+sizeof(size_t));
	}

	INTRA_FORCEINLINE size_t GetAllocationSize(void* ptr) const
	{return *(reinterpret_cast<size_t*>(ptr)-1);}
};
INTRA_END
