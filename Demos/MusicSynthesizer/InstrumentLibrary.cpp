#include "Cpp/Warnings.h"

INTRA_DISABLE_REDUNDANT_WARNINGS

#include "Random/FastUniformNoise.h"

#include "Container/Sequential/Array.h"
#include "Container/Utility/Array2D.h"

#include "InstrumentLibrary.h"
#include "Audio/Synth/Generators/DrumPhysicalModel.h"

#include "Audio/Synth/WaveTableSampler.h"
#include "Audio/Synth/ADSR.h"
#include "Audio/Synth/MusicalInstrument.h"
#include "Audio/Synth/DrumInstrument.h"

#include "Audio/Synth/Generators.hh"
#include "Audio/Synth/PostEffects.hh"

using namespace Intra;
using namespace Audio;
using namespace Synth;

static float DrumSample(float freq, float t)
{
	using namespace Math;
	
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
	a += (Random::FastUniformNoise::Linear(t*freq*freq/10) +
		Random::FastUniformNoise::Linear(t*freq*freq))*0.1220703f;
	a *= Exp(-15*t);
	a = Clamp(a, -0.30517578f, 0.30517578f);
	a *= 2.831055f;
	return a;
}


InstrumentLibrary::InstrumentLibrary()
{
	{
		auto& harm1 = Piano.Waves.AddLast();
		harm1.Scale = 0.05f;
		harm1.ExpCoeff = 3;
		harm1.FreqMultiplier = 0.5f;

		auto& harm2 = Piano.Waves.AddLast();
		harm2.Scale = 0.05f;
		harm2.ExpCoeff = 3.5f;
		harm2.FreqMultiplier = 1;

		auto& harm3 = Piano.Waves.AddLast();
		harm3.Scale = -0.025f;
		harm3.ExpCoeff = 4;
		harm3.FreqMultiplier = 1.5f;

		auto& harm4 = Piano.Waves.AddLast();
		harm4.Scale = -0.05f;
		harm4.ExpCoeff = 4.5f;
		harm4.FreqMultiplier = 2;

		auto& harm5 = Piano.Waves.AddLast();
		harm5.Scale = -0.0125f;
		harm5.ExpCoeff = 6;
		harm5.FreqMultiplier = 3.5f;

		auto& harm6 = Piano.Waves.AddLast();
		harm6.Scale = -0.0125f;
		harm6.ExpCoeff = 7;
		harm6.FreqMultiplier = 4.5f;

		auto& harm7 = Piano.Waves.AddLast();
		harm7.Scale = 0.0125f;
		harm7.ExpCoeff = 8;
		harm7.FreqMultiplier = 5.5f;

		auto& harm8 = Piano.Waves.AddLast();
		harm8.Scale = -0.0125f;
		harm8.ExpCoeff = 9;
		harm8.FreqMultiplier = 6.5f;
	}

	ElectricPiano2 = Piano;

	{
		auto& wave = ElectricPiano.Waves.AddLast();
		wave.Scale = 0.19f;
		wave.ExpCoeff = 5;
	}

	{
		auto& wave = Bass1.Waves.AddLast();
		wave.Scale = 0.2f;
		wave.ExpCoeff = 3;
		wave.RateAcceleration = -0.1f;
	}

	{
		auto& wave = ElectricBassFinger.Waves.AddLast();
		wave.Scale = 0.4f;
		wave.ExpCoeff = 4;
		wave.RateAcceleration = -0.1f;
	}

	{
		auto& wave = Bass2.Waves.AddLast();
		wave.Scale = 0.3f;
		wave.ExpCoeff = 4;
		wave.Octaves = 2;
		wave.RateAcceleration = -0.15f;
	}

	{
		auto& wave = Bass3.Waves.AddLast();
		wave.Scale = 0.7f;
		wave.ExpCoeff = 4;
		wave.RateAcceleration = -0.15f;
	}

	{
		auto& wave = SynthBass1.Waves.AddLast();
		wave.Type = WaveType::Sawtooth;
		wave.Scale = 0.6f;
		wave.ExpCoeff = 10;
		wave.FreqMultiplier = 0.5f;
		wave.UpdownRatio = 20;
	}

	{
		auto& wave = SynthBass2.Waves.AddLast();
		wave.Scale = 0.8f;
		wave.ExpCoeff = 7;
		Wave.RateAcceleration = -0.12f;
	}


	{
		auto& wave = SynthBrass.Waves.AddLast();
		wave.Type = WaveType::Sawtooth;
		wave.Scale = 0.12f;
		wave.FreqMultiplier = 0.5f;
		wave.UpdownRatio = 20;
		wave.Octaves = 3;

		SynthBrass.ADSR.AttackTime = 0.05f;
		SynthBrass.ADSR.ReleaseTime = 0.1f;
	}

	{
		auto& wave = Lead5Charang.Waves.AddLast();
		wave.Type = WaveType::Sawtooth;
		wave.Scale = 0.22f;
		wave.FreqMultiplier = 0.5f;
		wave.UpdownRatio = 20;
		wave.Octaves = 3;

		Lead5Charang.ADSR.AttackTime = 0.03f;
		Lead5Charang.ADSR.ReleaseTime = 0.05f;
	}


	{
		auto& wave = Flute.Waves.AddLast();
		wave.Type = WaveType::Sawtooth;
		wave.Scale = 0.15f;
		wave.FreqMultiplier = 0.5f;
		wave.UpdownRatio = 0.5f;

		Flute.ADSR.AttackTime = 0.1f;
		Flute.ADSR.ReleaseTime = 0.05f;
	}

	{
		auto& wave = PanFlute.Waves.AddLast();
		wave.Type = WaveType::Sawtooth;
		wave.Scale = 0.3f;
		wave.FreqMultiplier = 0.5f;
		wave.UpdownRatio = 0.4f;
		wave.Octaves = 2;

		PanFlute.ADSR.AttackTime = 0.25f;
		PanFlute.ADSR.ReleaseTime = 0.05f;
	}


	{
		auto& wave = Birds.Waves.AddLast();
		wave.Octaves = 2;

		Birds.ADSR.AttackTime = 0.1f;
		Birds.ADSR.ReleaseTime = 0.2f;
		Birds.Chorus = PostEffects::ChorusFactory(0.3f, 3, 0.75, 0.25);
	}

	{
		auto& wave = SynthVoice.Waves.AddLast();
		wave.Type = WaveType::Sawtooth;
		wave.Scale = 0.35f;
		wave.FreqMultiplier = 0.5f;
		wave.Octaves = 2;

		SynthVoice.ADSR.AttackTime = 0.1f;
		SynthVoice.ADSR.ReleaseTime = 0.2f;

		SynthVoice.Chorus = ChorusFactory(0.002f, 2, 0.8f, 0.2f);
		//SynthVoice.PostEffects.AddLast(PostEffects::FilterHP(0.8f));
	}

	{
		auto& wave = PadChoir.Waves.AddLast();
		wave.Type = WaveType::Sawtooth;
		wave.Scale = 0.5f;
		wave.FreqMultiplier = 0.5f;
	
		PadChoir.ADSR.AttackTime = 0.2f;
		PadChoir.ADSR.ReleaseTime = 0.3f;
	}

	{
		auto& wave = SoundTrackFX2.Waves.AddLast();
		wave.Type = WaveType::Sawtooth;
		wave.Scale = 0.15f;
		wave.FreqMultiplier = 0.5f;
		wave.UpdownRatio = 5;
		wave.Octaves = 2;

		SoundTrackFX2.ADSR.AttackTime = 0.7f;
		SoundTrackFX2.ADSR.DecayTime = 0.3f;
		SoundTrackFX2.ADSR.SustainVolume = 0.5f;
		SoundTrackFX2.ADSR.ReleaseTime = 0.3f;
	}

	
	{
		auto& wave = Pad7Halo.Waves.AddLast();
		wave.Type = WaveType::Sawtooth;
		wave.Scale = 0.2f;
		wave.FreqMultiplier = 0.5f;
		wave.UpdownRatio = 2;

		Pad7Halo.ADSR.AttackTime = 0.1;
		Pad7Halo.ADSR.ReleaseTime = 0.15f;
		Pad7Halo.Chorus = PostEffects::ChorusFactory(0.002f, 2, 0.75, 0.25);
	}

	{
		auto& wave = Pad8Sweep.Sawtoothes.AddLast({0.12f, 0, 0.5f, 2, 1, 0});
		wave.Type = WaveType::Sawtooth;
		wave.Scale = 0.12f;
		wave.FreqMultiplier = 0.5f;
		wave.UpdownRatio = 2;

		Pad8Sweep.ADSR.AttackTime = 0.1;
		Pad8Sweep.ADSR.ReleaseTime = 0.15f;
		Pad8Sweep.Chorus = ChorusFactory(0.003f, 2, 0.75, 0.25);
	}

	{
		auto& wave = StringEnsemble.Sawtoothes.AddLast({0.15f, 0, 0.5f, 2, 1, 0});
		wave.Type = WaveType::Sawtooth;
		wave.Scale = 0.15f;
		wave.FreqMultiplier = 0.5f;
		wave.UpdownRatio = 2;
		
		StringEnsemble.ADSR.AttackTime = 0.1;
		StringEnsemble.ADSR.ReleaseTime = 0.15f;
		StringEnsemble.Chorus = ChorusFactory(0.004f, 2, 0.75, 0.25);
	}

	ReverseCymbal.WhiteNoise.FreqMultiplier = 2;
	ReverseCymbal.WhiteNoise.VolumeScale = 0.2f;
	ReverseCymbal.ADSR.AttackTime = 1;
	ReverseCymbal.ADSR.ReleaseTime = 0.8f;

	{
		auto& wave = Sawtooth.Waves.AddLast();
		wave.Type = WaveType::Sawtooth;
		wave.Scale = 0.12f;
		wave.FreqMultiplier = 0.5f;
		wave.UpdownRatio = 2;

		Sawtooth.Sawtoothes.AddLast({0.25f, 0, 0.5f, 10, 1, 0});
		Sawtooth.ADSR.AttackTime = 0.15f;
		Sawtooth.ADSR.DecayTime = 1;
		Sawtooth.ADSR.SustainVolume = 0.7f;
		Sawtooth.ADSR.ReleaseTime = 0.5f;
	}

	{
		auto& wave = Accordion.Waves.AddLast();
		wave.Type = WaveType::Sawtooth;
		wave.Scale = 0.4f;
		wave.FreqMultiplier = 0.5f;
		wave.UpdownRatio = 5;
		wave.Octaves = 3;

		Accordion.ADSR.ReleaseTime = 0.3f;
	}

	{
		auto& wave = LeadSquare.Waves.AddLast({0.2f, 0, 0.5f, 1, 1, 0});
		//LeadSquare.Sines.AddLast({0.2f, 0, 0.5f, 1, -0.2f});
		wave.Type = WaveType::Square; //раньше было Sine
		//wave.RateAcceleration = -0.2f;
		wave.Scale = 0.2f;
		wave.FreqMultiplier = 0.5f;

		LeadSquare.ADSR.AttackTime = 0.1f;
		LeadSquare.ADSR.ReleaseTime = 0.5f;
	}

	{
		auto& wave = Crystal.Sines.AddLast({0.4f, 7, 1, 3, -0.1f});
		wave.ExpCoeff = 7;
		wave.Octaves = 3;
		wave.RateAcceleration = -0.1f;
	}

	{
		auto& sine = BassLead.Waves.AddLast();
		sine.Volume = 0.45f;
		sine.ExpCoeff = 6.5f;
		sine.FreqMultiplier = 0.5f;
		sine.RateAcceleration = -0.1f;

		auto& saw = BassLead.Sawtoothes.AddLast();
		saw.Type = WaveType::Sawtooth;
		saw.Scale = 0.35f;
		saw.ExpCoeff = 8;
		saw.FreqMultiplier = 0.5f;
		saw.UpdownRatio = 10;
	}

	{
		auto& sine = ElectricBassPick.Waves.AddLast();
		sine.Volume = 0.45f;
		sine.ExpCoeff = 6.5f;
		sine.FreqMultiplier = 0.5f;
		sine.RateAcceleration = -0.1f;

		auto& saw = ElectricBassPick.Waves.AddLast();
		saw.Type = WaveType::Sawtooth;
		saw.Scale = 0.25f;
		saw.ExpCoeff = 8;
		saw.FreqMultiplier = 0.5f;
		saw.UpdownRatio = 3;
	}

	{
		auto& sine = OrchestraHit.Waves.AddLast();
		sine.Volume = 0.3f;
		sine.ExpCoeff = 7;
		sine.FreqMultiplier = 0.5f;
		sine.RateAcceleration = -0.1f;

		auto& saw = OrchestraHit.Waves.AddLast();
		saw.Type = WaveType::Sawtooth;
		saw.Scale = 0.2f;
		saw.ExpCoeff = 7;
		saw.FreqMultiplier = 0.5f;
		saw.UpdownRatio = 5;
	}


	{
		auto& wave = Atmosphere.Waves.AddLast();
		wave.Scale = 0.6f;
		wave.FreqMultiplier = 0.5f;
		wave.Octaves = 4;

		Atmosphere.ADSR.AttackTime = 0.1f;
		Atmosphere.ADSR.DecayTime = 0.3f;
		Atmosphere.ADSR.SustainVolume = 0.3f;
		Atmosphere.ADSR.ReleaseTime = 0.2f;
	}

	{
		auto& wave = Rain.Waves.AddLast();
		wave.Type = WaveType::Sawtooth;
		wave.Scale = 0.5f;
		wave.UpdownRatio = 2;

		Rain.Chorus.MaxDelay = 0.005f;
		Rain.Chorus.DelayFrequency = 3;
		Rain.Chorus.MainVolume = 0.75f;
		Rain.Chorus.SecondaryVolume = 0.25f;

		Rain.ADSR.AttackTime = 0.03f;
		Rain.ADSR.DecayTime = 0.8f;
		Rain.ADSR.SustainVolume = 0.4f;
		Rain.ADSR.ReleaseTime = 0.3f;
	}

	//Guitar = CreateGuitar(15, 128, 3.5f, 1.1f, 1, 1, 0.35f);
	Guitar = CreateGuitar(15, 3, 1.7f, 1.15f, 1, 0.5f, 0.35f);
	//GuitarSteel = CreateGuitar(15, 224, 3.5f, 1.7f, 1, 0.5f, 0.3f);
	GuitarSteel = CreateGuitar(15, 5, 2.5f, 0.75f, 1.2f, 0.5f, 0.3f);

	{
		auto& wave = OverdrivenGuitar.Waves.AddLast();
		wave.Type = WaveType::WhiteNoise;
		wave.Scale = 0.35f;
		wave.ExpCoeff = 4;
		wave.RateAcceleration = -0.1f;
	}

	/*OverdrivenGuitar.Synth = CreateSawtoothSynthPass(9.5, 0.35f, 1, 1);
	
	OverdrivenGuitar.Modifiers.AddLast( CreateModifierPass(Modifiers::RelPulsator(6)) );
	OverdrivenGuitar.Attenuation = CreateTableAttenuationPass(
		{Norm8(0.1), 0.3, 0.7, 0.8, 0.75, 0.3, 0.25, 0.2, 0.16, 0.12, 0.09, 0.06, 0.03, 0.01, 0});*/


	{
		auto& wave = Trumpet.Waves.AddLast();
		wave.Type = WaveType::Sawtooth;
		wave.Scale = 0.2f;
		wave.FreqMultiplier = 0.5f;
		wave.UpdownRatio = 8;

		Trumpet.ADSR.AttackTime = 0.05f;
		Trumpet.ADSR.ReleaseTime = 0.3f;
	}

	{
		auto& wave = Piccolo.Waves.AddLast();
		wave.Type = WaveType::Sawtooth;
		wave.Scale = 0.3f;
		wave.FreqMultiplier = 0.5f;
		wave.UpdownRatio = 6;

		Piccolo.ADSR.ReleaseTime = 0.3f;
	}

	{
		auto& wave = Oboe.Waves.AddLast();
		wave.Type = WaveType::Sawtooth;
		wave.Scale = 0.45f;
		wave.UpdownRatio = 0.6f;
		wave.Octaves = 2;

		Oboe.ADSR.AttackTime = 0.07f;
		Oboe.ADSR.ReleaseTime = 0.07f;
		Oboe.Chorus = ChorusFactory(0.002f, 1, 0.75, 0.25);
	}

	{
		auto wave = FretlessBass.Waves.AddLast();
		wave.Type = WaveType::Sawtooth;
		wave.ExpCoeff = 8;
		wave.Scale = 0.4f;
		wave.FreqMultiplier = 0.5f;
		wave.UpdownRatio = 15;
	}

	{
		auto& wave = Sax.Waves.AddLast();
		wave.Type = WaveType::Sawtooth;
		wave.Scale = 0.2f;
		wave.FreqMultiplier = 0.5f;
		wave.UpdownRatio = 0.2f;
		wave.Octaves = 2;

		Sax.ADSR.AttackTime = 0.02f;
		Sax.ADSR.ReleaseTime = 0.05f;
	}


	{
		auto& wave = Calliope.Waves.AddLast();
		wave.Scale = 0.2f;
		wave.FreqMultiplier = 0.5f;
		wave.RateAcceleration = -0.2f;

		Calliope.ADSR.AttackTime = 0.1f;
		Calliope.ADSR.DecayTime = 0.5f;
		Calliope.ADSR.SustainVolume = 0.5f;
		Calliope.ADSR.ReleaseTime = 0.5f;
	}


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

	//Violin.Synth = CreateGeneratorSynthPass(Generators::ViolinPhysicalModel(), 0.25f, 1, 0.375f);
	//Violin.Attenuation = CreateAttackDecayPass(0.1, 0.1);

	{
		auto& wave = Organ.Waves.AddLast();
		wave.Type = WaveType::Sawtooth;
		wave.Scale = 0.03f;
		wave.UpdownRatio = 0.1f;
		wave.Octaves = 4;

		Organ.ADSR.AttackTime = 0.01f;
		Organ.ADSR.ReleaseTime = 0.01f;
	}

	{
		auto& wave = PercussiveOrgan.Sawtoothes.AddLast({0.2f, 0, 1, 2, 3, 0});
		wave.Type = WaveType::Sawtooth;
		wave.Scale = 0.2f;
		wave.UpdownRatio = 2;
		wave.Octaves = 3;

		PercussiveOrgan.ADSR.AttackTime = 0.2f;
		PercussiveOrgan.ADSR.ReleaseTime = 0.3f;
	}

	{
		auto& wave = Whistle.Waves.AddLast();
		wave.Scale = 0.3f;
		wave.FreqMultiplier = 0.5f;
		
		Whistle.ADSR.AttackTime = 0.2f;
		Whistle.ADSR.ReleaseTime = 0.25f;
	
		Whistle.Chorus.MaxDelay = 0.03f;
		Whistle.Chorus.MainVolume = 0.75f;
		Whistle.Chorus.SecondaryVolume = 0.25f;
		Whistle.Chorus.DelayFrequency = 10;
	}

	{
		auto& wave = Sine2Exp.Waves.AddLast();
		wave.ExpCoeff = 9;
		wave.Octaves = 2;
	}

	{
		auto& wave = Vibraphone.Waves.AddLast({0.3f, 7, 0.5f, 5, 0});
		wave.Scale = 0.3f;
		wave.ExpCoeff = 7;
		wave.FreqMultiplier = 0.5f;
		wave.Octaves = 5;
	}

	{
		auto& wave = Glockenspiel.Waves.AddLast();
		wave.Scale = 0.35f;
		wave.ExpCoeff = 8;
		wave.FreqMultiplier = 0.5f;
		wave.Octaves = 6;
	}


	{
		auto& wave = NewAge.Waves.AddLast();
		wave.Scale = 0.2f;
		wave.FreqMultiplier = 0.5f;
		wave.Octaves = 5;
	
		NewAge.ADSR.AttackTime = 0.05f;
		NewAge.ADSR.DecayTime = 0.3f;
		NewAge.ADSR.SustainVolume = 0.7f;
		NewAge.ADSR.ReleaseTime = 0.5f;
	}


	Applause.WhiteNoise.FreqMultiplier = 2;
	Applause.WhiteNoise.VolumeScale = 0.5f;
	Applause.Chorus.DelayFrequency = 10;
	Applause.Chorus.MainVolume = 0.7f;
	Applause.Chorus.SecondaryVolume = 0.3f;
	Applause.Chorus.MaxDelay = 0.05f;
	Applause.ADSR.AttackTime = 0.7f;
	Applause.ADSR.ReleaseTime = 0.5f;

	Helicopter.WhiteNoise.FreqMultiplier = 0.1f;
	Helicopter.WhiteNoise.VolumeScale = 1;
	Helicopter.Chorus.DelayFrequency = 3;
	Helicopter.Chorus.MainVolume = 0.5f;
	Helicopter.Chorus.SecondaryVolume = 0.5f;
	Helicopter.Chorus.MaxDelay = 0.1f;
	Helicopter.ADSR.AttackTime = 0.4f;
	Helicopter.ADSR.ReleaseTime = 0.4f;

	Seashore.WhiteNoise.FreqMultiplier = 20;
	Seashore.WhiteNoise.VolumeScale = 0.07f;
	Seashore.ADSR.AttackTime = 1;
	Seashore.ADSR.ReleaseTime = 0.7f;

	{
		auto& wave = PhoneRing.Waves.AddLast();
		wave.Scale = 0.5f;
		wave.FreqMultiplier = 0.5f;

		PhoneRing.ADSR.AttackTime = 0.2f;
		PhoneRing.ADSR.ReleaseTime = 0.2f;
		//PhoneRing.Modifiers.AddLast( CreateModifierPass(Modifiers::AbsPulsator(5)) );
	}
	

	GunShot.WhiteNoise.FreqMultiplier = 1;
	GunShot.WhiteNoise.VolumeScale = 0.4f;
	GunShot.ExponentAttenuation.ExpCoeff = 5;


	/*DrumSound2.Synth = CreateGeneratorSynthPass(
		Generators::FunctionGenerator<float(*)(float, float)>(DrumSample), 0.3f, 1, 0.01f);
	DrumSound2.MinNoteDuration = 0.3f;*/

	/*SynthesizedInstrument kalimba;
	kalimba.Synth = CreateSineSynthPass(0.1f, 3, 1);
	kalimba.Attenuation = CreateExponentialAttenuationPass(11);
	kalimba.MinNoteDuration = 0.3f;*/

	{
		auto& wave = Kalimba.Waves.AddLast();
		wave.Scale = 0.24f;
		wave.ExpCoeff = 8;
		wave.FreqMultiplier = 0.5f;
	}
	
	UniDrum.Data = Generators::DrumPhysicalModel(2, 16, 16, 0.342f, 0.00026f, 0.20f);
	UniDrum.VolumeScale = 0.03f;
	//UniDrum->Synth = CreateGeneratorSynthPass(Generators::FunctionGenerator<float(*)(float, float)>(DrumSample), 0.1f, 1, 0.2f);
	//UniDrum->PostEffects.AddLast(PostEffects::FilterQ(4000, 0.6f));
	//UniDrum->PostEffects.AddLast(PostEffects::FilterHP(0.9f));
	//UniDrum->PostEffects.AddLast(PostEffects::Fade(0, 1000));
	//UniDrum->MinNoteDuration = 0.25f;


	/*auto ClosedHiHat = new SynthesizedInstrument;
	ClosedHiHat->Synth = CreateGeneratorSynthPass(DrumGenerator(2, 16, 16, 0.342f, 0.00026f, 0.20f), 0.03f, 1, 0.35f);
	ClosedHiHat->PostEffects.AddLast(PostEffects::FilterQ(12000, 0.2f));
	//ClosedHiHat->PostEffects.AddLast(PostEffects::FilterHP(0.9f));
	ClosedHiHat->PostEffects.AddLast(PostEffects::Fade(0, 1000));
	ClosedHiHat->MinNoteDuration = 0.25f;*/

	ClosedHiHat.Data = Range::Take(Generators::DrumPhysicalModel(2, 16, 16, 0.338f, 0.04928f, 0.10f), 44100);
	ClosedHiHat.VolumeScale = 0.03f;
	//ClosedHiHat->PostEffects.AddLast(PostEffects::FilterQ(5000, 0.9f));
	//ClosedHiHat->PostEffects.AddLast(PostEffects::FilterHP(0.9f));
	//ClosedHiHat->PostEffects.AddLast(PostEffects::Fade(0, 1000));
	//ClosedHiHat->MinNoteDuration = 0.25f;


	AcousticBassDrum.Data = Range::Take(Generators::DrumPhysicalModel(2, 8, 8, 0.092f, 0.0072f, 0.20f), 44100);
	AcousticBassDrum.VolumeScale = 0.03f;
	//AcousticBassDrum->PostEffects.AddLast(PostEffects::FilterQ(8500, 0.3f));
	//AcousticBassDrum->PostEffects.AddLast(PostEffects::FilterHP(0.4f));
	//AcousticBassDrum->PostEffects.AddLast(PostEffects::Fade(0, 1000));
	//AcousticBassDrum->MinNoteDuration = 0.2f;

	//auto instr1 = new SynthesizedInstrument;
	//instr1->Synth = CreateGeneratorSynthPass(DrumPhysicalModel(2, 16, 16, 0.342f, 0.00026f, 0.20f), 0.05f, 1, 0.35f);
	//instr1->Synth = CreateGeneratorSynthPass(Generators::SoundSampler<float(*)(float, float)>(DrumSample), 0.4f, 1, 1);
	//instr1->MinNoteDuration = 0.3f;
}


MusicalInstrument InstrumentLibrary::CreateGuitar(size_t n, float c,
	float d, float e, float f, float freqMult, float volume)
{
	MusicalInstrument result;
	for(size_t i = 1; i <= n; i++)
	{
		float scale = Math::Mod(c*float(i*i) + 37.0f*float(i), 397.0f);
		scale = Math::Abs(scale / 200 - 1);
		scale *= Math::Pow(float(i), -f);
		auto& sine = result.Waves.AddLast();
		sine.Scale = scale * 0.5f*volume;
		sine.ExpCoeff = d + e*float(i-1);
		sine.FreqMultiplier = freqMult*float(i);
	}
	return result;
}

