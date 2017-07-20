#pragma once

#include "Utils/Span.h"

namespace Intra { namespace Audio {

void DiscreteFourierTransform(Span<float> outFreqs, CSpan<short> samples);

//void InplaceFFT(Span<cfloat> data);
//void InplaceInverseFFT(Span<cfloat> data);
void InplaceFFT(Span<float> real, Span<float> imag);
void InplaceInverseFFTNonNormalized(Span<float> real, Span<float> imag);
void InplaceInverseFFT(Span<float> real, Span<float> imag);

}}
