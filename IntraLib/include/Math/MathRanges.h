#pragma once

#include "Meta/Type.h"
#include "Range/Mixins/RangeMixins.h"

namespace Intra { namespace Math {

template<typename T> struct SineRange:
	Range::RangeMixin<SineRange<T>, T, Range::TypeEnum::Forward, false>
{
	SineRange(null_t=null): s1(0), s2(0), k(0) {}

	SineRange(T amplitude, T phi0, T dphi):
		s1(amplitude*Math::Sin(phi0)),
		s2(amplitude*Math::Sin(dphi)),
		k(2*Math::Cos(dphi)) {}

	forceinline bool Empty() const {return false;}
	forceinline T First() const {return s2;}
	
	forceinline void PopFirst()
	{
		const T newS = k*s2-s1;
		s1 = s2;
		s2 = newS;
	}

private:
	T s1, s2, k;
};

template<typename T> struct ExponentRange: Range::RangeMixin<ExponentRange<T>, T, Range::TypeEnum::Forward, false>
{
	ExponentRange(T scale=0, double step=0, T k=0): ek_sr(T(Exp(-k*step))), exponent(scale/ek_sr) {}

	forceinline bool Empty() const {return false;}
	forceinline void PopFirst() {exponent*=ek_sr;}
	forceinline T First() const {return exponent;}

private:
	T ek_sr, exponent;

};


}}

