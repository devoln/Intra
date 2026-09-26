#include "AdditiveSampler.h"
#include "PianoRegions.h"
#include "PianoAttackCoeffs.h"
#include "Container/Sequential/Array.h"

#ifndef INTRA_PIANO_ONSET_CACHE_MS
#define INTRA_PIANO_ONSET_CACHE_MS 500
#endif

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS



// Residual preset-level calibration for instruments that intentionally share the
// acoustic sample table. Quarter-dB signed values, indexed by acoustic region.
static const int8 gHonkyRegionGainQdb[25]  = {-28,-25,-23,-20,0,-22,-11,-8,-19,0,15,18,27,40,23,16,2,31,20,-14,-3,-3,-11,1,0};

// Titanic velocity residual after the common SF2 (v/127)^2 law. Each pair
// stores extra gain at velocity 50 and 85 in quarter-dB units; v115+ = 0.
static const int8 gVel2Q50[7] = {13,14,13,11,14,12,11};
static const int8 gVel2Q85[7] = {2,2,0,-3,3,1,1};
static const int8 gVel3Q50[25] = {17,13,11,12,11,8,3,2,5,2,4,0,1,1,2,0,-2,-1,-3,-6,-10,-16,-20,-31,-28};
static const int8 gVel3Q85[25] = {10,7,6,7,7,5,3,2,4,2,3,1,1,1,2,1,0,1,1,0,1,0,-1,-2,-4};
static const int8 gVel4Q50[3] = {7,3,-2};
static const int8 gVel4Q85[3] = {1,0,0};
static const int8 gVel5Q50[14] = {27,25,30,37,31,24,30,31,28,31,31,31,33,29};
static const int8 gVel5Q85[14] = {-1,1,2,5,-4,-3,-1,4,1,2,0,1,-3,0};
static const int8 gVel7Q50[11] = {6,6,6,6,5,4,4,3,1,0,0};
static const int8 gVel7Q85[11] = {4,4,3,4,3,3,2,2,1,0,0};

static forceinline void PianoVelocityCalibrationQ(uint8 profile, uint8 region, int8& q50, int8& q85)
{
	q50 = q85 = 0;
	switch(profile)
	{
	case 2: q50=gVel2Q50[region]; q85=gVel2Q85[region]; break;
	case 3: q50=gVel3Q50[region]; q85=gVel3Q85[region]; break;
	case 4: q50=gVel4Q50[region]; q85=gVel4Q85[region]; break;
	case 5: q50=gVel5Q50[region]; q85=gVel5Q85[region]; break;
	case 7: q50=gVel7Q50[region]; q85=gVel7Q85[region]; break;
	default: break; // Harpsichord already follows the common SF2 law.
	}
}

namespace
{

// Used by both onset-cache state reconstruction and the cache-independent
// analytic pre-contact string seek. Keep it available in cache-OFF builds too.
static noinline float PianoPowSamples(float x, size_t n)
{
	float r = 1.0f;
	while(n) { if(n & 1) r *= x; x *= x; n >>= 1; }
	return r;
}

static void PianoVelocityFilterCoeffs(unsigned sampleRate, float cutoff,
	float& c0, float& a1, float& a2, float& b1, float& b2)
{
	cutoff = Math::Clamp(cutoff, 20.0f, 0.49f*float(sampleRate));
	float q = 1.0f/Math::Tan(float(Math::PI)*cutoff/float(sampleRate));
	const float rez = 1.41421356237f;
	c0 = 1.0f/(1.0f + rez*q + q*q);
	a1 = 2.0f*c0; a2 = c0;
	b1 = -2.0f*(1.0f - q*q)*c0;
	b2 = (-1.0f + rez*q - q*q)*c0;
}
}


AdditiveSampler::~AdditiveSampler()
{
}

void AdditiveSampler::SetVelocity(float velocity01)
{
	velocity01 = Math::Clamp(velocity01, 0.0f, 1.0f);
	// MidiSynth applies the common SF2-like v^2 loudness correction to every
	// voice after construction.  Piano-specific work here is therefore only
	// residual timbre/level behaviour that the common law cannot express.
	if(mTableId == 0 && mVelocityProfile <= 1)
	{
		const float v = 127.0f*velocity01;
		// Harmonic-only upper-attack fit: the common transient residual weakens
		// at low velocity.  One shared exponent replaces any velocity-zone table.
		// Only the recorded upper-register transient has a velocity-dependent
		// residual.  The root-75 sample transient selected by the common-residual
		// fitter is essentially velocity-invariant from v57 through v100.
		mAttackBoostVelocityScale = mRegionIndex >= 21
			? Math::Pow(Math::Max(v, 1.0f)*0.01f, 0.25f) : 1.0f;
		if(v <= 0.0f) mVolume = 0.0f;
		else if(mVelocityProfile == 0 && mRegionIndex >= 21)
		{
			// Smooth upper-register residual: four constants instead of a literal
			// P1..P6 velocity-zone staircase. Gain@v100 and residual exponent are
			// both linear in the physical Titanic source root (96..105).
			const float rootOffset = Math::Clamp(float(mTable->Regions[mRegionIndex].RootKey) - 96.0f, 0.0f, 9.0f);
			const float gain100Db = -6.2983701f + 0.2577147f*rootOffset;
			const float exponentResidual = -0.5105314f - 0.1585468f*rootOffset;
			mVolume *= Math::Pow(10.0f, gain100Db*(1.0f/20.0f))*Math::Pow(v*0.01f, exponentResidual);
		}
	}
	if(mVelocityProfile > 1)
	{
		// Keep the accepted exp(v-1) velocity loudness law for every piano.
		// Preset-specific SF2 velocity behaviour is only a residual correction.
		int8 q50, q85; PianoVelocityCalibrationQ(mVelocityProfile, mRegionIndex, q50, q85);
		const float v = 127.0f*velocity01;
		float q = 0.0f;
		if(v <= 50.0f) q = float(q50);
		else if(v < 85.0f) q = float(q50) + (float(q85)-float(q50))*((v-50.0f)*(1.0f/35.0f));
		else if(v < 115.0f) q = float(q85)*(1.0f - (v-85.0f)*(1.0f/30.0f));
		// Titanic has real preset velocity-zone steps which the old 50->85
		// interpolation smoothed away.  Keep them as one compact profile-level
		// residual: quarter-dB units, fading to zero at v=85.  This matches the
		// measured SF2 relative-level curve without another table or another pow.
		if(mVelocityProfile == 2 && v >= 59.0f && v < 85.0f)
			q -= 9.356f*(85.0f-v)*(1.0f/26.0f); // -2.339 dB at the v59 zone edge.
		else if(mVelocityProfile == 5 && v >= 53.0f && v < 85.0f)
			q -= 25.717f*(85.0f-v)*(1.0f/32.0f); // -6.429 dB at the v53 zone edge.
		if(q != 0.0f) mVolume *= PianoQuarterDbGain(q);
	}
	if(!mVelocityFilterModel) return;
	mVelocityFilterInPartials = false;
	// Filter-domain handoff is a property of the SF2 velocity model, not of
	// how much raw PCM happens to be cached.  Keep the accepted 200 ms point
	// when the region cache window grows to 500 ms.
	mVelocityFilterHandoffSamples = size_t(0.20f*float(mSampleRate));

	const int vel = int(velocity01*127.0f + 0.5f);
	float cutoff;
	if(vel <= 35) cutoff = 800.0f;
	else if(vel <= 58) cutoff = 1000.0f;
	else if(vel <= 76) cutoff = 1700.0f;
	else if(vel <= 91) cutoff = 2700.0f;
	else if(vel <= 105) cutoff = 3900.0f;
	else cutoff = 19912.0f; // SF2 default initialFilterFc = 13500 cents.
	mVelocityFilterBaseCutoff = cutoff;
	mVelocityFilterCurrentCutoff = cutoff;
	mVelocityModEnvCents = vel >= 106 ? -2000.0f : (vel >= 92 ? -1000.0f : 0.0f);
	mVelocityModReleaseLevel = 0.0f;
	mVelocityModReleaseSample = 0;
	mVelocityModReleased = false;
	mVelocityModNextUpdate = mVelocityModEnvCents != 0.0f
		? size_t(0.04f*float(mSampleRate)) : mVelocityFilterHandoffSamples;

	mVelPrevSrcL = mVelPrevSrc2L = mVelPrevOutL = mVelPrevOut2L = 0.0f;
	mVelPrevSrcR = mVelPrevSrc2R = mVelPrevOutR = mVelPrevOut2R = 0.0f;
	SetVelocityFilterCutoff(cutoff);
}

void AdditiveSampler::SetVelocityFilterCutoff(float newCutoff)
{
	newCutoff = Math::Clamp(newCutoff, 20.0f, 0.49f*float(mSampleRate));
	mVelocityFilterCurrentCutoff = newCutoff;
	if(newCutoff >= 0.49f*float(mSampleRate))
	{
		mVelocityFilterBypass = true;
		mVelA1 = mVelA2 = mVelB1 = mVelB2 = 0.0f; mVelC = 1.0f;
		return;
	}
	mVelocityFilterBypass = false;
	PianoVelocityFilterCoeffs(mSampleRate, newCutoff, mVelC, mVelA1, mVelA2, mVelB1, mVelB2);
}

void AdditiveSampler::PromoteVelocityFilterToPartials()
{
	if(!mVelocityFilterModel || mVelocityFilterInPartials) return;
	mVelocityFilterInPartials = true;
	if(mVelocityFilterBypass) return;
	for(size_t p = 0; p < mCount; p++)
	{
		const float cw = Math::Clamp(0.5f*mK[p], -1.0f, 1.0f);
		const float w = Math::Acos(cw);
		const float sw = Math::Sin(w);
		if(Math::Abs(sw) < 1e-6f) continue;
		const float c2 = 2.0f*cw*cw - 1.0f;
		const float s2 = 2.0f*sw*cw;
		const float nr = mVelC + mVelA1*cw + mVelA2*c2;
		const float ni = -mVelA1*sw - mVelA2*s2;
		const float dr = 1.0f - mVelB1*cw - mVelB2*c2;
		const float di = mVelB1*sw + mVelB2*s2;
		const float den = dr*dr + di*di;
		if(den < 1e-20f) continue;
		const float hr = (nr*dr + ni*di)/den;
		const float hi = (ni*dr - nr*di)/den;
		const float b = hi/sw;
		const float a = hr - cw*b;
		const float x1 = mS1[p], x2 = mS2[p];
		const float x3 = mK[p]*x2 - x1;
		mS1[p] = a*x1 + b*x2;
		mS2[p] = a*x2 + b*x3;
	}
	// The filter is now represented by the carrier complex state. From here on
	// sustain returns to the old SIMD path; no scalar IIR work per sample.
	mVelocityFilterBypass = true;
	mVelocityModNextUpdate = mRendered + size_t(0.04f*float(mSampleRate));
}

void AdditiveSampler::ApplyVelocityFilterCutoffRatio(float newCutoff)
{
	if(!mVelocityFilterInPartials) return;
	newCutoff = Math::Clamp(newCutoff, 20.0f, 0.49f*float(mSampleRate));
	const float oldCutoff = Math::Clamp(mVelocityFilterCurrentCutoff, 20.0f, 0.49f*float(mSampleRate));
	if(Math::Abs(newCutoff - oldCutoff) < 0.01f) return;
	float oc,oa1,oa2,ob1,ob2,nc,na1,na2,nb1,nb2;
	PianoVelocityFilterCoeffs(mSampleRate, oldCutoff, oc, oa1, oa2, ob1, ob2);
	PianoVelocityFilterCoeffs(mSampleRate, newCutoff, nc, na1, na2, nb1, nb2);
	for(size_t p = 0; p < mCount; p++)
	{
		const float cw = Math::Clamp(0.5f*mK[p], -1.0f, 1.0f);
		const float sw2 = Math::Max(0.0f, 1.0f - cw*cw);
		const float sw = Math::Sqrt(sw2);
		if(sw < 1e-6f) continue;
		const float c2 = 2.0f*cw*cw - 1.0f, s2 = 2.0f*sw*cw;
		auto h = [cw,sw,c2,s2](float c0,float a1,float a2,float b1,float b2,float& hr,float& hi)
		{
			const float nr = c0 + a1*cw + a2*c2, ni = -a1*sw - a2*s2;
			const float dr = 1.0f - b1*cw - b2*c2, di = b1*sw + b2*s2;
			const float den = dr*dr + di*di;
			hr = (nr*dr + ni*di)/den; hi = (ni*dr - nr*di)/den;
		};
		float or_,oi,nr,ni; h(oc,oa1,oa2,ob1,ob2,or_,oi); h(nc,na1,na2,nb1,nb2,nr,ni);
		const float den = or_*or_ + oi*oi; if(den < 1e-20f) continue;
		const float rr = (nr*or_ + ni*oi)/den, ri = (ni*or_ - nr*oi)/den;
		const float b = ri/sw, a = rr - cw*b;
		const float x1 = mS1[p], x2 = mS2[p], x3 = mK[p]*x2 - x1;
		mS1[p] = a*x1 + b*x2; mS2[p] = a*x2 + b*x3;
	}
	mVelocityFilterCurrentCutoff = newCutoff;
}

void AdditiveSampler::UpdateVelocityModEnvelope()
{
	if(mVelocityModEnvCents == 0.0f) return;
	const float attackSamples = 7.000704f*float(mSampleRate); // attackModEnv = 3369 timecents.
	float env;
	if(mVelocityModReleased)
	{
		const float rel = float(mRendered - mVelocityModReleaseSample)/float(mSampleRate); // releaseModEnv=0 => 1 s.
		env = mVelocityModReleaseLevel*Math::Max(0.0f, 1.0f - rel);
	}
	else env = Math::Min(1.0f, float(mRendered)/attackSamples);
	const float cutoff = mVelocityFilterBaseCutoff*Math::Pow(2.0f, mVelocityModEnvCents*env/1200.0f);
	if(mVelocityFilterInPartials) ApplyVelocityFilterCutoffRatio(cutoff);
	else SetVelocityFilterCutoff(cutoff);
	mVelocityModNextUpdate = mRendered + size_t(0.04f*float(mSampleRate)); // 25 Hz control rate; envelope itself is ~7 s.
}


void AdditiveSampler::SetHeldStringStateAt(size_t target)
{
	// Reconstruct the held modal-string state analytically at an exact note age.
	// Used both by cache handoff and by the pre-contact reveal fast path, so the
	// first audible string sample does not require rendering the hidden prefix.
	const size_t delta = target;
	const size_t count = mCount;
	const size_t heldTarget = target;
	const size_t attackN = Math::Min(heldTarget, mDecayOnsetSamples);
	const float basis0 = PianoPowSamples(mAttackBasisStep0, Math::Min(target, mDecayOnsetSamples));
	const float basis1 = PianoPowSamples(mAttackBasisStep1, Math::Min(target, mDecayOnsetSamples));
	const float basis2 = PianoPowSamples(mAttackBasisStep2, Math::Min(target, mDecayOnsetSamples));

	for(size_t p = 0; p < count; p++)
	{
		const float x0 = mS1[p], x1 = mS2[p];
		const float c = Math::Clamp(0.5f*mK[p], -1.0f, 1.0f);
		const float ss = Math::Max(0.0f, 1.0f - c*c);
		const float sn = Math::Sqrt(ss);
		if(sn > 1e-7f && (x0 != 0.0f || x1 != 0.0f))
		{
			float zr = c, zi = sn, rr = 1.0f, ri = 0.0f;
			size_t n = delta;
			while(n)
			{
				if(n & 1)
				{
					const float tr = rr*zr - ri*zi;
					ri = rr*zi + ri*zr; rr = tr;
				}
				const float tr = zr*zr - zi*zi;
				zi = 2.0f*zr*zi; zr = tr;
				n >>= 1;
			}
			const float b = (x1 - x0*c)/sn;
			mS1[p] = x0*rr + b*ri;
			const float r1 = rr*c - ri*sn;
			const float i1 = ri*c + rr*sn;
			mS2[p] = x0*r1 + b*i1;
		}

		const float atkStep = mAtk[p];
		float a = atkStep != 0.0f ? 1.0f - PianoPowSamples(1.0f - atkStep, attackN) : 0.0f;
		if(heldTarget > mDecayOnsetSamples)
		{
			const size_t e1 = Math::Min(heldTarget, mSegSamples);
			if(e1 > mDecayOnsetSamples) a *= PianoPowSamples(mDecay1[p], e1 - mDecayOnsetSamples);
			const size_t e2 = Math::Min(heldTarget, mSegSamples2);
			if(e2 > mSegSamples) a *= PianoPowSamples(mDecay2[p], e2 - mSegSamples);
			const size_t e3 = Math::Min(heldTarget, mSegSamples3);
			if(e3 > mSegSamples2) a *= PianoPowSamples(mDecay3[p], e3 - mSegSamples2);
			if(heldTarget > mSegSamples3) a *= PianoPowSamples(mDecay4[p], heldTarget - mSegSamples3);
		}

		float naturalStep;
		if(heldTarget < mDecayOnsetSamples) naturalStep = 1.0f;
		else if(heldTarget < mSegSamples) naturalStep = mDecay1[p];
		else if(heldTarget < mSegSamples2) naturalStep = mDecay2[p];
		else if(heldTarget < mSegSamples3) naturalStep = mDecay3[p];
		else naturalStep = mDecay4[p];

		mDecay[p] = naturalStep;
		if(target >= mDecayOnsetSamples) mAtk[p] = 0.0f;
		mAmp[p] = a;
		mBeatPh[p] = mBeatStep[p]*float(target);
	}

	mReleased = false;
	mDecayStarted = target >= mDecayOnsetSamples;
	mSegSwitched = target >= mSegSamples;
	mSegSwitched2 = target >= mSegSamples2;
	mSegSwitched3 = target >= mSegSamples3;
	mAttackInterpPos = mAttackInterpLen = 0;
	if(target < mDecayOnsetSamples)
	{
		mAttackBasis0 = basis0; mAttackBasis1 = basis1; mAttackBasis2 = basis2;
	}
	else
	{
		mAttackBasis0 = mAttackBasisEnd0;
		mAttackBasis1 = mAttackBasisEnd1;
		mAttackBasis2 = mAttackBasisEnd2;
	}
}

#ifdef INTRA_UI_METERS
float AdditiveSampler::GetLevel() const
{
	float lvl = 0.0f;
	{
		const size_t count = mCount;
		const float* amp = mAmp.Data();
		for(size_t p = 0; p < count; p++)
		{
			const float a = amp[p] < 0.0f ? -amp[p] : amp[p];
			if(a > lvl) lvl = a;
		}
	}
	// The previous meter forgot the common SF2 release envelope, so released
	// piano notes looked much louder/longer in the UI than in the audio.
	if(mReleased && mSf2UniformRelease) lvl *= mSf2ReleaseGain;
	if(mEndSamples && mRendered + mFadeSamples >= mEndSamples)
		lvl *= float(mEndSamples > mRendered ? mEndSamples - mRendered : size_t(0))/float(mFadeSamples);
	return lvl > 1.0f ? 1.0f : lvl;
}
#endif

struct PianoSf2CommonAmFit
{
	float FreqHz, Gain0, RelDecay, Phase;
};

// Common-AM fits accepted by the quality gate.  Roots 99+ were rejected: the
// optimiser hit its 0.04 Hz lower bound and was fitting ordinary decay trend.
// No per-partial beat parameters are stored.
static const PianoSf2CommonAmFit gPianoSf2CommonAmFit[25] =
{
	{0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0},
	{0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0},
	{0,0,0,0}, {0,0,0,0},
		{2.0555895f,0.0527957f,0.0000000f,-0.9918837f}, // 69
		{4.7561314f,0.1032765f,1.1401322f,-2.6881953f}, // 72
		{1.4132774f,2.1643159f,5.8601925f,-2.9861449f}, // 75
		{1.7527649f,0.1431691f,0.5819905f,1.1313308f}, // 78
		{1.3217540f,0.1770851f,0.0000000f,1.3820464f}, // 81
		{1.8030221f,1.6103136f,2.8958629f,-2.1690283f}, // 84
		{1.4385468f,0.5434305f,1.1103205f,-1.5128575f}, // 87
		{2.3145363f,0.4510102f,0.0000000f,-1.0473212f}, // 90
		{5.3973945f,0.2051190f,0.0000000f,1.8214151f}, // 93
		{0,0,0,0}, // 96 rejected pending upper-region refit
	{0,0,0,0}, {0,0,0,0}, {0,0,0,0}
};

// Diagnostic v100 sweep-derived constant level residual per physical P1 region.
static const float gPianoSf2CommonLevel[25] =
{
	1.0000000f, 1.0000000f, 1.0000000f, 1.0000000f, 1.0000000f, 1.0000000f, 1.0000000f, 1.0000000f,
	1.0000000f, 1.0000000f, 1.0000000f, 1.0000000f, 0.9884750f, 0.8489246f, 0.9913867f, 1.0343440f,
	1.0061266f, 0.6894692f, 0.7117317f, 0.7081260f, 0.6988234f, 1.0000000f, 1.0000000f, 1.0000000f,
	1.0000000f
};

static size_t PianoAcousticRegionForKey(float midi)
{
	// Exact Titanic Grand P1 key-zone upper bounds.  Region ownership follows
	// the SF2 keyRange, not nearest-root distance.
	static const uint8 hi[25] =
	{25,31,35,40,45,49,52,55,58,61,64,67,70,73,76,79,82,85,88,91,94,97,100,103,127};
	const int key = Math::Clamp(int(midi + 0.5f), 0, 127);
	for(size_t i = 0; i < 25; i++) if(key <= int(hi[i])) return i;
	return 24;
}

AdditiveSampler::AdditiveSampler(float freq, float volume, unsigned sampleRate,
	size_t maxPartials, float brightness, float scale, float decayScale,
	float decayStiffness, float detuneCents,
	int unisonVoices, float velBrightness, float trebleTilt, float volumeScale,
	float beatScale, int tableId, float beatCents, uint8 velocityProfile)
{
	// Таблица коэффициентов: общая (acoustic) или per-instrument (SF2),
	// см. PianoGetTable в PianoRegions.h.
	mTable = &PianoGetTable(tableId);
	mTableId = uint8(tableId);
	// Titanic programs 0 and 1 share the same physical Clavinova P1 source.
	// Grand changes timbre with velocity by filtering that source; Bright keeps
	// P1 open at every velocity.  Keep the source recipe/cache shared and move
	// the program difference to the velocity/output layer.
	const bool acousticProgram0 = velocityProfile == 0;
	const bool sharedAcousticP1 = tableId == 0 && velocityProfile <= 1;
	mVelocityProfile = velocityProfile;
	mVelocityFilterModel = acousticProgram0;

	// Ближайший регион по MIDI-ноте (высота = равномерная темперация).
	const float midi = 69.0f + 12.0f*Math::Log(freq/440.0f)/0.6931471805599453f;
	size_t best = 0;
	if(sharedAcousticP1) best = PianoAcousticRegionForKey(midi);
	else
	{
		float bestDist = 1e30f;
		for(size_t i = 0; i < mTable->RegionCount; i++)
		{
			const float d = Math::Abs(float(mTable->Regions[i].RootKey) - midi);
			if(d < bestDist) { bestDist = d; best = i; }
		}
	}
	mRegionIndex = uint8(best);
	const PianoRegionData& region = mTable->Regions[best];
	// Acoustic program 0 models a transposed physical SF2 source region. Every
	// source-time event (attack, DecayOnset and later segment boundaries) must
	// scale by F0/freq so the complete note prefix can be shared as one region PCM.
	const float sourceTimeScale = sharedAcousticP1 && freq > 1e-6f ? region.F0/freq : 1.0f;
	const PianoSf2CommonAmFit& commonFit = gPianoSf2CommonAmFit[best < 25 ? best : 0];
	const bool sf2CommonAm = acousticProgram0 && commonFit.Gain0 > 0.0f;
	if(sf2CommonAm)
	{
		mCommonAmOn = true;
		mCommonAmFreqHz = commonFit.FreqHz;
		mCommonAmGain0 = commonFit.Gain0;
		mCommonAmLambda = commonFit.RelDecay;
		mCommonAmPhase = commonFit.Phase;
		mCommonAmPlaybackRate = freq/region.F0;
		const float z0 = Math::Sqrt(Math::Max(1e-12f, 1.0f + commonFit.Gain0*commonFit.Gain0
			+ 2.0f*commonFit.Gain0*Math::Cos(commonFit.Phase)));
		mCommonAmRefInv = 1.0f/z0;
	}

	// Число партиал: сколько влезает из региона (не больше maxPartials и не
	// выше Найквиста с запасом 8% — зависит от транспозиции). FreqRatio —
	// U16-квантованный (0.95+v/327675), декодируем до проверки Найквиста
	// (сырое значение v давало fk в тысячи Гц и обнуляло все партиалы).
	size_t partials = 0;
	for(size_t i = 0; i < size_t(region.PartCount) && i < maxPartials; i++)
	{
		const PianoPartial pp = PianoGetPartial(*mTable, region.PartOffset + i);
		const int k = pp.K;
		if(k <= 0) break;
		const float fr = 0.95f + float(pp.FreqRatio)*(1.0f/327675.0f);
		// A real SF2 region is one source sample whose spectrum is fixed before
		// transposition. For the shared acoustic-source path keep the same modal
		// set across the whole region; transposition happens on the time axis.
		const float spectrumFreq = sharedAcousticP1 ? region.F0 : freq;
		const float fk = float(k)*spectrumFreq*fr;
		if(fk >= 0.92f*float(sampleRate)*0.5f) break;
		partials = i + 1;
	}
	// 2026-08-26: per-key unison spread. Расстройка по регионам подогнана к
	// биениям, измеренным в сырых семплах SF2 (окна 100 мс, моно-сумма):
	//   root 43 (G2): h2 ~0.5 Гц  → ~4.0 цента
	//   root 47 (B2): биений нет  → 0
	//   root 51 (E3): h2 ~1.5 Гц  → ~7.0 цента
	//   root 54 (F#3), 57 (A3): нет → 0
	//   root 60 (C4): ~0.5 Гц (край слабый) → 0.3
	//   C5+: 0.3→1.4 цента (мерцание в первую секунду, Session 13).
	// Глубокий бас (≤ A#1) оставлен без биений: там семпл показывает
	// низкочастотную амплитудную «болтанку», которую нельзя объяснить
	// расстройкой струн (потребовалось бы 20–100+ центов) — не воспроизводим.
	// Верхние партиалы в басе не бьются по построению (per-partial вес
	// глубины ниже), поэтому «иииоуу» на длинных нотах исключено.
	// Региональный профиль биений унисона — общий для всех пиано, измерен по
	// семплам SF2 (2026-08-26, окна 100 мс, моно-сумма; повторно проверен
	// 2026-08-29 — см. ворклог):
	//   root 43 (G2): h2 ~0.5 Гц  → ~4.0 цента
	//   root 47 (B2): биений нет  → 0
	//   root 51 (E3): h2 ~1.5 Гц  → ~7.0 цента
	//   root 54 (F#3), 57 (A3): нет → 0
	//   root 60 (C4): ~0.5 Гц (край слабый) → 0.3
	//   C5+: 0.3→1.4 цента (мерцание в первую секунду, Session 13).
	// Глубокий бас (≤ A#1) без биений: там семпл даёт низкочастотную «болтанку»,
	// не объяснимую расстройкой струн (потребовались бы 20–100+ центов).
	// Эффективная расстройка = DetuneCents(инструмента) × base/spreadHi, т.е.
	// base — это spread в центах для эталонного AcousticPiano (1.4 цента).
	// Пер-инструментный BeatScale (AdditivePianoInstrument) умножает лестничный
	// вклад. Широкие пресеты (instDetune > spreadHi) на регионе 51 получают
	// base=0 (лестничные 7.0 дали бы honky-tonk 45 центов → деструктивные
	// биения AM 20–50 дБ на C#3-E3): D3-E3 у них плоский, как AGP, — так
	// принято на слух (вариант от 2026-08-30). Свой характер широких
	// пресетов живёт в басу G2 (~26 центов) и требли C5+ (9) — по коммитной
	// лестнице. Узкий acoustic (1.4 цента) — по чистой лестнице, «семпловая»
	// качка h2 ~1.5 Гц на регионе 51, как в коммите.
	{
		// BeatCents > 0 — плоская расстройка биений (EP-инструменты): лестница
		// ниже откалибрована по acoustic-семплам Clavinova, а семплы EP бьются
		// примерно постоянной скоростью ~2-3 Гц на C4-C5 (хорус тайн DX7/Rhodes).
		// Лестница на C4 дала бы 0.214·DetuneCents (base 0.3) — почти без биений
		// в середине, поэтому для них лестница заменяется плоским значением.
		const float spreadHi = 1.4f;  // эталонная расстройка на C5+ (AcousticPiano)
		const float instDetune = detuneCents;  // собственная расстройка инструмента (до умножения)
		if(beatCents > 0.0f)
			detuneCents = beatCents;
		else
		{
			float base;
			if(midi <= 40.0f) base = 0.0f;
			else if(midi < 45.0f) base = 4.0f;   // регион 43 (G2)
			else if(midi < 49.0f) base = 0.0f;   // регион 47 (B2)
			else if(midi < 53.0f) base = 7.0f;   // регион 51 (E3)
			else if(midi < 56.0f) base = 0.0f;   // регион 54 (F#3)
			else if(midi < 59.0f) base = 0.0f;   // регион 57 (A3)
			else if(midi < 62.0f) base = 0.3f;   // регион 60 (C4)
			else base = 0.3f + 1.1f*Math::Min(1.0f, (midi - 60.0f)/12.0f);
			// Регион-51 base (7.0) откалиброван под УЗКУЮ расстройку эталонного
			// acoustic (spreadHi). Широкий пресет (honky-tonk, instDetune=9.0)
			// умножил бы его до ~45 центов — деструктивные биения (AM до 50 дБ)
			// именно на C#3-E3. Для широких пресетов регион 51 НЕ берёт лестничный
			// base (base=0 → D3-E3 плоский, как AGP; так принято на слух 2026-08-30
			// — «хоть и звучит как AGP»). Свой характер широких пресетов — в басу
			// G2 и требли C5+ (коммитная лестница), остальное как в коммите.
			if(midi >= 49.0f && midi < 53.0f && instDetune > spreadHi)
				base = 0.0f;
			detuneCents *= (base * beatScale) / spreadHi;
		}
	}
	// «Струны» унисона: 1-3, каждая со своей расстройкой, громкостью и фазой.
	unisonVoices = Math::Clamp(unisonVoices, 1, 3);
	const int voices = unisonVoices;
	float voiceCents[3] = {0, 0, 0};
	float voiceGain[3]  = {1.0f, 0.0f, 0.0f};
	if(voices == 1)
	{
		voiceCents[0] = 0;
		voiceGain[0] = 1.0f;
	}
	else if(voices == 2)
	{
		voiceCents[0] = -0.6f*detuneCents;
		voiceCents[1] = +0.6f*detuneCents;
		voiceGain[0] = 1.0f;
		// Глубина биений по клавишам (2026-08-26): у семплов SF2 середина
		// клавиатуры бьётся МЕЛКО (C5 h2 ~7 дБ, D#5 ~5 дБ, C4 ~4 дБ), требли
		// — глубоко (C6+ 20-50 дБ), низ почти не бьётся. Плоский баланс
		// 1.0/0.7 (провал 15 дБ) на C4-E5 давал «странный отзвук» — глубокую
		// медленную раскачку на длинных нотах, которой в семпле нет. Уровень
		// второй струны задаёт глубину (пик/провал = (1+g1)/|1−g1|): после
		// нормировки пика тембр и средний уровень не меняются (у обеих струн
		// одинаковые партиалы — меняется только размах биений). Session 14c:
		// мелкая огибающая (8.4 дБ) распространяется на весь бас и середину
		// (≤ E5), глубокая (15 дБ) остаётся только в требли (C6+), где семплы
		// бьются глубоко. Узлы: ≤76: 0.45, 76→84: 0.45→0.7, ≥84: 0.7.
		float g1 = 0.45f;
		if(midi >= 84.0f) g1 = 0.7f;
		else if(midi > 76.0f) g1 = 0.45f + 0.25f*(midi - 76.0f)/8.0f;
		voiceGain[1] = g1;
	}
	else
	{
		// Асимметричная расстройка (как у реально настроенного рояля: струны
		// не симметричны — симметрия давала глубокие периодические провалы).
		voiceCents[0] = -0.8f*detuneCents;
		voiceCents[1] = 0;
		voiceCents[2] = +1.2f*detuneCents;
		voiceGain[0] = 1.0f;
		voiceGain[1] = 0.7f;
		voiceGain[2] = 0.6f;
	}
	// Коллапс двух струн в один лейн на партиалу (оптимизация горячего
	// цикла, 2026-08-26): сумма двух расстроенных синусоид с одинаковой
	// фазой  a·(g0·sin(ω0t+φ) + g1·sin(ω1t+φ))  =  a·(g0+g1)·E(t)·sin(ωt+φ+θ),
	// E = sqrt(cos²(Δt) + r²·sin²(Δt)), r = (g0−g1)/(g0+g1), ω = (ω0+ω1)/2.
	// Лейн получает амплитуду a·(g0+g1) и номинальную частоту ω, биение
	// даёт огибающая E(t) в горячем цикле (см. RenderInto). Точность: по
	// амплитуде — точно, отброшен фазовый воббл |θ| ≤ atan(r) ≈ 10°.
	const bool beatCollapse = (voices == 2);
	const int lanes = beatCollapse ? 1 : voices;
	const float gSum = beatCollapse ? (voiceGain[0] + voiceGain[1]) : 1.0f;
	const size_t count = Math::Max(size_t(4),
		(partials*size_t(lanes) + 3) & ~size_t(3));
		mS1.SetCount(count);
	mS2.SetCount(count);
	mK.SetCount(count);
	mAmp.SetCount(count);
	mDecay.SetCount(count);
	mDecay1.SetCount(count);
	mDecay2.SetCount(count);
	mDecay3.SetCount(count);
	mDecay4.SetCount(count);
	mDecayRelease.SetCount(count);
	mAtk.SetCount(count);
	mBeatStep.SetCount(count);
	mBeatPh.SetCount(count);
	mBeatE0.SetCount(count);
	mBeatE1.SetCount(count);
	mBeatR2.SetCount(count);
	mStereoPartL.SetCount(count);
	mStereoPartRA.SetCount(count);
	mStereoPartRB.SetCount(count);
	mAttackCoeff0.SetCount(count);
	mAttackCoeff1.SetCount(count);
	mAttackCoeff2.SetCount(count);
	mAttackEnv0.SetCount(count);
	mAttackEnv1.SetCount(count);
	for(size_t p = 0; p < count; p++)
	{
		mBeatStep[p] = 0.0f; mBeatPh[p] = 0.0f; mBeatR2[p] = 1.0f;
		mStereoPartL[p] = 0.5f; mStereoPartRA[p] = 0.5f; mStereoPartRB[p] = 0.0f;
		mAttackCoeff0[p] = mAttackCoeff1[p] = mAttackCoeff2[p] = 0.0f;
		mAttackEnv0[p] = mAttackEnv1[p] = 1.0f;
	}
	// Базовая глубина биения r = (g0−g1)/(g0+g1); на партиалу докручивается
	// весом w(k) в цикле лейнов (mBeatR2 = (1−(1−r)·w)²). r², а не r:
	// огибающая E = sqrt(1 − (1−r²)·sin²) использует квадрат.
	const float beatR = beatCollapse ? (1.0f - voiceGain[1])/(1.0f + voiceGain[1]) : 0.0f;
	mBeatOn = beatCollapse && !sf2CommonAm && !acousticProgram0;
	const float twoPi = 2.0f*float(Math::PI);
	// Все lambda приходят из fitDecay для конкретного региона/партиала;
	// глобальные поправки по высоте намеренно не применяются.
	// Decay1/2 уже подогнаны к каждому SF2-семплу в PianoRegions.h;
	// дополнительной октавной эвристики здесь быть не должно.
	const float octCorr1 = 1.0f;
	const float octCorr2 = 1.0f;
	// Граница onset измерена для этого SF2-региона генератором таблицы;
	// никаких дополнительных поправок по высоте здесь нет.
	const float decayOnset = region.DecayOnset;
	// cr/ci — компоненты a·sin(φ + dphi·t) = ci·cos(dphi·t) + cr·sin(dphi·t),
	// dphis — фаза на сэмпл (для синтеза периода при нормировке).
	FixedArray<float> crs(count), cis(count), dphis(count);
	const float detuneRatio = Math::Pow(2.0f, 1.0f/1200.0f);
	// velocity→яркость: громче играешь — ярче тембр (volume уже кодирует
	// velocity: exp(vel/127 − 1), vel 60 → 0.59, vel 100 → 0.81, vel 127 → 1).
	const float velF = Math::Clamp((volume - 0.55f)*2.0f, 0.0f, 1.0f);
	const float effBright = brightness + velBrightness*velF;
	const float tilt = 0.8f*Math::Max(0.0f, effBright - 0.25f);
	// Верхние октавы: TrebleTilt подавляет обертона (0 = по семплу — у
	// Clavinova верхние семплы и так фундаментал-доминантны).
	const float treble = Math::Min(2.2f, Math::Max(0.0f, freq/261.63f - 1.0f))*trebleTilt;
	size_t o = 0;
	for(int v = 0; v < lanes; v++)
	{
		// При коллапсе — номинальная частота (без расстройки): расстройку
		// струн несёт огибающая биений.
		const float det = beatCollapse ? 1.0f : Math::Pow(detuneRatio, voiceCents[v]);
		for(size_t p = 0; p < partials; p++, o++)
		{
			const PianoPartial pp = PianoGetPartial(*mTable, region.PartOffset + p);
			const int k = pp.K;
			// Упакованные поля таблицы (Amp/Decay — U16, Phase — U8: шаг фазы
			// 360/256=1.41° — неслышим). Декодируем тут, дальше — чистый float.
			const float amp = float(pp.Amp)*(1.0f/65535.0f);
			const float phase0 = (float(pp.Phase)*(1.0f/255.0f) - 0.5f)*twoPi;
			const float decay1 = float(pp.Decay1)*(1.0f/2621.4f);
			const float decay2 = float(pp.Decay2)*(1.0f/2621.4f);
			const float decay3 = float(pp.Decay3)*(1.0f/5461.25f);
			// D4 — та же шкала, что у D3 (5461.25), а не 2621.4: в таблице
			// D4 == D3 (3-сегментная модель утра), и 4-й сегмент должен быть
			// no-op. При шкале 2621.4 получалось λ4 = 2.08·λ3 — хвост после
			// SegT3 гас вдвое быстрее семпла (у D#5: 11 дБ/с вместо 5 дБ/с),
			// и нота «обрывалась» в сустейне.
			const float decay4 = float(pp.Decay4)*(1.0f/5461.25f);
			const float freqRatio = 0.95f + float(pp.FreqRatio)*(1.0f/327675.0f);
			if(k <= 0 || pp.Amp == 0)
			{
				mK[o] = 2.0f;
				mDecay[o] = 1.0f;
				mDecay1[o] = 1.0f;
				mDecay2[o] = 1.0f;
				mDecay3[o] = 1.0f;
				mDecay4[o] = 1.0f;
				mDecayRelease[o] = 1.0f;
				mAtk[o] = 0.0f;
				mBeatStep[o] = 0.0f;
				mBeatR2[o] = 1.0f;
				mAmp[o] = 0.0f;
				crs[o] = 0.0f;
				cis[o] = 0.0f;
				dphis[o] = 0.0f;
				continue;
			}
			// Частота партиалы: измеренный в семпле ratio (растяжка/негармоничность)
			// × транспозиция × расстройка струны.
			const float fk = float(k)*freq*freqRatio*det;
			const float dphi = twoPi*(fk/float(sampleRate));
			// Амплитуда из семпла; brightness/velocity усиливают верха, а
			// treble-tilt на высоких нотах их глушит. При коллапсе унисона
			// амплитуда лейна — ПОЛНАЯ сумма струн (g0+g1): пик суммы при
			// совпадающей фазе = (g0+g1)·a, биение докручивает огибающая.
			float a = amp*Math::Pow(float(k), tilt - treble)*(beatCollapse ? (sharedAcousticP1 ? 1.0f : gSum) : voiceGain[v]);
			// Фаза: измеренная из семпла. Для дополнительных струн унисона
			// фаза НЕ сдвигается: удар молоточка возбуждает струны в фазе,
			// а биения возникают из-за расстройки (voiceCents), а не из-за
			// начального сдвига. Раньше здесь был сдвиг на фиксированную
			// задержку 0.9 мс (phase -= twoPi*fk*0.0009*v) — при суммировании
			// струн это давало гребенчатый фильтр: на D#5 (fk=623 Гц) пара
			// голосов складывалась в противофазе (201.9° → h1 ослаблен на
			// 7.2 дБ), а h2 (1249 Гц) — в фазе (+3.9 дБ). Итог: «октавный
			// гул» +10 дБ на всём острове D5–E5, хотя таблица амплитуд
			// сбалансирована. Без сдвига спектр каждой струны сохраняется
			// в сумме, и относительный баланс гармоник — ровно по таблице.
			float phase = phase0;
			crs[o] = a*Math::Cos(phase);
			cis[o] = a*Math::Sin(phase);
			dphis[o] = dphi;
			float ratioRtoL = 1.0f, phaseRminusL = 0.0f;
			PianoGetStereo(size_t(region.PartOffset) + p, ratioRtoL, phaseRminusL);
			// Preserve stereo energy: gl^2 + gr^2 = 0.5, while gr/gl
			// follows the measured R/L ratio. Right-channel phase is reconstructed
			// from the same two adjacent sine-recurrence states, so no second
			// oscillator recurrence is needed.
			const float gl = Math::Sqrt(0.5f/(1.0f + ratioRtoL*ratioRtoL));
			const float gr = ratioRtoL*gl;
			const float sd = Math::Sin(dphi);
			if(Math::Abs(sd) > 1e-5f)
			{
				const float q = Math::Sin(phaseRminusL)/sd;
				mStereoPartL[o] = gl;
				mStereoPartRA[o] = gr*(Math::Cos(phaseRminusL) - Math::Cos(dphi)*q);
				mStereoPartRB[o] = gr*q;
			}
			else { mStereoPartL[o] = gl; mStereoPartRA[o] = gr; mStereoPartRB[o] = 0.0f; }
			// The measured early-attack table is currently calibrated only for
			// the acoustic piano table. Other additive instruments keep the
			// accepted fast-onset trajectory (zero coefficients => multiplier 1).
			if(tableId == 0 && region.RootKey <= 81)
			{
				const PianoAttackCoeff ac = PianoGetAttackCoeff(size_t(region.PartOffset) + p);
				mAttackCoeff0[o] = ac.C0;
				mAttackCoeff1[o] = ac.C1;
				mAttackCoeff2[o] = ac.C2;
			}
			if(beatCollapse && !sf2CommonAm && !acousticProgram0)
			{
				// Шаг фазы биения Δ = π·fk·(det1−det0)/sr (знак не важен: E
				// зависит от cos²/sin²). Масштаб по партиале: fk ≈ k·f0, поэтому
				// у k-й гармоники биения в k раз быстрее, как в семпле.
				const float det0 = Math::Pow(detuneRatio, voiceCents[0]);
				const float det1 = Math::Pow(detuneRatio, voiceCents[1]);
				mBeatStep[o] = float(Math::PI)*(fk/float(sampleRate))*(det1 - det0);
				// Per-partial вес глубины биения (2026-08-26): у семпла бьются
				// низкие партиалы (h1/h2), а h3+ держат уровень — глубокая
				// медленная огибающая на h5/h6 при неподвижном h1 давала
				// «иииоуу» (спектр темнел за секунды). w=0 — партиала не
				// бьётся вовсе. В басе/середине h1 у семпла тоже не бьётся
				// (глубина 4-6 дБ = шум окна) — вес 0.25, чтобы не было
				// медленного «насоса»; к C5 плавно до 1 (в требли h1 бьётся
				// глубоко, C6+ 10-33 дБ).
				// EP-профиль (BeatCents>0, 2026-09-04): у DX7/Rhodes-семплов
				// бьются ВСЕ партиалы вместе (хорус тайн), h1 в середине тоже
				// (C4 h1 ~7 дБ — лестничные 0.25 дали бы 1.5 дБ) — плоский вес:
				// h1 полный, h2/h3 половинный, h4+ четверть.
				float w;
				if(beatCents > 0.0f)
				{
					if(k == 1) w = 1.0f;
					else if(k == 2 || k == 3) w = 0.5f;
					else w = 0.25f;
				}
				else
				{
					w = 0.0f;
					if(k == 1)
					{
						// Beat depth belongs to the recorded region, not to the key used
						// to transpose that region. This makes one region PCM reusable.
						const float beatMidi = sharedAcousticP1 ? float(region.RootKey) : midi;
						if(beatMidi <= 60.0f) w = 0.25f;
						else if(beatMidi >= 72.0f) w = 1.0f;
						else w = 0.25f + 0.75f*(beatMidi - 60.0f)/12.0f;
					}
					else if(k == 2) w = 1.0f;
					else if(k == 3) w = 0.5f;
				}
				const float rEff = 1.0f - (1.0f - beatR)*w;
				mBeatR2[o] = rEff*rEff;
			}
			mK[o] = 2.0f*Math::Cos(dphi);
			// Затухание: 3-скоростное из семпла (λ1 — начальный спад, λ2 —
			// средний, λ3 — хвост), масштабируется транспозицией (выше нота —
			// быстрее затухает) и DecayScale инструмента. DecayStiffness (0 у
			// пиано) ускоряет верха: λ·(1 + c·k²). До DecayOnset затухания нет
			// (пик атаки; для верхних нот onset сокращён), mDecay стартует 1.0
			// и на точных границах заменяется шагами λ1/λ2/λ3.
			const float trans = (freq/region.F0)*decayScale;
			const float stiff = 1.0f + decayStiffness*float(k*k);
			const float lam1 = decay1*trans*stiff*octCorr1;
			const float lam2 = decay2*trans*stiff*octCorr2;
			const float lam3 = decay3*trans*stiff;
			const float lam4 = decay4*trans*stiff;
			mAmp[o] = 0.0f;
			mDecay[o] = 1.0f;
			mDecay1[o] = Math::Exp(-lam1/float(sampleRate));
			mDecay2[o] = Math::Exp(-lam2/float(sampleRate));
			mDecay3[o] = Math::Exp(-lam3/float(sampleRate));
			mDecay4[o] = Math::Exp(-lam4/float(sampleRate));
			// Release (демпфер) — ЭТО ДОБАВКА к естественному затуханию, а не его
			// замена: dec_release = dec_natural × dec_damper. Физически демпфер
			// добавляет свой коэффициент затухания к собственным потерям струны.
			// Здесь mDecayRelease хранит ТОЛЬКО шаг демпфера; NoteRelease()
			// домножает его на текущий естественный шаг (dec[p] *= decR[p]).
			//
			// Скорость демпфера: выше нота — быстрее (короткая струна гасится
			// быстрее), выше гармоника — быстрее, но МЯГКО (√k, не k):
			//   λ_damper(k) = (1/0.13с)·(f_note/f_C4)^0.65 / √k
			// Скалирование по √k (а не по k) не даёт тембру «провалиться» до
			// чистого фундаментала на коротких нотах — именно это звучало как
			// странный призвук/квакание после release.
			// Калибровка по FL-рендеру (окно после NoteOff): C4 156→91 мс,
			// C5 92→56, D#5 104→57, C6 51→55 мс vs FL 100/58/54/40.
			const float tauR = 0.13f*Math::Pow(261.625565f/freq, 0.65f) / Math::Sqrt(float(k));
			mDecayRelease[o] = Math::Exp(-1.0f/(tauR*float(sampleRate)));
			// Per-partial attack rise для партиал, которых удар не успевает
			// раскачать (короткий контакт не передаёт энергию высоким модам):
			// τ = min(AttackT/k, 0.6 мс). Эти партиалы входят после буфера
			// контактной силы плавно; возбуждённые ударом приходят из буфера
			// уже на полном уровне (mAtk обнуляется в блоке контактной силы).
			// С таким mAtk голос после release добирается до −60 дБ-гейта так
			// же быстро, как в базисе; единый рамп 0.8 мс держал голоса выше
			// гейта заметно дольше и копил ~3× живых голосов на педальных MIDI.
			{
				const float tauK = Math::Min(region.AttackT / float(k), 0.0006f);
				mAtk[o] = 1.0f - Math::Exp(-1.0f/(tauK*float(sampleRate)));
			}
		}
	}
	// Лишние лейны (округление до кратного 4) — тишина.
	for(; o < count; o++)
	{
		mK[o] = 2.0f;
		mDecay[o] = 1.0f;
		mDecay1[o] = 1.0f;
		mDecay2[o] = 1.0f;
		mDecay3[o] = 1.0f;
		mDecay4[o] = 1.0f;
		mDecayRelease[o] = 1.0f;
		mAtk[o] = 0.0f;
		mBeatStep[o] = 0.0f;
		mAmp[o] = 0.0f;
		crs[o] = 0.0f;
		cis[o] = 0.0f;
		dphis[o] = 0.0f;
	}
	// Absolute packed Amp tables are already calibrated per region. Avoid the
	// legacy period peak-normalization for every calibrated piano preset.
	float c = scale;
	if(tableId == PianoTableHonkyTonk)
		c *= PianoQuarterDbGain(float(gHonkyRegionGainQdb[best]));

	// The packed Amp/Phase state is measured at region.DecayOnset. Runtime
	// reconstruction rewinds that state to raw-sample t=0 and lets the measured
	// modal/attack basis evolve from note-on. Keep the existing 5 ms internal
	// convergence here only for cache/seek state continuity; it is hidden from
	// the listener by the contact-prelude gate below and is not an audible string
	// attack envelope. The owner-selected string-only candidate applies only the
	// explicit 1 ms linear anti-click blend below.
	const size_t stringRiseSamples = Math::Max(size_t(1), size_t(0.005f*float(sampleRate)*sourceTimeScale + 0.5f));
	const float stringRiseStep = 1.0f - Math::Exp(Math::Log(0.005f)/float(stringRiseSamples));
	// String-only listening candidate: reveal the analytically-seeked coherent
	// string at the measured ~4.3 ms source-time boundary, then fade it in
	// linearly over 1 ms. This is intentionally the minimal anti-click bridge:
	// no hammer/contact head and no felt microtexture are mixed in.
	mStringRevealBlendSamples = sharedAcousticP1
		? Math::Max(size_t(1), size_t(0.001f*float(sampleRate)*sourceTimeScale + 0.5f))
		: 0;
	const size_t contactBoundarySamples = sharedAcousticP1
		? Math::Max(size_t(1), size_t(0.0043f*float(sampleRate)*sourceTimeScale + 0.5f))
		: 0;
	mStringRevealSamples = contactBoundarySamples;

	const bool acousticMeasuredState = (tableId == 0);
	const bool measuredAttack = (tableId == 0 && region.RootKey <= 81);
	const size_t measuredOnsetSamples = acousticMeasuredState
		? Math::Max(size_t(1), size_t(region.DecayOnset*float(sampleRate)*sourceTimeScale + 0.5f))
		: stringRiseSamples;
	for(size_t p = 0; p < count; p++)
	{
		// Early-attack reconstruction starts before the packed complex state.
		// Rewind by exactly DecayOnset so the free recurrence reaches that
		// accepted state at the measurement boundary.
		float ci0 = cis[p], cr0 = crs[p];
		if(acousticMeasuredState)
		{
			const float back = dphis[p]*float(measuredOnsetSamples);
			const float cb = Math::Cos(back), sb = Math::Sin(back);
			ci0 = cis[p]*cb - crs[p]*sb;
			cr0 = crs[p]*cb + cis[p]*sb;
		}
		mS1[p] = ci0*c;
		mS2[p] = (ci0*Math::Cos(dphis[p]) + cr0*Math::Sin(dphis[p]))*c;
		mAmp[p] = 0.0f;
		// Keep the existing hidden state evolution. The harmonic output is gated
		// until mStringRevealSamples, so this rise is not an audible post-contact
		// ramp; it only keeps cache/seek state identical to the accepted baseline.
		mAtk[p] = (crs[p]*crs[p] + cis[p]*cis[p] > 0.0f) ? stringRiseStep : 0.0f;
	}

	// Experimental path: t=0 is the original sample onset. The measured
	// harmonic state is reached at the real DecayOnset, then the accepted
	// sustain Decay1..4 path resumes unchanged.
	mDecayOnsetSamples = measuredOnsetSamples;
	{
		const float onsetT = float(measuredOnsetSamples)/float(sampleRate);
		const float attackTauScale = sharedAcousticP1 ? sourceTimeScale : 1.0f;
		mAttackBasis0 = mAttackBasis1 = mAttackBasis2 = 1.0f;
		mAttackBasisEnd0 = Math::Exp(-onsetT/(0.010f*attackTauScale));
		mAttackBasisEnd1 = Math::Exp(-onsetT/(0.032f*attackTauScale));
		mAttackBasisEnd2 = Math::Exp(-onsetT/(0.140f*attackTauScale));
		mAttackBasisStep0 = Math::Exp(-1.0f/(0.010f*attackTauScale*float(sampleRate)));
		mAttackBasisStep1 = Math::Exp(-1.0f/(0.032f*attackTauScale*float(sampleRate)));
		mAttackBasisStep2 = Math::Exp(-1.0f/(0.140f*attackTauScale*float(sampleRate)));
	}
	// Segment durations are shared by every calibrated piano table. Keeping
	// them out of every region saves metadata without changing a single value.
	// After the measured onset the source is a transposed SF2 sample: the whole
	// body time axis scales by F0/freq. Decay rates already scale by freq/F0,
	// so scaling the segment boundaries too makes the body exactly compatible
	// with constant-rate region-PCM resampling. The measured attack itself stays
	// note-local and unchanged.
	mSegSamples = measuredOnsetSamples + size_t(0.235f*float(sampleRate)*sourceTimeScale + 0.5f);
	mSegSamples2 = measuredOnsetSamples + size_t((0.235f + 0.550f)*float(sampleRate)*sourceTimeScale + 0.5f);
	mSegSamples3 = measuredOnsetSamples + size_t((0.235f + 0.550f + 0.900f)*float(sampleRate)*sourceTimeScale + 0.5f);
	mRendered = 0;
	mDecayStarted = false;
	mSegSwitched = false;
	mSegSwitched2 = false;
	mSegSwitched3 = false;
	// Конец ноты на длине семпла (SF2 без лупа). Транспозиция: семпл,
	// сыгранный быстрее/медленнее, короче/длиннее в той же пропорции.
	{
		const float ratio = freq/region.F0;
		mEndSamples = ratio > 1e-6f
			? size_t(region.SampleLen/ratio*float(sampleRate) + 0.5f) : 0;
		mFadeSamples = Math::Max(size_t(1), size_t(0.02f*float(sampleRate)));
	}

	mScratch.SetCount(4*mBlockSize);
	mScratchR.SetCount(4*mBlockSize);
	mCount = count;
	// Per-instrument калибровка громкости: множитель на выходе всей ноты
	// (атака+сустейн+буферы). Отдельно от Scale — чтобы не трогать
	// нормировку атаки (буфер контактной силы от Scale не зависит).
	mVolume = volume*volumeScale*(acousticProgram0 ? gPianoSf2CommonLevel[best < 25 ? best : 0] : 1.0f);
	// Mid-register transient residual for Titanic root 75. The regenerated
	// per-partial attack table fixes spectral shape; this short common envelope
	// restores the remaining 5-80 ms energy without touching sustain.
	if(acousticProgram0 && region.RootKey == 75)
	{
		mAttackBoostPolyDepth = 0.40f;
		mAttackBoostSamples = Math::Max(size_t(1), size_t(0.090f*float(sampleRate) + 0.5f));
	}
	// Compact upper-register attack residual, fitted only on harmonic-band energy
	// of dry Titanic g=0.6. Full-band fitting is deliberately avoided here:
	// low-velocity high notes expose recorded ~50/100-Hz sample hum that is not
	// part of the physical string model. Depth/tau are physical source-region
	// properties; one shared velocity exponent is applied in SetVelocity().
	else if(acousticProgram0 && region.RootKey == 96)
	{
		// Root 96 has a short 40 ms pre-decay plateau rather than the 115 ms
		// exponential transient of roots 99+.  A quartic taper is the smallest
		// common-envelope fit that follows it without per-partial coefficients.
		mAttackBoostPolyDepth = 1.2269134f;
		mAttackBoostSamples = Math::Max(size_t(1), size_t(region.DecayOnset*float(sampleRate) + 0.5f));
	}
	else if(acousticProgram0 && region.RootKey >= 99)
	{
		float tau;
		if(region.RootKey == 99) { mAttackBoostDepth = 18.0731969f; tau = 0.03146143f; }
		else if(region.RootKey == 102) { mAttackBoostDepth = 10.3358593f; tau = 0.08746335f; }
		else { mAttackBoostDepth = 11.5f; tau = 0.02544526f; } // root 105
		mAttackBoostSamples = Math::Max(size_t(1), size_t(region.DecayOnset*float(sampleRate) + 0.5f));
		mAttackBoostStep = Math::Exp(-1.0f/(tau*float(sampleRate)));
		mAttackBoostEndExp = Math::Exp(-float(mAttackBoostSamples)/(tau*float(sampleRate)));
		mAttackBoostExp = 1.0f;
	}
	mDone = false;
	mReleased = false;
	mReleasePending = false;
	mReleaseAt = 0;
	mSampleRate = sampleRate;
	mSf2UniformRelease = sharedAcousticP1;
	mSf2ReleaseGain = 1.0f;
	mSf2ReleaseSamplesLeft = 0;
	if(mSf2UniformRelease)
	{
		// Titanic P1: preset +702 tc + instrument -386 tc = +316 tc.
		// FluidSynth's volEnv value itself ramps linearly 1 -> 0, but output
		// amplitude is cb2amp(960 * (1-volEnv)): exactly 96 dB of exponential
		// attenuation over the release duration. Quantize duration to FluidSynth's
		// 64-sample renderer buffer count.
		const float seconds = Math::Pow(2.0f, 316.0f/1200.0f);
		const size_t buffers = 1 + size_t(seconds*float(sampleRate)/64.0f);
		mSf2ReleaseSamples = Math::Max(size_t(1), buffers*size_t(64));
		mSf2ReleaseStep = Math::Exp(-11.05240845f/float(mSf2ReleaseSamples)); // ln(10)*4.8
	}
	else
	{
		mSf2ReleaseSamples = 0;
		mSf2ReleaseStep = 1.0f;
	}

	// Stereo ratio R/L is precomputed in the region table: the source values
	// are fixed, so note-on must not evaluate 10^(dB/20). Keep L+R = 1 to
	// preserve the previous linear-pan loudness.
	{
		const float ratio = region.StereoRatioRtoL;
		const float inv = 1.0f/(1.0f + ratio);
		mStereoGainL = inv;
		mStereoGainR = ratio*inv;
	}
}

void AdditiveSampler::ApplyRelease()
{
	if(mReleased) return;
	if(mVelocityModEnvCents != 0.0f)
	{
		const float attackSamples = 7.000704f*float(mSampleRate);
		mVelocityModReleaseLevel = Math::Min(1.0f, float(mRendered)/attackSamples);
		mVelocityModReleaseSample = mRendered;
		mVelocityModReleased = true;
		mVelocityModNextUpdate = mRendered;
	}
	mReleased = true;
	if(mSf2UniformRelease)
	{
		mSf2ReleaseGain = 1.0f;
		mSf2ReleaseSamplesLeft = mSf2ReleaseSamples;
		return;
	}
#ifdef INTRA_PROBE_NAN
	fprintf(stderr, "[RELEASE] mRendered=%zu mEndSamples=%zu mFadeSamples=%zu count=%zu\n", mRendered, mEndSamples, mFadeSamples, mCount);
#endif
	// Демпфер добавляется к естественному затуханию: dec[p] уже содержит
	// текущий естественный шаг (1.0 до DecayOnset, потом mDecay1/2/3/4),
	// домножаем его на шаг демпфера. Так release НИКОГДА не медленнее
	// естественного спада (fix C7 «отпустил — стало длиннее, чем держу»),
	// и тембр меняется плавно (√k-растекание). atk[p] обнуляем — разгон
	// атаки при демпфировании уже не нужен.
	const size_t count = mCount;
	float* dec = mDecay.Data();
	const float* decR = mDecayRelease.Data();
	float* atk = mAtk.Data();
	for(size_t p = 0; p < count; p++) { dec[p] *= decR[p]; atk[p] = 0.0f; }
	// mEndSamples не трогаем — нота закончится естественным путём,
	// когда amp[p] → 0 для всех партиал. mDone установится в RenderInto.
}

// «Окно свободной атаки»: если NoteOff пришёл раньше, чем струна успела
// сформировать атаку (< 35 мс), демпфер не применяется сразу — иначе
// ультракороткие ноты (5-30 мс) обрезались бы на полупустой амплитуде
// и звучали как щелчок вместо удара. Release откладывается до конца окна
// и применяется в RenderInto (см. mReleasePending). Ноты длиннее окна
// работают как раньше.
void AdditiveSampler::NoteRelease()
{
	if(mReleased || mReleasePending) return;
	// Publish the clean pre-release raw prefix, but never cache the damped tail.
	// Warm playback reconstructs live state analytically before applying the
	// normal damper, so short repeated notes still benefit from the cache.
	const size_t freeWindow = size_t(0.035f*float(mSampleRate));
	if(mRendered < freeWindow)
	{
		mReleasePending = true;
		mReleaseAt = freeWindow;
		return;
	}
	ApplyRelease();
}

size_t AdditiveSampler::GenerateMono(Span<float> ioDst)
{
	if(mDone) return 0;
	const size_t original = ioDst.Length();
	// Keep only one non-envelope hot-loop instantiation. Mono is an adapter
	// over the already-specialized stereo path; web playback is stereo, while
	// this removes an otherwise complete duplicate of RenderInto from WASM.
	float right[mBlockSize];
	while(!ioDst.Empty() && !mDone)
	{
		const size_t n = Math::Min(ioDst.Length(), mBlockSize);
		for(size_t i = 0; i < n; i++) right[i] = 0.0f;
		Span<float> left = ioDst.Take(n);
		GenerateStereo(left, Span<float>(right, n));
		float* dst = left.Data();
		for(size_t i = 0; i < n; i++) dst[i] += right[i];
		ioDst.PopFirstExactly(n);
	}
	return mDone ? 0 : original;
}

size_t AdditiveSampler::GenerateStereo(Span<float> ioDstLeft, Span<float> ioDstRight)
{
	if(mDone) return 0;
	const size_t original = Math::Min(ioDstLeft.Length(), ioDstRight.Length());
	ioDstLeft = ioDstLeft.Take(original); ioDstRight = ioDstRight.Take(original);
	// Region cache starts at source sample zero, so there is no separate
	// pre-cache live prefix. Keeping a second RenderInto lambda here would
	// instantiate the full additive hot loop twice in WASM.
	if(!ioDstLeft.Empty() && !mDone)
	{
		float* dstL = ioDstLeft.Data();
		float* dstR = ioDstRight.Data();
		RenderInto(ioDstLeft.Length(), [dstL, dstR](float l, float r) mutable
		{
			*dstL++ += l;
			*dstR++ += r;
		});
	}
	return mDone ? 0 : original;
}

INTRA_WARNING_POP
