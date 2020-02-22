#pragma once

#include "Core/Range/Polymorphic/InputRange.h"
#include "IO/FormattedWriter.h"

#include "IO/OsFile.h"

INTRA_BEGIN
class StdInOut: public FormattedWriter, public InputStream
{
public:
	struct ConstructOnce;
	StdInOut(struct ConstructOnce);

	bool IsConsoleOutput() const {return mIsConsoleOutput;}
	bool IsConsoleInput() const {return mIsConsoleInput;}

private:
	bool mIsConsoleOutput;
	bool mIsConsoleInput;
	OsFile mOutputFile; //A non-owning file handle bound to stdout or null, if it's not a file.
	OsFile mInputFile; //A non-owning file handle bound to stdin or null, if it's not a file.

	StdInOut(const StdInOut&) = delete;
	StdInOut& operator=(const StdInOut&) = delete;
};

extern StdInOut Std;
extern FormattedWriter StdErr;
INTRA_END
