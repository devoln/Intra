#pragma once

#include "Range/Generators/Span.h"
#include "IO/FormattedWriter.h"
#include "Range/Generators/StringView.h"

namespace Intra {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

void PrintPerformanceResults(IO::FormattedWriter& logger, StringView testName,
	CSpan<StringView> comparedTypes,
	CSpan<double> stdTimes,
	CSpan<double> times);

INTRA_WARNING_POP

}
