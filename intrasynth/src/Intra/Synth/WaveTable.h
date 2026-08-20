#pragma once

#include <Cpp/Warnings.h>
#include <Container/Sequential/Array.h>

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

struct WaveTable
{
	/// Непрерывный массив, содержащий сразу все уровни детализации таблицы.
	Array<float> Data;

	/// Отношение частоты к частоте дискретизации, на которое рассчитан базовый уровень детализации.
	/// Для каждого следующего уровня это значение в 2 раза больше предыдущего.
	float BaseLevelRatio = 440.0f/48000;

	/// Размер базового уровня детализации. Должен быть степенью двойки.
	size_t BaseLevelLength = 0;

	/// Количество уровней детализации.
	size_t LevelCount = 1;

	/// Эти функции генерируют новые уровни детализации, увеличивая количество элементов в Data.
	/// Это может привести к его перераспределению и инвалидации всех указателей на него.
	void GenerateNextLevel();
	void UpsampleBaseLevel();
	void GenerateAllNextLevels();
	
	void ResetLevels()
	{
		Data.SetCount(BaseLevelLength);
		LevelCount = 1;
	}

	size_t LevelStartIndex(size_t level) const noexcept {return 2*(BaseLevelLength - (BaseLevelLength >> level));}
	size_t LevelSize(size_t level) const noexcept {return BaseLevelLength >> level;}
	bool CheckInvariant() const noexcept {return LevelStartIndex(LevelCount) == Data.Length();}
	Span<float> LevelSamples(size_t level) noexcept {return Data.Drop(LevelStartIndex(level)).Take(LevelSize(level));}
	Span<const float> LevelSamples(size_t level) const noexcept {return Data.Drop(LevelStartIndex(level)).Take(LevelSize(level));}

	size_t NearestLevelForRate(float rate) const noexcept {return rate < 1? 0: Min(size_t(Log2i(unsigned(rate*Sqrt2))), LevelCount-1);}
	size_t NearestLevelForRatio(float ratio) const noexcept {return NearestLevelForRate(ratio/BaseLevelRatio);}
	Span<float> LevelSamplesForRatio(float ratio) noexcept {return LevelSamples(NearestLevelForRatio(ratio));}
	Span<const float> LevelSamplesForRatio(float ratio) const noexcept {return LevelSamples(NearestLevelForRatio(ratio));}
	float LevelRatio(size_t level) const noexcept {return BaseLevelRatio*float(1 << level);}
};

INTRA_WARNING_POP
