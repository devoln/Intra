#pragma once

#ifndef INTRA_NO_AUDIO_SYNTH

#include "SynthesizedInstrument.h"
#include "DrumInstrument.h"
#include "Platform/CppWarnings.h"

namespace Intra { namespace Audio { namespace Synth {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

struct MusicalInstruments
{
	MusicalInstruments();

	CombinedSynthesizedInstrument Piano, ElectricPiano, ElectricPiano2;
	SynthesizedInstrument Vibraphone, Glockenspiel, NewAge, Crystal;
	CombinedSynthesizedInstrument Kalimba;
	SynthesizedInstrument Bass1, Bass2, Bass3, ElectricBassFinger;
	CombinedSynthesizedInstrument ElectricBassPick;
	SynthesizedInstrument SynthBass1, SynthBass2;
	SynthesizedInstrument SynthBrass, Lead5Charang;
	SynthesizedInstrument Birds, SynthVoice, SoundTrackFX2;
	SynthesizedInstrument Pad7Halo, Pad8Sweep, PadChoir, ReverseCymbal, Atmosphere, Rain, StringEnsemble;
	CombinedSynthesizedInstrument Flute, PanFlute;
	SynthesizedInstrument Guitar, GuitarSteel;
	SynthesizedInstrument Guitar1, OverdrivenGuitar;
	SynthesizedInstrument Trumpet, Oboe, FretlessBass, Sax, Calliope;
	SynthesizedInstrument Violin;
	SynthesizedInstrument Organ, PercussiveOrgan;
	SynthesizedInstrument Whistle;
	SynthesizedInstrument Sine2Exp;
	SynthesizedInstrument Sawtooth;
	CombinedSynthesizedInstrument BassLead;
	CombinedSynthesizedInstrument OrchestraHit;
	SynthesizedInstrument LeadSquare;
	SynthesizedInstrument GunShot, Applause, Seashore, Helicopter, PhoneRing;

	SynthesizedInstrument DrumSound2;
	DrumInstrument Drums;

	static SynthesizedInstrument CreateGuitar(size_t n=15, float c=128, float d=1.5,
		float e=0.75, float f=1, float freqMult=0.5f, float duration=0.7f, float volume=0.6f);

};


INTRA_WARNING_POP

}}}

#endif
