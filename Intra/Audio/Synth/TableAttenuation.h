#pragma once

#include "Math/FixedPoint.h"
#include "Utils/Span.h"
#include "Types.h"

namespace Intra { namespace Audio { namespace Synth {

AttenuationPass CreateTableAttenuationPass(CSpan<norm8> table);

}}}
