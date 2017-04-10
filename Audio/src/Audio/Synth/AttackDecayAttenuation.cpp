#include "Audio/Synth/AttackDecayAttenuation.h"
#include "Math/Math.h"
#include "Math/Simd.h"
#include "Range/Generators/Span.h"

#define OPTIMIZE


namespace Intra { namespace Audio { namespace Synth {

static void AttackDecayPassFunction(const AttackDecayParams& params,
	float noteDuration, Span<float> inOutSamples, uint sampleRate)
{
	const double halfAttackTime = Math::Min(noteDuration*0.5, params.AttackTime)*0.5;
	const double halfDecayTime = Math::Min(noteDuration*0.5, params.DecayTime)*0.5;
	const uint halfAttackSamples = uint(halfAttackTime*sampleRate);
	const uint attackSampleEnd = halfAttackSamples*2;
	const double decayTimeBegin = noteDuration-halfDecayTime*2;
	const uint halfDecaySamples = uint(halfDecayTime*sampleRate);
	const uint decaySampleBegin = uint(decayTimeBegin*sampleRate);

	float* ptr = inOutSamples.Begin;
	const float* const endHalfAttack = inOutSamples.Begin+halfAttackSamples;
	const float* const endAttack = inOutSamples.Begin+attackSampleEnd;
	float* const beginDecay = inOutSamples.Begin+decaySampleBegin;
	const float* const beginHalfDecay = beginDecay+halfDecaySamples;

#if !defined(OPTIMIZE)
	float u = 0;
	float du = 0.707107f/halfAttackSamples;
	while(ptr<endHalfAttack)
	{
		*ptr++ *= u*u;
		u += du;
	}

	u = 0;
	du = 0.25f/halfAttackSamples;
	while(ptr<endAttack)
	{
		*ptr++ *= Math::Sqrt(u)+0.5f;
		u += du;
	}

	ptr=beginDecay;
	u = 0.25f;
	du = -0.25f/halfDecaySamples;
	while(ptr<beginHalfDecay)
	{
		*ptr++ *= Math::Sqrt(u)+0.5f;
		u += du;
	}

	u = 0.707107f;
	du = -0.707107f/halfDecaySamples;
	while(ptr<inOutSamples.End)
	{
		*ptr++ *= u*u;
		u += du;
	}
#elif(INTRA_SIMD_SUPPORT==INTRA_SIMD_NONE)
	float u = 0;
	float du = 0.707107f/float(halfAttackSamples);
	float du4 = 4*du;
	while(ptr<endHalfAttack-3)
	{
		const float u2 = u*u;
		*ptr++ *= u2;
		*ptr++ *= u2;
		*ptr++ *= u2;
		*ptr++ *= u2;
		u += du4;
	}
	while(ptr<endHalfAttack) *ptr++ *= u*u, u += du;

	u = 0;
	du = 0.25f/float(halfAttackSamples);
	du4 = 4*du;
	while(ptr<endAttack-3)
	{
		float u2 = Math::Sqrt(u)+0.5f;
		*ptr++ *= u2;
		*ptr++ *= u2;
		*ptr++ *= u2;
		*ptr++ *= u2;
		u += du4;
	}
	while(ptr<endAttack)
	{
		*ptr++ *= Math::Sqrt(u)+0.5f;
		u += du;
	}

	ptr = beginDecay;
	u = 0.25f;
	du = -0.25f/float(halfDecaySamples);
	du4 = 4.0f*du;
	while(ptr<beginHalfDecay-3)
	{
		float u2 = Math::Sqrt(u)+0.5f;
		*ptr++ *= u2;
		*ptr++ *= u2;
		*ptr++ *= u2;
		*ptr++ *= u2;
		u += du4;
	}
	while(ptr<beginHalfDecay) *ptr++ *= Math::Sqrt(u)+0.5f, u+=du;

	u = 0.707107f;
	du = -0.707107f/float(halfDecaySamples);
	du4 = 4.0f*du;
	while(ptr<inOutSamples.End-3)
	{
		float u2 = u*u;
		*ptr++ *= u2;
		*ptr++ *= u2;
		*ptr++ *= u2;
		*ptr++ *= u2;
		u += du4;
	}
	while(ptr<inOutSamples.End)
	{
		*ptr++ *= u*u;
		u += du;
	}
#else
	//Атака
	
	//Первая половина атаки
	float du = 0.707107f/halfAttackSamples;
	Simd::float4 u4 = Simd::SetFloat4(0, du, 2*du, 3*du);
	Simd::float4 du4 = Simd::SetFloat4(4*du);
	while(ptr<endHalfAttack-3)
	{
		Simd::float4 v = Simd::SetFloat4U(ptr);
		Simd::float4 res = Simd::Mul(u4, u4);
		Simd::GetU(ptr, Simd::Mul(v, res));
		u4 = Simd::Add(u4, du4);
		ptr += 4;
	}
	while(ptr<endHalfAttack)
	{
		const float u_x = Simd::GetX(u4);
		*ptr++ *= u_x*u_x;
		u4 = Simd::SetFloat4(u_x+du);
	}

	//Вторая половина атаки
	du = 0.25f/halfAttackSamples;
	u4 = Simd::SetFloat4(0, du, 2*du, 3*du);
	du4 = Simd::SetFloat4(4*du);
	Simd::float4 half = Simd::SetFloat4(0.5f);
	while(ptr<endAttack-3)
	{
		Simd::float4 v = Simd::SetFloat4U(ptr);
		Simd::float4 res = Simd::Add(Simd::Sqrt(u4), half);
		Simd::GetU(ptr, Simd::Mul(v, res));
		u4 = Simd::Add(u4, du4);
		ptr+=4;
	}
	float u4x = Simd::GetX(u4);
	while(ptr<endAttack)
	{
		*ptr++ *= Math::Sqrt(u4x)+0.5f;
		u4x += du;
	}


	//Спад
	ptr = beginDecay;

	//Первая половина спада
	float u = 0.25f;
	du = -0.25f/halfDecaySamples;
	//u4 = Simd::SetFloat4(0.5f, 0.5f-du/2, 0.5f-du, 0.5f-3*du/2);
	u4 = Simd::SetFloat4(u, u+du, u+2*du, u+3*du);
	du4 = Simd::SetFloat4(4*du);
	while(ptr<beginHalfDecay-3)
	{
		Simd::float4 v = Simd::SetFloat4U(ptr);
		Simd::float4 Sqrt = Simd::Sqrt(u4);
		Simd::float4 res = Simd::Add(Sqrt, half);
		Simd::GetU(ptr, Simd::Mul(v, res));
		u4 = Simd::Add(u4, du4);
		ptr+=4;
	}
	float u4vals[4]; Simd::GetU(u4vals, u4);
	float u4w = u4vals[3];
	while(ptr<beginHalfDecay)
	{
		*ptr++ *= Math::Sqrt(u4w)+0.5f;
		u4w += du;
	}

	//Вторая половина спада
	u = 0.707107f;
	du = -0.707107f/halfDecaySamples;
	u4 = Simd::SetFloat4(u, u+du, u+2*du, u+3*du);
	du4 = Simd::SetFloat4(4*du);
	while(ptr<inOutSamples.End-3)
	{
		Simd::float4 v = Simd::SetFloat4U(ptr);
		Simd::float4 res = Simd::Mul(u4, u4);
		Simd::GetU(ptr, Simd::Mul(v, res));
		u4 = Simd::Add(u4, du4);
		ptr += 4;
	}
	u4x = Simd::GetX(u4);
	while(ptr<inOutSamples.End) *ptr++ *= u4x*u4x;
#endif
}

AttenuationPass CreateAttackDecayPass(double attackTime, double decayTime)
{return AttenuationPass(AttackDecayPassFunction, AttackDecayParams{attackTime, decayTime});}

}}}
