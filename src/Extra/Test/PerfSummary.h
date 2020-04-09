#pragma once

#include "Intra/Range/Span.h"
#include "Extra/IO/FormattedWriter.h"
#include "Intra/Range/StringView.h"

INTRA_BEGIN
void PrintPerformanceResults(FormattedWriter& logger, StringView testName,
	CSpan<StringView> comparedTypes,
	CSpan<double> stdTimes,
	CSpan<double> times);
INTRA_END
