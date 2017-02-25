#pragma once

#include "Platform/CppWarnings.h"
#include "IO/LogSystem.h"
#include "Utils/Delegate.h"
#include "Platform/Debug.h"
#include "Container/Sequential/String.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra {

struct TestInternalErrorInfo
{
	StringView Function;
	StringView File;
	int Line;
	String Info;

	String FullDesc;

	bool NoError() const {return Function==null && File==null && Line==0 && Info==null && FullDesc==null;}
};

class TestGroup
{
	TestGroup* mParentTestGroup;
public:
	bool Yes;
	IO::IFormattedWriter& Logger;
	IO::IFormattedWriter& Output;
	StringView Category;
	TestInternalErrorInfo ErrorInfo;

	operator bool() const {return Yes;}
	TestGroup(IO::IFormattedWriter& logger, IO::IFormattedWriter& output, StringView category);
	TestGroup(IO::IFormattedWriter& logger, IO::IFormattedWriter& output,
		StringView category, const Utils::Delegate<void(IO::IFormattedWriter&)>& functionToTest);
	TestGroup(StringView category);
	TestGroup(StringView category, const Utils::Delegate<void(IO::IFormattedWriter&)>& functionToTest);
	~TestGroup();

	static TestGroup* GetCurrent() {return currentTestGroup;}
	void PrintUnitTestResult();

	static int YesForNestingLevel;
private:
	static TestGroup* currentTestGroup;
	static int nestingLevel;
	bool mHadChildren;
	int mFailedChildren, mPassedChildren;
	TestGroup& operator=(const TestGroup&) = delete;

	void consoleAskToEnableTest();
};

}

INTRA_WARNING_POP
