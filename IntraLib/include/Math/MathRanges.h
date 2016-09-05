#pragma once

#include "Meta/Type.h"
#include "Algorithms/Range.h"
#include "Algorithms/RangeConstruct.h"
#include "Utils/Optional.h"

namespace Intra { namespace Math {


template<typename T, typename F> struct SequenceResult:
	Range::RangeMixin<SequenceResult<T, F>, T, Range::TypeEnum::RandomAccess, false>
{
	typedef T value_type;
	typedef T return_value_type;

	forceinline SequenceResult(null_t=null): Function(), Offset(0) {}
	forceinline SequenceResult(F function, size_t offset=0): Function(function), Offset(offset) {}

	forceinline T First() const {return Function()(Offset);}
	forceinline void PopFirst() {Offset++;}
	forceinline bool Empty() const {return false;}
	forceinline T operator[](size_t index) const {return Function()(Offset+index);}

	forceinline bool operator==(const SequenceResult<T, F>& rhs) const {return Offset==rhs.Offset;}

	forceinline Range::TakeResult<SequenceResult<T, F>> opSlice(size_t start, size_t end) const
	{
		auto result = *this;
		result.Offset += start;
		return result.Take(end-start);
	}

	Utils::Optional<F> Function;
	size_t Offset;
};

template<typename F> forceinline SequenceResult<Meta::ResultOf<F, size_t>, F> Sequence(F function)
{
	return SequenceResult<Meta::ResultOf<F, size_t>, F>(function);
}


template<typename T, typename F> struct Recurrence1Result:
	Range::RangeMixin<Recurrence1Result<T, F>, T, Range::TypeEnum::Forward, false>
{
	typedef T value_type;
	typedef T return_value_type;

	forceinline Recurrence1Result(null_t=null): func(), a(0) {}
	forceinline Recurrence1Result(F function, T f1, T f2): func(function), a(f1) {}

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


template<typename T> struct SineRange: Range::RangeMixin<SineRange<T>, T, Range::TypeEnum::Forward, false>
{
	SineRange() = default;
	SineRange(T amplitude, T phi0=0, T dphi=0)
	{
		S[0] = amplitude*Math::Sin(phi0);
		S[1] = amplitude*Math::Sin(dphi);
		K = 2*Math::Cos(dphi);
	}

	forceinline bool Empty() const {return false;}
	forceinline T First() const {return S[1];}
	forceinline void PopFirst() {const T newS = K*S[1]-S[0]; S[0]=S[1]; S[1]=newS;}

private:
	T S[2], K;
};

template<typename T> struct ExponentRange: Range::RangeMixin<ExponentRange<T>, T, Range::TypeEnum::Forward, false>
{
	ExponentRange(T scale=0, double step=0, T k=0): ek_sr((T)Exp(-k*step)) {exponent = scale/ek_sr;}

	forceinline bool Empty() const {return false;}
	forceinline void PopFirst() {exponent*=ek_sr;}
	forceinline T First() const {return exponent;}

private:
	T ek_sr, exponent;

};


}}

