#pragma once

#include "FormattedWriter.h"
#include "IntraX/Hash/ToHash.h"
#include "IntraX/Unstable/Random/FastUniform.h"
#include "Intra/Range/Polymorphic/OutputRange.h"

INTRA_BEGIN
FormattedWriter HtmlWriter(OutputStream stream, bool addDefinitions=false);
INTRA_END
