#pragma once

#include <Cpp/Features.h>
#include <Cpp/Warnings.h>

#include <Math/Math.h>
#include <Math/SineRange.h>

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

/// Sine that advances the two-term SineRange recursion once per block of
/// samples and linearly interpolates between those steps. Growing the step
/// moves the recursion's float32 degeneracy down by the same factor (1.87 Hz
/// -> 0.12 Hz at 48 kHz with a 16-sample step), while one step plus a lerp
/// costs less than a single Math::Sin — that is what modulation rates need:
/// they sit below the degeneracy, and a plain phase accumulator with Math::Sin
/// per sample is ~5 ns per harmonic (measured -16...-40 % of a flute's render
/// time). blockSamples IS the caller's step: a per-sample caller passes 0 or 1
/// (one recursion step per sample, no interpolation), a caller that steps the
/// oscillator once per N output samples and holds the value passes exactly N
/// (the interpolation inside the block is then unused, but the lattice value it
/// returns is the exact recursion sample at the block start).
struct SteppedSineRange
{
	SteppedSineRange(decltype(nullptr) = nullptr):
		mValue(0), mNext(0), mStep(0), mInvBlock(1), mSamplesLeft(1), mBlock(1) {}

	SteppedSineRange(float amplitude, float phase, float deltaPhase,
		unsigned blockSamples = 16):
		mOsc(amplitude, phase, deltaPhase*float(blockSamples < 1? 1: blockSamples)),
		mInvBlock(1.0f/float(blockSamples < 1? 1: blockSamples)),
		mBlock(blockSamples < 1? 1: blockSamples)
	{
		mValue = mOsc.Next();
		mNext = mOsc.Next();
		mStep = (mNext - mValue)*mInvBlock;
		mSamplesLeft = mBlock;
	}

	INTRA_FORCEINLINE float Next()
	{
		const float result = mValue;
		mValue += mStep;
		if(--mSamplesLeft == 0)
		{
			mValue = mNext;
			mNext = mOsc.Next();
			mStep = (mNext - mValue)*mInvBlock;
			mSamplesLeft = mBlock;
		}
		return result;
	}

private:
	/// The recursion itself, stepped once per mBlock samples.
	SineRange<float> mOsc;
	float mValue, mNext, mStep, mInvBlock;
	unsigned mSamplesLeft, mBlock;
};

INTRA_WARNING_POP