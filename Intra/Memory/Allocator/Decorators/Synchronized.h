#pragma once

#include "Cpp/Fundamental.h"
#include "Utils/Debug.h"
#include "Meta/Type.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED

namespace Intra { namespace Memory {

template<typename A, typename Sync> struct ASynchronized: A
{
	ASynchronized() = default;
	ASynchronized(A&& allocator): A(Cpp::Move(allocator)) {}

	AnyPtr Allocate(size_t& bytes, SourceInfo sourceInfo)
	{
		mSync.Lock();
		auto result = A::Allocate(bytes, sourceInfo);
		mSync.Unlock();
		return result;
	}

	void Free(void* ptr, size_t size)
	{
		if(ptr==null) return;
		mSync.Lock();
		A::Free(ptr, size);
		mSync.Unlock();
	}

private:
	Sync mSync;
};

}}

INTRA_WARNING_POP
