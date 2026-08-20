#include "MidiInstrumentMapping.h"

#include <Range/Mutation/Fill.h>

#include "Types.h"
#include "InstrumentLibrary.h"
#include "InstrumentSet.h"

using namespace Audio;

MidiInstrumentSet GetMapping()
{
	static InstrumentLibrary lib;
	MidiInstrumentSet result;

	Span<MusicalInstrument*> instruments = result.Instruments;

	// Порядок инструментов в точности повторяет gInstrumentMapping из
	// devoln/web-midisynth. Программы, которых там нет, остаются nullptr
	// (тишина), как в web-версии.
	instruments[0] = lib["AcousticPiano"];
	instruments[1] = lib["BrightAcousticPiano"];
	instruments[2] = lib["ElectricGrandPiano"];
	instruments[3] = lib["HonkyTonkPiano"];
	instruments[4] = lib["ElectricPiano1"];
	instruments[5] = lib["ElectricPiano2"];
	instruments[6] = lib["Harpsichord"];
	instruments[7] = lib["Clavinet"];
	instruments[8] = lib["Celesta"];
	instruments[9] = lib["Glockenspiel"];
	instruments[10] = lib["MusicBox"];
	instruments[11] = lib["Vibraphone"];
	instruments[12] = lib["Marimba"];
	instruments[13] = lib["Xylophone"];
	instruments[14] = lib["Marimba"];
	instruments[15] = lib["Kalimba"];
	instruments[16] = lib["SynthOrgan"];
	instruments[17] = lib["PercussiveOrgan"];
	instruments[18] = lib["RockOrgan"];
	instruments[19] = lib["SynthOrgan"];
	instruments[20] = lib["SynthOrgan"];
	instruments[21] = lib["Accordion"];
	instruments[22] = lib["Harmonica"];
	instruments[23] = lib["SynthOrgan"];
	instruments[24] = lib["AcousticGuitarNylon"];
	instruments[25] = lib["AcousticGuitarSteel"];
	instruments[26] = lib["ElectricGuitarJazz"];
	instruments[27] = lib["ElectricGuitarClean"];
	instruments[28] = lib["ElectricGuitarMuted"];
	instruments[29] = lib["OverdrivenGuitar"];
	instruments[30] = lib["DistortionGuitar"];
	instruments[31] = lib["AcousticGuitarNylon"];
	instruments[32] = lib["AcousticBass"];
	instruments[33] = lib["ElectricBassPick"];
	instruments[34] = lib["ElectricBassPick"];
	instruments[35] = lib["FretlessBass"];
	instruments[36] = lib["SlapBass"];
	instruments[37] = lib["SlapBass"];
	instruments[38] = lib["SynthBass1"];
	instruments[39] = lib["SynthBass2"];
	instruments[40] = lib["Violin"];
	instruments[41] = lib["Violin"];
	instruments[42] = lib["Violin"];
	instruments[44] = lib["TremoloStrings"];
	instruments[45] = lib["PizzicatoStrings"];
	instruments[46] = lib["AcousticGuitarNylon"];
	instruments[47] = lib["Timpani"];
	instruments[48] = lib["StringEnsemble"];
	instruments[49] = lib["StringEnsemble2"];
	instruments[50] = lib["SynthStrings"];
	instruments[51] = lib["Pad8Sweep"];
	instruments[52] = lib["ChoirAahs"];
	instruments[53] = lib["VoiceOohs"];
	instruments[54] = lib["SynthVoice"];
	instruments[55] = lib["OrchestraHit"];
	instruments[56] = lib["Trumpet"];
	instruments[57] = lib["TrumpetOld"];
	instruments[58] = lib["Tuba"];
	instruments[59] = lib["TrumpetOld"];
	instruments[60] = lib["FrenchHorn"];
	instruments[61] = lib["BrassSection"];
	instruments[62] = lib["SynthBrass"];
	instruments[63] = lib["SynthBrass"];
	instruments[64] = lib["Sax"];
	instruments[65] = lib["Sax"];
	instruments[66] = lib["Sax"];
	instruments[67] = lib["Sax"];
	instruments[68] = lib["Oboe"];
	instruments[69] = lib["EnglishHorn"];
	instruments[70] = lib["TrumpetOld"];
	instruments[71] = lib["Clarinet"];
	instruments[72] = lib["Flute"];
	instruments[73] = lib["Flute"];
	instruments[74] = lib["Recorder"];
	instruments[75] = lib["Flute"];
	instruments[76] = lib["Whistle"];
	instruments[77] = lib["Whistle"];
	instruments[78] = lib["Whistle"];
	instruments[79] = lib["Ocarina"];
	instruments[80] = lib["Lead1Square"];
	instruments[81] = lib["Lead2Sawtooth"];
	instruments[82] = lib["Calliope"];
	instruments[83] = lib["SynthVoice"];
	instruments[84] = lib["Lead5Charang"];
	instruments[85] = lib["SynthVoice"];
	instruments[86] = lib["SynthStrings"];
	instruments[87] = lib["BassLead"];
	instruments[88] = lib["NewAge"];
	instruments[89] = lib["Pad8Sweep"];
	instruments[90] = lib["Pad3Polysynth"];
	instruments[91] = lib["Pad4Choir"];
	instruments[92] = lib["Pad5Bowed"];
	instruments[93] = lib["Pad8Sweep"];
	instruments[94] = lib["Pad7Halo"];
	instruments[95] = lib["Pad8Sweep"];
	instruments[96] = lib["Fx1Rain"];
	instruments[97] = lib["Fx2SoundTrack"];
	instruments[98] = lib["Vibraphone"];
	instruments[99] = lib["Fx4Atmosphere"];
	instruments[100] = lib["SynthVoice"];
	instruments[101] = lib["Fx6Goblins"];
	instruments[102] = lib["Pad8Sweep"];
	instruments[103] = lib["SynthVoice"];
	instruments[104] = lib["Sitar"];
	instruments[105] = lib["AcousticPiano"];
	instruments[106] = lib["Timpani"];
	instruments[107] = lib["AcousticPiano"];
	instruments[108] = lib["Kalimba"];
	instruments[109] = lib["Oboe"];
	instruments[110] = lib["Fiddle"];
	instruments[111] = lib["Fiddle"];
	instruments[112] = lib["Kalimba"];
	instruments[113] = lib["Marimba"];
	instruments[114] = lib["SteelDrums"];
	instruments[119] = lib["ReverseCymbal"];
	instruments[122] = lib["Seashore"];
	instruments[124] = lib["PhoneRing"];
	instruments[125] = lib["Helicopter"];
	instruments[126] = lib["Applause"];
	instruments[127] = lib["Gunshot"];

	// Ударные (channel 10), которых нет в web-midisynth, остаются.
	Fill(result.DrumInstruments, &lib.UniDrum);
	result.DrumInstruments[35] = &lib.AcousticBassDrum;
	result.DrumInstruments[36] = &lib.AcousticBassDrum;
	result.DrumInstruments[38] = &lib.AcousticSnare;
	result.DrumInstruments[40] = &lib.AcousticSnare;
	result.DrumInstruments[42] = &lib.ClosedHiHat;
	result.DrumInstruments[44] = &lib.ClosedHiHat;
	result.DrumInstruments[46] = &lib.ClosedHiHat;

	return result;
}
