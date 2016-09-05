#include "Core/Time.h"
#include "Platform/HardwareInfo.h"
#include "IO/Stream.h"
#include "IO/File.h"
#include "IO/LogSystem.h"

#include "Test/PerformanceTest.h"
#include "PerfTestString.h"
#include "PerfTestArray.h"
#include "PerfTestMap.h"
#include "PerfTestSerialization.h"
#include "PerfTestRanges.h"
#include "PerfTestRandom.h"
#include "PerfTestSort.h"

#include <stdlib.h>

using namespace Intra;
using namespace Intra::IO;
using ::size_t;


DiskFile::Writer logFile;
HtmlWriter logWriter(&logFile);
ConsoleTextWriter logConsoleWriter(&Console);
Logger logger;

void InitLogSystem(int argc, const char* argv[])
{
#ifndef INTRA_NO_LOGGING
	//Инициализация лога
	const StringView logFileName = "logs.html";
	const bool logExisted = DiskFile::Exists(logFileName);
	logFile = DiskFile::Writer(logFileName, true);
	if(!logExisted) logWriter.RawPrint("<meta charset='utf-8'>\n<title>Логи</title>\n"+StringView(HtmlWriter::CssSpoilerCode));
	const String datetime = ToString(DateTime::Now());
	StringView appName = DiskFile::ExtractName(StringView(argv[0]));

	String cmdline;
	for(int i=1; i<argc; i++)
	{
		cmdline += StringView(argv[i]);
		if(i+1<argc) cmdline+=' ';
	}

	logWriter.BeginSpoiler(appName+' '+cmdline+' '+datetime, "Закрыть лог "+datetime);
	atexit([](){logWriter.EndAllSpoilers(); logFile=null;});
	
	logger.Attach(&logWriter);
	logger.Attach(&logConsoleWriter);

	Errors::CrashHandler=[](int signum)
	{
		logWriter.PushFont({1, 0, 0}, 5);
		logWriter.PushBold();
		logWriter.Print(Errors::CrashSignalDesc(signum));
		logWriter.PopBold();
		logWriter.PopFont();
		logWriter.EndAllSpoilers();
		logFile=null;
	};

	cmdline=null;
	for(int i=0; i<argc; i++)
	{
		cmdline += StringView(argv[i]);
		if(i+1<argc) cmdline+='\n';
	}
	logWriter << "Параметры командной строки:";
	logWriter.PrintCode(cmdline);
	logWriter.BeginSpoiler("Информация о системе", "[Скрыть] Информация о системе");
	auto memInfo = SystemMemoryInfo::Get();
	auto procInfo = ProcessorInfo::Get();
	logWriter.PrintCode(*String::Format("Процессор: <^>\r\n"
		"Свободно физической памяти: <^> ГиБ из <^> ГиБ\r\n")
		(procInfo.BrandString)
		(memInfo.FreePhysicalMemory/float(1 << 30))
		(memInfo.TotalPhysicalMemory/float(1 << 30))
	);
	logWriter.EndSpoiler();
#endif
}






#include "Containers/HashMap.h"
#include "Containers/List.h"
#include <unordered_map>
#include <vector>

void main(int argc, const char* argv[])
{
	Errors::InitSignals();
	InitLogSystem(argc, argv);

	if(argc>=2 && StringView(argv[1])=="-a")
        TestGroup::YesForNestingLevel=0;

#if INTRA_DISABLED
    HashMap<String, int> map;
	map["Строка"] = 6;
	map["Тест"] = 4;
	map["Вывод"] = 5;
	map["Ассоциативного"] = 14;
	map["Массива"] = 7;
	map["В отладчике"] = 11;
 
	auto mapRange = map.Find("Вывод");
	mapRange.PopLast();
 
	Console << map() << endl;

	map.SortByKey();
	Console << map() << endl;
#endif


	if(argc>=2 && StringView(argv[1])=="-a")
		TestGroup::YesForNestingLevel=0;

	if(TestGroup gr{logger, "Диапазоны"})
		RunRangeTests();

	if(TestGroup gr{logger, "Генераторы случайных чисел"})
		RunRandomPerfTests(logger);

	if(TestGroup gr{logger, "std::string vs String"})
		RunStringPerfTests(logger);

	if(TestGroup gr{logger, "std::vector и std::deque vs Array"})
		RunContainerPerfTests(logger);

	if(TestGroup gr{logger, "Ассоциативные контейнеры"})
		RunMapPerfTests(logger);

	if(TestGroup gr{logger, "Сериализация и десериализация"})
		RunSerializationPerfTests(logger);

	if(TestGroup gr{logger, "Сортировка"})
		RunSortPerfTests(logger);

	if(argc<2 || StringView(argv[1])!="-a") system("pause");
}
