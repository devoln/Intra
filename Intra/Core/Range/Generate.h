#pragma once

#include "Core/Range/Concepts.h"
#include "Core/Functional.h"

INTRA_CORE_RANGE_BEGIN
template<typename F> struct RGenerate: private F
{
	enum: bool {RangeIsInfinite = true};

	constexpr forceinline RGenerate(F function): F(Move(function)), mFront(F::operator()()) {}
	
	INTRA_NODISCARD constexpr forceinline bool Empty() const {return false;}
	INTRA_NODISCARD constexpr forceinline auto First() const {return mFront;}

	INTRA_CONSTEXPR2 forceinline void PopFirst() {mFront = F::operator()();}

private:
	TResultOf<F> mFront;
};

template<typename F> constexpr forceinline RGenerate<TFunctorOf<F>> Generate(F&& func)
{return ForwardAsFunc<F>(func);}
INTRA_CORE_RANGE_END
