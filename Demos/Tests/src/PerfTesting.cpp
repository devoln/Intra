
#include "Platform/Time.h"
#include "Platform/HardwareInfo.h"
#include "IO/Stream.h"
#include "IO/File.h"
#include "IO/FormattedWriter.h"
#include "IO/HtmlWriter.h"
#include "IO/ConsoleWriter.h"
#include "Algo/String/Path.h"
#include "Platform/CppWarnings.h"
#include "Test/PerfSummary.h"
#include "Container/String.h"
#include "Container/Array.h"
#include "Container/Map.h"
#include "Test/TestGroup.h"

#include "PerfTestSerialization.h"
#include "Range/Header.h"
#include "PerfTestRandom.h"
#include "PerfTestSort.h"

INTRA_DISABLE_REDUNDANT_WARNINGS


#ifdef _MSC_VER
#pragma warning(disable: 4350)
#endif

#include <stdlib.h>

using namespace Intra;
using namespace Intra::IO;


#ifndef INTRA_NO_FILE_LOGGING
DiskFile::Writer g_LogFile;
HtmlWriter g_LogWriter(&g_LogFile);
#endif

#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Emscripten)
HtmlWriter logConsoleWriter(&Console);
#else
ConsoleTextWriter logConsoleWriter(&Console);
#endif

CompositeFormattedWriter logger;
CompositeFormattedWriter emptyLogger;

void InitLogSystem(int argc, const char* argv[])
{
#ifndef INTRA_NO_LOGGING
#ifndef INTRA_NO_FILE_LOGGING
	//Инициализация лога
	const StringView logFileName = "logs.html";
	const bool logExisted = DiskFile::Exists(logFileName);
	g_LogFile = DiskFile::Writer(logFileName, true);
	if(!logExisted) g_LogWriter.RawPrint("<meta charset='utf-8'>\n<title>Logs</title>\n"+StringView(HtmlWriter::CssSpoilerCode));
	const String datetime = DateTime::Now().ToString();
	StringView appName = Algo::Path::ExtractName(StringView(argv[0]));

	String cmdline;
	for(int i=1; i<argc; i++)
	{
		cmdline += StringView(argv[i]);
		if(i+1<argc) cmdline+=' ';
	}

	g_LogWriter.BeginSpoiler(appName+' '+cmdline+' '+datetime);
	atexit([](){g_LogWriter.EndAllSpoilers(); g_LogFile=null;});
	

	Errors::CrashHandler=[](int signum)
	{
		g_LogWriter.PushFont({1, 0, 0}, 5, true);
		g_LogWriter.Print(Errors::CrashSignalDesc(signum));
		g_LogWriter.PopFont();
		g_LogWriter.EndAllSpoilers();
		g_LogFile=null;
	};

	cmdline=null;
	for(int i=0; i<argc; i++)
	{
		cmdline += StringView(argv[i]);
		if(i+1<argc) cmdline+='\n';
	}
	g_LogWriter << "Command line arguments:";
	g_LogWriter.PrintCode(cmdline);
	g_LogWriter.BeginSpoiler("System info");
	auto memInfo = SystemMemoryInfo::Get();
	auto procInfo = ProcessorInfo::Get();
	g_LogWriter.PrintCode(*String::Format(
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
	g_LogWriter.EndSpoiler();
	logger.Attach(&g_LogWriter);
#else
	(void)argc; (void)argv;
#endif
	logger.Attach(&logConsoleWriter);
#endif
}





#include "Container/Sequential/Array.h"
#include "Container/Associative/HashMap.h"
#include "Container/Sequential/List.h"

#if INTRA_DISABLED
#include <unordered_map>
#include <vector>
#endif

int main(int argc, const char* argv[])
{
	Errors::InitSignals();
	InitLogSystem(argc, argv);

#if(INTRA_PLATFORM_OS!=INTRA_PLATFORM_OS_Emscripten)
	if(argc>=2 && StringView(argv[1])=="-a")
#endif
	TestGroup::YesForNestingLevel=0;
	
	TestGroup(emptyLogger, logger, "Random number generation", RunRandomPerfTests);
	if(TestGroup gr{emptyLogger, logger, "Ranges"})
	{
		TestGroup("Polymorphic range performance", RunPolymorphicRangePerfTests);
	}
	TestGroup(emptyLogger, logger, "std::string vs String", RunStringPerfTests);
	TestGroup(emptyLogger, logger, "std::vector и std::deque vs Array", RunContainerPerfTests);
	TestGroup(emptyLogger, logger, "Associative containers", RunMapPerfTests);
	TestGroup(emptyLogger, logger, "Serialization and deserialization", RunSerializationPerfTests);
	TestGroup(emptyLogger, logger, "Sort algorithms", RunSortPerfTests);

	if(argc<2 || StringView(argv[1])!="-a")
	{
		Console.PrintLine("Press any key to exit...");
		Console.GetChar();
	}

	return 0;
}
