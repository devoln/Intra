#pragma once

#include "Core/Assert.h"
#include "Core/Type.h"

INTRA_BEGIN
INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED
template<typename A> struct ACounted: A
{
private:
	size_t mCounter = 0;
public:
	ACounted() = default;
	ACounted(A&& allocator): A(Move(allocator)) {}

	AnyPtr Allocate(size_t& bytes, SourceInfo sourceInfo)
	{
		auto result = A::Allocate(bytes, sourceInfo);
		if(result!=null) mCounter++;
		return result;
	}

	void Free(void* ptr, size_t size)
	{
		if(ptr==null) return;
		A::Free(ptr, size);
		mCounter--;
	}

	size_t AllocationCount() const {return mCounter;}
};
INTRA_END
