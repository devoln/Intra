#pragma once

#include "Core/Type.h"

INTRA_BEGIN
INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED

template<typename A> struct AStatic
{
	size_t GetAlignment() const {return Get().GetAlignment();}

	static A& Get()
	{
		static A allocator;
		return allocator;
	}

	static AnyPtr Allocate(size_t& bytes, SourceInfo sourceInfo = INTRA_DEFAULT_SOURCE_INFO)
	{return Get().Allocate(bytes, sourceInfo);}

	static void Free(void* ptr, size_t size)
	{Get().Free(ptr, size);}

	template<typename U=A> static forceinline Requires<
		CHasGetAllocationSize<U>,
	size_t> GetAllocationSize(void* ptr)
	{return Get().GetAllocationSize(ptr);}
};
INTRA_END
