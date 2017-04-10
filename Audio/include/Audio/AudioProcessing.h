#pragma once

#include "Range/Generators/Span.h"

namespace Intra { namespace Audio {

void DiscreteFourierTransform(Span<float> outFreqs, CSpan<short> samples);

}}
