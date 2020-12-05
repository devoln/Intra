#include "PerfSummary.h"
#include "IntraX/IO/FormattedWriter.h"

#include "Intra/Assert.h"

INTRA_BEGIN
void PrintPerformanceResults(FormattedWriter& logger, StringView testName, CSpan<StringView> comparedTypes,
	CSpan<double> set2Times, CSpan<double> set1Times)
{
	INTRA_DEBUG_ASSERT_EQUALS(comparedTypes.Length(), set1Times.Length() + set2Times.Length());
	static const Vec3 set1Color = {0, 0, 0.75f},
		goodSet1Color = {0, 0.25f, 1.0f},
		set2Color = {0.75f, 0, 0},
		goodSet2Color = {1, 0.25f, 0};
	
	logger.LineBreak();

	logger.PushFont(Vec3(-1), -1, false, false, true);
	logger.PrintLine("В тесте \"", testName, '"');
	logger.PopFont();

	for(index_t i = 0; i < set2Times.Length(); i++)
	{
		for(index_t j = 0; j < set1Times.Length(); j++)
		{
			const bool faster = set1Times[j] < set2Times[i];
			const double timesBetter = faster? set2Times[i] / set1Times[j]: set1Times[j] / set2Times[i];

			logger.PushFont(faster? set1Color: set2Color, 3, true, false, false);
			logger << comparedTypes[faster? set2Times.Length() + j: i];
			logger.PopFont();

			logger << " faster than ";

			logger.PushFont(!faster? set1Color: set2Color, 3, true, false, false);
			logger << comparedTypes[faster? i: set2Times.Length() + j];
			logger.PopFont();

			logger << ", ";
			logger.PushFont(!faster?
				(timesBetter > 1.25? goodSet2Color: set2Color):
				(timesBetter > 1.25? goodSet1Color: set1Color),
				3, true, false, timesBetter>2);
			logger << StringOf(timesBetter, 2);
			logger.PopFont();
			logger.PrintLine(" times .");
		}
	}

	logger.PrintLine("Time (ms):");
	String timeStr;
	for(index_t i = 0; i < comparedTypes.Length(); i++)
	{
		const bool isSet2Time = i<set2Times.Length();
		double time = isSet2Time? set2Times[i]: set1Times[i-set2Times.Length()];
		logger.PushFont(isSet2Time? set2Color: set1Color, 3, true);
		logger << comparedTypes[i];
		logger.PopFont();
		logger.PrintLine(" - ", StringOf(time*1000, 2));
	}

	logger.LineBreak();
}
INTRA_END
