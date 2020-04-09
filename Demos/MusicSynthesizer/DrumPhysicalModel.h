#pragma once

#include <Core/Core.h>

#ifndef INTRA_NO_AUDIO_SYNTH

#include <Random/FastUniform.h>
#include <Container/Utility/Array2D.h>

#include "Types.h"

namespace Generators {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

class DrumPhysicalModel
{
	byte mCnt;
	byte mDX, mDY;
	float mFrc, mK1, mK2;
	Array2D<float> mP, mS, mF;
	Random::FastUniform<float> mFRandom;
	float mPrevRand;

public:
	constexpr bool IsAnyInstanceInfinite = true};

	void PopFirst();

	float First() const {return mP(1, mDY/2u);}

	bool Empty() const {return mP.Width() == 0;}

	DrumPhysicalModel(decltype(null)=null);

	DrumPhysicalModel(byte count, byte dx, byte dy, float frc, float kDemp, float kRand);
	float sRand();

	Span<float> operator()(Span<float> dst, bool add);
};

INTRA_WARNING_POP

}

#endif
