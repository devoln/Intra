#include "Platform/CppWarnings.h"
INTRA_DISABLE_REDUNDANT_WARNINGS

#include "Sort.h"
#include "Test/TestGroup.h"
#include "Range/Header.h"
#include "IO/FileSystem.h"
#include "IO/HtmlWriter.h"
#include "IO/FileWriter.h"
#include "IO/Std.h"
#include "IO/ReferenceFormattedWriter.h"
#include "IO/CompositeFormattedWriter.h"
#include "Algo/String/Path.h"
#include "Platform/Time.h"
#include "Platform/Errors.h"

#include "Serialization.h"

using namespace Intra;
using namespace IO;

CompositeFormattedWriter& InitLogSystem(int argc, const char* argv[])
{
	static CompositeFormattedWriter logger;
#ifndef INTRA_NO_LOGGING
#ifndef INTRA_NO_FILE_LOGGING
	//Инициализация лога
	const StringView logFileName = "logs.html";
	const bool logExisted = OS.FileExists(logFileName);
	FileWriter logFile = OS.FileOpenAppend(logFileName);
	if(!logExisted && logFile!=null) logFile.Print("<meta charset='utf-8'>\n<title>Logs</title>\n");
	FormattedWriter logWriter = HtmlWriter(Meta::Move(logFile), !logExisted);

	const String datetime = DateTime::Now().ToString();
	StringView appName = Algo::Path::ExtractName(StringView(argv[0]));

	String cmdline;
	for(int i=1; i<argc; i++)
	{
		cmdline += StringView(argv[i]);
		if(i+1<argc) cmdline+=' ';
	}

	logWriter.BeginSpoiler(appName+' '+cmdline+' '+datetime);
	
	Errors::CrashHandler = [](int signum)
	{
		logger.PushFont({1, 0, 0}, 5, true);
		logger.PrintLine(Errors::CrashSignalDesc(signum));
		logger.PopFont();
	};

	cmdline = null;
	for(int i=0; i<argc; i++)
	{
		cmdline += StringView(argv[i]);
		if(i+1<argc) cmdline+='\n';
	}
	logWriter << "Command line arguments:";
	logWriter.PrintCode(cmdline);
	logger.Attach(Meta::Move(logWriter));
#else
	(void)argc; (void)argv;
#endif
#endif
	return logger;
}

int main(int argc, const char* argv[])
{
	Errors::InitSignals();
	auto& logger = InitLogSystem(argc, argv);

	CompositeFormattedWriter emptyLogger;
	FormattedWriter* output = &Std;

	StringView arg1 = argc>1? StringView(argv[1]): null;
	if(Algo::StartsWith(arg1, "-"))
	{
		if(Algo::Contains(arg1, 'a'))
			TestGroup::YesForNestingLevel = 0;
		if(Algo::Contains(arg1, 'u'))
			output = &emptyLogger;
		if(!Algo::Contains(arg1, 's'))
			logger.Attach(&Std);
	}
	else logger.Attach(&Std);

	if(TestGroup gr{logger, *output, "Ranges"})
	{
		TestGroup("Composing complex ranges", TestComposedRange);
		TestGroup("Polymorphic ranges", TestPolymorphicRange);
		TestGroup("Range-based streams", TestStreamRange);
		TestGroup("STL and ranges interoperability", TestRangeStlInterop);
		TestGroup("Unicode encoding conversions", TestUnicodeConversion);
	}
	TestGroup(logger, *output, "Text serialization", TestTextSerialization);
	TestGroup(logger, *output, "Binary serialization", TestBinarySerialization);
	TestGroup(logger, *output, "Sort algorithms", TestSort);
	return TestGroup::GetTotalTestsFailed();
}
