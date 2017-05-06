#pragma once

#include "Cpp/Fundamental.h"
#include "Utils/Debug.h"
#include "Meta/Type.h"

namespace Intra { namespace Memory {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED

template<typename A> struct ACounted: A
{
private:
	size_t mCounter=0;
public:
	ACounted() = default;
	ACounted(A&& allocator): A(Cpp::Move(allocator)) {}

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

INTRA_WARNING_POP

}}
