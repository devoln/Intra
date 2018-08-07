#include "WaveTable.h"
#include "Audio/Resample.h"
using namespace Audio;

void WaveTable::GenerateNextLevel()
{
	INTRA_DEBUG_ASSERT(CheckInvariant());
	size_t index = LevelStartIndex(LevelCount);
	Data.SetCountUninitialized(index + LevelSize(LevelCount++));
	DecimateX2Linear(LevelSamples(LevelCount-1), LevelSamples(LevelCount-2));
}

void WaveTable::UpsampleBaseLevel()
{
	INTRA_DEBUG_ASSERT(CheckInvariant());
	BaseLevelLength *= 2;
	BaseLevelRatio /= 2;
	LevelCount++;
	Data.AddLeftUninitialized(BaseLevelLength);
	UpsampleX2Linear(Data.Take(BaseLevelLength), LevelSamples(1));
}

void WaveTable::GenerateAllNextLevels()
{
	while(LevelSize(LevelCount) > 1) GenerateNextLevel();
}
