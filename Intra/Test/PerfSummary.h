#pragma once

#include "Core/Range/Span.h"
#include "IO/FormattedWriter.h"
#include "Core/Range/StringView.h"

INTRA_BEGIN


INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

void PrintPerformanceResults(IO::FormattedWriter& logger, StringView testName,
	CSpan<StringView> comparedTypes,
	CSpan<double> stdTimes,
	CSpan<double> times);

INTRA_WARNING_POP

}
