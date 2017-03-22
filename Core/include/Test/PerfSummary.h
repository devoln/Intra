#pragma once

#include "Range/Generators/ArrayRange.h"
#include "IO/FormattedWriter.h"
#include "Range/Generators/StringView.h"

namespace Intra {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

void PrintPerformanceResults(IO::FormattedWriter& logger, StringView testName,
	ArrayRange<const StringView> comparedTypes,
	ArrayRange<const double> stdTimes,
	ArrayRange<const double> times);

INTRA_WARNING_POP

}
