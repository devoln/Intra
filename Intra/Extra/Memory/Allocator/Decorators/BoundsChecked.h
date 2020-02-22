#pragma once

#include "Core/Assert.h"
#include "Core/Type.h"
#include "Memory/Align.h"
#include "Memory/Allocator/Concepts.h"

INTRA_BEGIN
INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED
template<typename A> struct ABoundsChecked: A
{
private:
	enum: uint {BoundValue = 0xbcbcbcbc};
public:
	ABoundsChecked() = default;

	size_t GetAlignment() const {return sizeof(uint);}

	AnyPtr Allocate(size_t& bytes, SourceInfo sourceInfo = INTRA_DEFAULT_SOURCE_INFO)
	{
		bytes = Aligned(bytes, 4);
		size_t totalBytes = bytes + 2*sizeof(uint);
		byte* plainMemory = A::Allocate(totalBytes, sourceInfo);
		if(plainMemory!=null)
		{
			bytes = totalBytes - 2*sizeof(uint);
			*reinterpret_cast<uint*>(plainMemory) = BoundValue;
			*reinterpret_cast<uint*>(plainMemory+sizeof(uint)+bytes) = BoundValue;
			return plainMemory+sizeof(uint);
		}
		bytes = 0;
		return null;
	}

	void Free(void* ptr, size_t size)
	{
		if(ptr == null) return;
		size = Aligned(size, 4);
		byte* const plainMemory = reinterpret_cast<byte*>(ptr)-sizeof(uint);

		const uint leftBoundValue = *reinterpret_cast<uint*>(plainMemory);
		if(leftBoundValue != BoundValue)
			INTRA_FATAL_ERROR("Allocator left bound check failed!");

		const uint rightBoundValue = *reinterpret_cast<uint*>(plainMemory+size+sizeof(uint));
		if(rightBoundValue != BoundValue)
			INTRA_FATAL_ERROR("Allocator right bound check failed!");

		A::Free(plainMemory, size+2*sizeof(uint));
	}

	template<typename U=A> Requires<
		CHasGetAllocationSize<U>,
	size_t> GetAllocationSize(void* ptr) const
	{
		byte* bptr = reinterpret_cast<byte*>(ptr);
		return A::GetAllocationSize(bptr-sizeof(uint)) - sizeof(uint)*2;
	}
};
INTRA_END
