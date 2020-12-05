#pragma once

#include <Core/Warnings.h>
#include <Container/Sequential/Array.h>

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

struct WaveTable
{
	/// ����������� ������, ���������� ����� ��� ������ ����������� �������.
	Array<float> Data;

	/// ��������� ������� � ������� �������������, �� ������� ��������� ������� ������� �����������.
	/// ��� ������� ���������� ������ ��� �������� � 2 ���� ������ �����������.
	float BaseLevelRatio = 440.0f/48000;

	/// ������ �������� ������ �����������. ������ ���� �������� ������.
	size_t BaseLevelLength = 0;

	/// ���������� ������� �����������.
	size_t LevelCount = 1;

	/// ��� ������� ���������� ����� ������ �����������, ���������� ���������� ��������� � Data.
	/// ��� ����� �������� � ��� ����������������� � ����������� ���� ���������� �� ����.
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
	CSpan<float> LevelSamples(size_t level) const noexcept {return Data.Drop(LevelStartIndex(level)).Take(LevelSize(level));}

	size_t NearestLevelForRate(float rate) const noexcept {return rate < 1? 0: Min(size_t(Log2i(unsigned(rate*Sqrt2))), LevelCount-1);}
	size_t NearestLevelForRatio(float ratio) const noexcept {return NearestLevelForRate(ratio/BaseLevelRatio);}
	Span<float> LevelSamplesForRatio(float ratio) noexcept {return LevelSamples(NearestLevelForRatio(ratio));}
	CSpan<float> LevelSamplesForRatio(float ratio) const noexcept {return LevelSamples(NearestLevelForRatio(ratio));}
	float LevelRatio(size_t level) const noexcept {return BaseLevelRatio*float(1 << level);}
};

INTRA_WARNING_POP
