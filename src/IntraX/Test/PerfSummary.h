#pragma once

#include "Intra/Range/Span.h"
#include "IntraX/IO/FormattedWriter.h"
#include "Intra/Range/StringView.h"

namespace Intra { INTRA_BEGIN
void PrintPerformanceResults(FormattedWriter& logger, StringView testName,
	Span<const StringView> comparedTypes,
	Span<const double> stdTimes,
	Span<const double> times);
} INTRA_END
