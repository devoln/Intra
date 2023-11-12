#pragma once

#include "Intra/Core.h"
#include "Intra/Range/Concepts.h"
#include "Intra/Misc/RawMemory.h"
#include "Intra/Range/Span.h"
#include "Intra/Assert.h"
#include "Intra/Range/Operations.h"
#include "Intra/Range/Mutation/Fill.h"

namespace Intra { INTRA_BEGIN
/// Call constructors for each element of unitialized span \p dst.
template<typename T> constexpr void Initialize(Span<T> dst)
{
	if constexpr(CTriviallyConstructible<T>) FillZeros(dst);
	else for(auto& x: dst) new(Construct, &x) T;
}

/// Call destructor for each Span element.
template<typename T> inline void Destruct(Span<T> dst)
{
	if constexpr(!CTriviallyDestructible<T>)
		for(auto& x: dst) x.~T();
	else if constexpr(Config::DebugCheckLevel >= 1)
		z_D::memset(dst.Data(), 0xDE, size_t(dst.Length())*sizeof(dst.First()));
}

/// Assign via destructor + copy constructor
template<typename T> void CopyRecreate(Span<T> dst, Span<const T> src)
{
	INTRA_PRECONDITION(dst.Length() == src.Length());
	if constexpr(CTriviallyCopyable<T>) BitwiseCopy(Unsafe, dst.Data(), src.Data(), dst.Length());
	else for(auto& dstVal: dst)
	{
		dstVal.~T();
		new(Construct, &dstVal) T(src|Next);
	}
}

/// Assign backwards via destructor + copy constructor
template<typename T> void CopyRecreateBackwards(Span<T> dst, Span<const T> src)
{
	INTRA_PRECONDITION(dst.Length() == src.Length());
	if constexpr(CTriviallyCopyable<T>) BitwiseCopyBackwardsUnsafe(dst, src);
	else while(!dst.Empty())
	{
		dst.Last().~T();
		new(Construct, &dst.Last()) T(src.Last());
		src.PopLast();
	}
}

template<typename T, typename U> void CopyInit(Span<T> dst, Span<const U> src)
{
	INTRA_PRECONDITION(dst.Length() == src.Length());
	if constexpr(CTriviallyCopyable<T>)
		BitwiseCopy(Unsafe, dst.Data(), src.Data(), dst.Length());
	else for(auto& dstVal: dst)
		new(Construct, &dstVal) T(src.Next());
}

template<typename T, typename U> void CopyInitBackwards(Span<T> dst, Span<const U> src)
{
	INTRA_PRECONDITION(dst.Length() == src.Length());
	if constexpr(CTriviallyCopyable<T>)
		BitwiseCopyBackwards(Unsafe, dst.Data(), src.Data(), dst.Length());
	else while(!src.Empty())
	{
		new(Construct, &dst.Last()) T(src.Last());
		dst.PopLast();
		src.PopLast();
	}
}

/// Init with move constructor
template<typename T> void MoveInit(Span<T> dst, Span<T> src)
{
	INTRA_PRECONDITION(dst.Length() == src.Length());
	if constexpr(CTriviallyCopyable<T>)
		BitwiseCopy(Unsafe, dst.Data(), src.Data(), dst.Length());
	else for(auto& dstVal: dst)
		new(Construct, &dstVal) T(Move(src.Next()));
}

/// Init with move constructor backwards.
template<typename T> void MoveInitBackwards(Span<T> dst, Span<T> src)
{
	INTRA_PRECONDITION(dst.Length() == src.Length());
	if constexpr(CTriviallyCopyable<T>)
		BitwiseCopyBackwards(Unsafe, dst.Data(), src.Data(), dst.Length());
	else while(!dst.Empty())
	{
		new(Construct, &dst.Last()) T(INTRA_MOVE(src.Last()));
		dst.PopLast();
		src.PopLast();
	}
}

/// Init with move constructor and delete all moved \p src elements
template<typename T> void MoveInitDelete(Span<T> dst, Span<T> src)
{
	INTRA_PRECONDITION(dst.Length() == src.Length());
	if constexpr(CTriviallyRelocatable<T>)
		BitwiseCopy(Unsafe, dst.Data(), src.Data(), dst.Length());
	else for(auto& srcVal: src)
	{
		new(Construct, &dst.Next()) T(INTRA_MOVE(srcVal));
		srcVal.~T();
	}
}

/// Init backwards with move constructor and delete all moved \p src elements
template<typename T> void MoveInitDeleteBackwards(Span<T> dst, Span<T> src)
{
	INTRA_PRECONDITION(dst.Length() == src.Length());
	if constexpr(CTriviallyRelocatable<T>)
		BitwiseCopyBackwards(Unsafe, dst.Data(), src.Data(), dst.Length());
	else while(!dst.Empty())
	{
		new(Construct, &dst.Last()) T(INTRA_MOVE(src.Last()));
		src.Last().~T();
		dst.PopLast();
		src.PopLast();
	}
}

/// Assign with move assignment and delete all moved `src` elements.
template<typename T> void MoveAssignDelete(Span<T> dst, Span<T> src)
{
	INTRA_PRECONDITION(dst.Length() == src.Length());
	if constexpr(CTriviallyRelocatable<T>) BitwiseCopy(Unsafe, dst.Data(), src.Data(), dst.Length());
	else for(auto& srcVal: src)
	{
		dst|Next = INTRA_MOVE(srcVal);
		srcVal.~T();
	}
}

/// Assign backwards with move assignment and delete all moved \p src elements.
template<typename T> void MoveAssignDeleteBackwards(Span<T> dst, Span<T> src)
{
	INTRA_PRECONDITION(dst.Length() == src.Length());
	if constexpr(CTriviallyCopyable<T>)
		BitwiseCopyBackwardsUnsafe(dst, src);
	else while(!src.Empty())
	{
		dst.Last() = Move(src.Last());
		src.Last().~T();
		dst.PopLast();
		src.PopLast();
	}
}


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
