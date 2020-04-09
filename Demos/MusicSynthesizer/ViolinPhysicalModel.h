#pragma once

#include <Core/Warnings.h>
#include <Math/SineRange.h>
#include <Utils/FixedArray.h>
#include <Audio/MusicNote.h>

INTRA_BEGIN
namespace Audio { namespace Synth { namespace Generators {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

struct ViolinPhysicalModel
{
	ViolinPhysicalModel():
		mAmplitude(0), mDT(0), mOscillator(0,0,0),
		mLen(0), mFrc(0), mK1(0), mK2(0),
		mP(), mS(), mBowIndex(0), mSoundIndex(0) {}

	void SetParams(float frequency, float ampl, double step)
	{
		float note = float(Log(frequency/MusicNote::BasicFrequencies[0]*44100.0*step)*12/0.6931471805599453)-14;
		float kDemp;
		if(note<13)
		{
			mFrc = 0.375f*Pow2(note/12*2);
			mLen = 230;
			kDemp = 0.005f;
		}
		else
		{
			float s = 230/Pow2((note-12)/12);
			mLen = Max(4u, unsigned(s));
			mFrc = 1.5f*(float(mLen)/s)*(float(mLen)/s);
			kDemp = 0.12f/(note+12);
		}
		mK1 = 1.0f - kDemp*0.333f*mFrc;
		mK2 = (1.0f-mK1)*0.5f;
		mBowIndex = mLen*9/10-3;
		mSoundIndex = mLen/100+1;
			
		mOscillator = SineRange<float>(0.9f, 0, float(2*PI*7*mDT)); //Частота вибратто 7 Гц

		mP.SetCount(mLen+1u);
		mS.SetCount(mLen+1u);

		mAmplitude = ampl*0.025f;
		mDT = float(step);

		for(int i=0; i<0.15/step; i++) PopFirst();
	}

	float NextSample()
	{
		PopFirst();
		return First();
	}

	float First() const {return mP[mSoundIndex]*mAmplitude;}

	void PopFirst()
	{
		float f = mFrc*(1.0f+mOscillator.First());
		mOscillator.PopFirst();

		float* sptr = mS.Data()+1;
		float* send = mS.Data()+mLen;
		float* pptr = mP.Data()+1;
		for(; sptr<send; pptr++) *sptr++ += f*(( *(pptr-1) + *(pptr+1))*0.5f - *pptr);

		sptr = mS.begin()+1;
		pptr = mP.begin()+1;
		while(sptr<send)
		{
			*sptr = (*sptr)*mK1 + ( *(sptr-1) + *(sptr+1) )*mK2;
			*pptr++ += *sptr++;
		}

		mS[mBowIndex] += 1.0f / ((mS[mBowIndex]-1)*(mS[mBowIndex]-1)+1); //Сила действия смычка на струну
	}

private:
	float mAmplitude;
	float mDT;

	SineRange<float> mOscillator;

	unsigned mLen;   //Длина струны
	float mFrc; //Натяжение струны
	float mK1, mK2; //Коэффициенты, определяющие затухание звука в струне

	FixedArray<float> mP; //Позиция участка струны
	FixedArray<float> mS; //Скорость участка струны

	unsigned mBowIndex;   //Участок струны, где работает смычок
	unsigned mSoundIndex; //Участок струны, где снимается звук
};

INTRA_WARNING_POP

}}}}
