#pragma once

#include "Range/Mixins/RangeMixins.h"
#include "Utils/Optional.h"

namespace Intra { namespace Range {

template<typename T, typename F> struct Recurrence1Result:
	Range::RangeMixin<Recurrence1Result<T, F>, T, Range::TypeEnum::Forward, false>
{
	typedef T value_type;
	typedef T return_value_type;

	forceinline Recurrence1Result(null_t=null): func(), a(0) {}
	forceinline Recurrence1Result(F function, T f1): func(function), a(f1) {}

	forceinline T First() const {return a;}
	forceinline void PopFirst() {a = func()(a);}
	forceinline bool Empty() const {return false;}

	forceinline bool operator==(const Recurrence1Result<T, F>& rhs) const {return a==rhs.a;}

private:
	Utils::Optional<F> func;
	T a;
};

template<typename T, typename F> struct Recurrence2Result:
	Range::RangeMixin<Recurrence2Result<T, F>, T, Range::TypeEnum::Forward, false>
{
	typedef T value_type;
	typedef T return_value_type;

	forceinline Recurrence2Result(null_t=null): func(), a(0), b(0) {}
	forceinline Recurrence2Result(F function, T f1, T f2): func(function), a(f1), b(f2) {}

	forceinline T First() const {return a;}
	forceinline void PopFirst() {core::swap(a, b); b = func()(b, a);}
	forceinline bool Empty() const {return false;}

	forceinline bool operator==(const Recurrence2Result<T, F>& rhs) const {return a==rhs.a && b==rhs.b;}

private:
	Utils::Optional<F> func;
	T a, b;
};

template<typename T, typename F> forceinline Recurrence1Result<T, F> Recurrence(F function, T f1)
{
	return Recurrence1Result<T, F>(function, f1);
}

template<typename T, typename F> forceinline Recurrence2Result<T, F> Recurrence(F function, T f1, T f2)
{
	return Recurrence2Result<T, F>(function, f1, f2);
}

}}
