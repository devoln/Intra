#include "Test/TestGroup.h"
#include "IO/LogSystem.h"
#include "Platform/CppWarnings.h"
#include "Platform/CppFeatures.h"
#include "Platform/Debug.h"
#include "IO/ConsoleOutput.h"
#include "IO/ConsoleInput.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra {

using namespace IO;

int TestGroup::nestingLevel=0;
int TestGroup::YesForNestingLevel=1000000000;
int TestGroup::totalTestsFailed = 0;
int TestGroup::totalTestsPassed = 0;

TestGroup::TestGroup(IO::FormattedWriter& logger, IO::FormattedWriter& output, StringView category):
	Yes(false), Logger(logger), Output(output), Category(category),
	mHadChildren(false),
	mFailedChildren(0), mPassedChildren(0)
{
	ErrorInfo = {null, null, 0, null, null};
	if(YesForNestingLevel<=nestingLevel) Yes = true;
	else consoleAskToEnableTest();
	if(Yes)
	{
		if(currentTestGroup!=null && !currentTestGroup->mHadChildren)
		{
			currentTestGroup->mHadChildren = true;
			Logger.BeginSpoiler(currentTestGroup->Category);
		}
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
	TestGroup(currentTestGroup==null? IO::ConsoleOut: currentTestGroup->Logger,
		currentTestGroup==null? IO::ConsoleOut: currentTestGroup->Output, category) {}

TestGroup::TestGroup(StringView category, const TestFunction& funcToTest):
	TestGroup(currentTestGroup==null? IO::ConsoleOut: currentTestGroup->Logger,
		currentTestGroup==null? IO::ConsoleOut: currentTestGroup->Output, category, funcToTest) {}

TestGroup::~TestGroup()
{
	nestingLevel--;
	currentTestGroup = mParentTestGroup;
	if(!Yes) return;

	if(mHadChildren) Logger.EndSpoiler();

	PrintUnitTestResult();
	if(mParentTestGroup!=null)
	{
		if(ErrorInfo.NoError())
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
	if(ErrorInfo.NoError() && mFailedChildren==0)
	{
		Logger.PushFont({0, 0, 1}, 3.0f, true, false, true);
		Logger.PrintLine("Test [ ", Category, " ] PASSED!");
		Logger.PopFont();
	}
	else
	{
		Logger.PushFont({1, 0, 0}, 3.0f, true, false, true);
		Logger.PrintLine("Test [ ", Category, " ] FAILED!");
		Logger.PopFont();

		Logger.HorLine();
		if(ErrorInfo.FullDesc!=null) Logger.PrintLine(ErrorInfo.FullDesc);
		if(mFailedChildren!=0)
			Logger.PrintLine(mFailedChildren, " of ", mFailedChildren+mPassedChildren, " subtests were FAILED!");
		Logger.HorLine();
	}
}

TestGroup* TestGroup::currentTestGroup = null;

}

INTRA_WARNING_POP
