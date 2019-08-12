#pragma once

#include "Core/Type.h"
#include "Core/Range/Concepts.h"
#include "Core/Misc/RawMemory.h"
#include "Core/Range/Span.h"
#include "Core/Assert.h"
#include "Core/Range/Operations.h"

INTRA_BEGIN
inline namespace Memory {

//! Call constructors for each element of unitialized span \p dst.
template<typename T> INTRA_CONSTEXPR2 Requires<
	!CTriviallyConstructible<T>
> Initialize(Span<T> dst)
{
	while(!dst.Empty())
	{
		new(&dst.First()) T;
		dst.PopFirst();
	}
}

//! Fill the memory of trivially constructible values of span \p dst.
template<typename T> forceinline Requires<
	CTriviallyConstructible<T>
> Initialize(Span<T> dst)
{
	C::memset(dst.Begin, 0, dst.Length()*sizeof(T));
}

//! Call destructor for each element of span.
template<typename T> Requires<
	!CTriviallyDestructible<T>
> Destruct(Span<T> dst)
{
	while(!dst.Empty())
	{
		dst.First().~T();
		dst.PopFirst();
	}
}

//! Call destructor for each element of span.
template<typename T> INTRA_CONSTEXPR2 forceinline Requires<
	CTriviallyDestructible<T>
> Destruct(Span<T> dst)
{
#ifdef INTRA_DEBUG
	if(dst.Empty()) return;
	C::memset(dst.Data(), 0xDE, dst.Length()*sizeof(dst.First()));
#endif
	(void)dst;
}

//! Call destructor for a single object.
template<typename T> forceinline Requires<
	!CTriviallyDestructible<T>
> DestructObj(T& dst) noexcept
{dst.~T();}

template<typename T> Requires<
	CTriviallyDestructible<T>
> DestructObj(T& dst) noexcept
{
#ifdef INTRA_DEBUG
	C::memset(&dst, 0xDE, sizeof(T));
#endif
	(void)dst;
}

/** Bitwise copying.

  \p dst and \p src must not overlap.
*/
template<typename T> INTRA_CONSTEXPR2 forceinline void CopyBits(Span<T> dst, CSpan<T> src) noexcept
{
	INTRA_DEBUG_ASSERT(dst.Length() >= src.Length());
	CopyBits(dst.Begin, src.Begin, src.Length());
}





template<typename T> void CopyBitsBackwards(Span<T> dst, CSpan<T> src)
{
	INTRA_DEBUG_ASSERT(dst.Length() >= src.Length());
	CopyBitsBackwards(dst.Begin, src.Begin, src.Length());
}

template<typename T> forceinline void CopyObjectBits(T& dst, const T& src)
{
	CopyBits<T>({&dst, 1}, {&src, 1});
}

//! Copy using assignment operator
template<typename OR, typename R> Requires<
	CAssignableRange<OR> &&
	CForwardRange<R> &&
	!CTriviallyCopyAssignable<TValueTypeOf<OR>>
> CopyAssign(const OR& dst, const R& src)
{
	auto dstCopy = dst;
	auto srcCopy = src;
	INTRA_DEBUG_ASSERT(Range::Count(dst) >= Range::Count(src));
	while(!srcCopy.Empty())
	{
		dstCopy.First() = srcCopy.First();
		dstCopy.PopFirst();
		srcCopy.PopFirst();
	}
}

template<typename OR, typename R> INTRA_CONSTEXPR2 forceinline Requires<
	CArrayClass<OR> &&
	CArrayClass<R> &&
	CTriviallyCopyAssignable<TArrayElement<OR>> &&
	CFiniteInputRangeOfExactly<R, TArrayElement<OR>>
> CopyAssign(const OR& dst, const R& src)
{
	INTRA_DEBUG_ASSERT(dst.Length() >= src.Length());
	CopyBits(dst.Data(), src.Data(), src.Length());
}

//! Copy backwards using assign operator
template<typename T> Requires<
	!CTriviallyCopyAssignable<T>
> CopyAssignBackwards(Span<T> dst, CSpan<T> src)
{
	INTRA_DEBUG_ASSERT(dst.Length() <= src.Length());
	while(!dst.Empty())
	{
		dst.Last() = src.Last();
		dst.PopLast();
		src.PopLast();
	}
}

template<typename T> INTRA_CONSTEXPR2 forceinline Requires<
	CTriviallyCopyAssignable<T>
> CopyAssignBackwards(Span<T> dst, CSpan<T> src) noexcept
{
	CopyBitsBackwards(dst, src);
}

//! Assign via destructor + copy constructor
template<typename T> Requires<
	!CTriviallyCopyable<T> ||
	!CTriviallyDestructible<T>
> CopyRecreate(Span<T> dst, CSpan<T> src)
{
	INTRA_DEBUG_ASSERT(dst.Length() == src.Length());
	while(!dst.Empty())
	{
		dst.First().~T();
		new(&dst.First()) T(src.First());
		src.PopFirst();
	}
}

template<typename T> INTRA_CONSTEXPR2 forceinline Requires<
	CTriviallyCopyable<T> &&
	CTriviallyDestructible<T>
> CopyRecreate(Span<T> dst, CSpan<T> src) noexcept
{
	CopyBits(dst, src);
}

//! Assign backwards via destructor + copy constructor
template<typename T> Requires<
	!CTriviallyCopyable<T> ||
	!CTriviallyDestructible<T>
> CopyRecreateBackwards(Span<T> dst, CSpan<T> src)
{
	INTRA_DEBUG_ASSERT(dst.Length() == src.Length());
	while(!dst.Empty())
	{
		dst.Last().~T();
		new(&dst.Last()) T(src.Last());
		src.PopLast();
	}
}

template<typename T> INTRA_CONSTEXPR2 forceinline Requires<
	CTriviallyCopyable<T> &&
	CTriviallyDestructible<T>
> CopyRecreateBackwards(Span<T> dst, CSpan<T> src) noexcept
{
	CopyBitsBackwards(dst, src);
}

template<typename T, typename U> Requires<
	!CTriviallyCopyable<T>
> CopyInit(Span<T> dst, CSpan<U> src)
{
	INTRA_DEBUG_ASSERT(dst.Length() == src.Length());
	while(!src.Empty())
	{
		new(&dst.First()) T(src.First());
		dst.PopFirst();
		src.PopFirst();
	}
}

template<typename T> INTRA_CONSTEXPR2 forceinline Requires<
	CTriviallyCopyable<T>
> CopyInit(Span<T> dst, CSpan<T> src) noexcept
{
	CopyBits(dst, src);
}

template<typename T, typename U> Requires<
	!CTriviallyCopyable<T>
> CopyInitBackwards(Span<T> dst, CSpan<U> src)
{
	INTRA_DEBUG_ASSERT(dst.Length()==src.Length());
	while(!src.Empty())
	{
		new(&dst.Last()) T(src.Last());
		dst.PopLast();
		src.PopLast();
	}
}

template<typename T> INTRA_CONSTEXPR2 forceinline Requires<
	CTriviallyCopyable<T>
> CopyInitBackwards(Span<T> dst, CSpan<T> src) noexcept
{
	CopyBitsBackwards(dst, src);
}

//! Init with move constructor
template<typename T> Requires<
	!CTriviallyMovable<T>
> MoveInit(Span<T> dst, Span<T> src)
{
	INTRA_DEBUG_ASSERT(dst.Length()==src.Length());
	while(!dst.Empty())
	{
		new(&dst.First()) T(Move(src.First()));
		dst.PopFirst();
		src.PopFirst();
	}
}

//! Init with move constructor.
template<typename T> INTRA_CONSTEXPR2 forceinline Requires<
	CTriviallyMovable<T>
> MoveInit(Span<T> dst, Span<T> src) noexcept
{
	CopyBits(dst, src.AsConstRange());
}

//! Init with move constructor backwards.
template<typename T> Requires<
	!CTriviallyCopyable<T>
> MoveInitBackwards(Span<T> dst, Span<T> src)
{
	INTRA_DEBUG_ASSERT(dst.Length() == src.Length());
	while(!dst.Empty())
	{
		new(&dst.Last()) T(Move(src.Last()));
		dst.PopLast();
		src.PopLast();
	}
}

//! Init with move constructor backwards.
template<typename T> INTRA_CONSTEXPR2 forceinline Requires<
	CTriviallyCopyable<T>
> MoveInitBackwards(Span<T> dst, Span<T> src) noexcept
{CopyBitsBackwards(dst, src.AsConstRange());}

//! Init with move constructor and delete all moved \p src elements
template<typename T> Requires<
	!CTriviallyRelocatable<T>
> MoveInitDelete(Span<T> dst, Span<T> src)
{
	INTRA_DEBUG_ASSERT(dst.Length() == src.Length());
	while(!dst.Empty())
	{
		new(&dst.First()) T(Move(src.First()));
		src.First().~T();
		dst.PopFirst();
		src.PopFirst();
	}
}

//! Init with move constructor and delete all moved \p src elements
template<typename T> INTRA_CONSTEXPR2 forceinline Requires<
	CTriviallyRelocatable<T>
> MoveInitDelete(Span<T> dst, Span<T> src) noexcept
{
	CopyBits(dst, src.AsConstRange());
}

//! Init backwards with move constructor and delete all moved \p src elements
template<typename T> Requires<
	!CTriviallyRelocatable<T>
> MoveInitDeleteBackwards(Span<T> dst, Span<T> src)
{
	INTRA_DEBUG_ASSERT(dst.Length() == src.Length());
	while(!dst.Empty())
	{
		new(&dst.Last()) T(Move(src.Last()));
		src.Last().~T();
		dst.PopLast();
		src.PopLast();
	}
}

//! Init backwards with move constructor and delete all moved \p src elements.
template<typename T> INTRA_CONSTEXPR2 forceinline Requires<
	CTriviallyRelocatable<T>
> MoveInitDeleteBackwards(Span<T> dst, Span<T> src) noexcept
{
	CopyBitsBackwards(dst, src.AsConstRange());
}

//! Assign with move assignment and delete all moved \p src elements.
template<typename T> Requires<
	!CTriviallyMovable<T>
> MoveAssignDelete(Span<T> dst, Span<T> src)
{
	INTRA_DEBUG_ASSERT(dst.Length() == src.Length());
	while(!dst.Empty())
	{
		dst.First() = Move(src.First());
		src.First().~T();
		dst.PopFirst();
		src.PopFirst();
	}
}

template<typename T> INTRA_CONSTEXPR2 forceinline Requires<
	CTriviallyMovable<T>
> MoveAssignDelete(Span<T> dst, Span<T> src) noexcept
{
	CopyBits(dst, src);
}

//! Assign backwards with move assignment and delete all moved \p src elements.
template<typename T> Requires<
	!CTriviallyMovable<T>
> MoveAssignDeleteBackwards(Span<T> dst, Span<T> src)
{
	INTRA_DEBUG_ASSERT(dst.Length() >= src.Length());
	while(!src.Empty())
	{
		dst.Last() = Move(src.Last());
		src.Last().~T();
		dst.PopLast();
		src.PopLast();
	}
}

template<typename T> INTRA_CONSTEXPR2 forceinline Requires<
	CTriviallyMovable<T>
> MoveAssignDeleteBackwards(Span<T> dst, Span<T> src)
{
	CopyBitsBackwards(dst, src);
}




template<typename T, typename Allocator> Span<T> AllocateRangeUninitialized(
	Allocator& allocator, size_t& count, const SourceInfo& sourceInfo = INTRA_DEFAULT_SOURCE_INFO)
{
	(void)allocator; //Чтобы устранить ложное предупреждение MSVC
	size_t size = count*sizeof(T);
	T* const result = allocator.Allocate(size, sourceInfo);
	count = size/sizeof(T);
	return Span<T>(result, count);
}

template<typename T, typename Allocator> Span<T> AllocateRange(
	Allocator& allocator, size_t& count, const SourceInfo& sourceInfo = INTRA_DEFAULT_SOURCE_INFO)
{
	auto result = AllocateRangeUninitialized(allocator, count, sourceInfo);
	Memory::Initialize(result);
	return result;
}

template<typename T, typename Allocator> void FreeRangeUninitialized(Allocator& allocator, Span<T> range)
{
	(void)allocator;
	if(range == null) return;
	allocator.Free(range.Begin, range.Length()*sizeof(T));
}

template<typename T, typename Allocator> void FreeRange(Allocator& allocator, Span<T> range)
{
	Memory::Destruct(range);
	FreeRangeUninitialized(allocator, range);
}

}
INTRA_END
