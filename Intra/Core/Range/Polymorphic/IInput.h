#pragma once

#include "Core/Type.h"

INTRA_BEGIN
template<typename T> struct Span;

// TODO: use single universal function to reduce code bloat
// virtual index_t Copy(T* dst, index_t n) = 0;
// virtual bool TryFirst(T* dst) const = 0;
// All other functions can be implemented with it with some loss of efficiency that becomes negligible for complex ranges.
// Perf tests for simple Fibonacci range:
// Iteration with First(), Empty() and PopFirst(): ~1.3x slower in total than direct implementation for one element and 9x slower than inline
// Iteration with First() and Next(): ~1.4x slower than directly implemented and 9x slower than inline
// Iteration with Copy(): ~1.8x faster than directly implemented First()+Empty()+PopFirst() and ~4x slower than inline
// To avoid performance loss for simple ranges prefer using bulk copying by chunks of ~8 elements: RBuffered<InputRange<T>, SArray<8>>.
// Perf tests: http://quick-bench.com/d4PM7BSJJvzCaIt_mJCjHYa9jNc; Compiler explorer: https://godbolt.org/z/60Pej2

template<typename T> class IInput
{
public:
	virtual ~IInput() {}

	virtual bool Empty() const = 0;
	virtual T First() const = 0;
	virtual void PopFirst() = 0;

	//! It is not necessary to override this method but it reduces the virtual call number in 2 times.
	virtual TRemoveReference<T> Next()
	{
		auto result = First();
		PopFirst();
		return result;
	}
};

template<typename T> class IInputEx: public IInput<T>
{
public:
	virtual size_t PopFirstN(size_t count)
	{
		for(size_t i=0; i<count; i++)
		{
			if(this->Empty()) return i;
			this->PopFirst();
		}
		return count;
	}

	virtual size_t ReadWrite(Span<TRemoveConstRef<T>>& dst) = 0;
};

typedef IInputEx<char> IInputStream;
INTRA_END
