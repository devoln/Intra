#pragma once

#include <Cpp/Warnings.h>
#include <Cpp/Features.h>

#include <Utils/Span.h>
#include <Utils/FixedArray.h>

#include <Math/SineRange.h>

#include "Types.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace PostEffects {

struct Echo
{
	float Delay;
	float MainVolume, SecondaryVolume;

	Echo(float delay=0.03f, float mainVolume=0.5f, float secondaryVolume=0.5f):
		Delay(delay), MainVolume(mainVolume), SecondaryVolume(secondaryVolume) {}

	void operator()(Span<float> inOutSamples, unsigned sampleRate) const;
};

struct FilterDrive
{
	float K;
	FilterDrive(float k): K(k) {}

	void operator()(Span<float> inOutSamples, unsigned sampleRate) const;
};

struct FilterHP
{
	float K;
	FilterHP(float k): K(k) {}

	static FilterHP FromCutoff(float cutoffFreqSampleRateRatio)
	{
		return {1.0f / (2*float(PI)*cutoffFreqSampleRateRatio + 1)};
	}

	static FilterHP FromCutoff(float cutoffFreq, unsigned sampleRate)
	{
		return FromCutoff(cutoffFreq / float(sampleRate));
	}

	void operator()(Span<float> inOutSamples, unsigned sampleRate) const;
};


struct FilterQ
{
	float F, K;
	float P, S;
	FilterQ(float f, float k): F(f), K(k), P(0), S(0) {}

	INTRA_FORCEINLINE float operator()(float sample)
	{
		P += S*F + sample;
		S = (S - P*F)*K;
		return P;
	}

	void operator()(Span<float> samples)
	{
		for(float& sample: samples)
			sample = operator()(sample);
	}
};

struct FilterQFactory
{
	float ResFreq, K;
	FilterQ operator()(unsigned sampleRate) {return FilterQ(float(ResFreq*2*PI/float(sampleRate)), K);}
};

struct Fade
{
	unsigned FadeIn, FadeOut;

	Fade(unsigned fadeIn=0, unsigned fadeOut=0):
		FadeIn(fadeIn), FadeOut(fadeOut) {}

	void operator()(Span<float> inOutSamples, unsigned sampleRate) const;
};

// Master-bus compressor approximating WebAudio's DynamicsCompressorNode with
// its default parameters (threshold -24 dB, knee 30 dB, ratio 12:1, attack 3 ms,
// release 250 ms) — exactly what web-midisynth uses on its master bus. It levels
// the mix the way the browser does: sustained wavetable pads and short piano
// plucks end up at comparable loudness instead of the pads drowning the melody.
// Cheap: one abs-max pass + one multiply per 128-sample block and a single
// log/exp per block (no per-sample transcendentals).
struct DynamicsCompressor
{
	static const size_t BlockSize = 128;

	float ThresholdDb = -24.0f;
	float KneeDb = 30.0f;
	float Ratio = 12.0f;
	float AttackSeconds = 0.003f;
	float ReleaseSeconds = 0.25f;
	// WebAudio не добавляет makeup-gain, поэтому его мастер-выход тихий (~-20 дБ).
	// В браузере это компенсируется громкостью колонок; у нас превью играет тот же
	// буфер напрямую, поэтому поднимаем уровень обратно, не меняя выравнивание.
	float MakeupDb = 18.0f;

	float mAttackCoeff = 0;
	float mReleaseCoeff = 0;
	float mDetector = 0;

	DynamicsCompressor(decltype(nullptr)=nullptr) {}
	explicit DynamicsCompressor(unsigned sampleRate) {Init(sampleRate);}

	void Init(unsigned sampleRate)
	{
		const float blockSeconds = float(BlockSize)/float(sampleRate);
		mAttackCoeff = 1.0f - Math::Exp(-blockSeconds/AttackSeconds);
		mReleaseCoeff = 1.0f - Math::Exp(-blockSeconds/ReleaseSeconds);
		mDetector = 0;
	}

	INTRA_FORCEINLINE float GainDb(float levelDb) const
	{
		const float kneeStart = ThresholdDb - KneeDb*0.5f;
		const float kneeEnd = ThresholdDb + KneeDb*0.5f;
		if(levelDb <= kneeStart) return 0;
		const float over = levelDb - ThresholdDb;
		if(levelDb >= kneeEnd) return over*(1.0f/Ratio - 1.0f);
		// Квадратичное мягкое колено (аппроксимация кривой WebAudio).
		const float x = over + KneeDb*0.5f;
		return (1.0f/Ratio - 1.0f)*x*x/(2.0f*KneeDb);
	}

	void operator()(Span<float> dstLeft, Span<float> dstRight)
	{
		const bool stereo = !dstRight.Empty();
		const size_t total = dstLeft.Length();
		// Ниже порога колена компрессор не давит (GainDb == 0), применяется только
		// makeup-gain. Держим его константой и умножаем всегда: это убирает скачок
		// громкости (клик) при переходе из тишины в первый звук.
		const float makeupGain = Math::Exp(0.11512925464970228f*MakeupDb);
		size_t off = 0;
		while(off < total)
		{
			const size_t n = Math::Min(total - off, BlockSize);

			// Пик текущего блока (связанный стерео-детектор, как у WebAudio).
			float peak = 0;
			for(size_t i = 0; i < n; i++)
			{
				const float a = Math::Abs(dstLeft[off+i]);
				if(a > peak) peak = a;
				if(stereo)
				{
					const float b = Math::Abs(dstRight[off+i]);
					if(b > peak) peak = b;
				}
			}

			// Огибающая с раздельными константами атаки/релиза.
			const float coeff = peak > mDetector ? mAttackCoeff : mReleaseCoeff;
			mDetector += coeff*(peak - mDetector);

			float gain = makeupGain;
			if(mDetector > 1e-9f)
			{
				const float levelDb = 8.685889638065037f*Math::Log(mDetector); // 20*log10(x)
				gain = Math::Exp(0.11512925464970228f*(GainDb(levelDb) + MakeupDb)); // 10^((dB+makeup)/20)
			}

			for(size_t i = 0; i < n; i++) dstLeft[off+i] *= gain;
			if(stereo) for(size_t i = 0; i < n; i++) dstRight[off+i] *= gain;

			off += n;
		}
	}
};

class HallReverb
{
	struct Delay
	{
		size_t Offset;
		float LeftVolume, RightVolume;
	};
	FixedArray<Delay> mD;
	FixedArray<float> mAccum;
	float mK = 0;
	float mS = 0;
	float mRF = 0;
	size_t mAccumIndex = 0;
	size_t mMaxDelay = 0;
	size_t mBufferedReverbSamples = 0;
public:
	HallReverb(decltype(nullptr)=nullptr) {}
	HallReverb(size_t delayLength, size_t numDelays, float reverbVolume=1, float k=0.5f);
	void ProcessSample(float* ioL, float* ioR, float reverbSample);
	void operator()(Span<float> dstLeft, Span<float> dstRight, Span<const float> reverbBuffer);
	INTRA_FORCEINLINE bool operator==(decltype(nullptr)) const noexcept {return mAccum.Empty();}
	INTRA_FORCEINLINE bool operator!=(decltype(nullptr)) const noexcept {return !operator==(nullptr);}
	INTRA_FORCEINLINE operator bool() const noexcept {return operator!=(nullptr);}

};

}

INTRA_WARNING_POP
