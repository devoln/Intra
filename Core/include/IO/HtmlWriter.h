#pragma once

#include "FormattedWriter.h"
#include "Algo/Hash/ToHash.h"
#include "Math/Random.h"
#include "Range/Polymorphic/OutputRange.h"

namespace Intra { namespace IO {

FormattedWriter HtmlWriter(OutputStream stream, bool addDefinitions=false);

}}
