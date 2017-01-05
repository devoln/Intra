#include "Audio/Synth/Generators/DrumPhysicalModel.h"

#ifndef INTRA_NO_AUDIO_SYNTH

namespace Intra { namespace Audio { namespace Synth { namespace Generators {

void DrumPhysicalModel::PopFirst()
{
	const float maxP = 0.3f;

	const uint maxX = mDX-1u;
	const uint maxY = mDY-1u;

	for(uint i=0; i<mCnt; i++)
	{
		for(uint y=0; y<mDY; y++)
		{
			for(uint x=0; x<mDX; x++)
			{
				mS(x, y) += ((mP((x-1) & maxX, y) + mP((x+1) & maxX, y) +
						mP(x, (y-1) & maxY) + mP(x, (y+1) & maxY))*0.25f - mP(x, y)
					)*mF(x, y);
			}
		}

		for(uint y=0; y<mDY; y++)
		{
			for(uint x=0; x<mDX; x++)
			{
				mS(x, y) = mS(x, y)*mK1 +
					(mS((x-1) & maxX, y) + mS((x+1) & maxX, y) +
					mS(x, (y-1) & maxY) + mS(x, (y+1) & maxY))*mK2;
			}
		}

		for(uint y=0; y<mDY; y++)
			for(uint x=0; x<mDX; x++)
				mP(x, y) += mS(x, y);

		for(uint x=0; x<mDX; x+=4)
		{
			if((mP(x, 0)>maxP && mS(x, 0)>0) ||
				(mP(x, 0)<maxP && mS(x, 0)<0))
				mS(x, 0) *= -0.5f;
		}

		mP(1, 1) *= 0.5f;
		mS(0, 0) = sRand()*0.00001f;
	}
}

DrumPhysicalModel::DrumPhysicalModel(null_t):
	mCnt(0), mDX(0), mDY(0),
	mFrc(0), mK1(0), mK2(0),
	mP(), mS(), mF(),
	mAmplitude(1), mdt(0), mFRandom(), mPrevRand(0) {}

DrumPhysicalModel::DrumPhysicalModel(byte count, byte dx, byte dy, float frc, float kDemp, float kRand):
	mCnt(count), mDX(dx), mDY(dy),
	mFrc(frc), mK1(0), mK2(0),
	mP(dx, dy), mS(dx, dy), mF(dx, dy),
	mAmplitude(1), mdt(1.0f/44100.0f), mFRandom(), mPrevRand(0)
{
	mK1 = 1.0f - kDemp*0.333f*frc;
	mK2 = (1.0f-mK1)*0.25f;

	for(uint y=0; y<dy; y++)
	{
		for(uint x=0; x<dx; x++)
		{
			float v = 1.0f+sRand()*kRand;
			mF(x, y) = mFrc*v;
		}
	}
	mF(0, 0) = frc;
	mF(dx/2u, dy/2u) = frc;
	mF(1, dy/2u) = frc;
	mF(dx/2u, 1) = frc;
	mS(0, 0) = 10;
	mS(dx/2u, dy/2u) = -10;
}

void DrumPhysicalModel::SetParams(float frequency, float amplitude, double step)
{
	(void)frequency;
	mAmplitude = amplitude;
	mdt = float(step);
}

float DrumPhysicalModel::sRand()
{
	const float rand = mFRandom();
	const float result = rand - mPrevRand;
	mPrevRand = rand;
	return result;
}

}}}}

#endif
