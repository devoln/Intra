#pragma once

#include <Cpp/Debug.h>
#include <Math/Math.h>
#include "ExponentialAttenuation.h"
#include <Audio/Synth/ExponentialAttenuation.h>

using Intra::Audio::Synth::ExponentialLinearAttenuate;
using Intra::Audio::Synth::ExponentialLinearAttenuateAdd;

struct Envelope
{
	// ШЕСТЬ сегментов, а не пять (Update 92): гитарным полосам нужен сегмент
	// НАРАСТАНИЯ перед щипком. Пять сегментов (атака + три узла + полка)
	// описывали только спад, поэтому огибающая полосы СТАРТОВАЛА в самой
	// громкой точке: замер .scratch/guitar-band-attack92.mjs показал, что в
	// первые 6 мс после note-on наша нота на 32 дБ громче банка (банк выходит
	// на пик за 20-30 мс), а слушалось это щелчком и «оглушительно». Кто
	// заполняет меньше сегментов — как раньше: незанятые (нулевой длины)
	// пропускаются в operator() и в StartNextSegment.
	enum: unsigned
	{
		N = 6
	};

	enum: int
	{
		NoNextSegmentIndicator = N - 1
	};

	struct Segment
	{
		unsigned Exponential: 1;

		unsigned SamplesLeft : 31;

		float Volume;

		/// Значение, которое прибавляется к (!Exponential) или умножается на (Exponential) Volume на каждом шаге
		float DU;

		INTRA_FORCEINLINE bool IsConstant() const {return float(Exponential) == DU;}
		INTRA_FORCEINLINE bool IsNoOp() const {return IsConstant() && Volume == 1;}
		INTRA_FORCEINLINE void Advance(size_t samples)
		{
			if(Exponential) Volume *= PowInt(DU, int(samples));
			else Volume += DU*float(samples);
			SamplesLeft -= unsigned(samples);
		}
	};
	Segment CurrentSegment;

	struct Point
	{
		/// Является ли отрезок, начинающийся в данной точке, экспоненциальным или линейным
		unsigned Exponential: 1;

		/// Длина отрезка, начинающегося в данной точке
		unsigned Length: 23;

		/// Громкость на конце отрезка, начинающегося в данной точке
		/// Число с фиксированной запятой, считается как Exponential? (Volume + 1) / 256.0f: Volume / 255.0f
		unsigned Volume: 8;

		INTRA_FORCEINLINE float CalcDU(float curVolume) const
		{
			if(Length == 0) return Exponential? 1.0f: 0.0f;
			if(Exponential)
			{
				// curVolume == 0 дал бы Pow(inf, 1/L) = inf (NaN на первом же
				// шаге). Экспоненциальная асимптота никогда не бывает ниже
				// 1/256 (~-48 дБ), поэтому нулевой старт клампуется до неё.
				const float start = Max(curVolume, 1.0f / 256.0f);
				return Intra::Pow((Volume + 1) / (start * 256.0f), 1.0f / Length);
			}
			return (Volume / 255.0f - curVolume) / Length;
		}

		/// Упаковать значение громкости.
		/// Вызывать эту функцию только после установки значения Exponential!
		INTRA_FORCEINLINE void SetVolume(float volume)
		{
			// Кламп ОБЯЗАТЕЛЕН (падение уровня на 18 дБ): Volume — поле в 8 бит, а уровень выше 1.0
			// (у гитар опорный уровень сдвинут к максимуму узлов, а узел бывает
			// выше среднего) давал volume*256−1 > 255 и усечение значения:
			// 1 дБ (×1.122) → 286 → 286&255 = 30 → −18 дБ вместо +1 — целое
			// падение полосы на 18 дБ в середине ноты (замер
			// .scratch/guitar-band-track.mjs 29 60).
			Volume = unsigned(Exponential?
				Intra::Min(Intra::Max(volume * 256.0f - 1.0f, 0.0f), 255.0f):
				Intra::Min(Intra::Max(volume*255.0f, 0.0f), 255.0f));
		}
	};
	Point Points[N - 1];
	int NextSegmentStartPointIndex;

	void StartNextSegment()
	{
		while(NextSegmentStartPointIndex < N - 1 && Points[NextSegmentStartPointIndex].Length == 0)
			NextSegmentStartPointIndex++;
		if(NextSegmentStartPointIndex < N - 1)
			StartSegment(NextSegmentStartPointIndex++);
	}

	/// Переход к ПОСЛЕДНЕМУ сегменту — это и есть релиз по note-off.
	/// Указатель: Points[N−2] ← Segments[N-1], поэтому штатный слот релиза —
	/// Segments[N-1]. Профили, написанные под прежние ПЯТЬ сегментов (вся семья
	/// флейт), кладут релиз в Segments[N-2] — фабрика переносит его в слот
	/// релиза сама (см. EnvelopeFactory::operator()), иначе нота гаснет за один
	/// семпл — «каждая нота заканчивается щелчком».
	INTRA_FORCEINLINE void StartLastSegment() {StartSegment(N - 2);}

	void StartSegment(int index)
	{
		NextSegmentStartPointIndex = index + 1;
		if(index >= N - 1) return;
		auto pt = Points[index];
		CurrentSegment.Exponential = pt.Exponential;
		CurrentSegment.SamplesLeft = pt.Length;
		CurrentSegment.DU = pt.CalcDU(CurrentSegment.Volume);
	}

	void MultiplyVolume(float volumeMultiplier)
	{
		CurrentSegment.Volume *= volumeMultiplier;
		for(int i = NextSegmentStartPointIndex; i < N - 1; i++)
		{
			auto& pt = Points[i];
			pt.Volume = unsigned(pt.Volume * volumeMultiplier + 0.5f);
		}
	}

	static Envelope Constant(float volume = 1)
	{
		Envelope result;
		result.CurrentSegment = {false, 0x7FFFFFFF, volume, 0};
		result.NextSegmentStartPointIndex = N - 1;
		return result;
	}
};

struct LinearAttenuator
{
	float Factor, FactorStep;
	LinearAttenuator(decltype(nullptr)=nullptr): Factor(1), FactorStep(0) {}
	LinearAttenuator(float startVolume, float deltaPerSample): Factor(1), FactorStep(deltaPerSample) {}

	INTRA_FORCEINLINE void SkipSamples(size_t count) {Factor += FactorStep*count;}
};

struct EnvelopeSegment
{
	ExponentAttenuator Exp;
	LinearAttenuator Linear;

	INTRA_FORCEINLINE EnvelopeSegment(decltype(nullptr)=nullptr) {}

	INTRA_FORCEINLINE EnvelopeSegment(Envelope::Segment segment)
	{
		if(segment.Exponential)
		{
			Exp.Factor = segment.Volume;
			Exp.FactorStep = segment.DU;
			Linear.Factor = 1;
			Linear.FactorStep = 0;
			return;
		}
		Linear.Factor = segment.Volume;
		Linear.FactorStep = segment.DU;
		Exp.Factor = 1;
		Exp.FactorStep = 1;
	}

	INTRA_FORCEINLINE void operator()(Span<float> inOutSamples)
	{
		auto src = inOutSamples.AsConstRange();
		ExponentialLinearAttenuate(inOutSamples, src, Exp.Factor, Exp.FactorStep, Linear.Factor, Linear.FactorStep);
	}

	INTRA_FORCEINLINE void operator()(Span<float> dstSamples, Span<const float> srcSamples)
	{ExponentialLinearAttenuateAdd(dstSamples, srcSamples, Exp.Factor, Exp.FactorStep, Linear.Factor, Linear.FactorStep);}

	INTRA_FORCEINLINE void SkipSamples(size_t count)
	{
		Exp.SkipSamples(count);
		Linear.SkipSamples(count);
	}
};

struct EnvelopeFactory
{
	enum {N = Envelope::N};

	float StartVolume;

	EnvelopeFactory(decltype(nullptr)=nullptr): StartVolume(1)
	{
		for(int i = 0; i < N; i++) Segments[i] = {false, 0, 0};
	}

	INTRA_FORCEINLINE explicit operator bool() const
	{
		for(int i = 0; i < N; i++)
			if(Segments[i].Duration != 0) return true;
		return false;
	}

	INTRA_FORCEINLINE bool operator==(decltype(nullptr)) const noexcept {return !operator bool();}
	INTRA_FORCEINLINE bool operator!=(decltype(nullptr)) const noexcept {return operator bool();}

	struct Segment
	{
		bool Exponential;
		float EndVolume;
		float Duration;

		INTRA_FORCEINLINE unsigned LengthInSamples(int sampleRate) const
		{
			const float durationSamples = sampleRate * Duration + 0.5f;
			return durationSamples <= float(Intra::uint_MAX)? unsigned(durationSamples): Intra::uint_MAX;
		}
	};

	Segment Segments[N];

	static EnvelopeFactory Constant(float volume)
	{
		EnvelopeFactory result;
		for(int i = 0; i < N; i++) result.Segments[i] = {false, volume, 0};
		result.Segments[N - 1].Duration = Intra::Infinity;
		return result;
	}

	static EnvelopeFactory ADSR(float attackTime, float decayTime, float sustainVolume, float releaseTime, bool exponential=false)
	{
		EnvelopeFactory result;
		result.StartVolume = 0;
		for(int i = 0; i < N - 4; i++) result.Segments[i] = {false, 0, 0};
		result.Segments[N - 4] = {exponential, 1, attackTime};
		result.Segments[N - 3] = {exponential, sustainVolume, decayTime};
		result.Segments[N - 2] = {false, sustainVolume, Intra::Infinity};
		// Release подчиняется общему флагу exponential (иначе fade-out всегда
		// линейный: амплитуда падает с постоянной скоростью, и нота «висит»
		// почти на полной громкости до последних миллисекунд). Экспоненциальный
		// релиз глушит начало быстрее (как у reference bank/reference renderer) и мягко добивает
		// хвост; из нулевой громкости экспонента вырождается в линейный спад
		// (StartSegment: CalcDU с curVolume == 0), NaN исключён тем, что
		// NoteRelease не вызывается для уже отпущенных голосов (MidiSynth).
		result.Segments[N - 1] = {exponential, 0, releaseTime};
		return result;
	}

	Envelope operator()(int sampleRate) const
	{
		int startIndex = 0;
		while(Segments[startIndex].Duration == 0) startIndex++;
		auto& startSeg = Segments[startIndex];

		// ЛЕГАСИ-РЕЛИЗ (5-сегментные профили). До Update 92 сегментов было
		// пять, и вся семья флейт кладёт релиз в Segments[N-2]. Штатный слот
		// релиза — последний (Points[N-2] ← Segments[N-1]), поэтому такой
		// релиз был ПУСТЫМ и нота гасла за один семпл: владелец слышал это
		// как щелчок на каждой ноте (сильнее всего на GM 73). Признак релиза —
		// КОНЕЧНЫЙ сегмент, гаснущий в ноль: у ADSR здесь бессрочная полка
		// сюстейна (Duration == Infinity), у гитарных полос — полка хвоста,
		// поэтому их схемы не меняются.
		const bool legacyRelease = Segments[N - 1].Duration == 0 &&
			Segments[N - 2].Duration != 0 &&
			Segments[N - 2].Duration < Intra::Infinity &&
			Segments[N - 2].EndVolume == 0;

		Envelope result;
		result.NextSegmentStartPointIndex = startIndex;
		auto& resSeg = result.CurrentSegment;
		// Экспоненциальный разгон из нулевой громкости вырожден:
		// 0 * Pow(Inf, n) даёт NaN на первом же Advance. Для такого
		// сегмента откатываемся на линейный разгон.
		resSeg.Exponential = startSeg.Exponential && StartVolume != 0;
		resSeg.SamplesLeft = startSeg.LengthInSamples(sampleRate);
		resSeg.Volume = StartVolume;
		resSeg.DU = resSeg.Exponential?
			Intra::Pow(startSeg.EndVolume / StartVolume, 1.0f / resSeg.SamplesLeft):
			(startSeg.EndVolume - StartVolume) / resSeg.SamplesLeft;

		for(int i = startIndex; i < N - 1; i++)
		{
			auto& pt = result.Points[i];
			auto& s = legacyRelease && i == N - 2? Segments[N - 2]: Segments[i + 1];
			pt.Exponential = unsigned(s.Exponential);
			pt.Length = s.LengthInSamples(sampleRate);
			pt.SetVolume(s.EndVolume);
		}

		return result;
	}
};
