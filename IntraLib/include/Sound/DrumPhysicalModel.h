#pragma once

#ifndef INTRA_NO_MIDI_SYNTH

#include "Math/Random.h"
#include "Containers/Array2D.h"

namespace Intra {

class DrumPhysicalModel
{
	byte mCnt;
	byte mDX, mDY;
	float mFrc, mK1, mK2;
	Array2D<float> mP, mS, mF;
	float mAmpl, mdt;
	Math::Random<float> mFRandom;
	float mPrevRand;

public:
	enum: bool {RangeIsInfinite = true};


	float NextSample() {PopFirst(); return First();}

	void PopFirst();

	float First() const {return mP(1, mDY/2u)*mAmpl;}

	bool Empty() const {return mP.Width()==0;}

	DrumPhysicalModel(null_t=null);

	DrumPhysicalModel(byte count, byte dx, byte dy, float frc, float kDemp, float kRand);
	void SetParams(float frequency, float amplitude, double step);
	float sRand();
};

}

#endif
