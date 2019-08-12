#include "Test/TestGroup.h"

#include "Core/Assert.h"
#include "Utils/Logger.h"


#include "Core/Core.h"

#include "IO/ConsoleOutput.h"
#include "IO/ConsoleInput.h"
#include "IO/Std.h"

#include <cstdlib>


INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

INTRA_BEGIN


using namespace IO;

int TestGroup::nestingLevel = 0;
int TestGroup::YesForNestingLevel = 1000000000;
int TestGroup::totalTestsFailed = 0;
int TestGroup::totalTestsPassed = 0;

TestGroup::TestGroup(ILogger* logger, IO::FormattedWriter& output, StringView category):
	Yes(false), Logger(logger), Output(output), Category(category),
	mHadChildren(false),
	mFailedChildren(0), mPassedChildren(0)
{
	if(YesForNestingLevel <= nestingLevel) Yes = true;
	else consoleAskToEnableTest();
	if(Yes)
	{
		if(currentTestGroup != null && !currentTestGroup->mHadChildren)
			currentTestGroup->mHadChildren = true;
		Output.BeginSpoiler(Category);
	}
	else Output.PrintLine("Test [ ", Category, " ] skipped!");
	nestingLevel++;

	mParentTestGroup = currentTestGroup;
	currentTestGroup = this;
}

void TestGroup::consoleAskToEnableTest()
{
	ConsoleOut.PrintLine("Press space to run test [ ", Category, " ] or press enter to skip it...");
	for(;;)
	{
		char32_t c = ConsoleIn.GetChar();
		if(c=='y')
		{
			YesForNestingLevel = nestingLevel + 1;
			Yes = true;
			break;
		}
		if(c=='a')
		{
			YesForNestingLevel = nestingLevel;
			Yes = true;
			break;
		}
		if(c==' ')
		{
			Yes = true;
			break;
		}
		if(c=='\r' || c=='\n') break;
	}
}

inline FormattedWriter& getOutput(TestGroup* currentTestGroup)
{
	if(currentTestGroup == null) return static_cast<IO::FormattedWriter&>(IO::Std);
	return currentTestGroup->Output;
}

TestGroup::TestGroup(StringView category):
	TestGroup(currentTestGroup == null? null: currentTestGroup->Logger,
		getOutput(currentTestGroup), category) {}

TestGroup::TestGroup(StringView category, const TestFunction& funcToTest):
	TestGroup(currentTestGroup == null? null: currentTestGroup->Logger,
		getOutput(currentTestGroup), category, funcToTest) {}

TestGroup::~TestGroup()
{
	nestingLevel--;
	currentTestGroup = mParentTestGroup;
	if(!Yes) return;

	PrintUnitTestResult();
	if(mParentTestGroup != null)
	{
		if(ErrorInfo == null)
		{
			mParentTestGroup->mPassedChildren++;
			totalTestsPassed++;
		}
		else
		{
			mParentTestGroup->mFailedChildren++;
			totalTestsFailed++;
		}
	}
	else if(mPassedChildren == 0 && mFailedChildren == 0)
	{
		if(ErrorInfo == null) totalTestsPassed++;
		else totalTestsFailed++;
	}
	Output.EndSpoiler();
	INTRA_HEAP_CHECK;
}

void TestGroup::PrintUnitTestResult()
{
	if(Logger == null) return;
	if(ErrorInfo == null && mFailedChildren == 0)
		Logger->Success("Test [ " + Category + " ] PASSED!");
	else
	{
		Logger->Error("Test [ " + Category + " ] FAILED!");

		if(mFailedChildren != 0)
			Logger->Error(String::Concat(mFailedChildren, " of ", mFailedChildren + mPassedChildren, " subtests FAILED!"));
	}
}

TestGroup* TestGroup::currentTestGroup = null;

void TestGroup::processError(const Utils::SourceInfo& srcInfo, StringView msg)
{
	if(GetCurrent() == null)
	{
		Utils::FatalErrorMessageAbort(srcInfo, msg);
		return;
	}
	GetCurrent()->ErrorInfo = srcInfo;
	auto logger = GetCurrent()->Logger;
	if(logger)
	{
		logger->Error(msg, srcInfo);
		char stackTraceBuf[8192];
		auto buf = SpanOfBuffer(stackTraceBuf);
		logger->Error(Utils::GetStackTrace(buf, 3, 50), {});
	}
}

void TestGroup::internalErrorTestFail(const Utils::SourceInfo& srcInfo, StringView msg)
{
	static bool alreadyCalled = false;
	if(alreadyCalled) abort();
	alreadyCalled = true;
	INTRA_FINALLY{alreadyCalled = false;};
	processError(srcInfo, msg);
	testFailException();
	exit(totalTestsFailed + 1);
}

}

INTRA_WARNING_POP
