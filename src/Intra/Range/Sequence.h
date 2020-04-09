#pragma once

#include "Intra/Range/Concepts.h"
#include "Intra/Functional.h"
#include "Intra/Range/Take.h"

INTRA_BEGIN
INTRA_IGNORE_WARNING_COPY_IMPLICITLY_DELETED
template<typename T, typename F> struct RSequence: private F
{
	constexpr bool IsAnyInstanceInfinite = true};

	INTRA_FORCEINLINE RSequence() = default;
	INTRA_FORCEINLINE RSequence(F function, size_t offset = 0): F(function), Offset(offset) {}

	[[nodiscard]] constexpr T First() const {return F::operator()(Offset);}
	INTRA_FORCEINLINE constexpr void PopFirst() {Offset++;}
	[[nodiscard]] constexpr bool Empty() const {return false;}
	[[nodiscard]] constexpr T operator[](size_t index) const {return F::operator()(Offset+index);}

	[[nodiscard]] constexpr auto operator()(size_t start, size_t end) const
	{
		INTRA_DEBUG_ASSERT(start <= end);
		auto result = *this;
		result.Offset += start;
		return RTake<RSequence<T, F>>(Move(result), end-start);
	}

	size_t Offset = 0;
};

template<typename F> [[nodiscard]] constexpr
RSequence<TResultOf<TRemoveConstRef<F>, size_t>, TFunctorOf<F>> Sequence(F&& function)
{return {ForwardAsFunc<F>(function)};}
INTRA_END
