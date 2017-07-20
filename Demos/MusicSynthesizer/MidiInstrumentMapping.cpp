#include "MidiInstrumentMapping.h"

#include "Range/Mutation/Fill.h"

#include "Audio/Synth/Types.h"

#include "InstrumentLibrary.h"
#include "Audio/Synth/InstrumentSet.h"

using namespace Intra;
using namespace Audio;
using namespace Synth;

MidiInstrumentSet GetMapping()
{
	static InstrumentLibrary lib;
	MidiInstrumentSet result;

	Span<MusicalInstrument*> instruments = result.Instruments;

	for(byte i = 0; i < 128; i++)
		instruments[i] = &lib.Sine2Exp;
	instruments[117] = null; //Melodic Tom не реализован, а стандартное пищание плохо звучит. Поэтому просто отключим этот инструмент

	static const byte pianos[] = {0,1,2,3,7, 105, 107};
	for(const byte code: pianos) instruments[code] = &lib.Piano;

	instruments[4] = &lib.ElectricPiano;
	instruments[5] = &lib.ElectricPiano2;

	static const byte organs[] = {16, 19, 20, 22, 23};
	for(const byte code: organs) instruments[code] = &lib.Organ;

	instruments[21] = &lib.Accordion;

	instruments[17] = &lib.PercussiveOrgan;

	static const byte guitars[] = {6, 24, 26, 27, 28, 29, 30, 31, 46};
	for(const byte code: guitars) instruments[code] = &lib.Guitar;

	instruments[25] = &lib.GuitarSteel;
	instruments[32] = &lib.Bass1;
	instruments[33] = &lib.ElectricBassFinger;
	instruments[34] = &lib.ElectricBassPick;

	static const byte basses2[] = {36, 37, 39};
	for(const byte code: basses2) instruments[code] = &lib.Bass2;

	instruments[47] = &lib.GunShot;

	static const byte padSweeps[] = {43, 45, 49, 50, 51,  89, 90, 93, 94, 95, 102};
	for(const byte code: padSweeps) instruments[code] = &lib.Pad8Sweep;

	instruments[40] = instruments[41] = instruments[42] = &lib.Violin;
	instruments[48] = &lib.StringEnsemble;

	static const byte panFlutes[] = {71, 75};
	for(const byte code: panFlutes) instruments[code] = &lib.PanFlute;

	static const byte flutes[] = {73, 60};
	for(const byte code: flutes) instruments[code] = &lib.Flute;

	static const byte whistles[] = {74, 76, 77,  78,  79};
	for(const byte code: whistles) instruments[code] = &lib.Whistle;

	instruments[80] = &lib.LeadSquare;
	instruments[81] = &lib.Sawtooth;
	instruments[87] = &lib.BassLead;
	instruments[94] = &lib.Pad7Halo;
	instruments[119] = &lib.ReverseCymbal;
	instruments[99] = &lib.Atmosphere;

	static const byte vibraphones[] = {8, 10, 11, 12, 88, 92, 108};
	for(const byte code: vibraphones) instruments[code] = &lib.Vibraphone;

	instruments[9] = &lib.Glockenspiel;

	static const byte newAges[] = {88, 92};
	for(const byte code: newAges) instruments[code] = &lib.NewAge;

	instruments[98] = &lib.Crystal;

	static const byte kalimbas[] = {13, 15, 108, 112};
	for(const byte code: kalimbas) instruments[code] = &lib.Kalimba;

	static const byte synthVoices[] = {18, 52, 53, 54, 83, 85, 100};
	for(const byte code: synthVoices) instruments[code] = &lib.SynthVoice;
	instruments[52] = &lib.ChoirAahs;

	instruments[18] = &lib.RockOrgan;

	static const byte soundTrackFx[] = {44, 97};
	for(const byte code: soundTrackFx) instruments[code] = &lib.SoundTrackFX2;

	static const byte trumpets[] = {56, 57, 58, 68, 69, 70};
	for(const byte code: trumpets) instruments[code] = &lib.Trumpet;

	instruments[72] = &lib.Piccolo;

	instruments[68] = &lib.Oboe;

	static const byte sax[] = {64, 65, 66, 67};
	for(const byte code: sax) instruments[code] = &lib.Sax;

	static const byte synthBrasses[] = {61, 62, 63};
	for(const byte code: synthBrasses) instruments[code] = &lib.SynthBrass;

	instruments[84] = &lib.Lead5Charang;
	instruments[82] = &lib.Calliope;
	instruments[35] = &lib.FretlessBass;
	instruments[55] = &lib.OrchestraHit;
	instruments[38] = &lib.SynthBass1;
	instruments[91] = &lib.PadChoir;
	instruments[96] = &lib.Rain;

	static const byte drumSounds[] = {115, 118, 120};
	for(const byte code: drumSounds) instruments[code] = &lib.DrumSound2;

	instruments[122] = &lib.Seashore;
	instruments[124] = &lib.PhoneRing;
	instruments[125] = &lib.Helicopter;
	instruments[126] = &lib.Applause;
	instruments[127] = &lib.GunShot;
	
	Range::Fill(result.DrumInstruments, &lib.UniDrum);
	result.DrumInstruments[41] = &lib.ClosedHiHat;
	result.DrumInstruments[34] = &lib.AcousticBassDrum;
	result.DrumInstruments[35] = &lib.AcousticBassDrum;

	return result;
}
