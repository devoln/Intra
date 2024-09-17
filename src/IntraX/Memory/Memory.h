#pragma once

#include "Intra/Core.h"
#include "Intra/Range/Concepts.h"
#include "Intra/Misc/RawMemory.h"
#include "Intra/Range/Span.h"
#include "Intra/Assert.h"
#include "Intra/Range/Operations.h"
#include "Intra/Range/Mutation/Fill.h"

namespace Intra { INTRA_BEGIN



template<typename T, typename Allocator> Span<T> AllocateRangeUninitialized(
	Allocator& allocator, size_t& count, SourceInfo sourceInfo = SourceInfo::Current())
{
	(void)allocator; //Чтобы устранить ложное предупреждение MSVC
	size_t size = count*sizeof(T);
	T* const result = allocator.Allocate(size, sourceInfo);
	count = size / sizeof(T);
	return Span<T>::FromPointerAndLength(result, count);
}

template<typename T, typename Allocator> Span<T> AllocateRange(
	Allocator& allocator, size_t& count, const SourceInfo& sourceInfo = SourceInfo::Current())
{
	auto result = AllocateRangeUninitialized(allocator, count, sourceInfo);
	Initialize(result);
	return result;
}

template<typename T, typename Allocator> void FreeRangeUninitialized(Allocator& allocator, Span<T> range)
{
	(void)allocator;
	if(range == nullptr) return;
	allocator.Free(range.Begin, size_t(range.Length())*sizeof(T));
}

template<typename T, typename Allocator> void FreeRange(Allocator& allocator, Span<T> range)
{
	Destruct(range);
	FreeRangeUninitialized(allocator, range);
}
} INTRA_END
