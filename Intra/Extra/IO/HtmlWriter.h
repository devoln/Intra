#pragma once

#include "FormattedWriter.h"
#include "Hash/ToHash.h"
#include "Math/Random/FastUniform.h"
#include "Core/Range/Polymorphic/OutputRange.h"

INTRA_BEGIN
FormattedWriter HtmlWriter(OutputStream stream, bool addDefinitions=false);
INTRA_END
