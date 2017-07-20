#include "Audio/AudioProcessing.h"
#include "Utils/Span.h"
#include "Math/Math.h"

#include "Range/Mutation/Transform.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Audio {
	
void DiscreteFourierTransform(Span<float> outFreqs, CSpan<short> samples)
{
    for(size_t i=0; i<outFreqs.Length(); i++)
    {
        const float wi = float(2*i*Math::PI/samples.Length());
        const float sii = Math::Sin(wi), coi = Math::Cos(wi);

        float co = 1, si = 0, acco = 0, acsi = 0;
        for(size_t j=0; j<samples.Length(); j += 2)
        {
            const float f = float(samples[j] + samples[j+1]);
            const float oco = co;
            acco += co*f;
			co = co*coi - si*sii;
            acsi += si*f;
			si = si*coi + oco*sii;
        }
        outFreqs[i] = Math::Sqrt(acco*acco + acsi*acsi);
    }
}

#if(INTRA_DISABLED)

static void rearrangeData(Span<cfloat> data)
{
    size_t targetIndex = 0;
	const size_t len = data.Length();
    for(size_t i = 0; i < len; i++)
    {
		if(targetIndex > i) Cpp::Swap(data.Begin[targetIndex], data.Begin[i]);

        size_t bitMask = len;
        while(targetIndex & (bitMask >>= 1))
            targetIndex &= ~bitMask;

        targetIndex |= bitMask;
    }
}

#endif

static void rearrangeData(Span<float> real, Span<float> imag)
{
	size_t targetIndex = 0;
	const size_t len = real.Length();
	for(size_t i = 0; i < len; i++)
	{
		if(targetIndex > i)
		{
			Cpp::Swap(real.Begin[targetIndex], real.Begin[i]);
			Cpp::Swap(imag.Begin[targetIndex], imag.Begin[i]);
		}

		size_t bitMask = len;
		while(targetIndex & (bitMask >>= 1))
			targetIndex &= ~bitMask;

		targetIndex |= bitMask;
	}
}

#if(INTRA_DISABLED)

// Принимает phase = -PI для прямого FFT и +PI для обратного.
static void makeTransform(Span<cfloat> data, float phase)
{
	float nextSine = 0;
    for(size_t i = 1; i < data.Length(); i <<= 1)
    {
        const size_t next = i << 1;
		const float sine = nextSine;
		phase *= 0.5f;
		nextSine = Math::Sin(phase);
		const cfloat mult = {-2*Math::Sqr(nextSine), sine};
        cfloat factor = 1;
        for(size_t j = 0; j < i; j++) // iterations through groups with different transform factors
        {
            for(size_t k = j; k < data.Length(); k += next) // iterations through pairs within group
            {
                const size_t match = k + i;
				const cfloat product = data[match] * factor;
				data[match] = data[k] - product;
				data[k] += product;
            }
            factor = mult*factor + factor;
        }
    }
}

#endif

// Принимает phase = -PI для прямого FFT и +PI для обратного.
static void makeTransform(Span<float> real, Span<float> imag, float phase)
{
	float nextSine = 0;
	for(size_t i = 1; i < real.Length(); i <<= 1)
	{
		const size_t next = i << 1;
		const float multImag = nextSine;
		phase *= 0.5f;
		nextSine = Math::Sin(phase);
		const float multReal = -2*Math::Sqr(nextSine);
		float factorReal = 1, factorImag = 0;
		for(size_t j = 0; j < i; j++) // iterations through groups with different transform factors
		{
			for(size_t k = j; k < real.Length(); k += next) // iterations through pairs within group
			{
				const size_t match = k + i;
				const float productReal = real[match] * factorReal - imag[match] * factorImag;
				const float productImag = imag[match] * factorReal + real[match] * factorImag;
				real[match] = real[k] - productReal;
				imag[match] = imag[k] - productImag;
				real[k] += productReal;
				imag[k] += productImag;
			}
			const float oldFactorReal = factorReal;
			factorReal = (multReal + 1)*factorReal - multImag*factorImag;
			factorImag = (multReal + 1)*factorImag + multImag*oldFactorReal;
		}
	}
}

#if(INTRA_DISABLED)

bool InplaceFFT(Span<cfloat> data)
{
	INTRA_DEBUG_ASSERT(Math::IsPow2(data.Length()));
	rearrangeData(data);
	makeTransform(data, -Math::PI);
}

bool InplaceInverseFFT(Span<cfloat> data)
{
	INTRA_DEBUG_ASSERT(Math::IsPow2(data.Length()));
	rearrangeData(data);
	makeTransform(data, Math::PI);
	Multiply(data.Reinterpret<float>(), 1.0f/data.Length());
}

#endif

void InplaceFFT(Span<float> real, Span<float> imag)
{
	INTRA_DEBUG_ASSERT(real.Length() == imag.Length());
	INTRA_DEBUG_ASSERT(Math::IsPow2(real.Length()));
	rearrangeData(real, imag);
	makeTransform(real, imag, float(-Math::PI));
}

void InplaceInverseFFTNonNormalized(Span<float> real, Span<float> imag)
{
	INTRA_DEBUG_ASSERT(real.Length() == imag.Length());
	INTRA_DEBUG_ASSERT(Math::IsPow2(real.Length()));
	rearrangeData(real, imag);
	makeTransform(real, imag, float(Math::PI));
}

void InplaceInverseFFT(Span<float> real, Span<float> imag)
{
	InplaceInverseFFTNonNormalized(real, imag);
	Multiply(imag, 1.0f/real.Length());
	Multiply(real, 1.0f/real.Length());
}



}}

INTRA_WARNING_POP
