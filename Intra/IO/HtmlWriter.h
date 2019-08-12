#pragma once

#include "FormattedWriter.h"
#include "Hash/ToHash.h"
#include "Random/FastUniform.h"
#include "Core/Range/Polymorphic/OutputRange.h"

INTRA_BEGIN
namespace IO {

FormattedWriter HtmlWriter(OutputStream stream, bool addDefinitions=false);

}}
