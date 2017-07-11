#pragma once

#include "Cpp/Features.h"
#include "Cpp/Warnings.h"

#include "Meta/Type.h"

#include "Math/Math.h"

namespace Intra { namespace Range {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename T> struct SineRange
{
	enum: bool {RangeIsInfinite = true};

	SineRange(null_t=null):
		mS1(0), mS2(0), mK(0) {}

	SineRange(T amplitude, T phi0, T dphi):
		mS1(amplitude*Math::Sin(phi0)),
		mS2(amplitude*Math::Sin(dphi)),
		mK(2*Math::Cos(dphi)) {}

	forceinline bool Empty() const noexcept {return false;}
	forceinline T First() const noexcept {return mS1;}

	forceinline T Next() noexcept
	{
		const T result = mS1;
		PopFirst();
		return result;
	}
	
	forceinline void PopFirst() noexcept
	{
		const T newS = mK*mS2 - mS1;
		mS1 = mS2;
		mS2 = newS;
	}

private:
	T mS1, mS2, mK;
};

INTRA_WARNING_POP

}

namespace Math {
using Range::SineRange;
}

}
