#pragma once

#include "Utils/Span.h"
#include "IO/FormattedWriter.h"
#include "Utils/StringView.h"

namespace Intra {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

void PrintPerformanceResults(IO::FormattedWriter& logger, StringView testName,
	CSpan<StringView> comparedTypes,
	CSpan<double> stdTimes,
	CSpan<double> times);

INTRA_WARNING_POP

}
