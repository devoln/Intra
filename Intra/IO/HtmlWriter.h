#pragma once

#include "FormattedWriter.h"
#include "Hash/ToHash.h"
#include "Random/FastUniform.h"
#include "Range/Polymorphic/OutputRange.h"

namespace Intra { namespace IO {

FormattedWriter HtmlWriter(OutputStream stream, bool addDefinitions=false);

}}
