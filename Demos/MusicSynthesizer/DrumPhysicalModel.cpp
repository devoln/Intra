#include "DrumPhysicalModel.h"

#include <Range/Mutation/Copy.h>
#include <Range/Mutation/Transform.h>

#ifndef INTRA_NO_AUDIO_SYNTH

namespace Generators {

void DrumPhysicalModel::PopFirst()
{
	const float maxP = 0.3f;

	const unsigned maxX = mDX - 1u;
	const unsigned maxY = mDY - 1u;

	for(unsigned i = 0; i < mCnt; i++)
	{
		for(unsigned y = 0; y < mDY; y++)
		{
			for(unsigned x = 0; x < mDX; x++)
			{
				mS(x, y) += ((mP((x-1) & maxX, y) + mP((x+1) & maxX, y) +
						mP(x, (y-1) & maxY) + mP(x, (y+1) & maxY))*0.25f - mP(x, y)
					)*mF(x, y);
			}
		}

		for(unsigned y = 0; y < mDY; y++)
		{
			for(unsigned x = 0; x < mDX; x++)
			{
				mS(x, y) = mS(x, y)*mK1 +
					(mS((x - 1) & maxX, y) + mS((x + 1) & maxX, y) +
					mS(x, (y - 1) & maxY) + mS(x, (y + 1) & maxY))*mK2;
			}
		}

		for(unsigned y = 0; y < mDY; y++)
			for(unsigned x = 0; x < mDX; x++)
				mP(x, y) += mS(x, y);

		for(unsigned x = 0; x < mDX; x += 4)
		{
			if((mP(x, 0) > maxP && mS(x, 0) > 0) ||
				(mP(x, 0) < maxP && mS(x, 0) < 0))
				mS(x, 0) *= -0.5f;
		}

		mP(1, 1) *= 0.5f;
		mS(0, 0) = sRand()*0.00001f;
	}
}

DrumPhysicalModel::DrumPhysicalModel(decltype(null)):
	mCnt(0), mDX(0), mDY(0),
	mFrc(0), mK1(0), mK2(0),
	mP(), mS(), mF(),
	mFRandom(), mPrevRand(0) {}

DrumPhysicalModel::DrumPhysicalModel(byte count, byte dx, byte dy, float frc, float kDemp, float kRand):
	mCnt(count), mDX(dx), mDY(dy),
	mFrc(frc), mK1(0), mK2(0),
	mP(dx, dy), mS(dx, dy), mF(dx, dy),
	mFRandom(), mPrevRand(0)
{
	mK1 = 1.0f - kDemp*0.333f*frc;
	mK2 = (1.0f - mK1)*0.25f;

	for(unsigned y = 0; y < dy; y++)
	{
		for(unsigned x = 0; x < dx; x++)
		{
			float v = 1.0f + sRand()*kRand;
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

float DrumPhysicalModel::sRand()
{
	const float rand = mFRandom();
	const float result = rand - mPrevRand;
	mPrevRand = rand;
	return result;
}

Span<float> DrumPhysicalModel::operator()(Span<float> dst, bool add)
{
	if(add) AddAdvance(dst, *this);
	else ReadWrite(*this, dst);
	return dst;
}

}

#endif
