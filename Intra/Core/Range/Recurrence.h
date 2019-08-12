#pragma once

#include "Core/Range/Concepts.h"
#include "Core/Functional.h"

INTRA_CORE_RANGE_BEGIN
INTRA_WARNING_DISABLE_ASSIGN_IMPLICITLY_DELETED
template<typename T, typename F> struct RRecurrence1: private F
{
	enum: bool {RangeIsInfinite = true};

	constexpr forceinline RRecurrence1(F function, T f1): F(function), a(f1) {}

	INTRA_NODISCARD constexpr forceinline T First() const {return a;}
	INTRA_CONSTEXPR2 forceinline void PopFirst() {a = F::operator()(a);}
	INTRA_NODISCARD constexpr forceinline bool Empty() const {return false;}

private:
	T a;
};

template<typename T, typename F> struct RRecurrence2: private F
{
	enum: bool {RangeIsInfinite = true};

	constexpr forceinline RRecurrence2(F function, T f1, T f2): F(function), a(f1), b(f2) {}

	INTRA_NODISCARD constexpr forceinline T First() const {return a;}
	INTRA_CONSTEXPR2 forceinline void PopFirst() {Swap(a, b); b = F::operator()(b, a);}
	INTRA_NODISCARD constexpr forceinline bool Empty() const {return false;}

private:
	T a, b;
};


template<typename T, typename F> INTRA_NODISCARD constexpr forceinline Requires<
	CCallable<F, T>,
RRecurrence1<TRemoveConstRef<T>, TFunctorOf<F>>> Recurrence(F&& function, T&& f1)
{return {ForwardAsFunc<F>(function), Forward<T>(f1)};}


template<typename T, typename F> INTRA_NODISCARD constexpr forceinline Requires<
	CCallable<F, T, T>,
RRecurrence2<TRemoveConstRef<T>, TRemoveConstRef<TFunctorOf<F>>>> Recurrence(F&& function, T&& f1, T&& f2)
{return {ForwardAsFunc<F>(function), Forward<T>(f1), Forward<T>(f2)};}
INTRA_CORE_RANGE_END
