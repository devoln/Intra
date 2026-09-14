#pragma once

#include <Cpp/Fundamental.h>
#include <Cpp/Warnings.h>

#include <Container/Sequential/Array.h>

#include <Audio/MusicNote.h>

#include "WaveFormSampler.h"
#include "WaveTableSampler.h"
#include "WhiteNoiseSampler.h"
#include "ADSR.h"
#include "Sampler.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

class NoteSampler: public Sampler
{
public:
	Array<WaveFormSampler> WaveFormSamplers;
	Array<WaveTableSampler> WaveTableSamplers;
	Array<WhiteNoiseSampler> WhiteNoiseSamplers;
	Array<GenericSamplerRef> GenericSamplers;
	// Модификаторы хранятся парой — по экземпляру на канал (см.
	// NoteSampler::applyModifiersStereo): у каждого канала своя память IIR-фильтров,
	// а временное расписание одинаковое, потому что обе копии обрабатывают одни и
	// те же по длине куски рендера.
	Array<GenericModifier> Modifiers;
	Array<GenericModifier> ModifiersRight;
	AdsrAttenuator ADSR;
	float Pan = 0;

	size_t GenerateMono(Span<float> ioDst);
	size_t GenerateStereo(Span<float> ioDstLeft, Span<float> ioDstRight);

	void MultiplyPitch(float freqMultiplier) override;
	void NoteRelease() override;
	void SetPan(float pan) override;
	void MultiplyVolume(float volumeMultiplier) override;
	void SetRenderParams(const RenderParams& params) override;

#ifdef INTRA_UI_METERS
	/// Уровень огибающей ноты для индикатора в веб-UI. Складывается из
	/// общеголосовой ADSR (если она есть) и огибающих звучащего тела — у
	/// волновых таблиц и форм своя огибающая (у флейты именно в ней атака и
	/// релиз, а верхний ADSR может отсутствовать и означать ровный уровень).
	float GetLevel() const override;

	/// У этой ноты уже когда-то было измеримое тело (волновая таблица/форма).
	/// Нужно, чтобы отличить «нота отзвучала, остался неизмеримый хвост» от
	/// «измерить тело этой ноты вообще нечем» (ударные/generic-семплеры):
	/// в первом случае индикатор гаснет, во втором остаётся ровным.
	mutable bool mBodySeen = false;
#endif

	bool Empty() const noexcept
	{
		return WaveFormSamplers.Empty() && WaveTableSamplers.Empty() &&
			WhiteNoiseSamplers.Empty() && GenericSamplers.Empty();
	}

	// Sampler (task-based) interface.
	void MoveConstruct(void* dst) override {new(dst) NoteSampler(Move(*this));}
	bool Generate(SamplerTaskContainer& dstTasks, size_t offsetInSamples, size_t numSamples) override;

private:
	bool CanRenderAdsrDirect() const;
	void fill(Span<float> ioDst);
	void fillStereo(Span<float> ioDstLeft, Span<float> ioDstRight);
	void applyModifiers(Span<float> ioDst);
	void applyModifiersStereo(Span<float> ioDstLeft, Span<float> ioDstRight);
};

INTRA_WARNING_POP
