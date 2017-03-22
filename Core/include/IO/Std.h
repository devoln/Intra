#pragma once

#include "Range/Polymorphic/InputRange.h"
#include "IO/FormattedWriter.h"

namespace Intra { namespace IO {

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
	OsFile mOutputFile; //Невладеющий хендл файла, привязанного к stdout, или null, если это не файл.
	OsFile mInputFile; //Невладеющий хендл файла, привязанного к stdin, или null, если это не файл.

	StdInOut(const StdInOut&) = delete;
	StdInOut& operator=(const StdInOut&) = delete;
};

extern StdInOut Std;
extern FormattedWriter StdErr;

}}
