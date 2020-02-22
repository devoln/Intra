#pragma once

#include "Core/Range/Concepts.h"
#include "Core/Functional.h"
#include "Core/Range/Take.h"

INTRA_BEGIN
INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED
template<typename T, typename F> struct RSequence: private F
{
	enum: bool {RangeIsInfinite = true};

	forceinline RSequence() = default;
	forceinline RSequence(F function, size_t offset = 0): F(function), Offset(offset) {}

	INTRA_NODISCARD constexpr forceinline T First() const {return F::operator()(Offset);}
	forceinline constexpr void PopFirst() {Offset++;}
	INTRA_NODISCARD constexpr forceinline bool Empty() const {return false;}
	INTRA_NODISCARD constexpr forceinline T operator[](size_t index) const {return F::operator()(Offset+index);}

	INTRA_NODISCARD constexpr forceinline auto operator()(size_t start, size_t end) const
	{
		INTRA_DEBUG_ASSERT(start <= end);
		auto result = *this;
		result.Offset += start;
		return RTake<RSequence<T, F>>(Move(result), end-start);
	}

	size_t Offset = 0;
};

template<typename F> INTRA_NODISCARD constexpr forceinline
RSequence<TResultOf<TRemoveConstRef<F>, size_t>, TFunctorOf<F>> Sequence(F&& function)
{return {ForwardAsFunc<F>(function)};}
INTRA_END
