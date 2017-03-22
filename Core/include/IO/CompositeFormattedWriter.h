#pragma once

#include "IO/FormattedWriter.h"
#include "Range/Generators/ArrayRange.h"

namespace Intra { namespace IO {

class CompositeFormattedWriter: public FormattedWriter
{
public:
	CompositeFormattedWriter();

	CompositeFormattedWriter(CompositeFormattedWriter&& rhs): FormattedWriter(Meta::Move(rhs)) {}

	CompositeFormattedWriter& operator=(CompositeFormattedWriter&& rhs)
	{
		FormattedWriter::operator=(Meta::Move(rhs));
		return *this;
	}

	size_t Attach(FormattedWriter&& stream);
	void Detach(size_t index);

private:
	CompositeFormattedWriter(FormattedWriter::Interface*);

	CompositeFormattedWriter(const CompositeFormattedWriter&) = delete;
	CompositeFormattedWriter& operator=(const CompositeFormattedWriter&) = delete;
};

}}
