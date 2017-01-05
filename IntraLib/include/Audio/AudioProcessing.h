#pragma once

#include "Range/ArrayRange.h"

namespace Intra { namespace Audio {

void DiscreteFourierTransform(ArrayRange<float> outFreqs, ArrayRange<const short> samples);

}}
