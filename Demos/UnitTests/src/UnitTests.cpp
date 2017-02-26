#include "Platform/CppWarnings.h"
INTRA_DISABLE_REDUNDANT_WARNINGS

#include "Sort.h"
#include "Test/TestGroup.h"
#include "Range/Header.h"
#include "IO/File.h"
#include "IO/HtmlWriter.h"
#include "IO/ConsoleWriter.h"
#include "IO/CompositeFormattedWriter.h"
#include "Algo/String/Path.h"
#include "Platform/Time.h"

#include "Serialization.h"


INTRA_PUSH_DISABLE_ALL_WARNINGS
#include <stdlib.h>
INTRA_WARNING_POP

using namespace Intra;
using namespace IO;

#ifndef INTRA_NO_FILE_LOGGING
DiskFile::Writer g_LogFile;
HtmlWriter g_LogWriter(&g_LogFile);
#endif

CompositeFormattedWriter gLogger;

void InitLogSystem(int argc, const char* argv[])
{
#ifndef INTRA_NO_LOGGING
#ifndef INTRA_NO_FILE_LOGGING
	//Инициализация лога
	const StringView logFileName = "logs.html";
	const bool logExisted = DiskFile::Exists(logFileName);
	g_LogFile = DiskFile::Writer(logFileName, true);
	if(!logExisted) g_LogWriter.RawPrint("<meta charset='utf-8'>\n<title>Logs</title>\n" + 
		StringView(HtmlWriter::CssSpoilerCode));
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
		g_LogWriter.PrintLine(Errors::CrashSignalDesc(signum));
		g_LogWriter.PopFont();
		g_LogFile.Flush();
	};

	cmdline=null;
	for(int i=0; i<argc; i++)
	{
		cmdline += StringView(argv[i]);
		if(i+1<argc) cmdline+='\n';
	}
	g_LogWriter << "Command line arguments:";
	g_LogWriter.PrintCode(cmdline);
#else
	(void)argc; (void)argv;
#endif
#endif
}

int main(int argc, const char* argv[])
{
	Errors::InitSignals();
	InitLogSystem(argc, argv);
	gLogger.Attach(&g_LogWriter);

	CompositeFormattedWriter emptyLogger;
	IFormattedWriter* output = &ConsoleWriter;

	if(argc>=2 && Algo::StartsWith(StringView(argv[1]), "-"))
	{
		if(Algo::Contains(StringView(argv[1]), 'a'))
			TestGroup::YesForNestingLevel = 0;
		if(Algo::Contains(StringView(argv[1]), 'u'))
			output = &emptyLogger;
		if(!Algo::Contains(StringView(argv[1]), 's'))
			gLogger.Attach(&ConsoleWriter);
	}
	else gLogger.Attach(&ConsoleWriter);

	if(TestGroup gr{gLogger, *output, "Ranges"})
	{
		TestGroup("Composing complex ranges", TestComposedRange);
		TestGroup("Polymorphic ranges", TestPolymorphicRange);
		TestGroup("Range-based streams", TestStreamRange);
		TestGroup("STL and ranges interoperability", TestRangeStlInterop);
		TestGroup("Unicode encoding conversions", TestUnicodeConversion);
	}
	TestGroup(gLogger, *output, "Text serialization", TestTextSerialization);
	TestGroup(gLogger, *output, "Binary serialization", TestBinarySerialization);
	TestGroup(gLogger, *output, "Sort algorithms", TestSort);
	return TestGroup::GetTotalTestsFailed();
}
