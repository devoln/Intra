#pragma once

#include "Core/Core.h"
#include "Core/Type.h"
#include "Memory/Allocator/Core.h"


INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED

INTRA_BEGIN
namespace Memory {

template<typename A> struct AStatic
{
	size_t GetAlignment() const {return Get().GetAlignment();}

	static A& Get()
	{
		static A allocator;
		return allocator;
	}

	static AnyPtr Allocate(size_t& bytes, const Utils::SourceInfo& sourceInfo = INTRA_DEFAULT_SOURCE_INFO)
	{return Get().Allocate(bytes, sourceInfo);}

	static void Free(void* ptr, size_t size)
	{Get().Free(ptr, size);}

	template<typename U=A> static forceinline Requires<
		HasGetAllocationSize<U>::_,
	size_t> GetAllocationSize(void* ptr)
	{return Get().GetAllocationSize(ptr);}
};

}}

INTRA_WARNING_POP
