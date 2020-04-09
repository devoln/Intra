#pragma once

#include "FormattedWriter.h"
#include "Extra/Hash/ToHash.h"
#include "Extra/Unstable/Random/FastUniform.h"
#include "Intra/Range/Polymorphic/OutputRange.h"

INTRA_BEGIN
FormattedWriter HtmlWriter(OutputStream stream, bool addDefinitions=false);
INTRA_END
