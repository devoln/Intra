#pragma once

#include "Intra/Range/Concepts.h"
#include "Intra/Functional.h"

INTRA_BEGIN
template<typename F> struct RGenerate: private F
{
	constexpr bool IsAnyInstanceInfinite = true};

	constexpr RGenerate(F function): F(Move(function)), mFront(F::operator()()) {}
	
	[[nodiscard]] constexpr bool Empty() const {return false;}
	[[nodiscard]] constexpr auto First() const {return mFront;}

	constexpr void PopFirst() {mFront = F::operator()();}

private:
	TResultOf<F> mFront;
};

template<typename F> constexpr RGenerate<TFunctorOf<F>> Generate(F&& func)
{return ForwardAsFunc<F>(func);}
INTRA_END
