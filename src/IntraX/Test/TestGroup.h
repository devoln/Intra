#pragma once

#include "IntraX/Utils/Logger.h"
#include "Intra/Assert.h"

#include "IntraX/Utils/Delegate.h"

#include "IntraX/IO/FormattedWriter.h"
#include "IntraX/Container/Sequential/String.h"

namespace Intra { INTRA_BEGIN
using TestFunction = Delegate<void(FormattedWriter&)>;

class TestGroup
{
	TestGroup* mParentTestGroup;
public:
	bool Yes;
	ILogger* Logger;
	FormattedWriter& Output;
	StringView Category;
	SourceInfo ErrorInfo{};

	operator bool() const {return Yes;}
	TestGroup(ILogger* logger, FormattedWriter& output, StringView category);
	
	TestGroup(ILogger* logger, FormattedWriter& output,
		StringView category, const TestFunction& funcToTest):
		TestGroup(logger, output, category)
	{
		if(!Yes) return;
		auto oldCallback = FatalErrorCallback();
		FatalErrorCallback() = internalErrorTestFail;
		tryCallTest(funcToTest);
		FatalErrorCallback() = oldCallback;
	}

	TestGroup(StringView category);
	TestGroup(StringView category, const TestFunction& funcToTest);
	~TestGroup();

	static TestGroup* GetCurrent() {return currentTestGroup;}
	void PrintUnitTestResult();

	static int GetTotalTests() {return totalTestsPassed + totalTestsFailed;}
	static int GetTotalTestsPassed() {return totalTestsPassed;}
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
	void tryCallTest(const TestFunction& funcToTest)
	{
#ifdef __cpp_exceptions
		try { //Исключение как способ безопасного выхода из ошибочного теста вместо падения всего приложения
#endif
			funcToTest(Output);
#ifdef __cpp_exceptions
			INTRA_IGNORE_WARNS_MSVC(4571)
		} catch(TestException) {}
		catch(...) {if(Logger) Logger->Error("Unknown exception caught!", INTRA_SOURCE_INFO);}
#endif
	}

	[[noreturn]] static void internalErrorTestFail(DebugStringView msg, SourceInfo srcInfo);

	static void testFailException()
	{
#ifdef __cpp_exceptions
		if(GetCurrent() != nullptr)
		{
			GetCurrent()->Logger->Error("Test stack unwinding...");
			throw TestException();
		}
#endif
	}

	static void processError(SourceInfo srcInfo, StringView msg);
};
} INTRA_END
