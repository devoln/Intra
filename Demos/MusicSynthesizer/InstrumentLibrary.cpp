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
#include "Audio/Synth/RecordedSampler.h"

#include "Audio/Synth/Generators.hh"
#include "Audio/Synth/PostEffects.hh"

#include "Audio/Synth/WaveTableGeneration.h"

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
	ChoirATables.Generator = [](float freq, uint sampleRate)
	{
		WaveTable tbl;
		tbl.BaseLevelLength = 32768;
		tbl.Data.Reserve(tbl.BaseLevelLength * 2);
		tbl.Data.SetCount(tbl.BaseLevelLength / 2);
		tbl.BaseLevelRatio = freq/sampleRate;
		for(int i = 1; i<64; i++)
		{
			using namespace Math;
			const float formants = Exp(-Sqr((i*freq - 600) / 150)) +
				Exp(-Sqr((i*freq - 900) / 250)) +
				Exp(-Sqr((i*freq - 2200) / 200)) +
				Exp(-Sqr((i*freq - 2600) / 250)) +
				Exp(-Sqr((i*freq) / 3000)) / 10;
			Synth::AddSineHarmonicGaussianProfile(tbl.Data, tbl.BaseLevelRatio, float(i), 1, formants/(i*i), 60);
		}
		ConvertAmplitudesToSamples(tbl);
		return tbl;
	};
	ChoirATables.MaxRateDistance = 1.2f;
	
	ChoirAahs.WaveTables.AddLast({&ChoirATables, 0, 0.25f});
	ChoirAahs.ADSR.AttackTime = 0.15f;
	ChoirAahs.ADSR.DecayTime = 0.5f;
	ChoirAahs.ADSR.SustainVolume = 0.8f;
	ChoirAahs.ADSR.ReleaseTime = 0.2f;

	RockOrgan.WaveTables.AddLast({&ChoirATables, 0, 0.5f});
	RockOrgan.ADSR.AttackTime = 0.003f;
	RockOrgan.ADSR.SustainVolume = 0.6f;
	RockOrgan.ADSR.DecayTime = 0.3f;
	RockOrgan.ADSR.ReleaseTime = 0.05f;

	ChoirOTables.Generator = [](float freq, uint sampleRate)
	{
		WaveTable tbl;
		tbl.BaseLevelLength = 32768;
		tbl.Data.Reserve(tbl.BaseLevelLength * 2);
		tbl.Data.SetCount(tbl.BaseLevelLength / 2);
		tbl.BaseLevelRatio = freq/sampleRate;
		for(int i = 1; i<64; i++)
		{
			using namespace Math;
			const float formants = Exp(-Sqr((i*freq - 1000) / 100)) +
				Exp(-Sqr((i*freq - 700) / 300)) +
				Exp(-Sqr((i*freq - 2200) / 200)) +
				Exp(-Sqr((i*freq - 2600) / 250)) +
				Exp(-Sqr((i*freq) / 3000)) / 10;
			Synth::AddSineHarmonicGaussianProfile(tbl.Data, tbl.BaseLevelRatio, float(i), 1, formants/(i*i), 60);
		}
		ConvertAmplitudesToSamples(tbl);
		return tbl;
	};
	ChoirOTables.MaxRateDistance = 1.2f;

	PadChoir.WaveTables.AddLast({&ChoirOTables, 0, 0.4f});
	PadChoir.ADSR.AttackTime = 0.2f;
	PadChoir.ADSR.ReleaseTime = 0.2f;


	SynthStringTables.Generator = [](float freq, uint sampleRate)
	{
		WaveTable tbl;
		tbl.BaseLevelLength = 32768;
		tbl.Data.Reserve(tbl.BaseLevelLength * 2);
		tbl.Data.SetCount(tbl.BaseLevelLength / 2);
		tbl.BaseLevelRatio = freq/sampleRate;
		for(int i = 1; i<64; i++)
		{
			using namespace Math;
			const float formants = Exp(-Sqr((i*freq - 600) / 150)) +
				Exp(-Sqr((i*freq - 900) / 350)) +
				Exp(-Sqr((i*freq - 2200) / 1200)) +
				Exp(-Sqr((i*freq - 2600) / 250)) +
				Exp(-Sqr((i*freq) / 3000)) / 10;
			Synth::AddSineHarmonicGaussianProfile(tbl.Data, tbl.BaseLevelRatio, float(i), 1, formants/(i*i), 40);
		}
		ConvertAmplitudesToSamples(tbl);
		return tbl;
	};
	SynthStringTables.MaxRateDistance = 2;

	SynthVoice.WaveTables.AddLast({&SynthStringTables, 0, 0.25f});
	SynthVoice.ADSR.AttackTime = 0.04f;
	SynthVoice.ADSR.ReleaseTime = 0.2f;
	
	Pad8Sweep.WaveTables.AddLast({&SynthStringTables, 0, 0.2f});
	Pad8Sweep.ADSR.AttackTime = 0.15f;
	Pad8Sweep.ADSR.ReleaseTime = 0.25f;

	StringEnsemble.WaveTables.AddLast({&SynthStringTables, 0, 0.2f});
	StringEnsemble.ADSR.AttackTime = 0.07f;
	StringEnsemble.ADSR.ReleaseTime = 0.05f;

	ViolinTables.Generator = [](float freq, uint sampleRate)
	{
		WaveTable tbl;
		tbl.BaseLevelLength = 32768;
		tbl.Data.Reserve(tbl.BaseLevelLength * 2);
		tbl.Data.SetCount(tbl.BaseLevelLength / 2);
		tbl.BaseLevelRatio = freq/sampleRate;
		for(int i = 1; i<64; i++)
		{
			using namespace Math;
			const float formants = Exp(-Sqr((i*freq - 600) / 100)) +
				Exp(-Sqr((i*freq - 900) / 350)) +
				Exp(-Sqr((i*freq - 2200) / 1200)) +
				Exp(-Sqr((i*freq - 2600) / 150)) +
				Exp(-Sqr((i*freq) / 3000)) / 3;
			Synth::AddSineHarmonicGaussianProfile(tbl.Data, tbl.BaseLevelRatio, float(i), 0.5f, formants/(i), 5);
		}
		ConvertAmplitudesToSamples(tbl);
		return tbl;
	};
	ViolinTables.MaxRateDistance = 2;

	Violin.WaveTables.AddLast({&ViolinTables, 0, 1.0f});
	Violin.ADSR.AttackTime = 0.1f;
	Violin.ADSR.DecayTime = 0.3f;
	Violin.ADSR.SustainVolume = 0.4f;
	Violin.ADSR.ReleaseTime = 0.08f;

	/*SynthOrganTables.Generator = [](float freq, uint sampleRate)
	{
		WaveTable tbl;
		tbl.BaseLevelLength = 32768;
		tbl.Data.Reserve(tbl.BaseLevelLength * 2);
		tbl.Data.SetCount(tbl.BaseLevelLength / 2);
		tbl.BaseLevelRatio = freq/sampleRate;
		for(int i = 1; i<10; i++)
		{
			float ampl = 1.0f/(i);
			Synth::AddSineHarmonicGaussianProfile(tbl.Data, tbl.BaseLevelRatio, float(1 << (i-1)), 1, ampl, 10);
		}
		ConvertAmplitudesToSamples(tbl, 0.25f);
		return tbl;
	}*/

	SynthBrassTables.Generator = [](float freq, uint sampleRate)
	{
		WaveTable tbl;
		tbl.BaseLevelLength = 32768;
		tbl.Data.Reserve(tbl.BaseLevelLength * 2);
		tbl.Data.SetCount(tbl.BaseLevelLength / 2);
		tbl.BaseLevelRatio = freq/sampleRate;
		for(int i = 1; i<64; i++)
		{
			float ampl = 1.0f/i;
			Synth::AddSineHarmonicGaussianProfile(tbl.Data, tbl.BaseLevelRatio, float(i), 0.85f, ampl, 15);
		}
		ConvertAmplitudesToSamples(tbl);
		return tbl;
	};
	SynthBrassTables.MaxRateDistance = 4;

	SynthBrass.WaveTables.AddLast({&SynthBrassTables, 0, 0.25f});
	SynthBrass.ADSR.AttackTime = 0.005f;
	SynthBrass.ADSR.DecayTime = 0.5f;
	SynthBrass.ADSR.SustainVolume = 0.7f;
	SynthBrass.ADSR.ReleaseTime = 0.1f;

	{
		auto& harm1 = Piano.Waves.EmplaceLast();
		harm1.Scale = 0.05f;
		harm1.ExpCoeff = 3;
		harm1.FreqMultiplier = 1;

		auto& harm2 = Piano.Waves.EmplaceLast();
		harm2.Scale = -0.025f;
		harm2.ExpCoeff = 3;
		harm2.FreqMultiplier = 3;

		auto& harm3 = Piano.Waves.EmplaceLast();
		harm3.Scale = -0.025f;
		harm3.ExpCoeff = 3;
		harm3.FreqMultiplier = 5;

		auto& harm4 = Piano.Waves.EmplaceLast();
		harm4.Scale = 0.0125f;
		harm4.ExpCoeff = 3;
		harm4.FreqMultiplier = 7;

		auto& harm5 = Piano.Waves.EmplaceLast();
		harm5.Scale = -0.0125f;
		harm5.ExpCoeff = 3;
		harm5.FreqMultiplier = 9;


		auto& harm6 = Piano.Waves.EmplaceLast();
		harm6.Scale = 0.05f;
		harm6.ExpCoeff = 4;
		harm6.FreqMultiplier = 2;

		auto& harm7 = Piano.Waves.EmplaceLast();
		harm7.Scale = -0.05f;
		harm7.ExpCoeff = 4;
		harm7.FreqMultiplier = 4;

		auto& harm8 = Piano.Waves.EmplaceLast();
		harm8.Scale = 0.025f;
		harm8.ExpCoeff = 4;
		harm8.FreqMultiplier = 5;

		auto& harm9 = Piano.Waves.EmplaceLast();
		harm9.Scale = -0.025f;
		harm9.ExpCoeff = 4;
		harm9.FreqMultiplier = 7;

		auto& harm10 = Piano.Waves.EmplaceLast();
		harm10.Scale = 0.0125f;
		harm10.ExpCoeff = 4;
		harm10.FreqMultiplier = 11;

		auto& harm11 = Piano.Waves.EmplaceLast();
		harm11.Scale = -0.0125f;
		harm11.ExpCoeff = 4;
		harm11.FreqMultiplier = 13;

		Piano.ADSR.AttackTime = 0.003f;
		Piano.ADSR.ReleaseTime = 0.15f;
	}

	{
		auto& harm1 = ElectricPiano.Waves.EmplaceLast();
		harm1.Scale = 0.05f;
		harm1.ExpCoeff = 3;
		harm1.FreqMultiplier =1;

		auto& harm2 = ElectricPiano.Waves.EmplaceLast();
		harm2.Scale = 0.05f;
		harm2.ExpCoeff = 3.1f;
		harm2.FreqMultiplier = 2;

		auto& harm3 = ElectricPiano.Waves.EmplaceLast();
		harm3.Scale = -0.025f;
		harm3.ExpCoeff = 3.2f;
		harm3.FreqMultiplier = 3;

		auto& harm4 = ElectricPiano.Waves.EmplaceLast();
		harm4.Scale = -0.05f;
		harm4.ExpCoeff = 3.3f;
		harm4.FreqMultiplier = 4;

		auto& harm5 = ElectricPiano.Waves.EmplaceLast();
		harm5.Scale = -0.0125f;
		harm5.ExpCoeff = 3.6f;
		harm5.FreqMultiplier = 7;

		auto& harm6 = ElectricPiano.Waves.EmplaceLast();
		harm6.Scale = -0.0125f;
		harm6.ExpCoeff = 3.8f;
		harm6.FreqMultiplier = 9;

		auto& harm7 = ElectricPiano.Waves.EmplaceLast();
		harm7.Scale = 0.0125f;
		harm7.ExpCoeff = 4;
		harm7.FreqMultiplier = 11;

		auto& harm8 = ElectricPiano.Waves.EmplaceLast();
		harm8.Scale = -0.0125f;
		harm8.ExpCoeff = 4.2f;
		harm8.FreqMultiplier = 13;

		ElectricPiano.ADSR.AttackTime = 0.003f;
		ElectricPiano.ADSR.ReleaseTime = 0.15f;
	}

	ElectricPiano2 = ElectricPiano;

	{
		auto& wave = Bass1.Waves.EmplaceLast();
		wave.Scale = 0.3f;
		wave.ExpCoeff = 5;
		wave.RateAcceleration = -0.03f;
		wave.FreqMultiplier = 2;

		Bass1.ADSR.AttackTime = 0.02f;
		Bass1.ADSR.ReleaseTime = 0.1f;
	}

	{
		auto& wave = ElectricBassFinger.Waves.EmplaceLast();
		wave.Scale = 0.4f;
		wave.ExpCoeff = 4;
		//wave.RateAcceleration = -0.1f;
		wave.FreqMultiplier = 2;

		ElectricBassFinger.ADSR.AttackTime = 0.01f;
		ElectricBassFinger.ADSR.ReleaseTime = 0.1f;
	}

	{
		auto& wave = Bass2.Waves.EmplaceLast();
		wave.Scale = 0.3f;
		wave.ExpCoeff = 4;
		wave.Octaves = 2;
		//wave.RateAcceleration = -0.15f;
		wave.FreqMultiplier = 2;

		Bass2.ADSR.AttackTime = 0.01f;
		Bass2.ADSR.ReleaseTime = 0.1f;
	}

	{
		auto& wave = Bass3.Waves.EmplaceLast();
		wave.Scale = 0.7f;
		wave.ExpCoeff = 4;
		//wave.RateAcceleration = -0.15f;
		wave.FreqMultiplier = 2;

		Bass3.ADSR.AttackTime = 0.01f;
		Bass3.ADSR.ReleaseTime = 0.1f;
	}

	{
		auto& wave = SynthBass1.Waves.EmplaceLast();
		wave.Type = WaveType::Sawtooth;
		wave.Scale = 0.6f;
		wave.ExpCoeff = 10;
		wave.UpdownRatio = 20;

		SynthBass1.ADSR.AttackTime = 0.01f;
		SynthBass1.ADSR.ReleaseTime = 0.1f;
	}

	{
		auto& wave = SynthBass2.Waves.EmplaceLast();
		wave.Scale = 0.5f;
		wave.ExpCoeff = 7;
		//wave.RateAcceleration = -0.12f;
		wave.FreqMultiplier = 2;

		SynthBass2.ADSR.AttackTime = 0.01f;
		SynthBass2.ADSR.ReleaseTime = 0.1f;
	}

	{
		auto& wave = Lead5Charang.Waves.EmplaceLast();
		wave.Type = WaveType::Sawtooth;
		wave.Scale = 0.3f;
		wave.UpdownRatio = 20;
		wave.Octaves = 3;

		Lead5Charang.ADSR.AttackTime = 0.03f;
		Lead5Charang.ADSR.ReleaseTime = 0.05f;
	}


	{
		auto& wave = Flute.Waves.EmplaceLast();
		wave.Type = WaveType::Sawtooth;
		wave.Scale = 0.25f;
		wave.UpdownRatio = 0.5f;

		Flute.ADSR.AttackTime = 0.1f;
		Flute.ADSR.ReleaseTime = 0.05f;
	}

	{
		auto& wave = PanFlute.Waves.EmplaceLast();
		wave.Type = WaveType::Sawtooth;
		wave.Scale = 0.3f;
		wave.UpdownRatio = 0.4f;
		wave.Octaves = 2;

		PanFlute.ADSR.AttackTime = 0.25f;
		PanFlute.ADSR.ReleaseTime = 0.05f;
	}


	{
		auto& wave = Birds.Waves.EmplaceLast();
		wave.Octaves = 2;

		Birds.ADSR.AttackTime = 0.1f;
		Birds.ADSR.ReleaseTime = 0.2f;
		Birds.Chorus = ChorusFactory(0.3f, 3, 0.75, 0.25);
	}

	{
		auto& wave = SoundTrackFX2.Waves.EmplaceLast();
		wave.Type = WaveType::Sawtooth;
		wave.Scale = 0.15f;
		wave.UpdownRatio = 5;
		wave.Octaves = 2;

		SoundTrackFX2.ADSR.AttackTime = 0.7f;
		SoundTrackFX2.ADSR.DecayTime = 0.3f;
		SoundTrackFX2.ADSR.SustainVolume = 0.5f;
		SoundTrackFX2.ADSR.ReleaseTime = 0.2f;
	}

	
	{
		auto& wave = Pad7Halo.Waves.EmplaceLast();
		wave.Type = WaveType::Sawtooth;
		wave.Scale = 0.2f;
		wave.UpdownRatio = 2;

		Pad7Halo.ADSR.AttackTime = 0.1f;
		Pad7Halo.ADSR.ReleaseTime = 0.15f;
		//Pad7Halo.Chorus = ChorusFactory(0.002f, 2, 0.75, 0.25);
	}

	ReverseCymbal.WhiteNoise.FreqMultiplier = 5;
	ReverseCymbal.WhiteNoise.VolumeScale = 0.2f;
	ReverseCymbal.ADSR.AttackTime = 1;
	ReverseCymbal.ADSR.ReleaseTime = 0.8f;

	{
		auto& wave = Sawtooth.Waves.EmplaceLast();
		wave.Type = WaveType::Sawtooth;
		wave.Scale = 0.12f;
		wave.UpdownRatio = 2;

		auto& wave2 = Sawtooth.Waves.EmplaceLast();
		wave2.Type = WaveType::Sawtooth;
		wave2.Scale = 0.25f;
		wave2.UpdownRatio = 10;

		Sawtooth.ADSR.AttackTime = 0.01f;
		Sawtooth.ADSR.DecayTime = 0.5f;
		Sawtooth.ADSR.SustainVolume = 0.4f;
		Sawtooth.ADSR.ReleaseTime = 0.15f;
	}

	{
		auto& wave = Accordion.Waves.EmplaceLast();
		wave.Type = WaveType::Sawtooth;
		wave.Scale = 0.4f;
		wave.UpdownRatio = 5;
		wave.Octaves = 3;

		Accordion.ADSR.AttackTime = 0.01f;
		Accordion.ADSR.ReleaseTime = 0.2f;
	}

	{
		auto& wave = LeadSquare.Waves.EmplaceLast();
		//LeadSquare.Sines.AddLast({0.2f, 0, 0.5f, 1, -0.2f});
		//wave.Type = WaveType::Square; //раньше было Sine
		
		//wave.RateAcceleration = 0.01f;
		wave.Scale = 0.2f;

		LeadSquare.ADSR.AttackTime = 0.1f;
		LeadSquare.ADSR.DecayTime = 0.5f;
		LeadSquare.ADSR.SustainVolume = 0.8f;
		LeadSquare.ADSR.ReleaseTime = 0.3f;
	}

	{
		auto& wave = Crystal.Waves.EmplaceLast();
		wave.Scale = 0.4f;
		wave.ExpCoeff = 7;
		wave.Octaves = 3;
		//wave.RateAcceleration = -0.1f;
		wave.FreqMultiplier = 2;

		Crystal.ADSR.AttackTime = 0.01f;
		Crystal.ADSR.ReleaseTime = 0.1f;
	}

	{
		auto& sine = BassLead.Waves.EmplaceLast();
		sine.Scale = 0.45f;
		sine.ExpCoeff = 6.5f;
		//sine.RateAcceleration = -0.1f;

		auto& saw = BassLead.Waves.EmplaceLast();
		saw.Type = WaveType::Sawtooth;
		saw.Scale = 0.35f;
		saw.ExpCoeff = 8;
		saw.UpdownRatio = 10;

		BassLead.ADSR.AttackTime = 0.01f;
		BassLead.ADSR.ReleaseTime = 0.1f;
	}

	{
		auto& sine = ElectricBassPick.Waves.EmplaceLast();
		sine.Scale = 0.45f;
		sine.ExpCoeff = 6.5f;
		//sine.RateAcceleration = -0.1f;

		auto& saw = ElectricBassPick.Waves.EmplaceLast();
		saw.Type = WaveType::Sawtooth;
		saw.Scale = 0.25f;
		saw.ExpCoeff = 8;
		saw.UpdownRatio = 3;

		ElectricBassPick.ADSR.AttackTime = 0.01f;
		ElectricBassPick.ADSR.ReleaseTime = 0.1f;
	}

	{
		auto& sine = OrchestraHit.Waves.EmplaceLast();
		sine.Scale = 0.3f;
		sine.ExpCoeff = 7;
		//sine.RateAcceleration = -0.1f;

		auto& saw = OrchestraHit.Waves.EmplaceLast();
		saw.Type = WaveType::Sawtooth;
		saw.Scale = 0.2f;
		saw.ExpCoeff = 7;
		saw.UpdownRatio = 5;

		OrchestraHit.ADSR.AttackTime = 0.01f;
		OrchestraHit.ADSR.ReleaseTime = 0.1f;
	}


	{
		auto& wave = Atmosphere.Waves.EmplaceLast();
		wave.Scale = 0.2f;
		wave.Octaves = 4;

		Atmosphere.ADSR.AttackTime = 0.01f;
		Atmosphere.ADSR.DecayTime = 0.4f;
		Atmosphere.ADSR.SustainVolume = 0.3f;
		Atmosphere.ADSR.ReleaseTime = 0.1f;
	}

	{
		auto& wave = Rain.Waves.EmplaceLast();
		wave.Type = WaveType::Sawtooth;
		wave.Scale = 0.3f;
		wave.UpdownRatio = 2;
		wave.FreqMultiplier = 1;
		wave.ExpCoeff = 3;

		Rain.Chorus.MaxDelay = 0.005f;
		Rain.Chorus.DelayFrequency = 3;
		Rain.Chorus.MainVolume = 0.75f;
		Rain.Chorus.SecondaryVolume = 0.25f;

		Rain.ADSR.AttackTime = 0.01f;
		Rain.ADSR.DecayTime = 0.3f;
		Rain.ADSR.SustainVolume = 0.7f;
		Rain.ADSR.ReleaseTime = 0.2f;
	}

	//Guitar = CreateGuitar(15, 128, 3.5f, 1.1f, 1, 1, 0.35f);
	Guitar = CreateGuitar(15, 3, 1.7f, 1.15f, 1, 1, 0.35f);
	//GuitarSteel = CreateGuitar(15, 224, 3.5f, 1.7f, 1, 1, 0.3f);
	GuitarSteel = CreateGuitar(15, 5, 2.5f, 0.75f, 1.2f, 1, 0.3f);

	{
		auto& wave = OverdrivenGuitar.Waves.EmplaceLast();
		wave.Type = WaveType::WhiteNoise;
		wave.Scale = 0.35f;
		wave.ExpCoeff = 4;
		//wave.RateAcceleration = -0.1f;
		wave.FreqMultiplier = 2;

		OverdrivenGuitar.ADSR.AttackTime = 0.01f;
		OverdrivenGuitar.ADSR.ReleaseTime = 0.1f;
	}

	/*OverdrivenGuitar.Synth = CreateSawtoothSynthPass(9.5, 0.35f, 1, 1);
	
	OverdrivenGuitar.Modifiers.AddLast( CreateModifierPass(Modifiers::RelPulsator(6)) );
	OverdrivenGuitar.Attenuation = CreateTableAttenuationPass(
		{Norm8(0.1), 0.3, 0.7, 0.8, 0.75, 0.3, 0.25, 0.2, 0.16, 0.12, 0.09, 0.06, 0.03, 0.01, 0});*/


	{
		auto& wave = Trumpet.Waves.EmplaceLast();
		wave.Type = WaveType::Sawtooth;
		wave.Scale = 0.25f;
		wave.UpdownRatio = 8;

		Trumpet.ADSR.AttackTime = 0.01f;
		Trumpet.ADSR.ReleaseTime = 0.05f;
	}

	{
		auto& wave = Piccolo.Waves.EmplaceLast();
		wave.Type = WaveType::Sawtooth;
		wave.Scale = 0.3f;
		wave.UpdownRatio = 6;

		Piccolo.ADSR.AttackTime = 0.01f;
		Piccolo.ADSR.ReleaseTime = 0.2f;
	}

	{
		auto& wave = Oboe.Waves.EmplaceLast();
		wave.Type = WaveType::Sawtooth;
		wave.Scale = 0.45f;
		wave.UpdownRatio = 0.6f;
		wave.Octaves = 2;
		wave.FreqMultiplier = 2;

		Oboe.ADSR.AttackTime = 0.07f;
		Oboe.ADSR.ReleaseTime = 0.07f;
		//Oboe.Chorus = ChorusFactory(0.002f, 1, 0.75, 0.25);
	}

	{
		auto wave = FretlessBass.Waves.EmplaceLast();
		wave.Type = WaveType::Sawtooth;
		wave.ExpCoeff = 8;
		wave.Scale = 0.4f;
		wave.UpdownRatio = 15;

		FretlessBass.ADSR.AttackTime = 0.01f;
		FretlessBass.ADSR.ReleaseTime = 0.1f;
	}

	{
		auto& wave = Sax.Waves.EmplaceLast();
		wave.Type = WaveType::Sawtooth;
		wave.Scale = 0.15f;
		wave.UpdownRatio = 0.2f;
		wave.Octaves = 2;

		Sax.ADSR.AttackTime = 0.02f;
		Sax.ADSR.ReleaseTime = 0.05f;
	}


	{
		auto& wave = Calliope.Waves.EmplaceLast();
		wave.Scale = 0.2f;
		//wave.RateAcceleration = -0.2f;

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
		auto& wave = Organ.Waves.EmplaceLast();
		wave.Type = WaveType::Sawtooth;
		wave.Scale = 0.03f;
		wave.UpdownRatio = 0.1f;
		wave.Octaves = 4;
		wave.FreqMultiplier = 1;

		Organ.ADSR.AttackTime = 0.01f;
		Organ.ADSR.ReleaseTime = 0.01f;
	}

	{
		auto& wave = PercussiveOrgan.Waves.EmplaceLast();
		wave.Type = WaveType::Sawtooth;
		wave.Scale = 0.2f;
		wave.UpdownRatio = 2;
		wave.Octaves = 3;
		wave.FreqMultiplier = 2;

		PercussiveOrgan.ADSR.AttackTime = 0.2f;
		PercussiveOrgan.ADSR.ReleaseTime = 0.3f;
	}

	{
		auto& wave = Whistle.Waves.EmplaceLast();
		wave.Scale = 0.3f;
		
		Whistle.ADSR.AttackTime = 0.03f;
		Whistle.ADSR.ReleaseTime = 0.25f;
	
		/*Whistle.Chorus.MaxDelay = 0.03f;
		Whistle.Chorus.MainVolume = 0.75f;
		Whistle.Chorus.SecondaryVolume = 0.25f;
		Whistle.Chorus.DelayFrequency = 10;*/
	}

	{
		auto& wave = Sine2Exp.Waves.EmplaceLast();
		wave.ExpCoeff = 9;
		wave.Octaves = 2;
		wave.FreqMultiplier = 2;

		Sine2Exp.ADSR.AttackTime = 0.01f;
		Sine2Exp.ADSR.ReleaseTime = 0.1f;
	}

	{
		auto& wave = Vibraphone.Waves.EmplaceLast();
		wave.Scale = 0.25f;
		wave.ExpCoeff = 9;
		wave.Octaves = 5;

		Vibraphone.ADSR.AttackTime = 0.01f;
		Vibraphone.ADSR.ReleaseTime = 0.8f;
	}

	{
		auto& wave = Pad5Bowed.Waves.EmplaceLast();
		wave.Scale = 0.05f;
		wave.Octaves = 4;

		Pad5Bowed.ADSR.AttackTime = 0.3f;
		Pad5Bowed.ADSR.ReleaseTime = 0.2f;
	}

	{
		auto& wave = Glockenspiel.Waves.EmplaceLast();
		wave.Scale = 0.25f;
		wave.ExpCoeff = 12;
		wave.Octaves = 5;

		Glockenspiel.ADSR.AttackTime = 0.005f;
		Glockenspiel.ADSR.ReleaseTime = 0.2f;
	}


	{
		auto& wave = NewAge.Waves.EmplaceLast();
		wave.Scale = 0.07f;
		wave.Octaves = 5;
	
		NewAge.ADSR.AttackTime = 0.02f;
		NewAge.ADSR.DecayTime = 0.5f;
		NewAge.ADSR.SustainVolume = 0.6f;
		NewAge.ADSR.ReleaseTime = 0.4f;
	}


	Applause.WhiteNoise.FreqMultiplier = 20;
	Applause.WhiteNoise.VolumeScale = 0.5f;
	Applause.Chorus.DelayFrequency = 10;
	Applause.Chorus.MainVolume = 0.7f;
	Applause.Chorus.SecondaryVolume = 0.3f;
	Applause.Chorus.MaxDelay = 0.05f;
	Applause.ADSR.AttackTime = 0.7f;
	Applause.ADSR.ReleaseTime = 0.5f;

	Helicopter.WhiteNoise.FreqMultiplier = 0.2f;
	Helicopter.WhiteNoise.VolumeScale = 1;
	Helicopter.Chorus.DelayFrequency = 3;
	Helicopter.Chorus.MainVolume = 0.5f;
	Helicopter.Chorus.SecondaryVolume = 0.5f;
	Helicopter.Chorus.MaxDelay = 0.1f;
	Helicopter.ADSR.AttackTime = 0.4f;
	Helicopter.ADSR.ReleaseTime = 0.4f;

	Seashore.WhiteNoise.FreqMultiplier = 40;
	Seashore.WhiteNoise.VolumeScale = 0.07f;
	Seashore.ADSR.AttackTime = 1;
	Seashore.ADSR.ReleaseTime = 0.7f;

	{
		auto& wave = PhoneRing.Waves.EmplaceLast();
		wave.Scale = 0.5f;

		PhoneRing.ADSR.AttackTime = 0.2f;
		PhoneRing.ADSR.ReleaseTime = 0.2f;
		//PhoneRing.Modifiers.AddLast( CreateModifierPass(Modifiers::AbsPulsator(5)) );
	}
	

	GunShot.WhiteNoise.FreqMultiplier = 40;
	GunShot.WhiteNoise.VolumeScale = 0.4f;
	GunShot.ExponentAttenuation.ExpCoeff = 5;
	GunShot.ADSR.ReleaseTime = 0.1f;


	/*DrumSound2.Synth = CreateGeneratorSynthPass(
		Generators::FunctionGenerator<float(*)(float, float)>(DrumSample), 0.3f, 1, 0.01f);
	DrumSound2.MinNoteDuration = 0.3f;*/

	/*SynthesizedInstrument kalimba;
	kalimba.Synth = CreateSineSynthPass(0.1f, 3, 1);
	kalimba.Attenuation = CreateExponentialAttenuationPass(11);
	kalimba.MinNoteDuration = 0.3f;*/

	{
		auto& wave = Kalimba.Waves.EmplaceLast();
		wave.Scale = 0.24f;
		wave.ExpCoeff = 8;

		Kalimba.ADSR.AttackTime = 0.01f;
		Kalimba.ADSR.ReleaseTime = 0.1f;
	}
	
	UniDrum = CachedDrumInstrument(Generators::DrumPhysicalModel(2, 16, 16, 0.342f, 0.00026f, 0.20f), 44100, 0.06f);
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

	ClosedHiHat = CachedDrumInstrument(Generators::DrumPhysicalModel(2, 16, 16, 0.338f, 0.04928f, 0.10f), 44100, 0.06f);
	//ClosedHiHat->PostEffects.AddLast(PostEffects::FilterQ(5000, 0.9f));
	//ClosedHiHat->PostEffects.AddLast(PostEffects::FilterHP(0.9f));
	//ClosedHiHat->PostEffects.AddLast(PostEffects::Fade(0, 1000));
	//ClosedHiHat->MinNoteDuration = 0.25f;

	AcousticBassDrum = CachedDrumInstrument(Generators::DrumPhysicalModel(2, 8, 8, 0.092f, 0.0072f, 0.20f), 44100, 0.06f);
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
		auto& sine = result.Waves.EmplaceLast();
		sine.Scale = scale * 0.5f*volume;
		sine.ExpCoeff = d + e*float(i-1);
		sine.FreqMultiplier = freqMult*float(i);
	}
	result.ADSR.AttackTime = 0.005f;
	result.ADSR.ReleaseTime = 0.25f;
	return result;
}

