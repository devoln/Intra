#pragma once

#include "Platform/CppFeatures.h"
#include "Platform/CppWarnings.h"
#include "Meta/Type.h"
#include "Range/Concepts.h"
#include "Math.h"

namespace Intra { namespace Math {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename T> struct SineRange
{
	enum: bool {RangeIsInfnite = true};

	SineRange(null_t=null):
		mS1(0), mS2(0), mK(0) {}

	SineRange(T amplitude, T phi0, T dphi):
		mS1(amplitude*Math::Sin(phi0)),
		mS2(amplitude*Math::Sin(dphi)),
		mK(2*Math::Cos(dphi)) {}

	forceinline bool Empty() const {return false;}
	forceinline T First() const {return mS2;}
	
	forceinline void PopFirst()
	{
		const T newS = mK*mS2-mS1;
		mS1 = mS2;
		mS2 = newS;
	}

private:
	T mS1, mS2, mK;
};

template<typename T> struct ExponentRange
{
	enum: bool {RangeIsFinite = false};

	ExponentRange(T scale=0, double step=0, T k=0):
		mEkSr(T(Exp(-k*step))), mExponent(scale/mEkSr) {}

	forceinline bool Empty() const {return false;}
	forceinline void PopFirst() {mExponent *= mEkSr;}
	forceinline T First() const {return mExponent;}

private:
	T mEkSr, mExponent;
};

INTRA_WARNING_POP

}}
