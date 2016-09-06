#include "Test/PerformanceTest.h"
#include "Algorithms/RangeIteration.h"
#include "IO/LogSystem.h"

namespace Intra {

using namespace Range;
using namespace IO;


void PrintPerformanceResults(Logger& logger, StringView testName, ArrayRange<const StringView> comparedTypes,
	ArrayRange<const double> otherTimes, ArrayRange<const double> times)
{
	INTRA_ASSERT(comparedTypes.Length() == otherTimes.Length()+times.Length());
	static const Math::Vec3 intraColor = {0, 0, 0.75f},
		goodIntraColor = {0, 0.25f, 1.0f},
		otherColor = {0.75f, 0, 0},
		goodOtherColor = {1, 0.25f, 0};
	
	logger.PrintLine();

	logger.PushUnderline();
	logger.PrintLine("В тесте \"", testName, '"');
	logger.PopUnderline();

	for(size_t i=0; i<otherTimes.Length(); i++)
	{
		for(size_t j=0; j<times.Length(); j++)
		{
			const bool faster = (times[j]<otherTimes[i]);
			const double timesBetter = faster? otherTimes[i]/times[j]: times[j]/otherTimes[i];

			logger.PushBold();
			if(faster) logger.PushFont(intraColor);
			else logger.PushFont(otherColor);
			logger << comparedTypes[faster? otherTimes.Length()+j: i];
			logger.PopFont();
			logger.PopBold();

			logger << " быстрее, чем ";

			logger.PushBold();
			if(!faster) logger.PushFont(intraColor);
			else logger.PushFont(otherColor);
			logger << comparedTypes[faster? i: otherTimes.Length()+j];
			logger.PopFont();
			logger.PopBold();

			logger << ", в ";
			logger.PushBold();
			if(timesBetter>2) logger.PushUnderline();
			if(!faster) logger.PushFont(timesBetter>1.25? goodOtherColor: otherColor);
			else logger.PushFont(timesBetter>1.25? goodIntraColor: intraColor);
			logger << ToString(timesBetter, 2);
			logger.PopFont();
			if(timesBetter>2) logger.PopUnderline();
			logger.PopBold();
			logger.PrintLine(" раз(а).");
		}
	}

	logger.PrintLine("Время (мс):");
	String timeStr;
	for(size_t i=0; i<comparedTypes.Length(); i++)
	{
		bool isOtherTime = i<otherTimes.Length();
		double time = isOtherTime? otherTimes[i]: times[i-otherTimes.Length()];
		logger.PushBold();
		logger.PushFont(isOtherTime? otherColor: intraColor);
		logger << comparedTypes[i];
		logger.PopFont();
		logger.PopBold();
		logger.PrintLine(" - ", ToString(time*1000, 2));
	}

	logger.PrintLine();
}


int TestGroup::nestingLevel=0;
int TestGroup::YesForNestingLevel=1000000000;

TestGroup::TestGroup(Logger& Log, StringView category): logger(Log)
{
	if(YesForNestingLevel<=nestingLevel) yes = true;
	else
	{
		Console.PrintLine("Нажмите пробел для теста [ "+category+" ] или enter, чтобы пропустить тест...");
		for(;;)
		{
			dchar c = Console.GetChar();
			if(c=='y')
			{
				YesForNestingLevel = nestingLevel+1;
				yes = true;
				break;
			}
			if(c=='a')
			{
				YesForNestingLevel = nestingLevel;
				yes = true;
				break;
			}
			if(c==' ')
			{
				yes = true;
				break;
			}
			if(c=='\r' || c=='n')
			{
				yes = false;
				break;
			}
		}
	}
	if(yes) logger.BeginSpoiler(category, "Скрыть [ "+category+" ]");
	else logger << "Тест [ "+category+" ] пропущен!" << endl;
	nestingLevel++;
}

TestGroup::~TestGroup() {if(yes) logger.EndSpoiler(); nestingLevel--;}

}

