#pragma once

#include "Core/Range/Span.h"
#include "IO/FormattedWriter.h"
#include "Core/Range/StringView.h"

INTRA_BEGIN


INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

void RunUnitTests(IO::FormattedWriter& logger);

INTRA_WARNING_POP

}
