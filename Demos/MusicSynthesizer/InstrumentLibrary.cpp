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
#include "Audio/Synth/Filter.h"

#include "Audio/Synth/Generators.hh"
#include "Audio/Synth/PostEffects.hh"

#include "Audio/Synth/WaveTableGeneration.h"

using namespace Intra;
using namespace Audio;
using namespace Synth;

InstrumentLibrary::~InstrumentLibrary() {}

InstrumentLibrary::InstrumentLibrary()
{
	auto& choirATables = Tables["ChoirA"] = CreateWaveTablesFromFormants({
		{600, 0.007f, 1},
		{900, 0.004f, 1},
		{2200, 0.005f, 1},
		{2600, 0.004f, 1},
		{0, 0.00033f, 0.1f}
	}, 64, 2, 60, 1, 32768);
	
	{
		auto& wt = Instruments["ChoirAahs"].WaveTables.EmplaceLast();
		wt.Tables = &choirATables;
		wt.VolumeScale = 0.4f;
		wt.ADSR.AttackTime = 0.02f;
		wt.ADSR.DecayTime = 0.5f;
		wt.ADSR.SustainVolume = 0.8f;
		wt.ADSR.ReleaseTime = 0.05f;
	}
	
	auto& choirOTables = Tables["ChoirO"] = CreateWaveTablesFromFormants({
		{275, 0.01f, 1},
		{850, 0.0033f, 1},
		{2400, 0.005f, 1},
		{2600, 0.004f, 1},
		{0, 0.00033f, 0.1f}
	}, 64, 2, 100, 0.5f, 32768);

	{
		auto& wt = Instruments["PadChoir"].WaveTables.EmplaceLast();
		wt.Tables = &choirOTables;
		wt.VolumeScale = 0.7f;
		wt.ADSR.AttackTime = 0.07f;
		wt.ADSR.DecayTime = 0.5f;
		wt.ADSR.SustainVolume = 0.5f;
		wt.ADSR.ReleaseTime = 0.5f;
	}

	{
		auto& wt = Instruments["Pad7Halo"].WaveTables.EmplaceLast();
		wt.Tables = &choirOTables;
		wt.VolumeScale = 0.6f;
		wt.ADSR.AttackTime = 0.03f;
		wt.ADSR.DecayTime = 0.5f;
		wt.ADSR.SustainVolume = 0.7f;
		wt.ADSR.ReleaseTime = 0.3f;
	}


	auto& synthStringTables = Tables["SynthString"] = CreateWaveTablesFromFormants({
		{500, 0.005f, 1},
		{900, 0.0013f, 1},
		{2100, 0.0005f, 1},
		{3700, 0.00033f, 1},
		{4700, 0.00025f, 1}
	}, 64, 1, 40, 1, 32768);

	{
		auto& wt = Instruments["RockOrgan"].WaveTables.EmplaceLast();
		wt.Tables = &synthStringTables;
		wt.VolumeScale = 0.4f;
		wt.ADSR.AttackTime = 0.001f;
		wt.ADSR.SustainVolume = 0.6f;
		wt.ADSR.DecayTime = 0.1f;
		wt.ADSR.ReleaseTime = 0.05f;
		wt.VibratoValue = 0.004f;
		wt.VibratoFrequency = 5;
	}

	


	auto& synthVoiceTables = Tables["SynthVoice"] = CreateWaveTablesFromFormants({
		{600, 0.007f, 1},
		{900, 0.003f, 1},
		{2200, 0.00083f, 1},
		{2600, 0.004f, 1},
		{0, 0.00033f, 0.1f}
	}, 64, 2, 40, 1, 32768);

	{
		auto& wt = Instruments["SynthVoice"].WaveTables.EmplaceLast();
		wt.Tables = &synthVoiceTables;
		wt.VolumeScale = 0.2f;
		wt.ADSR.AttackTime = 0.04f;
		wt.ADSR.ReleaseTime = 0.1f;
	}

	{
		auto& wt = Instruments["VoiceOohs"].WaveTables.EmplaceLast();
		wt.Tables = &choirOTables;
		wt.VolumeScale = 0.25f;
		wt.ADSR.AttackTime = 0.005f;
		wt.ADSR.DecayTime = 0.3f;
		wt.ADSR.SustainVolume = 0.6f;
		wt.ADSR.ReleaseTime = 0.2f;
	}
	
	{
		auto& wt = Instruments["Pad8Sweep"].WaveTables.EmplaceLast();
		wt.Tables = &synthStringTables;
		wt.VolumeScale = 0.15f;
		wt.ADSR.AttackTime = 0.15f;
		wt.ADSR.ReleaseTime = 0.25f;
	}

	{
		auto& wt = Instruments["SynthStrings"].WaveTables.EmplaceLast();
		wt.Tables = &synthStringTables;
		wt.VolumeScale = 0.15f;
		wt.ADSR.AttackTime = 0.3f;
		wt.ADSR.ReleaseTime = 0.2f;
	}

	{
		auto& wt = Instruments["StringEnsemble"].WaveTables.EmplaceLast();
		wt.Tables = &synthStringTables;
		wt.VolumeScale = 0.2f;
		wt.ADSR.AttackTime = 0.07f;
		wt.ADSR.ReleaseTime = 0.3f;
	}

	{
		auto& wt = Instruments["TremoloStrings"].WaveTables.EmplaceLast();
		wt.Tables = &synthStringTables;
		wt.VolumeScale = 0.25f;
		wt.ADSR.AttackTime = 0.01f;
		wt.ADSR.ReleaseTime = 0.07f;
	}


	auto& harmonicaTables = Tables["Harmonica"] = CreateWaveTablesFromFormants({
		{500, 0.005f, 1},
		{900, 0.0013f, 1},
		{2100, 0.0005f, 1},
		{3700, 0.00033f, 1},
		{4700, 0.00025f, 1}
	}, 64, 0.125f, 8, 1.1f, 16384);

	{
		auto& wave = Instruments["Harmonica"].WaveTables.EmplaceLast();
		wave.Tables = &harmonicaTables;
		wave.VolumeScale = 0.15f;

		wave.ADSR.AttackTime = 0.03f;
		wave.ADSR.ReleaseTime = 0.04f;
	}

	auto& fiddleTables = Tables["Fiddle"] = CreateWaveTablesFromFormants({
		{500, 0.005f, 1},
		{900, 0.0013f, 1},
		{2100, 0.0005f, 1},
		{3700, 0.00033f, 1},
		{4700, 0.00025f, 1}
	}, 16, 0.5f, 4, 1.0f, 16384);

	{
		auto& wave = Instruments["Fiddle"].WaveTables.EmplaceLast();
		wave.Tables = &fiddleTables;
		wave.VolumeScale = 0.15f;

		wave.ADSR.AttackTime = 0.03f;
		wave.ADSR.ReleaseTime = 0.04f;
	}


	auto& pizzicatoStringsTables = Tables["PizzicatoStrings"] = CreateWaveTablesFromFormants({
		{500, 0.014f, 1},
		{800, 0.0022f, 1},
		{2100, 0.0005f, 1},
		{3700, 0.00033f, 1},
		{0, 0.1f, 1}
	}, 64, 2, 10, 1, 32768);

	{
		auto& wt = Instruments["PizzicatoStrings"].WaveTables.EmplaceLast();
		wt.Tables = &pizzicatoStringsTables;
		wt.VolumeScale = 0.25f;
		wt.ExpCoeff = 6;
		wt.ADSR.AttackTime = 0.02f;
		wt.ADSR.ReleaseTime = 0.07f;
	}

	{
		auto& wt = Instruments["PercussiveOrgan"].WaveTables.EmplaceLast();
		wt.Tables = &synthVoiceTables;
		wt.VolumeScale = 0.6f;
		wt.ADSR.AttackTime = 0.003f;
		wt.ADSR.DecayTime = 0.3f;
		wt.ADSR.SustainVolume = 0.6f;
		wt.ADSR.ReleaseTime = 0.05f;
	}


	auto& violinTables = Tables["Violin"] = CreateWaveTablesFromFormants({
		{500, 0.014f, 1},
		{800, 0.0022f, 1},
		{2100, 0.0005f, 1},
		{3700, 0.00033f, 1},
		{0, 0.1f, 1}
	}, 32, 1, 8, 1, 16384);

	{
		auto& wt = Instruments["Violin"].WaveTables.EmplaceLast();
		wt.Tables = &violinTables;
		wt.VolumeScale = 0.3f;
		wt.VibratoFrequency = 5;
		wt.VibratoValue = 0.001f;

		wt.ADSR.AttackTime = 0.08f;
		wt.ADSR.DecayTime = 0.5f;
		wt.ADSR.SustainVolume = 0.8f;
		wt.ADSR.ReleaseTime = 0.1f;
	}

	Tables["SynthOrgan"].Generator = [](float freq, uint sampleRate)
	{
		WaveTable tbl;
		tbl.BaseLevelLength = 32768;
		tbl.Data.Reserve(tbl.BaseLevelLength * 2);
		tbl.Data.SetCount(tbl.BaseLevelLength / 2);
		tbl.BaseLevelRatio = freq/sampleRate;
		for(int i = 2; i<20; i++)
		{
			float ampl = 1.0f/i;
			Synth::AddSineHarmonicGaussianProfile(tbl.Data, tbl.BaseLevelRatio, float(1 << (i-2)), 1.25f, ampl, 10);
		}
		ConvertAmplitudesToSamples(tbl, 1);
		return tbl;
	};

	auto& synthBrassTables = Tables["SynthBrass"] = CreateWaveTablesFromHarmonics(CreateHarmonicArray(15, 0, 1, 1, 64, false), 0.85f, 32768, false /*true*/);

	{
		auto& wt = Instruments["SynthBrass"].WaveTables.EmplaceLast();
		wt.Tables = &synthBrassTables;
		wt.VolumeScale = 0.25f;
		wt.VibratoValue = 0.003f;
		wt.VibratoFrequency = 5;

		wt.ADSR.AttackTime = 0.01f;
		wt.ADSR.DecayTime = 0.5f;
		wt.ADSR.SustainVolume = 0.7f;
		wt.ADSR.ReleaseTime = 0.1f;
	}

	{
		auto& wt = Instruments["PadPolysynth"].WaveTables.EmplaceLast();
		wt.Tables = &synthBrassTables;
		wt.VolumeScale = 0.1f;
		wt.ADSR.AttackTime = 0.01f;
		wt.ADSR.ReleaseTime = 0.15f;
	}

	auto& rainTables = Tables["Rain"] = CreateWaveTablesFromFormants({
		{600, 0.007f, 1},
		{900, 0.003f, 1},
		{2200, 0.00083f, 1},
		{2600, 0.0005f, 1},
		{0, 0.0033f, 0.1f}
	}, 64, 2, 40, 1, 8192);

	{
		auto& wt = Instruments["Rain"].WaveTables.EmplaceLast();
		wt.Tables = &rainTables;
		wt.ExpCoeff = 3;
		wt.VolumeScale = 0.45f;
		wt.VibratoFrequency = 15;
		wt.VibratoValue = 0.01f;
		wt.ADSR.AttackTime = 0.003f;
		wt.ADSR.DecayTime = 0.3f;
		wt.ADSR.SustainVolume = 0.7f;
		wt.ADSR.ReleaseTime = 0.2f;
	}

	{ //TODO: это заглушка
		auto& wt = Instruments["SteelDrums"].WaveTables.EmplaceLast();
		wt.Tables = &rainTables;
		//wt.ExpCoeff = 3;
		wt.VolumeScale = 0.3f;
		wt.ADSR.AttackTime = 0.005f;
		wt.ADSR.DecayTime = 0.3f;
		wt.ADSR.SustainVolume = 0.7f;
		wt.ADSR.ReleaseTime = 0.2f;
	}

	{
		auto& wt = Instruments["FxGoblins"].WaveTables.EmplaceLast();
		wt.Tables = &rainTables;
		wt.VolumeScale = 0.07f;
		wt.ADSR.AttackTime = 0.05f;
		wt.ADSR.ReleaseTime = 0.15f;
	}


	auto& orchestraHitTables = Tables["OrchestraHit"] = CreateWaveTablesFromHarmonics(CreateHarmonicArray(15, 1, 1, 1, 64, false), 1, 16384, true);

	{
		auto& instr = Instruments["OrchestraHit"];
		auto& wt = instr.WaveTables.EmplaceLast();
		wt.Tables = &orchestraHitTables;
		wt.VolumeScale = 0.3f;

		instr.WhiteNoise.FreqMultiplier = 10;
		instr.WhiteNoise.VolumeScale = 0.12f;
		instr.ExponentAttenuation.ExpCoeff = 10;

		instr.ADSR.AttackTime = 0.02f;
		instr.ADSR.ReleaseTime = 0.1f;
	}


	auto& fluteTables = Tables["Flute"] = CreateWaveTablesFromHarmonics(CreateHarmonicArray(15, 0.8f, 2, 2, 64, true), 1, 32768, false);

	auto& calliopeTables = Tables["Calliope"] = CreateWaveTablesFromFormants({
		{275, 0.001f, 1},
		{650, 0.0005f, 1},
		{1100, 0.001f, 1},
		{2700, 0.004f, 1},
		{0, 0.00033f, 0.1f}
	}, 64, 1, 8, 2, 16384);

	{
		auto& wt = Instruments["Calliope"].WaveTables.EmplaceLast();
		wt.Tables = &calliopeTables;
		wt.ExpCoeff = 4;
		wt.VolumeScale = 0.5f;

		wt.ADSR.AttackTime = 0.01f;
		wt.ADSR.DecayTime = 0.2f;
		wt.ADSR.SustainVolume = 0.5f;
		wt.ADSR.ReleaseTime = 0.2f;
	}

	auto& vibraphoneTables = Tables["Vibraphone"] = CreateWaveTablesFromHarmonics({
		{1, 1, 7},
		{0.25f, 4, 15.4f},
		{0.125f, 8, 23.8f},
		{0.0625f, 16, 40.6f},
		{0.03125f, 32, 74.2f}
	}, 1, 32768, true);

	{
		auto& wt = Instruments["Vibraphone"].WaveTables.EmplaceLast();
		wt.Tables = &vibraphoneTables;
		wt.VolumeScale = 0.15f;
		wt.ExpCoeff = 4;
		wt.ADSR.AttackTime = 0.01f;
		wt.ADSR.ReleaseTime = 0.8f;
	}

	Instruments["Crystal"] = Instruments["Vibraphone"];


	auto& marimbaTables = Tables["Marimba"] = CreateWaveTablesFromHarmonics({
		{1, 1, 5},
		{0.25f, 4, 5},
		{1.0f/9.2f, 9.2f, 5},
		{0.0835f, 12, 3},
		{0.04175f, 24, 3},
		{0.021f, 48, 3}
	}, 1.25f, 32768, true);

	{
		auto& wt = Instruments["Marimba"].WaveTables.EmplaceLast();
		wt.Tables = &marimbaTables;
		wt.VolumeScale = 0.25f;
		wt.ExpCoeff = 13;
		wt.ADSR.AttackTime = 0.004f;
		wt.ADSR.ReleaseTime = 0.2f;
	}

	auto& xylophoneTables = Tables["Xylophone"] = CreateWaveTablesFromHarmonics({
		{1, 1, 30},
		{0.333f, 3, 20},
		{1.0f/9.2f, 9.2f, 10},
		{1/13.0f, 13, 10},
		{0.033f, 30, 10}
	}, 1, 32768, true);

	{
		auto& wt = Instruments["Xylophone"].WaveTables.EmplaceLast();
		wt.Tables = &xylophoneTables;
		wt.VolumeScale = 0.25f;
		wt.ExpCoeff = 10;

		wt.ADSR.AttackTime = 0.003f;
		wt.ADSR.ReleaseTime = 0.4f;
	}

	auto& atmosphereTables = Tables["Atmosphere"] = CreateWaveTablesFromHarmonics(CreateHarmonicArray(15, 0, 2, 1, 64, false), 0.85f, 16384, true);

	{
		auto& wt = Instruments["Atmosphere"].WaveTables.EmplaceLast();
		wt.Tables = &atmosphereTables;
		wt.VolumeScale = 0.5f;

		wt.ADSR.AttackTime = 0.01f;
		wt.ADSR.DecayTime = 0.4f;
		wt.ADSR.SustainVolume = 0.3f;
		wt.ADSR.ReleaseTime = 0.1f;
	}

	auto& newAgeTables = Tables["NewAge"] = CreateWaveTablesFromHarmonics({
		{1, 1, 20},
		{0.5f, 4, 15},
		{0.25f, 8, 15},
		{0.125f, 16, 15},
		{0.0625f, 32, 15}
	}, 1, 16384, true);

	{
		auto& wt = Instruments["NewAge"].WaveTables.EmplaceLast();
		wt.Tables = &newAgeTables;
		wt.ExpCoeff = 1;
		wt.VolumeScale = 0.2f;

		wt.ADSR.AttackTime = 0.005f;
		wt.ADSR.DecayTime = 0.5f;
		wt.ADSR.SustainVolume = 0.6f;
		wt.ADSR.ReleaseTime = 0.4f;
	}

	Instruments["Pad5Bowed"] = Instruments["NewAge"];


	/*LeadSquareTables.Generator = [](float freq, uint sampleRate)
	{
		WaveTable tbl;
		tbl.BaseLevelLength = 32768;
		tbl.Data.Reserve(tbl.BaseLevelLength * 2);
		tbl.Data.SetCount(tbl.BaseLevelLength / 2);
		tbl.BaseLevelRatio = freq/float(sampleRate);
		const float d = 0.5f;
		for(int i = 1; i<64; i++)
		{
			if(d == 0.5f && (i & 1) == 0) continue;
			const float ai = 2*Math::Sin(float(Math::PI)*d*float(i))/float(i);
			Synth::AddSineHarmonicGaussianProfile(tbl.Data, tbl.BaseLevelRatio, float(i), 0.25f, ai, 50);
		}
		ConvertAmplitudesToSamples(tbl);
		return tbl;
	};*/
	//LeadSquareTables.AllowMipmaps = true;

	{
		//LeadSquare.WaveTables.AddLast({&LeadSquareTables, 0, 0.3f});
		auto& wave = Instruments["LeadSquare"].Waves.EmplaceLast();
		wave.Scale = 0.2f;
		wave.VibratoFrequency = -2;
		wave.VibratoValue = 0.9f;

		wave.ADSR.AttackTime = 0.02f;
		wave.ADSR.DecayTime = 0.5f;
		wave.ADSR.SustainVolume = 0.8f;
		wave.ADSR.ReleaseTime = 0.1f;
	}


	auto& leadSawtoothTables = Tables["LeadSawtooth"] = CreateWaveTablesFromHarmonics(CreateHarmonicArray(15, 0, 1, 1, 64, true), 0.85f, 16384, false);

	{
		auto& wt = Instruments["LeadSawtooth"].WaveTables.EmplaceLast();
		wt.VolumeScale = 0.2f;
		wt.Tables = &leadSawtoothTables;

		wt.ADSR.AttackTime = 0.02f;
		wt.ADSR.DecayTime = 0.5f;
		wt.ADSR.SustainVolume = 0.7f;
		wt.ADSR.ReleaseTime = 0.15f;
	}


	{
		const float volume = 0.1f;
		const float D = 0.75f, E = 0.93f, F = 0.5f, G = 0.8f, H = 0.7f;
		float scaleSum = 0;
		auto& instr = Instruments["Glockenspiel"];
		for(float k = 0; k < 10; k++)
		{
			auto& wave = instr.Waves.EmplaceLast();
			wave.ExpCoeff = Math::Exp(5*(H-0.5f)+1.25f*G*k)+3;
			wave.Scale = Math::Exp(-0.625f*D*k)*Math::Sin((1+7*E)*k*k + 1);
			wave.FreqMultiplier = Math::Exp(1.25f*F*k) * 4;
			scaleSum += wave.Scale;
		}
		for(auto& wave: instr.Waves)
			wave.Scale *= volume/scaleSum;
		instr.ADSR.AttackTime = 0.003f;
		instr.ADSR.ReleaseTime = 0.1f;
	}

	{
		auto& instr = Instruments["Piano"];
		const float volume = 0.25f;
		auto& harm1 = instr.Waves.EmplaceLast();
		harm1.Scale = 0.15f*volume;
		harm1.ExpCoeff = 3;
		harm1.FreqMultiplier = 1;

		auto& harm2 = instr.Waves.EmplaceLast();
		harm2.Scale = -0.075f*volume;
		harm2.ExpCoeff = 3;
		harm2.FreqMultiplier = 3;

		auto& harm3 = instr.Waves.EmplaceLast();
		harm3.Scale = -0.075f*volume;
		harm3.ExpCoeff = 3;
		harm3.FreqMultiplier = 5;

		auto& harm4 = instr.Waves.EmplaceLast();
		harm4.Scale = 0.0375f*volume;
		harm4.ExpCoeff = 3;
		harm4.FreqMultiplier = 7;

		auto& harm5 = instr.Waves.EmplaceLast();
		harm5.Scale = -0.0375f*volume;
		harm5.ExpCoeff = 3;
		harm5.FreqMultiplier = 9;


		auto& harm6 = instr.Waves.EmplaceLast();
		harm6.Scale = 0.15f*volume;
		harm6.ExpCoeff = 4;
		harm6.FreqMultiplier = 2;

		auto& harm7 = instr.Waves.EmplaceLast();
		harm7.Scale = -0.15f*volume;
		harm7.ExpCoeff = 4;
		harm7.FreqMultiplier = 4;

		auto& harm8 = instr.Waves.EmplaceLast();
		harm8.Scale = 0.075f*volume;
		harm8.ExpCoeff = 4;
		harm8.FreqMultiplier = 5;

		auto& harm9 = instr.Waves.EmplaceLast();
		harm9.Scale = -0.075f*volume;
		harm9.ExpCoeff = 4;
		harm9.FreqMultiplier = 7;

		auto& harm10 = instr.Waves.EmplaceLast();
		harm10.Scale = 0.0375f*volume;
		harm10.ExpCoeff = 4;
		harm10.FreqMultiplier = 11;

		auto& harm11 = instr.Waves.EmplaceLast();
		harm11.Scale = -0.0375f*volume;
		harm11.ExpCoeff = 4;
		harm11.FreqMultiplier = 13;

		instr.ADSR.AttackTime = 0.003f;
		instr.ADSR.ReleaseTime = 0.2f;
	}

	{
		auto& instr = Instruments["ElectricPiano"];
		const float volume = 0.27f;
		auto& harm1 = instr.Waves.EmplaceLast();
		harm1.Scale = 0.15f*volume;
		harm1.ExpCoeff = 3;
		harm1.FreqMultiplier =1;

		auto& harm2 = instr.Waves.EmplaceLast();
		harm2.Scale = 0.15f*volume;
		harm2.ExpCoeff = 3.1f;
		harm2.FreqMultiplier = 2;

		auto& harm3 = instr.Waves.EmplaceLast();
		harm3.Scale = -0.075f*volume;
		harm3.ExpCoeff = 3.2f;
		harm3.FreqMultiplier = 3;

		auto& harm4 = instr.Waves.EmplaceLast();
		harm4.Scale = -0.15f*volume;
		harm4.ExpCoeff = 3.3f;
		harm4.FreqMultiplier = 4;

		auto& harm5 = instr.Waves.EmplaceLast();
		harm5.Scale = -0.0375f*volume;
		harm5.ExpCoeff = 3.6f;
		harm5.FreqMultiplier = 7;

		auto& harm6 = instr.Waves.EmplaceLast();
		harm6.Scale = -0.0375f*volume;
		harm6.ExpCoeff = 3.8f;
		harm6.FreqMultiplier = 9;

		auto& harm7 = instr.Waves.EmplaceLast();
		harm7.Scale = 0.0375f*volume;
		harm7.ExpCoeff = 4;
		harm7.FreqMultiplier = 11;

		auto& harm8 = instr.Waves.EmplaceLast();
		harm8.Scale = -0.0375f*volume;
		harm8.ExpCoeff = 4.2f;
		harm8.FreqMultiplier = 13;

		instr.ADSR.AttackTime = 0.003f;
		instr.ADSR.ReleaseTime = 0.2f;
	
		Instruments["ElectricPiano2"] = instr;
	}


	{
		auto& wave = Instruments["Bass1"].Waves.EmplaceLast();
		wave.Scale = 0.3f;
		wave.ExpCoeff = 5;
		wave.FreqMultiplier = 2;

		wave.ADSR.AttackTime = 0.02f;
		wave.ADSR.ReleaseTime = 0.1f;
	}

	{
		auto& wave = Instruments["ElectricBassFinger"].Waves.EmplaceLast();
		wave.Scale = 0.4f;
		wave.ExpCoeff = 4;
		wave.FreqMultiplier = 2;

		wave.ADSR.AttackTime = 0.01f;
		wave.ADSR.ReleaseTime = 0.1f;
	}

	{
		auto& wave = Instruments["Bass2"].Waves.EmplaceLast();
		wave.Scale = 0.3f;
		wave.ExpCoeff = 4;
		wave.Octaves = 2;
		//wave.RateAcceleration = -0.15f;
		wave.FreqMultiplier = 2;

		wave.ADSR.AttackTime = 0.01f;
		wave.ADSR.ReleaseTime = 0.1f;
	}

	{
		auto& wave = Instruments["SlapBass"].Waves.EmplaceLast();
		wave.Wave = SawtoothWaveForm{0.25f};
		wave.Scale = 0.3f;
		wave.ExpCoeff = 4;

		wave.ADSR.AttackTime = 0.01f;
		wave.ADSR.ReleaseTime = 0.1f;
	}

	{
		auto& wave = Instruments["Bass3"].Waves.EmplaceLast();
		wave.Scale = 0.7f;
		wave.ExpCoeff = 4;
		wave.VibratoFrequency = -1.5f;
		wave.VibratoValue = 0.3f;

		wave.ADSR.AttackTime = 0.01f;
		wave.ADSR.ReleaseTime = 0.1f;
	}

	{
		auto& wave = Instruments["SynthBass1"].Waves.EmplaceLast();
		wave.Wave = SawtoothWaveForm{20};
		wave.Scale = 0.4f;
		wave.ExpCoeff = 5;
		wave.SmoothingFactor = 0.5f;
		wave.VibratoFrequency = 7;
		wave.VibratoValue = 0.002f;

		wave.ADSR.AttackTime = 0.01f;
		wave.ADSR.DecayTime = 0.2f;
		wave.ADSR.SustainVolume = 0.5f;
		wave.ADSR.ReleaseTime = 0.7f;
	}

	{
		auto& wave = Instruments["SynthBass2"].Waves.EmplaceLast();
		wave.Scale = 0.3f;
		wave.ExpCoeff = 7;

		wave.ADSR.AttackTime = 0.01f;
		wave.ADSR.ReleaseTime = 0.1f;
	}

	{
		auto& wave = Instruments["Lead5Charang"].Waves.EmplaceLast();
		wave.Wave = SawtoothWaveForm{20};
		wave.Scale = 0.2f;

		wave.ADSR.AttackTime = 0.03f;
		wave.ADSR.ReleaseTime = 0.05f;
	}


	{
		auto& wt = Instruments["Flute"].WaveTables.EmplaceLast();
		wt.Tables = &fluteTables;
		wt.VolumeScale = 0.7f;
		wt.VibratoValue = 0.005f;
		wt.VibratoFrequency = 7;

		wt.ADSR.AttackTime = 0.04f;
		wt.ADSR.DecayTime = 0.3f;
		wt.ADSR.SustainVolume = 0.6f;
		wt.ADSR.ReleaseTime = 0.1f;
	}

	{
		auto& wt = Instruments["PanFlute"].WaveTables.EmplaceLast();
		wt.Tables = &fluteTables;
		wt.VolumeScale = 0.7f;
		wt.VibratoValue = 0.003f;
		wt.VibratoFrequency = 6;

		wt.ADSR.AttackTime = 0.05f;
		wt.ADSR.DecayTime = 0.2f;
		wt.ADSR.SustainVolume = 0.6f;
		wt.ADSR.ReleaseTime = 0.1f;
	}

	{
		auto& wt = Instruments["Piccolo"].WaveTables.EmplaceLast();
		wt.Tables = &fluteTables;
		wt.VolumeScale = 0.7f;
		wt.VibratoValue = 0.003f;
		wt.VibratoFrequency = 6;

		wt.ADSR.AttackTime = 0.05f;
		wt.ADSR.DecayTime = 0.2f;
		wt.ADSR.SustainVolume = 0.6f;
		wt.ADSR.ReleaseTime = 0.1f;
	}


	auto& clarinetTables = Tables["Clarinet"] = CreateWaveTablesFromHarmonics({
		{1, 1, 15},
		{0.75f, 3, 13},
		{0.5f, 5, 11},
		{0.14f, 7, 9},
		{0.5f, 9, 7},
		{0.12f, 11, 5},
		{0.17f, 13, 3}
	}, 1.25f, 16384, false);

	{
		auto& wt = Instruments["Clarinet"].WaveTables.EmplaceLast();
		wt.Tables = &clarinetTables;
		wt.VolumeScale = 0.5f;
		wt.VibratoValue = 0.005f;
		wt.VibratoFrequency = 5;

		wt.ADSR.AttackTime = 0.1f;
		wt.ADSR.DecayTime = 0.05f;
		wt.ADSR.SustainVolume = 0.75f;
		wt.ADSR.ReleaseTime = 0.1f;
	}

	{
		auto& wave = Instruments["Birds"].Waves.EmplaceLast();
		wave.Octaves = 2;

		wave.ADSR.AttackTime = 0.1f;
		wave.ADSR.ReleaseTime = 0.2f;
		Instruments["Birds"].Chorus = ChorusFactory(0.3f, 3, 0.75, 0.25);
	}

	{
		auto& wt = Instruments["SoundTrackFX2"].WaveTables.EmplaceLast();
		wt.Tables = &synthStringTables;
		wt.VolumeScale = 0.1f;

		wt.ADSR.AttackTime = 0.7f;
		wt.ADSR.DecayTime = 0.3f;
		wt.ADSR.SustainVolume = 0.5f;
		wt.ADSR.ReleaseTime = 0.2f;
	}

	{
		auto& instr = Instruments["ReverseCymbal"];
		instr.WhiteNoise.FreqMultiplier = 5;
		instr.WhiteNoise.VolumeScale = 0.2f;
		instr.ADSR.AttackTime = 1;
		instr.ADSR.ReleaseTime = 0.8f;
	}

	{
		auto& wave = Instruments["Accordion"].Waves.EmplaceLast();
		wave.Wave = SawtoothWaveForm{5};
		wave.Scale = 0.5f;
		//wave.Octaves = 3;

		//Accordion.WaveTables.AddLast({&AccordionTables, 0, 0.35f});

		wave.ADSR.AttackTime = 0.02f;
		wave.ADSR.ReleaseTime = 0.05f;
	}

	{
		auto& sine = Instruments["BassLead"].Waves.EmplaceLast();
		sine.Scale = 0.15f;
		sine.ExpCoeff = 6.5f;

		auto& saw = Instruments["BassLead"].Waves.EmplaceLast();
		saw.Wave = SawtoothWaveForm{10};
		saw.Scale = 0.1f;
		saw.ExpCoeff = 8;

		Instruments["BassLead"].ADSR.AttackTime = 0.01f;
		Instruments["BassLead"].ADSR.ReleaseTime = 0.1f;
	}

	{
		auto& instr = Instruments["ElectricBassPick"];
		auto& sine = instr.Waves.EmplaceLast();
		sine.Scale = 0.45f;
		sine.ExpCoeff = 6.5f;

		auto& saw = instr.Waves.EmplaceLast();
		saw.Wave = SawtoothWaveForm{3};
		saw.Scale = 0.25f;
		saw.ExpCoeff = 8;

		instr.ADSR.AttackTime = 0.01f;
		instr.ADSR.ReleaseTime = 0.1f;
	}

	Instruments["Sitar"] = CreateGuitar(15, 6.5f, 0.2f, 0.5f, 1, 1, 0.3f, 0.01f, 0.4f);
	//Guitar = CreateGuitar(15, 128, 3.5f, 1.1f, 1, 1, 0.35f);
	//Instruments["Guitar"] = CreateGuitar(15, 3, 1.7f, 1.15f, 1, 1, 0.25f);
	//GuitarSteel = CreateGuitar(15, 224, 3.5f, 1.7f, 1, 1, 0.3f);
	//Instruments["GuitarSteel"] = CreateGuitar(15, 5, 2.5f, 0.75f, 1.2f, 1, 0.25f);

	{
		auto& guitar = Instruments["Guitar"];
		auto& wave = guitar.Waves.EmplaceLast();
		wave.Wave = GuitarWaveForm{0.15f};
		wave.SmoothingFactor = 0.7f;
		wave.ExpCoeff = 1.5f;
		wave.Scale = 3;
		wave.ADSR.AttackTime = 0.01f;
		wave.ADSR.ReleaseTime = 0.3f;

		Instruments["GuitarSteel"] = guitar;
	}

	{
		auto& wave = Instruments["OverdrivenGuitar"].Waves.EmplaceLast();
		wave.Wave = WhiteNoiseWaveForm();
		wave.Scale = 0.35f;
		wave.ExpCoeff = 4;
		wave.FreqMultiplier = 2;

		wave.ADSR.AttackTime = 0.01f;
		wave.ADSR.ReleaseTime = 0.1f;
	}

	/*OverdrivenGuitar.Synth = CreateSawtoothSynthPass(9.5, 0.35f, 1, 1);
	
	OverdrivenGuitar.Modifiers.AddLast( CreateModifierPass(Modifiers::RelPulsator(6)) );
	OverdrivenGuitar.Attenuation = CreateTableAttenuationPass(
		{Norm8(0.1), 0.3, 0.7, 0.8, 0.75, 0.3, 0.25, 0.2, 0.16, 0.12, 0.09, 0.06, 0.03, 0.01, 0});*/


	/*auto& trumpetTables = Tables["Trumpet"] = CreateWaveTablesFromHarmonics(CreateUpdownHarmonicArray(3, 0, 8, 64), 1.0f, 32768, false);

	{
		auto& wt = Instruments["Trumpet"].WaveTables.EmplaceLast();
		wt.VolumeScale = 0.25f;
		wt.Tables = &trumpetTables;

		wt.ADSR.AttackTime = 0.01f;
		wt.ADSR.ReleaseTime = 0.02f;
	}*/

	{
		auto& wave = Instruments["Trumpet"].Waves.EmplaceLast();
		wave.Wave = SawtoothWaveForm{8};
		wave.Scale = 0.15f;

		wave.ADSR.AttackTime = 0.01f;
		wave.ADSR.ReleaseTime = 0.02f;
	}

	{
		auto& wave = Instruments["Tuba"].Waves.EmplaceLast();
		wave.Wave = SawtoothWaveForm{8};
		wave.Scale = 0.2f;

		wave.ADSR.AttackTime = 0.01f;
		wave.ADSR.ReleaseTime = 0.05f;
	}

	{
		auto& wave = Instruments["FrenchHorn"].Waves.EmplaceLast();
		wave.Wave = SawtoothWaveForm{8};
		wave.Scale = 0.15f;

		wave.ADSR.AttackTime = 0.05f;
		wave.ADSR.ReleaseTime = 0.05f;
	}

	{
		auto& wave = Instruments["Oboe"].Waves.EmplaceLast();
		wave.Wave = SawtoothWaveForm{0.6f};
		wave.Scale = 0.45f;

		//Oboe.WaveTables.AddLast({&FluteTables, 0, 0.5f});

		wave.ADSR.AttackTime = 0.07f;
		wave.ADSR.ReleaseTime = 0.07f;
	}

	{
		auto& wave = Instruments["FretlessBass"].Waves.EmplaceLast();
		wave.Wave = SawtoothWaveForm{15};
		wave.ExpCoeff = 5;
		wave.Scale = 0.2f;

		wave.ADSR.AttackTime = 0.01f;
		wave.ADSR.ReleaseTime = 0.1f;
	}

	{
		auto& wave = Instruments["Sax"].Waves.EmplaceLast();
		wave.Wave = SawtoothWaveForm{0.2f};
		wave.Scale = 0.15f;
		wave.Octaves = 2;

		wave.ADSR.AttackTime = 0.02f;
		wave.ADSR.ReleaseTime = 0.05f;
	}

	{
		/*auto& wave = Instruments["Organ"].Waves.EmplaceLast();
		wave.Wave = SawtoothWaveForm{0.1f};
		wave.Scale = 0.03f;
		wave.Octaves = 4;*/

		auto& wave = Instruments["Organ"].WaveTables.EmplaceLast();
		wave.Tables = &Tables["SynthOrgan"];
		wave.VolumeScale = 0.1f;

		wave.ADSR.AttackTime = 0.01f;
		wave.ADSR.ReleaseTime = 0.01f;
	}

	{
		auto& wave = Instruments["Whistle"].Waves.EmplaceLast();
		wave.Scale = 0.2f;
		wave.VibratoFrequency = 5;
		wave.VibratoValue = 0.003f;
		
		wave.ADSR.AttackTime = 0.03f;
		wave.ADSR.ReleaseTime = 0.1f;
	}

	{
		auto& wave = Instruments["Ocarina"].Waves.EmplaceLast();
		wave.Scale = 0.3f;
		wave.VibratoFrequency = 5;
		wave.VibratoValue = 0.005f;

		wave.ADSR.AttackTime = 0.003f;
		wave.ADSR.ReleaseTime = 0.1f;
	}

	{
		auto& wave = Instruments["Sine2Exp"].Waves.EmplaceLast();
		wave.ExpCoeff = 9;
		wave.Octaves = 2;
		wave.FreqMultiplier = 1;

		wave.ADSR.AttackTime = 0.01f;
		wave.ADSR.ReleaseTime = 0.1f;
	}

	{
		auto& instr = Instruments["Applause"];
		instr.WhiteNoise.FreqMultiplier = 20;
		instr.WhiteNoise.VolumeScale = 0.15f;
		instr.Chorus.DelayFrequency = 10;
		instr.Chorus.MainVolume = 0.7f;
		instr.Chorus.SecondaryVolume = 0.3f;
		instr.Chorus.MaxDelay = 0.05f;
		instr.ADSR.AttackTime = 0.7f;
		instr.ADSR.ReleaseTime = 0.5f;
	}

	{
		auto& instr = Instruments["Helicopter"];
		instr.WhiteNoise.FreqMultiplier = 1;
		instr.WhiteNoise.VolumeScale = 1;
		/*instr.Chorus.DelayFrequency = 3;
		instr.Chorus.MainVolume = 0.5f;
		instr.Chorus.SecondaryVolume = 0.5f;
		instr.Chorus.MaxDelay = 0.1f;*/
		instr.ADSR.AttackTime = 0.4f;
		instr.ADSR.ReleaseTime = 0.4f;
	}

	{
		auto& instr = Instruments["Seashore"];
		instr.WhiteNoise.FreqMultiplier = 40;
		instr.WhiteNoise.VolumeScale = 0.04f;
		instr.ADSR.AttackTime = 1;
		instr.ADSR.ReleaseTime = 0.7f;
	}

	{
		auto& wave = Instruments["PhoneRing"].Waves.EmplaceLast();
		wave.Scale = 0.5f;
		wave.VibratoValue = 0.03f;
		wave.VibratoFrequency = 5;

		wave.ADSR.AttackTime = 0.2f;
		wave.ADSR.ReleaseTime = 0.2f;
	}
	

	{
		auto& instr = Instruments["GunShot"];
		instr.WhiteNoise.FreqMultiplier = 40;
		instr.WhiteNoise.VolumeScale = 0.2f;
		instr.ExponentAttenuation.ExpCoeff = 4;
		instr.ADSR.AttackTime = 0.005f;
		instr.ADSR.ReleaseTime = 0.1f;
	}

	{
		auto& instr = Instruments["Timpani"];
		instr.WhiteNoise.FreqMultiplier = 20;
		instr.WhiteNoise.VolumeScale = 0.25f;
		instr.ExponentAttenuation.ExpCoeff = 7;
		instr.ADSR.AttackTime = 0.005f;
		instr.ADSR.DecayTime = 0.1f;
		instr.ADSR.SustainVolume = 0.3f;
		instr.ADSR.ReleaseTime = 0.1f;
	}

	{
		auto& wave = Instruments["Kalimba"].Waves.EmplaceLast();
		wave.Scale = 0.24f;
		wave.ExpCoeff = 8;

		wave.ADSR.AttackTime = 0.01f;
		wave.ADSR.ReleaseTime = 0.1f;
	}
	
	UniDrum = CachedDrumInstrument(Generators::DrumPhysicalModel(2, 16, 16, 0.342f, 0.00026f, 0.20f), 44100, 0.02f);

	ClosedHiHat = CachedDrumInstrument(Generators::DrumPhysicalModel(2, 16, 16, 0.338f, 0.04928f, 0.10f), 44100, 0.02f);

	AcousticBassDrum = CachedDrumInstrument(Generators::DrumPhysicalModel(2, 8, 8, 0.092f, 0.0072f, 0.20f), 88200, 0.02f);
}


MusicalInstrument InstrumentLibrary::CreateGuitar(size_t n, float c,
	float d, float e, float f, float freqMult, float volume, float attackTime, float releaseTime)
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
	result.ADSR.AttackTime = attackTime;
	result.ADSR.ReleaseTime = releaseTime;
	return result;
}

