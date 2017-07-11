#pragma once

#include "Cpp/Warnings.h"

#include "Audio/Synth/MusicalInstrument.h"
#include "Audio/Synth/DrumInstrument.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

using Intra::Audio::Synth::MusicalInstrument;
using Intra::Audio::Synth::DrumInstrument;

struct InstrumentLibrary
{
	InstrumentLibrary();

	MusicalInstrument Piano, ElectricPiano, ElectricPiano2;
	MusicalInstrument Vibraphone, Glockenspiel, NewAge, Crystal;
	MusicalInstrument Kalimba;
	MusicalInstrument Bass1, Bass2, Bass3, ElectricBassFinger;
	MusicalInstrument ElectricBassPick;
	MusicalInstrument SynthBass1, SynthBass2;
	MusicalInstrument SynthBrass, Lead5Charang;
	MusicalInstrument Birds, SynthVoice, SoundTrackFX2;
	MusicalInstrument Pad7Halo, Pad8Sweep, PadChoir, ReverseCymbal, Atmosphere, Rain, StringEnsemble;
	MusicalInstrument Flute, PanFlute;
	MusicalInstrument Guitar, GuitarSteel;
	MusicalInstrument Guitar1, OverdrivenGuitar;
	MusicalInstrument Trumpet, Piccolo, Oboe, FretlessBass, Sax, Calliope;
	MusicalInstrument Violin;
	MusicalInstrument Organ, PercussiveOrgan;
	MusicalInstrument Whistle;
	MusicalInstrument Sine2Exp;
	MusicalInstrument Sawtooth;
	MusicalInstrument Accordion;
	MusicalInstrument BassLead;
	MusicalInstrument OrchestraHit;
	MusicalInstrument LeadSquare;
	MusicalInstrument GunShot, Applause, Seashore, Helicopter, PhoneRing;

	MusicalInstrument DrumSound2;
	RecordedDrumInstrument UniDrum, AcousticBassDrum, ClosedHiHat;

	static MusicalInstrument CreateGuitar(size_t n=15, float c=128, float d=1.5,
		float e=0.75, float f=1, float freqMult=0.5f, float volume=0.6f);
};


INTRA_WARNING_POP
