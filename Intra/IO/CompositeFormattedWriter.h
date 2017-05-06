#pragma once

#include "IO/FormattedWriter.h"
#include "Utils/Span.h"

namespace Intra { namespace IO {

class CompositeFormattedWriter: public FormattedWriter
{
public:
	CompositeFormattedWriter();

	CompositeFormattedWriter(CompositeFormattedWriter&& rhs): FormattedWriter(Cpp::Move(rhs)) {}

	CompositeFormattedWriter& operator=(CompositeFormattedWriter&& rhs)
	{
		FormattedWriter::operator=(Cpp::Move(rhs));
		return *this;
	}

	size_t Attach(FormattedWriter&& stream);
	size_t Attach(FormattedWriter* streamByRef);
	void Detach(size_t index);

	CompositeFormattedWriter& operator=(null_t)
	{
		FormattedWriter::operator=(null);
		return *this;
	}

private:
	CompositeFormattedWriter(FormattedWriter::Interface*);

	CompositeFormattedWriter(const CompositeFormattedWriter&) = delete;
	CompositeFormattedWriter& operator=(const CompositeFormattedWriter&) = delete;
};

}}
