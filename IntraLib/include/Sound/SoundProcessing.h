#pragma once

#include "Algorithms/Range.h"

namespace Intra {

void DiscreteFourierTransform(ArrayRange<float> outFreqs, ArrayRange<const short> samples);

}
