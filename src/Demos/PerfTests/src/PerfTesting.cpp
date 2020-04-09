

#include "Extra/Utils/Logger.h"

#include "Extra/System/DateTime.h"
#include "Extra/System/Environment.h"
#include "Extra/System/Stopwatch.h"
#include "Extra/System/ProcessorInfo.h"
#include "Extra/System/RamInfo.h"
#include "Extra/System/Signal.h"

#include "Extra/IO/FileSystem.h"
#include "Extra/IO/FormattedWriter.h"
#include "Extra/IO/HtmlWriter.h"
#include "Extra/IO/FileWriter.h"
#include "Extra/IO/ConsoleInput.h"
#include "Extra/IO/ConsoleOutput.h"
#include "Extra/IO/FilePath.h"
#include "Extra/IO/FormattedLogger.h"

#include "Test/PerfSummary.h"
#include "Test/TestGroup.h"

#include "Extra/Container/String.h"
#include "Extra/Container/Array.h"
#include "Extra/Container/Map.h"

#include "PerfTestSerialization.h"
#include "Intra/Range/Header.h"
#include "PerfTestRandom.h"
#include "PerfTestSort.h"

INTRA_DISABLE_REDUNDANT_WARNINGS


#ifdef _MSC_VER
#pragma warning(disable: 4350)
#endif

#include <stdlib.h>

using namespace Intra;
using namespace IO;

FormattedWriter& InitOutput()
{
	static FormattedWriter logger;
#ifndef INTRA_NO_LOGGING
#ifndef INTRA_NO_FILE_LOGGING
	//Инициализация лога
	const StringView logFileName = "logs.html";
	const bool logExisted = OS.FileExists(logFileName);
	FileWriter logFile = OS.FileOpenAppend(logFileName, Error::Skip());
	logFile.Print("<meta charset='utf-8'>\n<title>Logs</title>\n");
	FormattedWriter logWriter = HtmlWriter(Move(logFile), !logExisted);
	String datetime;
	ToString(LastAppender(datetime), System::DateTime::Now());
	StringView appName = IO::Path::ExtractName(System::Environment.CommandLine.First());

	logWriter.BeginSpoiler(appName+StringOf(System::Environment.CommandLine.Drop(), " ", " ", " ")+datetime);
	

	System::CrashHandler = [](int signum)
	{
		logger.PushFont({1, 0, 0}, 5, true);
		logger.Print(System::CrashSignalDesc(signum));
		logger.PopFont();
		logger = null;
	};

	logWriter << "Command line arguments:";
	logWriter.PrintCode(StringOf(System::Environment.CommandLine, "\n", "", ""));
	logWriter.BeginSpoiler("System info");
	auto memInfo = System::RamInfo::Get();
	auto procInfo = System::ProcessorInfo::Get();
	logWriter.PrintCode(*String::Format(
		"CPU:\r\n<^>\r\n"
		"Cores: <^>\r\n"
		"Logical cores: <^>\r\n"
		"Frequency: <^> MHz\r\n\r\n"
		"Free physical memory: <^> GiB of <^> GiB\r\n")
		(procInfo.BrandString)
		(procInfo.CoreNumber)
		(procInfo.LogicalProcessorNumber)
		(double(procInfo.Frequency)/1000000.0, 1)
		(double(memInfo.FreePhysicalMemory)/double(1 << 30), 2)
		(double(memInfo.TotalPhysicalMemory)/double(1 << 30), 2)
	);
	logWriter.EndSpoiler();
	logger.Attach(Move(logWriter));
#endif
	logger.Attach(ConsoleOutput());
#endif
	return logger;
}


int main()
{
	System::InitSignals();
	auto& output = InitOutput();

	if(System::Environment.CommandLine.Get(1, null) == "-a")
		TestGroup::YesForNestingLevel=0;
	
	TestGroup(null, output, "Random number generation", RunRandomPerfTests);
	if(TestGroup gr{null, output, "Ranges"})
	{
		TestGroup("Polymorphic range performance", RunPolymorphicRangePerfTests);
	}
	TestGroup(null, output, "std::string vs String", RunStringPerfTests);
	TestGroup(null, output, "std::vector и std::deque vs Array", RunContainerPerfTests);
	TestGroup(null, output, "Associative containers", RunMapPerfTests);
	TestGroup(null, output, "Serialization and deserialization", RunSerializationPerfTests);
	TestGroup(null, output, "Sort algorithms", RunSortPerfTests);

	if(System::Environment.CommandLine.Get(1, null) != "-a")
	{
		ConsoleOut.PrintLine("Press any key to exit...");
		ConsoleIn.GetChar();
	}

	return 0;
}
