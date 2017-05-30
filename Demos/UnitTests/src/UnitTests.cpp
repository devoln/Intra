#include "Cpp/Warnings.h"
INTRA_DISABLE_REDUNDANT_WARNINGS

#include "Test/TestGroup.h"

#include "IO/FileSystem.h"
#include "IO/HtmlWriter.h"
#include "IO/FileWriter.h"
#include "IO/Std.h"
#include "IO/ReferenceFormattedWriter.h"
#include "IO/CompositeFormattedWriter.h"
#include "IO/FilePath.h"

#include "Platform/Time.h"
#include "Platform/Signal.h"
#include "Platform/Environment.h"

#include "Container/SparseArray.h"
#include "IO/FormattedLogger.h"

#include "Sort.h"
#include "Range/Range.h"
#include "IO/IO.h"
#include "Serialization.h"

using namespace Intra;
using namespace IO;

CompositeFormattedWriter& InitOutput()
{
	static CompositeFormattedWriter logger;

	//Инициализация лога
	const StringView logFileName = "logs.html";
	const bool logExisted = OS.FileExists(logFileName);
	FileWriter logFile = OS.FileOpenAppend(logFileName);
	if(logFile==null)
	{
		Std.PrintLine("Cannot open file ", logFileName, " for writing!");
	}
	if(!logExisted && logFile!=null) logFile.Print("<meta charset='utf-8'>\n<title>Logs</title>\n");
	FormattedWriter logWriter = HtmlWriter(Cpp::Move(logFile), !logExisted);

	String datetime;
	ToString(LastAppender(datetime), DateTime::Now());
	StringView appName = IO::Path::ExtractName(CommandLineArguments.First());

	logWriter.BeginSpoiler(appName + StringOf(CommandLineArguments.Drop(), " ", " ", " ") + datetime);
	
	Errors::CrashHandler = [](int signum)
	{
		logger.PushFont({1, 0, 0}, 5, true);
		logger.PrintLine(Errors::CrashSignalDesc(signum));
		logger.PopFont();
	};

	logWriter << "Command line arguments:";
	logWriter.PrintCode(StringOf(CommandLineArguments, "\n", " ", " "));
	logger.Attach(Cpp::Move(logWriter));

	return logger;
}

int main(int argc, const char* argv[])
{
	Errors::InitSignals();
	auto& loggerOut = InitOutput();

	FormattedWriter output = ReferenceFormattedWriter(Std);

	StringView arg1 = argc>1? StringView(argv[1]): null;
	if(arg1.StartsWith("-"))
	{
		if(arg1.Contains('a'))
			TestGroup::YesForNestingLevel = 0;
		if(arg1.Contains('u'))
			output = CompositeFormattedWriter();
		if(arg1.Contains('s'))
			loggerOut.Attach(&Std);
	}
	else loggerOut.Attach(&Std);

	FormattedLogger logger(ReferenceFormattedWriter(loggerOut));

	if(TestGroup gr{&logger, output, "Ranges"})
	{
		TestGroup("Composing complex ranges", TestComposedRange);
		TestGroup("Polymorphic ranges", TestPolymorphicRange);
		TestGroup("Range-based streams", TestStreamRange);
		TestGroup("STL and ranges interoperability", TestRangeStlInterop);
		TestGroup("Unicode encoding conversions", TestUnicodeConversion);
	}
	if(TestGroup gr{&logger, output, "Containers"})
	{
		TestGroup("Sparse Range", TestSparseRange);
		TestGroup("Sparse Array", TestSparseArray);
	}
	if(TestGroup gr{&logger, output, "IO"})
	{
		TestGroup("File", TestFileSyncIO);
		TestGroup("Socket", TestSocketIO);
		//TestGroup("HttpServer", TestHttpServer);
	}
	TestGroup(&logger, output, "Text serialization", TestTextSerialization);
	TestGroup(&logger, output, "Binary serialization", TestBinarySerialization);
	TestGroup(&logger, output, "Sort algorithms", TestSort);

	if(TestGroup::GetTotalTestsFailed() != 0)
		loggerOut.PushFont({1, 0, 0}, 5, true, false, true);
	else loggerOut.PushFont({0, 0.75f, 0}, 5, true, false, true);
	loggerOut.PrintLine("Tests passed: ", TestGroup::GetTotalTestsPassed(), "/", TestGroup::GetTotalTests(), ".");
	loggerOut.PopFont();

	return 0;
}
