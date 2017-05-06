#include "Test/TestGroup.h"
#include "IO/LogSystem.h"
#include "Cpp/Warnings.h"
#include "Cpp/Features.h"
#include "Utils/Debug.h"
#include "IO/ConsoleOutput.h"
#include "IO/ConsoleInput.h"
#include "IO/Std.h"
#include "Utils/Logger.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra {

using namespace IO;

int TestGroup::nestingLevel=0;
int TestGroup::YesForNestingLevel=1000000000;
int TestGroup::totalTestsFailed = 0;
int TestGroup::totalTestsPassed = 0;

TestGroup::TestGroup(ILogger* logger, IO::FormattedWriter& output, StringView category):
	Yes(false), Logger(logger), Output(output), Category(category),
	mHadChildren(false),
	mFailedChildren(0), mPassedChildren(0)
{
	if(YesForNestingLevel<=nestingLevel) Yes = true;
	else consoleAskToEnableTest();
	if(Yes)
	{
		if(currentTestGroup!=null && !currentTestGroup->mHadChildren)
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
		dchar c = ConsoleIn.GetChar();
		if(c=='y')
		{
			YesForNestingLevel = nestingLevel+1;
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

TestGroup::TestGroup(StringView category):
	TestGroup(currentTestGroup==null? null: currentTestGroup->Logger,
		currentTestGroup==null? IO::Std: currentTestGroup->Output, category) {}

TestGroup::TestGroup(StringView category, const TestFunction& funcToTest):
	TestGroup(currentTestGroup==null? null: currentTestGroup->Logger,
		currentTestGroup==null? IO::Std: currentTestGroup->Output, category, funcToTest) {}

TestGroup::~TestGroup()
{
	nestingLevel--;
	currentTestGroup = mParentTestGroup;
	if(!Yes) return;

	PrintUnitTestResult();
	if(mParentTestGroup!=null)
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
	Output.EndSpoiler();
	INTRA_HEAP_CHECK;
}

void TestGroup::PrintUnitTestResult()
{
	if(ErrorInfo==null && mFailedChildren==0)
	{
		Output.PushFont({0, 0.75f, 0}, 3.0f, true);
		Output.PrintLine("Test [ ", Category, " ] PASSED!");
		Output.PopFont();
	}
	else
	{
		Output.PushFont({1, 0, 0}, 3.0f, true);
		Output.PrintLine("Test [ ", Category, " ] FAILED!");
		Output.PopFont();

		Output.HorLine();
		if(mFailedChildren!=0)
			Output.PrintLine(mFailedChildren, " of ", mFailedChildren+mPassedChildren, " subtests were FAILED!");
		Output.HorLine();
	}
}

TestGroup* TestGroup::currentTestGroup = null;

void TestGroup::processError(SourceInfo srcInfo, StringView msg)
{
	if(GetCurrent() == null)
	{
		FatalErrorMessageAbort(srcInfo, msg);
		return;
	}
	GetCurrent()->ErrorInfo = srcInfo;
	auto logger = GetCurrent()->Logger;
	if(logger)
	{
		logger->Error(msg, srcInfo);
		char stackTraceBuf[8192];
		auto buf = SpanOfBuffer(stackTraceBuf);
		logger->Error(GetStackTrace(buf, 2, 50), srcInfo);
	}
}

}

INTRA_WARNING_POP
