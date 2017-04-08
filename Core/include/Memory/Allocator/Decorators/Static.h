#pragma once

#include "Platform/FundamentalTypes.h"
#include "Meta/Type.h"
#include "Memory/Allocator/Concepts.h"
#include "Platform/CppWarnings.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED

namespace Intra { namespace Memory {

template<typename A> struct AStatic
{
	size_t GetAlignment() const {return Get().GetAlignment();}

	static A& Get()
	{
		static A allocator;
		return allocator;
	}

	static AnyPtr Allocate(size_t& bytes, SourceInfo sourceInfo)
	{return Get().Allocate(bytes, sourceInfo);}

	static void Free(void* ptr, size_t size)
	{Get().Free(ptr, size);}

	template<typename U=A> static forceinline Meta::EnableIf<
		HasGetAllocationSize<U>::_,
	size_t> GetAllocationSize(void* ptr)
	{return Get().GetAllocationSize(ptr);}
};

}}

INTRA_WARNING_POP
