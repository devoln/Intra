#include "Audio/Synth/SineExpSynth.h"
#include "Audio/Synth/SineSynth.h"
#include "Audio/Synth/ExponentialAttenuation.h"
#include "Audio/Synth/PeriodicSynth.h"
#include "Utils/Span.h"
#include "Math/Simd.h"
#include "Algo/Mutation/Copy.h"
#include "Algo/Mutation/Fill.h"
#include "Container/Sequential/Array.h"
#include "Math/Random.h"
#include "Audio/AudioBuffer.h"

namespace Intra { namespace Audio { namespace Synth {

void FastSineExp(float volume, float coeff, float freq,
	uint sampleRate, Span<float> inOutSamples, bool add)
{
#if INTRA_DISABLED
	float phi0 = 0;
	float dphi = float(2*Math::PI*freq/sampleRate);
	float S0 = volume*Math::Sin(phi0);
	float S1 = volume*Math::Sin(dphi);
	float K = 2.0f*Math::Exp(-coeff/sampleRate)*Math::Cos(dphi);
	float b = Math::Exp(-2*coeff/sampleRate);
	auto ptr = inOutSamples.Begin;
	auto end = inOutSamples.end;

#if(INTRA_MINEXE<3)
#ifndef INTRA_USE_PDO
#elif INTRA_PLATFORM_ARCH==INTRA_PLATFORM_X86 || INTRA_PLATFORM_ARCH==INTRA_PLATFORM_X86_64
	float K4 = 2*Math::Exp(-4*coeff/sampleRate)*Math::Cos(4*dphi);
	float b4 = Math::Exp(-8*coeff/sampleRate);

	Simd::float4 vS0 = Simd::SetFloat4(
		volume*Math::Sin(phi0),
		volume*Math::Sin(phi0+dphi)*Math::Exp(-2*coeff/sampleRate),
		volume*Math::Sin(phi0+2*dphi)*Math::Exp(-4*coeff/sampleRate),
		volume*Math::Sin(phi0+3*dphi)*Math::Exp(-6*coeff/sampleRate));

	Simd::float4 vS1 = Simd::SetFloat4(volume*Math::Sin(4*dphi));
	Simd::float4 vK4 = Simd::SetFloat4(K4);
	Simd::float4 vb4 = Simd::SetFloat4(b4);

	if(!add) while(ptr<end-3)
	{
		Simd::GetU(ptr, vS1);
		ptr+=4;

		Simd::float4 newvS = Simd::Sub(Simd::Mul(vK4, vS1), vS0);
		vS0 = Simd::Mul(vS1, vb4);
		vS1 = newvS;
	}
	else while(ptr<end-3)
	{
		Simd::GetU(ptr, Simd::Add(Simd::SetU(ptr), vS1));
		ptr+=4;

		Simd::float4 newvS = Simd::Sub(Simd::Mul(vK4, vS1), vS0);
		vS0 = Simd::Mul(vS1, vb4);
		vS1 = newvS;
	}

	int i = 0;
	if(!add) while(ptr<end)
		*ptr++ = vS1.m128_f32[i++];
	else while(ptr<end)
		*ptr++ += vS1.m128_f32[i++];
#else

#endif
#endif

	if(!add) while(ptr<end)
	{
		*ptr++ = S1;

		const auto newS = K*S1-S0;
		S0 = S1*b;
		S1 = newS;
	}
	else while(ptr<end)
	{
		*ptr++ += S1;

		const auto newS = K*S1-S0;
		S0 = S1*b;
		S1 = newS;
	}
#endif
//#if INTRA_DISABLED
	const double samplesPerPeriod = float(sampleRate)/freq;
	const size_t count = GetGoodSignalPeriod(samplesPerPeriod, Math::Max(uint(freq/50), 5u));

	//Генерируем фрагмент, который будем повторять, пока не заполним буфер целиком
	const size_t sampleCount = uint(Math::Round(samplesPerPeriod*double(count)));
	const size_t N = (500/sampleCount+1);
	AudioBuffer sineFragment;
	sineFragment.SampleRate = sampleRate;
	sineFragment.Samples.SetCountUninitialized(sampleCount*N);
	PerfectSine(volume, freq, sampleRate, sineFragment.Samples(0, sampleCount), false);

	//Если этот фрагмент короче 500 семплов, то повторим его, 
	//там как экспоненциальное затухание эффективнее для больших массивов
	for(uint i=1; i<N; i++) sineFragment.CopyFrom(i*sampleCount, sampleCount, &sineFragment, 0);

	const float ek = Math::Exp(-coeff/float(sampleRate));
	float exp = 1.0f;
	if(!add) while(!inOutSamples.Empty())
		ExponentialAttenuate(inOutSamples, sineFragment.Samples, exp, ek);
	else while(!inOutSamples.Empty())
		ExponentialAttenuateAdd(inOutSamples, sineFragment.Samples, exp, ek);

	if(!add) ExponentialAttenuate(inOutSamples, sineFragment.Samples, exp, ek);
	else ExponentialAttenuateAdd(inOutSamples, sineFragment.Samples, exp, ek);
//#endif
}


struct SineExpParams
{
	byte Len;
	SineExpHarmonic Harmonics[20]; //Требуется, чтобы harmonics[0] имела наибольший lengthMultiplyer
};

static void SineExpSynthPassFunction(const SineExpParams& params,
	float freq, float volume, Span<float> inOutSamples, uint sampleRate, bool add)
{
	if(inOutSamples==null) return;
	size_t start = Math::Random<ushort>::Global(20);
	if(start>inOutSamples.Length()) start=inOutSamples.Length();
	if(!add) Algo::FillZeros(inOutSamples.Take(start));
	inOutSamples.PopFirstN(start);
	for(ushort h=0; h<params.Len; h++)
	{
		const SineExpHarmonic harm = params.Harmonics[h];
		const size_t samplesToProcess = size_t(float(inOutSamples.Length())*float(harm.LengthMultiplyer));
		FastSineExp(volume*float(harm.Scale), float(harm.AttenCoeff),
			freq*float(harm.FreqMultiplyer), sampleRate,
			inOutSamples.Take(samplesToProcess),
			add || h>0);
	}
	const bool allSamplesInitialized = add || params.Harmonics[0].LengthMultiplyer==norm8s(1);
	if(!allSamplesInitialized)
	{
		const size_t samplesProcessed = size_t(float(inOutSamples.Length())*float(params.Harmonics[0].LengthMultiplyer));
		Algo::FillZeros(inOutSamples.Drop(samplesProcessed));
	}
}

SynthPass CreateSineExpSynthPass(CSpan<SineExpHarmonic> harmonics)
{
	SineExpParams params;
	const auto src = harmonics.Take(Utils::LengthOf(params.Harmonics));
	params.Len = byte(src.Length());
	Algo::CopyTo(src, params.Harmonics);
	return SynthPass(SineExpSynthPassFunction, params);
}

}}}
