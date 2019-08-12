#pragma once

#include "Core/Range/Polymorphic/InputRange.h"
#include "IO/FormattedWriter.h"

#include "IO/OsFile.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

INTRA_BEGIN
namespace IO {

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
	OsFile mOutputFile; //����������� ����� �����, ������������ � stdout, ��� null, ���� ��� �� ����.
	OsFile mInputFile; //����������� ����� �����, ������������ � stdin, ��� null, ���� ��� �� ����.

	StdInOut(const StdInOut&) = delete;
	StdInOut& operator=(const StdInOut&) = delete;
};

extern StdInOut Std;
extern FormattedWriter StdErr;

}}

INTRA_WARNING_POP
