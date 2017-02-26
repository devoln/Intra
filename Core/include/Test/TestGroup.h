#pragma once

#include "Platform/CppWarnings.h"
#include "Platform/CppFeatures.h"
#include "IO/FormattedWriter.h"
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
		StringView category, const Utils::Delegate<void(IO::IFormattedWriter&)>& funcToTest):
		TestGroup(logger, output, category)
	{
		if(!Yes) return;
		auto oldCallback = gInternalErrorCallback;
		gInternalErrorCallback = internalErrorTestFail;
		tryCallTest(funcToTest);
		gInternalErrorCallback = oldCallback;
	}

	TestGroup(StringView category);
	TestGroup(StringView category, const Utils::Delegate<void(IO::IFormattedWriter&)>& funcToTest);
	~TestGroup();

	static TestGroup* GetCurrent() {return currentTestGroup;}
	void PrintUnitTestResult();

	static int GetTotalTestsFailed() {return totalTestsFailed;}


	static int YesForNestingLevel;
private:
	static TestGroup* currentTestGroup;
	static int nestingLevel;
	bool mHadChildren;
	int mFailedChildren, mPassedChildren;
	static int totalTestsFailed, totalTestsPassed;
	TestGroup& operator=(const TestGroup&) = delete;

	void consoleAskToEnableTest();

	struct TestException {};
	void tryCallTest(const Utils::Delegate<void(IO::IFormattedWriter&)>& funcToTest)
	{
	#ifdef INTRA_EXCEPTIONS_ENABLED
		try { //Исключение как способ безопасного выхода из ошибочного теста вместо падения всего приложения
	#endif
			funcToTest(Output);
	#ifdef INTRA_EXCEPTIONS_ENABLED
		} catch(TestException) {}
		catch(...) {ErrorInfo.FullDesc += "\nUnknown exception caught!";}
	#endif
	}

	static void internalErrorTestFail(StringView func, StringView file, int line, StringView info)
	{
		INTRA_ASSERT(GetCurrent() != null);
		GetCurrent()->ErrorInfo = TestInternalErrorInfo{func, file, line, info,
			BuildErrorMessage(func, file, line, info, 1)};
	#ifdef INTRA_EXCEPTIONS_ENABLED
		throw TestException();
	#else
		INTRA_HEAP_CHECK;
	#endif
	}
};

}

INTRA_WARNING_POP
