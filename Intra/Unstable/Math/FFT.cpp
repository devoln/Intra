#include "FFT.h"
#include "Math.h"

#include "Core/Functional.h"
#include "Core/Range/Span.h"
#include "Core/Range/Mutation/Copy.h"
#include "Core/Range/Map.h"

INTRA_BEGIN
#if(INTRA_DISABLED)

static void rearrangeData(Span<cfloat> data)
{
    size_t targetIndex = 0;
	const size_t len = data.Length();
    for(size_t i = 0; i < len; i++)
    {
		if(targetIndex > i) Swap(data.Begin[targetIndex], data.Begin[i]);

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
			Swap(real.Begin[targetIndex], real.Begin[i]);
			Swap(imag.Begin[targetIndex], imag.Begin[i]);
		}

		size_t bitMask = len;
		while(targetIndex & (bitMask >>= 1))
			targetIndex &= ~bitMask;

		targetIndex |= bitMask;
	}
}

#if(INTRA_DISABLED)

// Takes phase = -PI for forward FFT and +PI for inverse FFT.
static void makeTransform(Span<cfloat> data, float phase)
{
	float nextSine = 0;
    for(size_t i = 1; i < data.Length(); i <<= 1)
    {
        const size_t next = i << 1;
		const float sine = nextSine;
		phase *= 0.5f;
		nextSine = Sin(phase);
		const cfloat mult = {-2*Sqr(nextSine), sine};
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

// Takes phase = -PI for forward FFT and +PI for inverse FFT.
static void makeTransform(Span<float> real, Span<float> imag, float phase)
{
	float nextSine = 0;
	for(size_t i = 1; i < real.Length(); i <<= 1)
	{
		const size_t next = i << 1;
		const float multImag = nextSine;
		phase *= 0.5f;
		nextSine = Sin(phase);
		const float multReal = -2*Sqr(nextSine);
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
	INTRA_PRECONDITION(IsPow2(data.Length()));
	rearrangeData(data);
	makeTransform(data, -Constants.PI);
}

bool InplaceInverseFFT(Span<cfloat> data)
{
	INTRA_PRECONDITION(IsPow2(data.Length()));
	rearrangeData(data);
	makeTransform(data, Constants.PI);
	Multiply(data.Reinterpret<float>(), 1.0f/data.Length());
}

#endif

void InplaceFFT(Span<float> real, Span<float> imag)
{
	INTRA_PRECONDITION(real.Length() == imag.Length());
	INTRA_PRECONDITION(IsPow2(real.Length()));
	rearrangeData(real, imag);
	makeTransform(real, imag, float(-Constants.PI));
}

void InplaceInverseFFTNonNormalized(Span<float> real, Span<float> imag)
{
	INTRA_PRECONDITION(real.Length() == imag.Length());
	INTRA_PRECONDITION(IsPow2(real.Length()));
	rearrangeData(real, imag);
	makeTransform(real, imag, float(Constants.PI));
}

void InplaceInverseFFT(Span<float> real, Span<float> imag)
{
	InplaceInverseFFTNonNormalized(real, imag);
	const auto normalize = Bind(FMul, 1.0f/float(real.Length()));
	// TODO: use Simd
	CopyTo(Map(imag, normalize), imag);
	CopyTo(Map(real, normalize), real);
}
INTRA_END
