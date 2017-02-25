#pragma once

#include "FormattedWriter.h"
#include "Math/Vector.h"

namespace Intra { namespace IO {

class ConsoleTextWriter: public PlainTextWriter
{
public:
	ConsoleTextWriter(IOutputStream* s): PlainTextWriter(s) {}

	void PushFont(Math::Vec3 color = {1,1,1}, float size=3,
		bool bold=false, bool italic=false, bool underline=false) override;

	void PopFont() override;

	FontDesc GetCurrentFont() const override
	{
		if(mFontStack.Empty()) return {Math::Vec3(-1), 3, false, false, false, false};
		return mFontStack.Last();
	}

private:
	ConsoleTextWriter(const ConsoleTextWriter&) = delete;
	ConsoleTextWriter& operator=(const ConsoleTextWriter&) = delete;
};

extern ConsoleTextWriter ConsoleWriter;

}}
