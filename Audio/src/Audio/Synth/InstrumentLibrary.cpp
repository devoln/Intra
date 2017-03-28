#ifndef INTRA_NO_AUDIO_SYNTH

#include "Platform/CppWarnings.h"
INTRA_DISABLE_REDUNDANT_WARNINGS
#include "Audio/Synth/InstrumentLibrary.h"
#include "Audio/Synth/Generators/DrumPhysicalModel.h"
#include "Audio/Synth/GeneratorSynth.h"
#include "Container/Sequential/Array.h"
#include "Container/Utility/Array2D.h"

#include "Audio/Synth/SineSynth.h"
#include "Audio/Synth/SineExpSynth.h"
#include "Audio/Synth/ModifierPass.h"
#include "Audio/Synth/ExponentialAttenuation.h"
#include "Audio/Synth/TableAttenuation.h"
#include "Audio/Synth/SawtoothSynth.h"
#include "Audio/Synth/AttackDecayAttenuation.h"

#include "Audio/Synth/Generators.hh"
#include "Audio/Synth/Modifiers.hh"
#include "Audio/Synth/PostEffects.hh"

namespace Intra { namespace Audio { namespace Synth {

using namespace IO;
using namespace Math;


static float DrumSample(float freq, float t)
{
	float e = 0.5f, f = 0.5f;
	const float x = 2*float(PI)*t;
	float resonanse = 20*0.00390625f;
	const float cutoff = 5*0.0019531f;
	const float exponent = Exp(-20*t);
	float a = Sin(218*x)*0.3051758f;
	a += Sin(343*x)*0.06103516f;
	a += Sin(570*x)*0.1831055f;
	a += Sin(625*x)*0.1831055f;
	a *= exponent;

	float k3 = float(PI)*cutoff;
	k3 = 1.0f/Tan(k3);
	resonanse *= k3;
	k3 *= k3;
	const float km = 1.0f/(1.0f+resonanse+k3);
	resonanse = (1.0f-resonanse+k3)/(1.0f-k3);
	k3 = 2.0f*(1.0f-k3)*km;

	e = (2.0f-k3)*a-k3*e+f;
	f = (1.0f-resonanse)*a+resonanse*(e-f)*0.5f;
	a = km*(e+a);

	a += Sin(625*x)*0.0305176f;
	a += Sin(825*x)*0.06103516f;
	a += Sin(1025*x)*0.03051758f;
	a *= exponent;
	a += (RandomNoise::Linear(t*freq*freq/10) + RandomNoise::Linear(t*freq*freq))*0.1220703f;
	a *= Exp(-15*t);
	a = Clamp(a, -0.30517578f, 0.30517578f);
	a *= 2.831055f;
	return a;
}







static DrumInstrument CreateDrums()
{
	DrumInstrument result;

	auto UniDrum = new SynthesizedInstrument;
	UniDrum->Synth = CreateGeneratorSynthPass(
		Generators::DrumPhysicalModel(2, 16, 16, 0.342f, 0.00026f, 0.20f), 0.03f, 1, 0.2f);
	//UniDrum->Synth = CreateGeneratorSynthPass(Generators::FunctionGenerator<float(*)(float, float)>(DrumSample), 0.1f, 1, 0.2f);
	UniDrum->PostEffects.AddLast(PostEffects::FilterQ(4000, 0.6f));
	//UniDrum->PostEffects.AddLast(PostEffects::FilterHP(0.9f));
	UniDrum->PostEffects.AddLast(PostEffects::Fade(0, 1000));
	UniDrum->MinNoteDuration = 0.25f;
	for(uint id=0; id<=127; id++)
		result.Generators[id] = UniDrum;


	/*auto ClosedHiHat = new SynthesizedInstrument;
	ClosedHiHat->Synth = CreateGeneratorSynthPass(DrumGenerator(2, 16, 16, 0.342f, 0.00026f, 0.20f), 0.03f, 1, 0.35f);
	ClosedHiHat->PostEffects.AddLast(PostEffects::FilterQ(12000, 0.2f));
	//ClosedHiHat->PostEffects.AddLast(PostEffects::FilterHP(0.9f));
	ClosedHiHat->PostEffects.AddLast(PostEffects::Fade(0, 1000));
	ClosedHiHat->MinNoteDuration = 0.25f;*/

	auto ClosedHiHat = new SynthesizedInstrument;
	ClosedHiHat->Synth = CreateGeneratorSynthPass(
		Generators::DrumPhysicalModel(2, 16, 16, 0.338f, 0.04928f, 0.10f), 0.03f, 1, 0.25f);
	ClosedHiHat->PostEffects.AddLast(PostEffects::FilterQ(5000, 0.9f));
	//ClosedHiHat->PostEffects.AddLast(PostEffects::FilterHP(0.9f));
	ClosedHiHat->PostEffects.AddLast(PostEffects::Fade(0, 1000));
	ClosedHiHat->MinNoteDuration = 0.25f;

	result.Generators[41] = ClosedHiHat;

	auto AcousticBassDrum = new SynthesizedInstrument;
	AcousticBassDrum->Synth = CreateGeneratorSynthPass(
		Generators::DrumPhysicalModel(2, 8, 8, 0.092f, 0.0072f, 0.20f), 0.03f, 1, 0.25f);
	AcousticBassDrum->PostEffects.AddLast(PostEffects::FilterQ(8500, 0.3f));
	AcousticBassDrum->PostEffects.AddLast(PostEffects::FilterHP(0.4f));
	AcousticBassDrum->PostEffects.AddLast(PostEffects::Fade(0, 1000));
	AcousticBassDrum->MinNoteDuration = 0.2f;

	result.Generators[34] = AcousticBassDrum;
	result.Generators[35] = AcousticBassDrum;

	//auto instr1 = new SynthesizedInstrument;
	//instr1->Synth = CreateGeneratorSynthPass(DrumPhysicalModel(2, 16, 16, 0.342f, 0.00026f, 0.20f), 0.05f, 1, 0.35f);
	//instr1->Synth = CreateGeneratorSynthPass(Generators::SoundSampler<float(*)(float, float)>(DrumSample), 0.4f, 1, 1);
	//instr1->MinNoteDuration = 0.3f;

	//for(auto id: {39}) result.Generators[id]=instr1;

	return result;
}


MusicalInstruments::MusicalInstruments()
{
	SynthesizedInstrument pianoInstr1;
	pianoInstr1.Synth = CreateSineSynthPass(0.42f*0.25f, 3, 1);
	pianoInstr1.Modifiers.AddLast( CreateModifierPass(Modifiers::RelPulsator(0.5f)) );
	pianoInstr1.Attenuation = CreateExponentialAttenuationPass(4);
	pianoInstr1.MinNoteDuration = 1.2f;

	SynthesizedInstrument pianoInstr2;
	pianoInstr2.Synth = CreateSineSynthPass(0.38f*0.25f, 3, 1.5f);
	pianoInstr2.Modifiers.AddLast( CreateModifierPass(Modifiers::RelPulsator(0.5f)) );
	pianoInstr2.Attenuation = CreateExponentialAttenuationPass(3);
	pianoInstr2.MinNoteDuration = 1.5f;

	Piano.Combination.AddLast(pianoInstr1);
	Piano.Combination.AddLast(pianoInstr2);


	//Piano=CreateGuitar(20, 1, 2.15f, 1.3f, 1.3f, 1.0f, 1.2f);

	

	/*SynthesizedInstrument epiano2Instr1;
	epiano2Instr1.Synth = CreateSineSynthPass(0.52f*0.7f, 3, 1);
	epiano2Instr1.Modifiers.AddLast( CreateModifierPass(SoundModifiers::RelPulsator(0.5f)) );
	epiano2Instr1.Attenuation = CreateExponentialAttenuationPass(7);
	epiano2Instr1.MinNoteDuration = 0.5f;*/

	SynthesizedInstrument epiano2Instr2;
	epiano2Instr2.Synth = CreateSineExpSynthPass({{0.48f*0.3f, 5, 1.0f, 1.0f}});
	epiano2Instr2.MinNoteDuration = 0.4f;

	ElectricPiano2 = Piano;//{{epiano2Instr1/*, epianoInstr2*/}};
	ElectricPiano.Combination.AddLast(epiano2Instr2);
	//ElectricPiano=/*ElectricPiano2;*/{{epiano2Instr2/*, epianoInstr2*/}};


	/*SynthesizedInstrument ElectricPiano={
		CreateGeneratorSynthPass(Generators::RelModSine(2.5, 1), 0.5f, 1),
		null,
		CreateTableAttenuationPass(
			{0.7f, 1.0f, 1.0f, 0.9f, 0.8f, 0.7f, 0.65f, 0.6f, 0.5f, 0.42f, 0.37f, 0.33f, 0.28f, 0.24f, 0.2f, 0.15f, 0.11f, 0.0f})
	};*/

	/*SynthesizedInstrument ElectricPiano={
		CreateGeneratorSynthPass(Generators::RelModSine(0.75, 1), 0.25f, 1),
		null,
		//CreateAttenuationPass(SoundAttenuators::Exponential(1.0f, 7.0))
		CreateAttackDecayPass(0.003, 0.08)
	};*/


	Bass1.Synth = CreateGeneratorSynthPass(Generators::RelModSine(0.25, 0.25), 0.2f, 1, 1.0f);
	Bass1.Attenuation = CreateTableAttenuationPass(
			{norm8(0.7), 0.99, 0.99, 0.9, 0.8, 0.7, 0.65, 0.6, 0.5, 0.42, 0.28, 0.15, 0.11, 0.0});

	ElectricBassFinger.Synth = CreateGeneratorSynthPass(Generators::RelModSine(0.25, 0.25), 0.4f, 1, 1.0f);
	ElectricBassFinger.Attenuation = CreateTableAttenuationPass(
			{norm8(0.7), 0.99, 0.99, 0.9, 0.8, 0.7, 0.65, 0.6, 0.5, 0.42, 0.28, 0.15, 0.11, 0.0});

	Bass2.Synth = CreateGeneratorSynthPass(Generators::RelModSine(0.75, 1), 0.3f, 2);
	Bass2.Attenuation = CreateTableAttenuationPass(
			{norm8(0.7), 0.99, 0.99, 0.9, 0.8, 0.7, 0.65, 0.6, 0.5,
		        0.42, 0.37, 0.33, 0.28, 0.24, 0.2, 0.15, 0.11, 0.0});



	Bass3.Synth = CreateGeneratorSynthPass(Generators::RelModSine(0.75, 1), 1, 1);
	Bass3.Attenuation = CreateTableAttenuationPass(
			{norm8(0.1), 0.4, 0.99, 0.9, 0.8, 0.6, 0.55, 0.48, 0.45, 0.42,
		        0.37, 0.33, 0.28, 0.24, 0.2, 0.15, 0.11, 0.0});

	
	
	SynthBass1.Synth = CreateSawtoothSynthPass(20, 0.6f, 1, 0.5f);
	SynthBass1.Attenuation = CreateExponentialAttenuationPass(10);

	SynthBass2.Synth = CreateGeneratorSynthPass(Generators::RelModSine(0.75, 0.5f), 1, 1);
	SynthBass2.Attenuation = CreateExponentialAttenuationPass(7);



	SynthBrass.Synth = CreateSawtoothSynthPass(20, 0.12f, 3, 0.5f);
	SynthBrass.Attenuation = CreateAttackDecayPass(0.05, 0.1);

	Lead5Charang.Synth = CreateSawtoothSynthPass(20, 0.22f, 3, 0.5f);
	Lead5Charang.Attenuation = CreateAttackDecayPass(0.03, 0.05);

	/*SynthesizedInstrument InterestingInstrument1={ //Интересный звук типа скрипки. Может пригодится
		CreateSawtoothSynthPass(0.75, 1, 3, 1),
		{CreateModifierPass(SoundModifiers::RelPulsator(6))},
		//CreateAttenuationPass(SoundAttenuators::Exponential(1.0f, 7.0))
		CreateAttackDecayPass(0.1, 0.2)
	};

	SynthesizedInstrument InterestingInstrument2={ //Аналогично
		CreateSawtoothSynthPass(0.75, 1, 3, 1),
		{CreateModifierPass(SoundModifiers::RelPulsator(1))},
		//CreateAttenuationPass(SoundAttenuators::Exponential(1.0f, 7.0))
		CreateAttackDecayPass(0.1, 0.2)
	};*/


	SynthesizedInstrument fluteSine;
	fluteSine.Synth = CreateSineSynthPass(0.075f, 2, 0.5f);
	fluteSine.Attenuation = CreateAttackDecayPass(0.15, 0.05);

	SynthesizedInstrument fluteSawtooth;
	fluteSawtooth.Synth = CreateSawtoothSynthPass(0.5f, 0.15f, 1, 0.5f);
	fluteSawtooth.Attenuation = CreateAttackDecayPass(0.1, 0.05);

	Flute.Combination.AddLast(Meta::Move(fluteSine));
	Flute.Combination.AddLast(Meta::Move(fluteSawtooth));

	SynthesizedInstrument panFluteSine;
	panFluteSine.Synth = CreateSineSynthPass(0.2f, 2, 0.5f);
	panFluteSine.Attenuation = CreateAttackDecayPass(0.2, 0.05);

	SynthesizedInstrument panFluteSawtooth;
	panFluteSawtooth.Synth = CreateSawtoothSynthPass(0.4f, 0.3f, 2, 0.5f);
	panFluteSawtooth.Attenuation = CreateAttackDecayPass(0.25, 0.05);

	PanFlute.Combination.AddLast(panFluteSine);
	PanFlute.Combination.AddLast(panFluteSawtooth);


	Birds.Synth = CreateSineSynthPass(1, 2);
	Birds.Attenuation = CreateAttackDecayPass(0.1, 0.2);
	Birds.PostEffects.AddLast(PostEffects::Chorus(0.3f, 3, 0.75, 0.25));

	SynthVoice.Synth = CreateSawtoothSynthPass(2, 0.35f, 1, 0.5f);
	SynthVoice.Attenuation = CreateAttackDecayPass(0.1, 0.2);
	//SynthVoice.PostEffects.AddLast(PostEffects::FilterQ(6000, 0.6f));
	SynthVoice.PostEffects.AddLast(PostEffects::Chorus(0.002f, 2, 0.8f, 0.2f));
	SynthVoice.PostEffects.AddLast(PostEffects::FilterHP(0.8f));

	PadChoir.Synth = CreateSawtoothSynthPass(1, 0.5f, 1, 0.5f);
	PadChoir.Attenuation = CreateAttackDecayPass(0.2, 0.3);

	SoundTrackFX2.Synth = CreateSawtoothSynthPass(5, 0.15f, 2, 0.5f);
	SoundTrackFX2.Attenuation = CreateTableAttenuationPass(
			{norm8(0.1), 0.27, 0.41, 0.55, 0.74, 0.90, 0.99, 0.81, 0.62, 0.44, 0.25, 0.05});

	

	Pad7Halo.Synth = CreateSawtoothSynthPass(2, 0.2f, 1, 0.5f);
	Pad7Halo.Attenuation = CreateAttackDecayPass(0.08, 0.05);
	Pad7Halo.PostEffects.AddLast(PostEffects::Chorus(0.002f, 2, 0.75, 0.25));
	Pad7Halo.MinNoteDuration=0.3f;


	Pad8Sweep.Synth = CreateSawtoothSynthPass(2, 0.12f, 1, 0.5f);
	Pad8Sweep.Attenuation = CreateAttackDecayPass(0.08, 0.05);
	Pad8Sweep.PostEffects.AddLast(PostEffects::Chorus(0.002f, 2, 0.75, 0.25));
	Pad8Sweep.MinNoteDuration=0.3f;

	StringEnsemble.Synth = CreateSawtoothSynthPass(2, 0.15f, 1, 0.5f);
	StringEnsemble.Attenuation = CreateAttackDecayPass(0.08, 0.05);
	StringEnsemble.PostEffects.AddLast(PostEffects::Chorus(0.002f, 2, 0.75, 0.25));
	StringEnsemble.MinNoteDuration=0.3f;

	ReverseCymbal.Synth = CreateWhiteNoiseSynthPass(0.2f, 1, 20);//CreateSawtoothSynthPass(1.0f, 0.5f, 1, 1.0f);
	ReverseCymbal.Attenuation = CreateTableAttenuationPass(
			{norm8(0.1), 0.2, 0.45, 0.7, 0.99, 1.0, 0.32});
	ReverseCymbal.FadeOffTime = 1.0f;

	Sawtooth.Synth = CreateSawtoothSynthPass(10, 0.25f, 1, 0.5f);
	Sawtooth.Attenuation = CreateTableAttenuationPass(
			{norm8(0.5), 0.8, 0.996, 0.95, 0.92, 0.85, 0.72, 0.55, 0.42, 0.31, 0.17, 0.05});


	LeadSquare.Synth = CreateGeneratorSynthPass(Generators::RelModSine(1.5, 2), 0.2f, 1, 0.5f);
	LeadSquare.Attenuation = CreateTableAttenuationPass(
			{norm8(0.4), 0.7, 0.995, 0.98, 0.95, 0.92, 0.90, 0.89, 0.79, 0.57, 0.41, 0.22, 0.05});

	Crystal.Synth = CreateGeneratorSynthPass(Generators::RelModSine(0.75, 0.5f), 0.4f, 3, 1);
	Crystal.Attenuation = CreateExponentialAttenuationPass(7);

	

	SynthesizedInstrument bassLead;
	bassLead.Synth = CreateGeneratorSynthPass(Generators::RelModSine(0.75, 1), 0.45f, 1, 0.5f);
	bassLead.Attenuation = CreateExponentialAttenuationPass(6.5f);
	bassLead.MinNoteDuration = 0.4f;

	SynthesizedInstrument sawtoothBassLead;
	sawtoothBassLead.Synth = CreateSawtoothSynthPass(10, 0.35f, 1, 0.5f);
	sawtoothBassLead.Attenuation = CreateExponentialAttenuationPass(8);
	sawtoothBassLead.MinNoteDuration = 0.4f;

	BassLead.Combination.AddLast(bassLead);
	BassLead.Combination.AddLast(sawtoothBassLead);

	SynthesizedInstrument sawtoothElectricBassPick;
	sawtoothElectricBassPick.Synth = CreateSawtoothSynthPass(3, 0.25f, 1, 0.5f);
	sawtoothElectricBassPick.Attenuation = CreateExponentialAttenuationPass(8);
	sawtoothElectricBassPick.MinNoteDuration = 0.4f;

	ElectricBassPick.Combination.AddLast(bassLead);
	ElectricBassPick.Combination.AddLast(sawtoothElectricBassPick);


	SynthesizedInstrument orchestraHit = {
		CreateGeneratorSynthPass(Generators::RelModSine(0.85f, 1), 0.5f, 1, 0.5f),
		null,
		CreateExponentialAttenuationPass(7),
		//CreateTableAttenuationPass({0.996n8, 0.9f, 0.8f, 0.6f, 0.25f, 0.1f})
		null,
		0.4f
	};

	SynthesizedInstrument sawtoothOrchestraHit;
	sawtoothOrchestraHit.Synth = CreateSawtoothSynthPass(5, 0.4f, 1, 0.5f);
	sawtoothOrchestraHit.Attenuation = CreateExponentialAttenuationPass(7);

	OrchestraHit.Combination.AddLast(orchestraHit);
	OrchestraHit.Combination.AddLast(sawtoothOrchestraHit);


	Atmosphere.Synth = CreateSineSynthPass(0.6f, 4, 0.5f);
	Atmosphere.Attenuation = CreateTableAttenuationPass(
		{norm8(0.995), 0.3, 0.6, 0.5, 0.45, 0.3, 0.2, 0.15, 0.11, 0.08, 0.06, 0.02, 0.00});

	Rain.Synth = CreateSawtoothSynthPass(2, 0.5f, 1, 1);
	Rain.Modifiers.AddLast(CreateModifierPass(Modifiers::AddPulsator(1, 0.75f, 0.25f)));
	Rain.Attenuation = CreateTableAttenuationPass(
			{norm8(0.7), 0.96, 0.7, 0.6, 0.5, 0.45, 0.3, 0.2, 0.15, 0.11, 0.08, 0.06, 0.02, 0.00});

	Guitar = CreateGuitar(15, 3, 1.7f, 1.15f, 1, 0.5f, 5.0f, 0.35f);//CreateGuitar(15, 128, 3.5f, 1.1f);
	GuitarSteel = CreateGuitar(15, 5, 2.5f, 0.75f, 1.2f, 0.5f, 5.0f, 0.3f);//CreateGuitar(15, 224, 3.5f, 1.7f);

	OverdrivenGuitar.Synth = CreateSawtoothSynthPass(9.5, 0.35f, 1, 1);
	
	OverdrivenGuitar.Modifiers.AddLast( CreateModifierPass(Modifiers::RelPulsator(6)) );
	OverdrivenGuitar.Attenuation = CreateTableAttenuationPass(
			{norm8(0.1), 0.3, 0.7, 0.8, 0.75, 0.3, 0.25, 0.2, 0.16, 0.12, 0.09, 0.06, 0.03, 0.01, 0});

	Trumpet.Synth = CreateSawtoothSynthPass(8, 0.15f, 1, 0.5f);
	Trumpet.Attenuation = CreateAttackDecayPass(0.05, 0.7);

	Oboe.Synth = CreateSawtoothSynthPass(0.6f, 0.45f, 2, 1.0f);
	Oboe.Attenuation = CreateAttackDecayPass(0.07, 0.07);
	Oboe.PostEffects.AddLast(PostEffects::Chorus(0.002f, 1, 0.75, 0.25));

	FretlessBass.Synth = CreateSawtoothSynthPass(15, 0.4f, 1, 0.5f);
	FretlessBass.Attenuation = CreateExponentialAttenuationPass(8);

	Sax.Synth = CreateSawtoothSynthPass(0.2f, 0.2f, 2, 0.5f);
	Sax.Attenuation = CreateAttackDecayPass(0.02, 0.05);


	Calliope.Synth = CreateGeneratorSynthPass(Generators::RelModSine(2.5, 1), 0.2f, 1, 0.5f);
	Calliope.Attenuation = CreateTableAttenuationPass(
		{norm8(0.4), 0.9, 0.99, 0.9, 0.8, 0.7, 0.65, 0.6, 0.5, 0.42, 0.37, 0.33, 0.28, 0.24, 0.2, 0.15, 0.11, 0.0});


	/*static SynthesizedInstrument violinInstr1={
		CreateSawtoothSynthPass(0.85f, 0.5f/2, 1, 1),
		//CreateGeneratorSynthPass(Generators::Sawtooth(0.85f), 0.5f/3, 1),
		null,
		CreateAttackDecayPass(0.1, 0.1)
	};
	static SynthesizedInstrument violinInstr2={
		CreateSawtoothSynthPass(1.21f, 0.5f/2, 1, 1.01f),
		null,
		CreateAttackDecayPass(0.1, 0.1)
	};
	
	CombinedSynthesizedInstrument OldViolin={{violinInstr1, violinInstr2}};*/

	Violin.Synth = CreateGeneratorSynthPass(Generators::ViolinPhysicalModel(), 0.25f, 1, 0.375f);
	Violin.Attenuation = CreateAttackDecayPass(0.1, 0.1);

	Organ.Synth = CreateSawtoothSynthPass(0.1f, 0.03f, 4);
	Organ.Attenuation = CreateAttackDecayPass(0.01, 0.01);

	PercussiveOrgan.Synth = CreateSawtoothSynthPass(2, 0.2f, 3);
	PercussiveOrgan.Attenuation = CreateAttackDecayPass(0.2, 0.3);

	Whistle.Synth = CreateSineSynthPass(0.3f, 1, 0.5f);
	Whistle.Attenuation = CreateAttackDecayPass(0.2, 0.25);

	Sine2Exp.Synth = CreateSineSynthPass(1, 2);
	Sine2Exp.Attenuation = CreateExponentialAttenuationPass(9);

	Vibraphone.Synth = CreateSineSynthPass(0.3f, 5, 0.5f);
	Vibraphone.Attenuation = CreateExponentialAttenuationPass(7);
	Vibraphone.MinNoteDuration = 0.8f;

	Glockenspiel.Synth = CreateSineSynthPass(0.35f, 6, 0.5f);
	Glockenspiel.Attenuation = CreateExponentialAttenuationPass(8);
	Glockenspiel.MinNoteDuration = 0.8f;


	NewAge.Synth = CreateSineSynthPass(0.15f, 5, 0.5f);
	NewAge.Attenuation = CreateTableAttenuationPass(
	    {norm8(0.8), 0.995, 0.7, 0.5, 0.4, 0.05});
	NewAge.MinNoteDuration = 0.55f;


	Applause.Synth = CreateWhiteNoiseSynthPass(0.5f, 1, 10);
	Applause.Modifiers.AddLast( CreateModifierPass(Modifiers::AbsPulsator(20, 0.7f, 0.3f)) );
	Applause.Attenuation = CreateAttackDecayPass(1, 0.5);

	Helicopter.Synth = CreateWhiteNoiseSynthPass(4, 1, 0.1f);
	Helicopter.Modifiers.AddLast( CreateModifierPass(Modifiers::AbsPulsator(10, 0, 1)) );
	Helicopter.Attenuation = CreateAttackDecayPass(0.4, 0.4);

	Seashore.Synth = CreateWhiteNoiseSynthPass(0.07f, 1, 20);
	Seashore.Attenuation = CreateAttackDecayPass(1, 0.7);

	PhoneRing.Synth = CreateSineSynthPass(0.5f, 1, 0.5f);
	PhoneRing.Modifiers.AddLast( CreateModifierPass(Modifiers::AbsPulsator(5)) );
	PhoneRing.Attenuation = CreateAttackDecayPass(0.2, 0.2);
	

	GunShot.Synth = CreateWhiteNoiseSynthPass(0.4f, 1, 16);
	GunShot.Attenuation = CreateExponentialAttenuationPass(5);
	GunShot.MinNoteDuration = 0.7f;


	DrumSound2.Synth = CreateGeneratorSynthPass(
		Generators::FunctionGenerator<float(*)(float, float)>(DrumSample), 0.3f, 1, 0.01f);
	DrumSound2.MinNoteDuration = 0.3f;

	/*SynthesizedInstrument kalimba;
	kalimba.Synth = CreateSineSynthPass(0.1f, 3, 1);
	kalimba.Attenuation = CreateExponentialAttenuationPass(11);
	kalimba.MinNoteDuration = 0.3f;*/

	SynthesizedInstrument kalimba;
	kalimba.Synth = CreateSineExpSynthPass({{0.48f*0.5f, 8, 0.5f, 1}});
	kalimba.MinNoteDuration = 0.2f;

	Kalimba.Combination.AddLast(kalimba);
	
	Drums = CreateDrums();
}


SynthesizedInstrument MusicalInstruments::CreateGuitar(size_t n, float c,
	float d, float e, float f, float freqMult, float duration, float volume)
{
	if(n>20) n=20;
	SynthesizedInstrument result;
	result.FadeOffTime = duration;
	SineExpHarmonic harmonics[20];
	for(size_t i=1; i<=n; i++)
	{
		const float scale = Abs( ((Mod(c*float(i*i)+37.0f*float(i), 397.0f)/200.0f)-1.0f) )*Pow(float(i), -f);
		harmonics[i-1] = {scale * 0.5f*volume,   d+e*float(i-1),   freqMult*float(i), 1.0f/(float(i)*2.0f-1.0f)};
	}
	result.Synth = CreateSineExpSynthPass({harmonics, n});
	return result;
}

#endif

}}}
