#pragma once

#include "Intra/Assert.h"
#include "Intra/Type.h"

INTRA_BEGIN
INTRA_IGNORE_WARNING_COPY_IMPLICITLY_DELETED
template<typename A, typename Sync> struct ASynchronized: A
{
	ASynchronized() = default;
	ASynchronized(A&& allocator): A(Move(allocator)) {}

	AnyPtr Allocate(size_t& bytes, SourceInfo sourceInfo = SourceInfo())
	{
		mSync.Lock();
		auto result = A::Allocate(bytes, sourceInfo);
		mSync.Unlock();
		return result;
	}

	void Free(void* ptr, size_t size)
	{
		if(ptr == null) return;
		mSync.Lock();
		A::Free(ptr, size);
		mSync.Unlock();
	}

private:
	Sync mSync;
};
INTRA_END
