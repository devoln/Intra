#pragma once

#include "Range/ArrayRange.h"

namespace Intra {

void DiscreteFourierTransform(ArrayRange<float> outFreqs, ArrayRange<const short> samples);

}
