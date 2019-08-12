#pragma once


#include "Core/Core.h"

#include "Utils/Logger.h"
#include "Core/Assert.h"
#include "Utils/Finally.h"

#include "Funal/Delegate.h"

#include "IO/FormattedWriter.h"
#include "Container/Sequential/String.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

INTRA_BEGIN


typedef Delegate<void(IO::FormattedWriter&)> TestFunction;

class TestGroup
{
	TestGroup* mParentTestGroup;
public:
	bool Yes;
	ILogger* Logger;
	IO::FormattedWriter& Output;
	StringView Category;
	Utils::SourceInfo ErrorInfo{};

	operator bool() const {return Yes;}
	TestGroup(ILogger* logger, IO::FormattedWriter& output, StringView category);
	
	TestGroup(ILogger* logger, IO::FormattedWriter& output,
		StringView category, const TestFunction& funcToTest):
		TestGroup(logger, output, category)
	{
		if(!Yes) return;
		auto oldCallback = Utils::gFatalErrorCallback;
		Utils::gFatalErrorCallback = internalErrorTestFail;
		tryCallTest(funcToTest);
		Utils::gFatalErrorCallback = oldCallback;
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
			INTRA_WARNING_DISABLE_CATCH_ALL
		} catch(TestException) {}
		catch(...) {if(Logger) Logger->Error("Unknown exception caught!", INTRA_SOURCE_INFO);}
	#endif
	}

	static void internalErrorTestFail(const Utils::SourceInfo& srcInfo, StringView msg);

	static void testFailException()
	{
#ifdef __cpp_exceptions
		if(GetCurrent() != null)
		{
			GetCurrent()->Logger->Error("Test stack unwinding...");
			throw TestException();
		}
#endif
	}

	static void processError(const Utils::SourceInfo& srcInfo, StringView msg);
};

}

INTRA_WARNING_POP
