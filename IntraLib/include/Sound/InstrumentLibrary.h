#pragma once

#ifndef NO_MIDI_SYNTH

#include "SynthesizedInstrument.h"

namespace Intra {

struct MusicalInstruments
{
	MusicalInstruments();

	CombinedSynthesizedInstrument Piano, ElectricPiano, ElectricPiano2;
	SynthesizedInstrument Vibraphone, NewAge, Crystal;
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

	static SynthesizedInstrument CreateGuitar(int n=15, float c=128, float d=1.5, float e=0.75, float f=1, float freqMult=0.5f, float duration=0.7f, float volume=0.6f);

};

#endif

}
