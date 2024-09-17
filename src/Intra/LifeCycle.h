#pragma once

#include <Intra/Binary.h>

namespace Intra { INTRA_BEGIN

namespace LifeCycle {
constexpr auto ConstructOne = []<typename T, typename... Args>(T* dst, Args&&... args) INTRA_FORCEINLINE_LAMBDA requires CConstructible<T, Args...>
{
	if constexpr(CTriviallyDestructible<T> && CMoveAssignable<T>)
		if(IsConstantEvaluated())
		{
			auto p = __builtin_launder(dst);
			*p = T(INTRA_FWD(args)...);
			return p;
		}
#if defined(_STL_CONSTRUCT_H) || defined(_XUTILITY_) // #include <memory> before including any Intra header to make ConstructOneInplace constexpr in general case
	return std::construct_at(dst, INTRA_FWD(args)...);
#else
#if defined(_MSC_VER) && _MSC_VER >= 1933 && !defined(__clang__)
	[[msvc::constexpr]] // MSVC 2022.3+ allows to implement constexpr ConstructOneInplace without std
#endif
	return new(Construct, dst) T(INTRA_FWD(args)...);
#endif
};

INTRA_DEFINE_FUNCTOR(DestructOne)<CDestructible T>(T* dst) INTRA_FORCEINLINE_LAMBDA
{
	if constexpr(!CTriviallyDestructible<T>) dst->~T();
};

template<CConstructible T> constexpr void ValueInitialize(TUnsafe, T* dst, size_t count)
{
	if constexpr(CTriviallyConstructible<T>) MemoryZero(Unsafe, dst, count);
	else while(count--) ConstructOne(*dst++);
}
template<CConstructible T> constexpr void ValueInitialize(Span<T> dst) {ValueInitialize(Unsafe, dst.Begin, size_t(dst.Length()));}

template<CDestructible T> constexpr void Destruct(TUnsafe, T* dst, size_t count)
{
	if constexpr(!CTriviallyDestructible<T>) while(count--) *dst++->~T();
}
template<CDestructible T> constexpr void Destruct(Span<T> dst) {Destruct(Unsafe, dst.Begin, size_t(dst.Length()));}

template<typename U, CConstructible<const U&> T> constexpr void CopyConstruct(TUnsafe, T* __restrict dst, const U* __restrict src, size_t count)
{
	if constexpr(CSame<T, U> && CTriviallyCopyable<T>) MemoryCopy(Unsafe, dst, src, count);
	else while(count--) ConstructOne(dst++, *src++);
}
template<typename U, CCopyConstructible T> constexpr void CopyConstruct(Span<T> dst, Span<const U> src)
{
	INTRA_PRECONDITION(dst.Length() == src.Length());
	CopyConstruct(Unsafe, dst.Begin, src.Begin, size_t(dst.Length()));
}

template<typename T> constexpr void MoveInit(TUnsafe, T* __restrict dst, T* __restrict src, size_t count)
{
	if constexpr(CTriviallyCopyable<T>) MemoryCopy(Unsafe, dst, src, count);
	else while(count--) ConstructOne(dst++, INTRA_MOVE(*src++));
}
template<typename T> constexpr void MoveInit(Span<T> dst, Span<T> src)
{
	INTRA_PRECONDITION(dst.Length() == src.Length());
	MoveInit(Unsafe, dst.Begin, src.Begin, size_t(dst.Length()));
}

template<typename T> constexpr void MoveConstructDestruct(TUnsafe, T* __restrict dst, T* __restrict src, size_t count)
{
	if constexpr(CTriviallyRelocatable<T>) MemoryCopy(Unsafe, dst, src, count);
	else while(count--) ConstructOne(dst++, INTRA_MOVE(*src)), src++->~T();
}
template<typename T> constexpr void MoveConstructDestruct(Span<T> dst, Span<T> src)
{
	INTRA_PRECONDITION(dst.Length() == src.Length());
	MoveInitDestruct(Unsafe, dst.Begin, src.Begin, size_t(dst.Length()));
}

template<typename T> constexpr void MoveConstructDestructForward(TUnsafe, T* dst, T* src, size_t count)
{
	if constexpr(CTriviallyRelocatable<T>) MemoryCopyForward(Unsafe, dst, src, count);
	else while(count--) ConstructOne(dst++, INTRA_MOVE(*src)), src++->~T();
}
template<typename T> constexpr void MoveConstructDestructForward(Span<T> dst, Span<T> src)
{
	INTRA_PRECONDITION(dst.Length() == src.Length());
	MoveInitDestructForward(Unsafe, dst.Begin, src.Begin, size_t(dst.Length()));
}

template<typename T> constexpr void MoveConstructDestructBackward(TUnsafe, T* dst, T* src, size_t count)
{
	if constexpr(CTriviallyRelocatable<T>) MemoryCopyBackward(Unsafe, dst, src, count);
	else while(count--) ConstructOne(dst + count, INTRA_MOVE(src[count])), src[count].~T();
}
template<typename T> constexpr void MoveConstructDestructBackward(Span<T> dst, Span<T> src)
{
	INTRA_PRECONDITION(dst.Length() == src.Length());
	MoveConstructDestructBackward(Unsafe, dst.Begin, src.Begin, size_t(dst.Length()));
}
}
} INTRA_END
