#pragma once

#include "Intra/Type.h"
#include "Intra/Range/Decorators.h"
#include "Intra/Range/Buffered.h"

INTRA_BEGIN
template<typename T> struct Span;

// All other functions can be implemented with it with some loss of efficiency that becomes negligible for complex ranges.
// Perf tests for simple Fibonacci range:
// Iteration with First(), Empty() and PopFirst(): ~1.3x slower in total than direct implementation for one element and 9x slower than inline
// Iteration with First() and Next(): ~1.4x slower than directly implemented and 9x slower than inline
// Iteration with Read(): ~1.8x faster than directly implemented First()+Empty()+PopFirst() and ~4x slower than inline
// To avoid performance loss for simple ranges prefer using bulk copying by chunks of N ~ (128 + sizeof(T) - 1) / sizeof(T) elements: RBuffered<InputRange<T>, Array<T, N>>.
// Perf tests: http://quick-bench.com/d4PM7BSJJvzCaIt_mJCjHYa9jNc; Compiler explorer: https://godbolt.org/z/60Pej2

template<typename T> class IRange
{
	static_assert(CConstructible<T> && CCopyAssignable<T>, "Not all cases are implemented yet.");
public:
	virtual ~IRange() {}

	constexpr bool Empty() const
	{
		TP unused;
		return tryFirst(&unused);
	}

	constexpr T First() const
	{
		INTRA_PRECONDITION(!Empty());
		TP res;
		tryFirst(&res);
		if constexpr(CReference<T>) return *res;
		else return res;
	};

	constexpr void PopFirst()
	{
		INTRA_PRECONDITION(!Empty());
		popFirstCount(1);
	}

	constexpr T Next()
	{
		INTRA_PRECONDITION(!Empty());
		TP res;
		read(&res, 1);
		if constexpr(CReference<T>) return *res;
		else return res;
	}

	constexpr index_t PopFirstCount(ClampedSize maxElementsToPop)
	{
		return index_t(popFirstCount(maxElementsToPop));
	}

	template<COutputOf<T> R> constexpr index_t ReadWrite(R&& dst)
	{
		using DstT = TValueType<R>;
		size_t res = 0;
		if constexpr(CArrayList<R> && CSame<DstT, T>)
		{
			res = read(DataOf(dst), LengthOf(dst));
			dst|Intra::PopFirstCount(res);
		}
		else
		{
			res = read(DataOf(dst), LengthOf(dst));
			dst|Intra::PopFirstCount(res);
		}
		return res;
	}
protected:
	using TP = TSelect<TRemoveReference<T>*, T, CReference<T>>;

	// Range interfaces should be implemented internally using a different basis than First/PopFirst/Empty.
	// We define them in terms of streams to minimize virtual call overhead for bulk operations and allow buffering.
	// Method prototypes don't use any typesafe types to avoid any overhead of passing and returning structs that some compilers have.
	constexpr virtual size_t read(TP* dst, size_t maxElements) = 0;
	virtual bool tryFirst(TP* dst) const = 0;
	virtual size_t popFirstCount(size_t maxElementsToPop) = 0;
};
INTRA_END
