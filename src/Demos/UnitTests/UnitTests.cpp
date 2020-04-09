#include "Intra/Core.h"

INTRA_DISABLE_REDUNDANT_WARNINGS

#include "Test/TestGroup.h"

#include "Extra/IO/FileSystem.h"
#include "Extra/IO/HtmlWriter.h"
#include "Extra/IO/FileWriter.h"
#include "Extra/IO/Std.h"
#include "Extra/IO/FilePath.h"

#include "Extra/System/DateTime.h"
#include "Extra/System/Signal.h"
#include "Extra/System/Environment.h"

#include "Extra/Container/SparseArray.h"
#include "Extra/IO/FormattedLogger.h"

#include "Extra/Container/HashMap.h"
#include "Extra/Concurrency/Thread.h"

#include "Sort.h"
#include "Range/Range.h"
#include "Extra/IO/IO.h"
#include "Serialization.h"

#include "Extra/Concurrency/Concurrency.h"

using namespace Intra;

FormattedWriter& InitOutput()
{
	static FormattedWriter logger;

	//Инициализация лога
	const StringView logFileName = "logs.html";
	const bool logExisted = OS.FileExists(logFileName);
	FileWriter logFile = OS.FileOpenAppend(logFileName, IgnoreErrors);
	if(logFile==null)
	{
		Std.PrintLine("Cannot open file ", logFileName, " for writing!");
	}
	if(!logExisted && logFile!=null) logFile.Print("<meta charset='utf-8'>\n<title>Logs</title>\n");
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

	StringView arg1 = argc>1? StringView(argv[1]): null;
	if(arg1.StartsWith("-"))
	{
		if(arg1.Contains('a'))
			TestGroup::YesForNestingLevel = 0;
		if(arg1.Contains('u'))
			output = null;
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
