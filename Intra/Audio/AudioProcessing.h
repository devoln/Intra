#pragma once

#include "Utils/Span.h"

namespace Intra { namespace Audio {

void DiscreteFourierTransform(Span<float> outFreqs, CSpan<short> samples);

}}
