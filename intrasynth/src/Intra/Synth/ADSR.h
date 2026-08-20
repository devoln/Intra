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

	AdsrAttenuator(decltype(nullptr)=nullptr): Active(false) {}
	explicit AdsrAttenuator(Envelope env): Env(Move(env)), Active(true) {}

	size_t SamplesLeft() const {return Active ? size_t(Env.CurrentSegment.SamplesLeft) : 0;}

	INTRA_FORCEINLINE explicit operator bool() const {return Active && SamplesLeft() > 0;}
	INTRA_FORCEINLINE bool operator==(decltype(nullptr)) const {return !operator bool();}
	INTRA_FORCEINLINE bool operator!=(decltype(nullptr)) const {return operator bool();}

	void NoteRelease() {if(Active) Env.StartLastSegment();}

	void operator()(Span<float> dst)
	{
		if(!Active) return;
		while(!dst.Empty())
		{
			if(Env.CurrentSegment.SamplesLeft == 0) Env.StartNextSegment();
			if(Env.CurrentSegment.SamplesLeft == 0) {Active = false; FillZeros(dst); return;}
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
	}
};

INTRA_WARNING_POP
