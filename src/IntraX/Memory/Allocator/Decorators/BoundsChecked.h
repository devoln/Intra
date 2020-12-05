#pragma once

#include "Intra/Assert.h"
#include "Intra/Type.h"
#include "IntraX/Memory/Align.h"
#include "IntraX/Memory/Allocator/Concepts.h"

INTRA_BEGIN
INTRA_IGNORE_WARN_COPY_IMPLICITLY_DELETED
template<typename A> struct ABoundsChecked: A
{
private:
	enum: unsigned {BoundValue = 0xbcbcbcbc};
public:
	ABoundsChecked() = default;

	size_t GetAlignment() const {return sizeof(unsigned);}

	AnyPtr Allocate(size_t& bytes, SourceInfo sourceInfo = SourceInfo())
	{
		bytes = Aligned(bytes, 4);
		size_t totalBytes = bytes + 2*sizeof(unsigned);
		byte* plainMemory = A::Allocate(totalBytes, sourceInfo);
		if(plainMemory!=null)
		{
			bytes = totalBytes - 2*sizeof(unsigned);
			*reinterpret_cast<unsigned*>(plainMemory) = BoundValue;
			*reinterpret_cast<unsigned*>(plainMemory+sizeof(unsigned)+bytes) = BoundValue;
			return plainMemory+sizeof(unsigned);
		}
		bytes = 0;
		return null;
	}

	void Free(void* ptr, size_t size)
	{
		if(ptr == null) return;
		size = Aligned(size, 4);
		byte* const plainMemory = reinterpret_cast<byte*>(ptr)-sizeof(unsigned);

		const unsigned leftBoundValue = *reinterpret_cast<unsigned*>(plainMemory);
		if(leftBoundValue != BoundValue)
			INTRA_FATAL_ERROR("Allocator left bound check failed!");

		const unsigned rightBoundValue = *reinterpret_cast<unsigned*>(plainMemory+size+sizeof(unsigned));
		if(rightBoundValue != BoundValue)
			INTRA_FATAL_ERROR("Allocator right bound check failed!");

		A::Free(plainMemory, size+2*sizeof(unsigned));
	}

	size_t GetAllocationSize(void* ptr) const requires CHasGetAllocationSize<A>
	{
		byte* bptr = reinterpret_cast<byte*>(ptr);
		return A::GetAllocationSize(bptr-sizeof(unsigned)) - sizeof(unsigned)*2;
	}
};
INTRA_END
