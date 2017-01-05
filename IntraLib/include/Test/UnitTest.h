#pragma once

#include "Core/FundamentalTypes.h"
#include "Platform/CppWarnings.h"
#include "Platform/CppFeatures.h"

namespace Intra {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

struct UnitTest
{
	template<typename F> forceinline UnitTest(F f)
	{
		(void)f;
#ifdef INTRA_RUN_UNITTESTS
		f();
#endif
	}

	struct Dummy {template<typename F> forceinline F operator*(F rhs) {return rhs;}};

	static Dummy Set(const char* file, uint line, const char* name)
	{
		CurFile = file;
		CurLine = line;
		CurName = name;
		return Dummy();
	}

	static const char* CurFile;
	static uint CurLine;
	static const char* CurName;

};

INTRA_WARNING_POP

}

//#ifdef INTRA_RUN_UNITTESTS

#define INTRA_UNITTEST(name) \
	static const Intra::UnitTest INTRA_CONCATENATE_TOKENS(UNITTEST_instance_, __LINE__) = Intra::UnitTest::Set(__FILE__, __LINE__, (name)) * []()

/*#else

#define INTRA_UNITTEST(name) \
	static const Intra::UnitTest INTRA_CONCATENATE_TOKENS(UNITTEST_instance_, __LINE__) = []()

#endif*/

#define INTRA_TEST_ASSERT(expression) {(expression) || (INTRA_DEBUGGER_BREAKPOINT && \
    (INTRA_INTERNAL_ERROR(Intra::StringView(Intra::UnitTest::CurFile) + " (" + Intra::ToString(Intra::UnitTest::CurLine) + ")\n" +\
	Intra::StringView("\nUnitTest ") + Intra::StringView(Intra::UnitTest::CurName) + " FAILED!\n" # expression "\n"), true));}

#define INTRA_TEST_ASSERT_EQUALS(expr1, expr2) {auto e1 = (expr1); auto e2=(expr2);\
	e1==e2 || (INTRA_DEBUGGER_BREAKPOINT && \
    (Intra::InternalError("UnitTest", __FILE__, __LINE__, \
	Intra::StringView(Intra::UnitTest::CurFile) + " (" + Intra::ToString(Intra::UnitTest::CurLine) + ")\n" +\
	"\nUnitTest " + Intra::StringView(Intra::UnitTest::CurName) + " FAILED\n" # expr1\
	" != " # expr2 "\nwhere\n" # expr1 " = " + Intra::ToString(expr1) + "\n" # expr2 " = " + Intra::ToString(expr2)), true));}

