#pragma once

#include "Platform/FundamentalTypes.h"
#include "Platform/Debug.h"
#include "Meta/Type.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED

namespace Intra { namespace Memory {

template<typename A, typename Sync> struct ASynchronized: A
{
	ASynchronized() = default;
	ASynchronized(const ASynchronized&) = default;
	ASynchronized(A&& allocator): A(Meta::Move(allocator)) {}
	ASynchronized(ASynchronized&& rhs): A(Meta::Move(rhs)), mSync(Meta::Move(rhs.mSync)) {}

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

	ASynchronized& operator=(ASynchronized&& rhs)
	{
		A::operator=(Meta::Move(rhs));
		mSync = Meta::Move(rhs.mSync);
		return *this;
	}

private:
	Sync mSync;
};

}}

INTRA_WARNING_POP
