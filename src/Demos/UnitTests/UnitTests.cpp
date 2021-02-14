#include "Intra/Core.h"

INTRA_DISABLE_REDUNDANT_WARNINGS

#include "Test/TestGroup.h"

#include "IntraX/IO/FileSystem.h"
#include "IntraX/IO/HtmlWriter.h"
#include "IntraX/IO/FileWriter.h"
#include "IntraX/IO/Std.h"
#include "IntraX/IO/FilePath.h"

#include "IntraX/System/DateTime.h"
#include "IntraX/System/Signal.h"
#include "IntraX/System/Environment.h"

#include "IntraX/Container/SparseArray.h"
#include "IntraX/IO/FormattedLogger.h"

#include "IntraX/Container/HashMap.h"
#include "IntraX/Concurrency/Thread.h"

#include "Sort.h"
#include "Range/Range.h"
#include "IntraX/IO/IO.h"
#include "Serialization.h"

#include "IntraX/Concurrency/Concurrency.h"

using namespace Intra;

FormattedWriter& InitOutput()
{
	static FormattedWriter logger;

	//Инициализация лога
	const StringView logFileName = "logs.html";
	const bool logExisted = OS.FileExists(logFileName);
	FileWriter logFile = OS.FileOpenAppend(logFileName, IgnoreErrors);
	if(logFile==nullptr)
	{
		Std.PrintLine("Cannot open file ", logFileName, " for writing!");
	}
	if(!logExisted && logFile!=nullptr) logFile.Print("<meta charset='utf-8'>\n<title>Logs</title>\n");
	FormattedWriter logWriter = HtmlWriter(Move(logFile), !logExisted);

	String datetime;
	ToString(LastAppender(datetime), DateTime::Now());
	StringView appName = ExtractName(Environment.CommandLine.First());

	logWriter.BeginSpoiler(appName + StringOf(Environment.CommandLine.Drop(), " ", " ", " ") + datetime);

	logWriter.Print("Command line arguments:");
	logWriter.PrintCode(StringOf(Environment.CommandLine, "\n", " ", " "));
	logger.Attach(Move(logWriter));

	return logger;
}

int main(int argc, const char* argv[])
{
	InitSignals();
	auto& loggerOut = InitOutput();

	FormattedWriter output(&Std);

	StringView arg1 = argc>1? StringView(argv[1]): nullptr;
	if(arg1.StartsWith("-"))
	{
		if(arg1.Contains('a'))
			TestGroup::YesForNestingLevel = 0;
		if(arg1.Contains('u'))
			output = nullptr;
		if(arg1.Contains('s'))
			loggerOut.Attach(&Std);
	}
	else loggerOut.Attach(&Std);

	FormattedLogger logger{FormattedWriter(&loggerOut)};
	logger.WriteLevelType = false;

	if(TestGroup gr{&logger, output, "Ranges"})
	{
		TestGroup("Composing complex ranges", TestComposedRange);
		TestGroup("Polymorphic ranges", TestPolymorphicRange);
		TestGroup("Range-based streams", TestStreamRange);
		TestGroup("STL and ranges interoperability", TestRangeStlInterop);
		TestGroup("Unicode encoding conversions", TestUnicodeConversion);
		TestGroup("Heap algorithms", TestHeap);
	}
	if(TestGroup gr{&logger, output, "Containers"})
	{
		TestGroup("Sparse Range", TestSparseRange);
		TestGroup("Sparse Array", TestSparseArray);
		TestGroup("Map", TestMaps);
	}
	if(TestGroup gr{&logger, output, "IO"})
	{
		TestGroup("File", TestFileSyncIO);
#if !defined(INTRA_NO_CONCURRENCY) && !defined(INTRA_NO_NETWORKING) && INTRA_LIBRARY_THREAD != INTRA_LIBRARY_THREAD_None
		TestGroup("Socket", TestSocketIO);
		//TestGroup("HttpServer", TestHttpServer);
#endif
	}
	TestGroup(&logger, output, "Text serialization", TestTextSerialization);
	TestGroup(&logger, output, "Binary serialization", TestBinarySerialization);
	TestGroup(&logger, output, "Sort algorithms", TestSort);
#if !defined(INTRA_NO_CONCURRENCY) && INTRA_LIBRARY_THREAD != INTRA_LIBRARY_THREAD_None
	if(TestGroup gr{&logger, output, "Concurrency"})
	{
#if INTRA_LIBRARY_ATOMIC != INTRA_LIBRARY_ATOMIC_None
		TestGroup("Atomics", TestAtomics);
#endif
		TestGroup("Thread interruption", TestInterruption);
	}
#endif

	if(TestGroup::GetTotalTestsFailed() != 0)
		loggerOut.PushFont({1, 0, 0}, 5, true, false, true);
	else loggerOut.PushFont({0, 0.75f, 0}, 5, true, false, true);
	loggerOut.PrintLine("Tests passed: ", TestGroup::GetTotalTestsPassed(), "/", TestGroup::GetTotalTests(), ".");
	loggerOut.PopFont();

	return TestGroup::GetTotalTestsFailed();
}
