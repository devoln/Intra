#pragma once

#include "Core/Range/Span.h"
#include "IO/FormattedWriter.h"
#include "Core/Range/StringView.h"

INTRA_BEGIN
void PrintPerformanceResults(FormattedWriter& logger, StringView testName,
	CSpan<StringView> comparedTypes,
	CSpan<double> stdTimes,
	CSpan<double> times);
INTRA_END
