#pragma once

#include "Core/Range/Span.h"

INTRA_BEGIN
//void InplaceFFT(Span<cfloat> data);
//void InplaceInverseFFT(Span<cfloat> data);
void InplaceFFT(Span<float> real, Span<float> imag);
void InplaceInverseFFTNonNormalized(Span<float> real, Span<float> imag);
void InplaceInverseFFT(Span<float> real, Span<float> imag);
INTRA_END
