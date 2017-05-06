#pragma once

#include "Cpp/Features.h"
#include "Cpp/Warnings.h"

#include "Meta/Type.h"

#include "Math/Math.h"

namespace Intra { namespace Range {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename T> struct ExponentRange
{
	enum: bool {RangeIsFinite = false};

	ExponentRange(T scale=0, double step=0, T k=0):
		mEkSr(T(Math::Exp(-k*step))), mExponent(scale/mEkSr) {}

	forceinline bool Empty() const {return false;}
	forceinline void PopFirst() {mExponent *= mEkSr;}
	forceinline T First() const {return mExponent;}

private:
	T mEkSr, mExponent;
};

INTRA_WARNING_POP

}

namespace Math {
using Range::ExponentRange;
}

}
