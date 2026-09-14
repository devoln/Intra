#pragma once

#include "Envelope.h"
#include <Range/Mutation/Fill.h>

#ifdef INTRA_PROBE_NAN
#include <stdio.h>
#endif

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

/// Тонкая обёртка над Envelope, используемая NoteSampler для применения
/// общей ADSR-огибающей ко всему голосу.
struct AdsrAttenuator
{
	Envelope Env;
	bool Active = false;

#ifdef INTRA_UI_METERS
	/// Огибающая ДОРАБОТАЛА (а не «её не было вовсе»): только у индикатора
	/// уровня в веб-UI нужно отличать эти два случая. Без флага уровень после
	/// завершения релиза подскакивал обратно к 1.0 (Active == false читалось
	/// как «огибающей нет, звук ровный») — индикатор вместо затухания вспыхивал
	/// на полную яркость.
	bool Done = false;
#endif

	AdsrAttenuator(decltype(nullptr)=nullptr): Active(false) {}
	explicit AdsrAttenuator(Envelope env): Env(Move(env)), Active(true) {}

	/// Выключает аттенюатор, запоминая, что огибающая отработала полностью
	/// (нужно только индикатору уровня: см. Done).
	INTRA_FORCEINLINE void Deactivate()
	{
		Active = false;
#ifdef INTRA_UI_METERS
		Done = true;
#endif
	}

	size_t SamplesLeft() const {return Active ? size_t(Env.CurrentSegment.SamplesLeft) : 0;}

	INTRA_FORCEINLINE explicit operator bool() const {return Active && SamplesLeft() > 0;}
	INTRA_FORCEINLINE bool operator==(decltype(nullptr)) const {return !operator bool();}
	INTRA_FORCEINLINE bool operator!=(decltype(nullptr)) const {return operator bool();}

	void NoteRelease() {if(Active) Env.StartLastSegment();}

	void Advance(size_t samples)
	{
		if(!Active) return;
		Env.CurrentSegment.Advance(samples);
		if(Env.CurrentSegment.SamplesLeft == 0)
		{
			Env.StartNextSegment();
			if(Env.CurrentSegment.SamplesLeft == 0) Deactivate();
		}
	}

	void operator()(Span<float> dst)
	{
		if(!Active) return;
		while(!dst.Empty())
		{
			if(Env.CurrentSegment.SamplesLeft == 0) Env.StartNextSegment();
			if(Env.CurrentSegment.SamplesLeft == 0) {Deactivate(); FillZeros(dst); return;}
			auto part = dst.Take(size_t(Env.CurrentSegment.SamplesLeft));
			// Сегмент с постоянной громкостью 1 — нечего применять, только
			// сдвигаем состояние огибающей (экономим целый проход на каждую ноту).
			if(Env.CurrentSegment.IsNoOp())
			{
				Env.CurrentSegment.Advance(part.Length());
				dst.PopFirstExactly(part.Length());
				continue;
			}
			EnvelopeSegment seg(Env.CurrentSegment);
			seg(part);
#ifdef INTRA_PROBE_NAN
			{
				static int probeLines = 0;
				if(probeLines < 15)
				{
					float mx = 0;
					for(size_t pi = 0; pi < part.Length(); pi++)
					{
						float a = part[pi]; if(a < 0) a = -a; if(a > mx) mx = a;
					}
					if(mx > 1e12f)
					{
						fprintf(stderr, "[ADSR] mx=%.3e expStep=%.3e lin=%.3e linStep=%.3e segExp=%u segLen=%u segVol=%.3f segDU=%.3e\n",
							double(mx), double(seg.Exp.FactorStep), double(seg.Linear.Factor), double(seg.Linear.FactorStep),
							Env.CurrentSegment.Exponential, Env.CurrentSegment.SamplesLeft, double(Env.CurrentSegment.Volume), double(Env.CurrentSegment.DU));
						probeLines++;
					}
				}
			}
#endif
			Env.CurrentSegment.Advance(part.Length());
			dst.PopFirstExactly(part.Length());
		}
		// Сегмент мог закончиться ровно на границе буфера (dst.Empty() при
		// SamplesLeft == 0): сразу переходим к следующему сегменту, чтобы
		// состояние между вызовами оставалось консистентным. Тогда
		// SamplesLeft() == 0 после вызова означает только ПОЛНОЕ завершение
		// огибающей (Active == false), а не «сегмент закончился на хвосте
		// буфера» — раньше такая граница убивала ноту в applyModifiersStereo
		// (цикл по сегментам выходил, хвост обнулялся, голос снимался), а
		// точное попадание границы на конец региона оставляло «зависший»
		// pending-сегмент, из-за которого следующий кадр пропускал ADSR.
		if(Active && Env.CurrentSegment.SamplesLeft == 0)
		{
			Env.StartNextSegment();
			if(Env.CurrentSegment.SamplesLeft == 0) Deactivate();
		}
	}
};

INTRA_WARNING_POP
