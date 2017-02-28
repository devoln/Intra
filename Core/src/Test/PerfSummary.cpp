#include "Test/PerfSummary.h"
#include "IO/FormattedWriter.h"
#include "Platform/CppWarnings.h"
#include "Platform/Debug.h"

namespace Intra {

using namespace Range;
using namespace IO;

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

void PrintPerformanceResults(IFormattedWriter& logger, StringView testName, ArrayRange<const StringView> comparedTypes,
	ArrayRange<const double> set2Times, ArrayRange<const double> set1Times)
{
	INTRA_DEBUG_ASSERT_EQUALS(comparedTypes.Length(), set1Times.Length()+set2Times.Length());
	static const Math::Vec3 set1Color = {0, 0, 0.75f},
		goodSet1Color = {0, 0.25f, 1.0f},
		set2Color = {0.75f, 0, 0},
		goodSet2Color = {1, 0.25f, 0};
	
	logger.PrintLine();

	logger.PushFont(Math::Vec3(-1), -1, false, false, true);
	logger.PrintLine("В тесте \"", testName, '"');
	logger.PopFont();

	for(size_t i=0; i<set2Times.Length(); i++)
	{
		for(size_t j=0; j<set1Times.Length(); j++)
		{
			const bool faster = (set1Times[j]<set2Times[i]);
			const double timesBetter = faster? set2Times[i]/set1Times[j]: set1Times[j]/set2Times[i];

			logger.PushFont(faster? set1Color: set2Color, 3, true, false, false);
			logger << comparedTypes[faster? set2Times.Length()+j: i];
			logger.PopFont();

			logger << " быстрее, чем ";

			logger.PushFont(!faster? set1Color: set2Color, 3, true, false, false);
			logger << comparedTypes[faster? i: set2Times.Length()+j];
			logger.PopFont();

			logger << ", в ";
			logger.PushFont(!faster?
				(timesBetter>1.25? goodSet2Color: set2Color):
				(timesBetter>1.25? goodSet1Color: set1Color),
				3, true, false, timesBetter>2);
			logger << ToString(timesBetter, 2);
			logger.PopFont();
			logger.PrintLine(" раз(а).");
		}
	}

	logger.PrintLine("Время (мс):");
	String timeStr;
	for(size_t i=0; i<comparedTypes.Length(); i++)
	{
		bool isSet2Time = i<set2Times.Length();
		double time = isSet2Time? set2Times[i]: set1Times[i-set2Times.Length()];
		logger.PushFont(isSet2Time? set2Color: set1Color, 3, true);
		logger << comparedTypes[i];
		logger.PopFont();
		logger.PrintLine(" - ", ToString(time*1000, 2));
	}

	logger.PrintLine();
}




INTRA_WARNING_POP

}
