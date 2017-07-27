#pragma once

#include "Cpp/Features.h"
#include "Cpp/Warnings.h"

#include "Meta/Type.h"

#include "Math/Math.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Range {

template<typename T> struct SineRange
{
	enum: bool {RangeIsInfinite = true};

	SineRange(null_t=null):
		mS1(0), mS2(0), mK(2) {}

	SineRange(T amplitude, T phi0, T dphi):
		mS1(amplitude*Math::Sin(phi0)),
		mS2(amplitude*Math::Sin(phi0 + dphi)),
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

	forceinline bool operator==(null_t) const noexcept {return mS1 == 0 && mS2 == 0 && mK == 2;}
	forceinline bool operator!=(null_t) const noexcept {return !operator==(null);}

private:
	T mS1, mS2, mK;
};

}

namespace Math {
using Range::SineRange;
}

}

INTRA_WARNING_POP
