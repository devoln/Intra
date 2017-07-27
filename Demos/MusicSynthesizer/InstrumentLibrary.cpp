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
#include "Audio/Synth/TubeSampler.h"
#include "Audio/Synth/Filter.h"

#include "Audio/Synth/Generators.hh"
#include "Audio/Synth/PostEffects.hh"

#include "Audio/Synth/WaveTableGeneration.h"

using namespace Intra;
using namespace Audio;
using namespace Synth;


InstrumentLibrary::InstrumentLibrary()
{
	ChoirATables.Generator = [](float freq, uint sampleRate)
	{
		WaveTable tbl;
		tbl.BaseLevelLength = 32768;
		tbl.Data.Reserve(tbl.BaseLevelLength * 2);
		tbl.Data.SetCount(tbl.BaseLevelLength / 2);
		tbl.BaseLevelRatio = freq/float(sampleRate);
		for(int i = 1; i<64; i++)
		{
			using namespace Math;
			const float ifreq = float(i)*freq;
			const float formants = Exp(-Sqr((ifreq - 600) / 150)) +
				Exp(-Sqr((ifreq - 900) / 250)) +
				Exp(-Sqr((ifreq - 2200) / 200)) +
				Exp(-Sqr((ifreq - 2600) / 250)) +
				Exp(-Sqr(ifreq / 3000)) / 10;
			Synth::AddSineHarmonicGaussianProfile(tbl.Data, tbl.BaseLevelRatio, float(i), 1, formants/float(i*i), 60);
		}
		ConvertAmplitudesToSamples(tbl);
		return tbl;
	};
	
	{
		auto& wt = ChoirAahs.WaveTables.EmplaceLast();
		wt.Tables = &ChoirATables;
		wt.VolumeScale = 0.25f;
		wt.ADSR.AttackTime = 0.02f;
		wt.ADSR.DecayTime = 0.5f;
		wt.ADSR.SustainVolume = 0.8f;
		wt.ADSR.ReleaseTime = 0.05f;
	}

	ChoirOTables.Generator = [](float freq, uint sampleRate)
	{
		WaveTable tbl;
		tbl.BaseLevelLength = 32768;
		tbl.Data.Reserve(tbl.BaseLevelLength * 2);
		tbl.Data.SetCount(tbl.BaseLevelLength / 2);
		tbl.BaseLevelRatio = freq/float(sampleRate);
		for(int i = 1; i<64; i++)
		{
			using namespace Math;
			const float ifreq = float(i)*freq;
			const float formants =
				Exp(-Sqr((ifreq - 275) / 100)) +
				Exp(-Sqr((ifreq - 850) / 300)) +
				Exp(-Sqr((ifreq - 2400) / 200)) +
				Exp(-Sqr((ifreq - 2600) / 250)) +
				Exp(-Sqr((ifreq) / 3000)) / 10;
			Synth::AddSineHarmonicGaussianProfile(tbl.Data, tbl.BaseLevelRatio, float(i), 0.5f, formants/float(i*i), 100);
		}
		ConvertAmplitudesToSamples(tbl);
		return tbl;
	};

	{
		auto& wt = PadChoir.WaveTables.EmplaceLast();
		wt.Tables = &ChoirOTables;
		wt.VolumeScale = 0.6f;
		wt.ADSR.AttackTime = 0.07f;
		wt.ADSR.DecayTime = 0.5f;
		wt.ADSR.SustainVolume = 0.5f;
		wt.ADSR.ReleaseTime = 0.5f;
	}

	{
		auto& wt = Pad7Halo.WaveTables.EmplaceLast();
		wt.Tables = &ChoirOTables;
		wt.VolumeScale = 0.6f;
		wt.ADSR.AttackTime = 0.03f;
		wt.ADSR.DecayTime = 0.5f;
		wt.ADSR.SustainVolume = 0.7f;
		wt.ADSR.ReleaseTime = 0.3f;
	}


	SynthStringTables.Generator = [](float freq, uint sampleRate)
	{
		WaveTable tbl;
		tbl.BaseLevelLength = 32768;
		tbl.Data.Reserve(tbl.BaseLevelLength * 2);
		tbl.Data.SetCount(tbl.BaseLevelLength / 2);
		tbl.BaseLevelRatio = freq/float(sampleRate);
		for(int i = 1; i<64; i++)
		{
			using namespace Math;
			const float ifreq = (i)*freq;
			const float formants =
				Exp(-Sqr((ifreq - 500) / 200)) +
				Exp(-Sqr((ifreq - 900) / 750)) +
				Exp(-Sqr((ifreq - 2100) / 2000)) +
				Exp(-Sqr((ifreq - 3700) / 3000)) +
				Exp(-Sqr((ifreq - 4700) / 4000))
				/* +
				Exp(-Sqr((ifreq) / 150))*/;
			Synth::AddSineHarmonicGaussianProfile(tbl.Data, tbl.BaseLevelRatio, float(i), 1, formants/(i), 40);
		}
		ConvertAmplitudesToSamples(tbl);
		return tbl;
	};

	{
		auto& wt = RockOrgan.WaveTables.EmplaceLast();
		wt.Tables = &SynthStringTables;
		wt.VolumeScale = 0.4f;
		wt.ADSR.AttackTime = 0.001f;
		wt.ADSR.SustainVolume = 0.6f;
		wt.ADSR.DecayTime = 0.1f;
		wt.ADSR.ReleaseTime = 0.05f;
		wt.VibratoValue = 0.004f;
		wt.VibratoFrequency = 5;
	}

	RainTables.Generator = [](float freq, uint sampleRate)
	{
		WaveTable tbl;
		tbl.BaseLevelLength = 32768;
		tbl.Data.Reserve(tbl.BaseLevelLength * 2);
		tbl.Data.SetCount(tbl.BaseLevelLength / 2);
		tbl.BaseLevelRatio = freq/float(sampleRate);
		for(int i = 1; i<64; i++)
		{
			using namespace Math;
			const float ifreq = float(i)*freq;
			const float formants = Exp(-Sqr((ifreq - 600) / 150)) +
				Exp(-Sqr((ifreq - 900) / 350)) +
				Exp(-Sqr((ifreq - 2200) / 1200)) +
				Exp(-Sqr((ifreq - 2600) / 2050)) +
				Exp(-Sqr((ifreq) / 300)) / 10;
			Synth::AddSineHarmonicGaussianProfile(tbl.Data, tbl.BaseLevelRatio, float(i), 1, formants/float(i*i), 40);
		}
		ConvertAmplitudesToSamples(tbl);
		return tbl;
	};

	{
		auto& wt = Rain.WaveTables.EmplaceLast();
		wt.Tables = &RainTables;
		wt.ExpCoeff = 3;
		wt.VolumeScale = 1;
		wt.VibratoFrequency = 7;
		wt.VibratoValue = 0.005f;
		wt.ADSR.AttackTime = 0.003f;
		wt.ADSR.DecayTime = 0.3f;
		wt.ADSR.SustainVolume = 0.7f;
		wt.ADSR.ReleaseTime = 0.2f;
	}

	SynthVoiceTables.Generator = [](float freq, uint sampleRate)
	{
		WaveTable tbl;
		tbl.BaseLevelLength = 32768;
		tbl.Data.Reserve(tbl.BaseLevelLength * 2);
		tbl.Data.SetCount(tbl.BaseLevelLength / 2);
		tbl.BaseLevelRatio = freq/float(sampleRate);
		for(int i = 1; i<64; i++)
		{
			using namespace Math;
			const float ifreq = float(i)*freq;
			const float formants =
				Exp(-Sqr((ifreq - 600) / 150)) +
				Exp(-Sqr((ifreq - 900) / 350)) +
				Exp(-Sqr((ifreq - 2200) / 1200)) +
				Exp(-Sqr((ifreq - 2600) / 250)) +
				Exp(-Sqr((ifreq) / 3000)) / 10;
			Synth::AddSineHarmonicGaussianProfile(tbl.Data, tbl.BaseLevelRatio, float(i), 1, formants/float(i*i), 40);
		}
		ConvertAmplitudesToSamples(tbl);
		return tbl;
	};

	{
		auto& wt = SynthVoice.WaveTables.EmplaceLast();
		wt.Tables = &SynthVoiceTables;
		wt.VolumeScale = 0.2f;
		wt.ADSR.AttackTime = 0.04f;
		wt.ADSR.ReleaseTime = 0.1f;
	}

	{
		auto& wt = VoiceOohs.WaveTables.EmplaceLast();
		wt.Tables = &ChoirOTables;
		wt.VolumeScale = 0.25f;
		wt.ADSR.AttackTime = 0.005f;
		wt.ADSR.DecayTime = 0.3f;
		wt.ADSR.SustainVolume = 0.6f;
		wt.ADSR.ReleaseTime = 0.2f;
	}
	
	{
		auto& wt = Pad8Sweep.WaveTables.EmplaceLast();
		wt.Tables = &SynthStringTables;
		wt.VolumeScale = 0.15f;
		wt.ADSR.AttackTime = 0.15f;
		wt.ADSR.ReleaseTime = 0.25f;
	}

	{
		auto& wt = SynthStrings.WaveTables.EmplaceLast();
		wt.Tables = &SynthStringTables;
		wt.VolumeScale = 0.2f;
		wt.ADSR.AttackTime = 0.3f;
		wt.ADSR.ReleaseTime = 0.2f;
	}

	{
		auto& wt = StringEnsemble.WaveTables.EmplaceLast();
		wt.Tables = &SynthStringTables;
		wt.VolumeScale = 0.15f;
		wt.ADSR.AttackTime = 0.05f;
		wt.ADSR.ReleaseTime = 0.07f;
	}

	{
		auto& wt = TremoloStrings.WaveTables.EmplaceLast();
		wt.Tables = &SynthStringTables;
		wt.VolumeScale = 0.25f;
		wt.ADSR.AttackTime = 0.01f;
		wt.ADSR.ReleaseTime = 0.07f;
	}

	{
		auto& wt = PercussiveOrgan.WaveTables.EmplaceLast();
		wt.Tables = &SynthVoiceTables;
		wt.VolumeScale = 0.5f;
		wt.ADSR.AttackTime = 0.003f;
		wt.ADSR.DecayTime = 0.3f;
		wt.ADSR.SustainVolume = 0.6f;
		wt.ADSR.ReleaseTime = 0.05f;
	}

	ViolinTables.Generator = [](float freq, uint sampleRate)
	{
		WaveTable tbl;
		tbl.BaseLevelLength = 16384;
		tbl.Data.Reserve(tbl.BaseLevelLength * 2);
		tbl.Data.SetCount(tbl.BaseLevelLength / 2);
		tbl.BaseLevelRatio = freq/float(sampleRate);
		for(int i = 1; i<32; i++)
		{
			using namespace Math;
			const float ifreq = float(i)*freq;
			const float formants =
				Exp(-Sqr((ifreq - 500) / 70)) +
				Exp(-Sqr((ifreq - 800) / 450)) +
				Exp(-Sqr((ifreq - 2100) / 2000)) +
				Exp(-Sqr((ifreq - 3700) / 3000)) +
				Exp(-Sqr(ifreq / 10));
			Synth::AddSineHarmonicGaussianProfile(tbl.Data, tbl.BaseLevelRatio, float(i), 1, formants/float(i), 8);
		}
		ConvertAmplitudesToSamples(tbl);
		return tbl;
	};

	{
		auto& wt = Violin.WaveTables.EmplaceLast();
		wt.Tables = &ViolinTables;
		wt.VolumeScale = 0.4f;
		wt.VibratoFrequency = 5;
		wt.VibratoValue = 0.001f;

		wt.ADSR.AttackTime = 0.08f;
		wt.ADSR.DecayTime = 0.5f;
		wt.ADSR.SustainVolume = 0.8f;
		wt.ADSR.ReleaseTime = 0.1f;
	}
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
		tbl.BaseLevelRatio = freq/float(sampleRate);
		for(int i = 1; i<64; i++)
		{
			Synth::AddSineHarmonicGaussianProfile(tbl.Data, tbl.BaseLevelRatio, float(i), 0.85f, 1.0f/float(i), 15);
		}
		ConvertAmplitudesToSamples(tbl);
		return tbl;
	};

	{
		auto& wt = SynthBrass.WaveTables.EmplaceLast();
		wt.Tables = &SynthBrassTables;
		wt.VolumeScale = 0.3f;
		wt.VibratoValue = 0.003f;
		wt.VibratoFrequency = 5;

		wt.ADSR.AttackTime = 0.01f;
		wt.ADSR.DecayTime = 0.5f;
		wt.ADSR.SustainVolume = 0.7f;
		wt.ADSR.ReleaseTime = 0.1f;
	}

	{
		auto& wt = PadPolysynth.WaveTables.EmplaceLast();
		wt.Tables = &SynthBrassTables;
		wt.VolumeScale = 0.1f;
		wt.ADSR.AttackTime = 0.01f;
		wt.ADSR.ReleaseTime = 0.15f;
	}

	{
		auto& wt = FxGoblins.WaveTables.EmplaceLast();
		wt.Tables = &RainTables;
		wt.VolumeScale = 0.07f;
		wt.ADSR.AttackTime = 0.05f;
		wt.ADSR.ReleaseTime = 0.15f;
	}


	OrchestraHitTables.Generator = [](float freq, uint sampleRate)
	{
		WaveTable tbl;
		tbl.BaseLevelLength = 32768;
		tbl.Data.Reserve(tbl.BaseLevelLength * 2);
		tbl.Data.SetCount(tbl.BaseLevelLength / 2);
		tbl.BaseLevelRatio = freq/float(sampleRate);
		for(int i = 1; i<64; i++)
		{
			Synth::AddSineHarmonicGaussianProfile(tbl.Data, tbl.BaseLevelRatio, float(i), 0.25f, 1.0f/float(i), 50);
		}
		ConvertAmplitudesToSamples(tbl);
		return tbl;
	};
	OrchestraHitTables.AllowMipmaps = true;

	{
		auto& wt = OrchestraHit.WaveTables.EmplaceLast();
		wt.Tables = &OrchestraHitTables;
		wt.VolumeScale = 0.3f;

		OrchestraHit.WhiteNoise.FreqMultiplier = 10;
		OrchestraHit.WhiteNoise.VolumeScale = 0.12f;
		OrchestraHit.ExponentAttenuation.ExpCoeff = 10;

		OrchestraHit.ADSR.AttackTime = 0.02f;
		OrchestraHit.ADSR.ReleaseTime = 0.1f;
	}

	FluteTables.Generator = [](float freq, uint sampleRate)
	{
		WaveTable tbl;
		tbl.BaseLevelLength = 16384;
		tbl.Data.Reserve(tbl.BaseLevelLength * 2);
		tbl.Data.SetCount(tbl.BaseLevelLength / 2);
		tbl.BaseLevelRatio = freq/float(sampleRate);
		float sign = 1;
		for(int i = 1; i<64; i+=2)
		{
			Synth::AddSineHarmonicGaussianProfile(tbl.Data, tbl.BaseLevelRatio, float(i), 1, sign/float(i*i), 15*(1 + 0.1f*float(i)));
			sign = -sign;
		}
		ConvertAmplitudesToSamples(tbl);
		return tbl;
	};

	CalliopeTables.Generator = [](float freq, uint sampleRate)
	{
		WaveTable tbl;
		tbl.BaseLevelLength = 32768;
		tbl.Data.Reserve(tbl.BaseLevelLength * 2);
		tbl.Data.SetCount(tbl.BaseLevelLength / 2);
		tbl.BaseLevelRatio = freq/float(sampleRate);
		for(int i = 1; i<64; i++)
		{
			using namespace Math;
			const float ifreq = float(i)*freq;
			const float formants =
				Exp(-Sqr((ifreq - 275) / 1000)) +
				Exp(-Sqr((ifreq - 650) / 2000)) +
				Exp(-Sqr((ifreq - 1100) / 1000)) +
				Exp(-Sqr((ifreq - 2700) / 250)) +
				Exp(-Sqr((ifreq) / 3000)) / 10;
			Synth::AddSineHarmonicGaussianProfile(tbl.Data, tbl.BaseLevelRatio, float(i), 2, formants/float(i), 8);
		}
		ConvertAmplitudesToSamples(tbl);
		return tbl;
	};

	{
		auto& wt = Calliope.WaveTables.EmplaceLast();
		wt.Tables = &CalliopeTables;
		wt.ExpCoeff = 4;
		wt.VolumeScale = 0.5f;

		wt.ADSR.AttackTime = 0.01f;
		wt.ADSR.DecayTime = 0.2f;
		wt.ADSR.SustainVolume = 0.5f;
		wt.ADSR.ReleaseTime = 0.2f;
	}

	VibraphoneTables.Generator = [](float freq, uint sampleRate)
	{
		WaveTable tbl;
		tbl.BaseLevelLength = 16384;
		tbl.Data.Reserve(tbl.BaseLevelLength * 2);
		tbl.Data.SetCount(tbl.BaseLevelLength / 2);
		tbl.BaseLevelRatio = freq/float(sampleRate);
		Synth::AddSineHarmonicGaussianProfile(tbl.Data, tbl.BaseLevelRatio, 1, 2, 1.0f, 7);
		for(int i = 4; i<64; i *= 2)
		{
			Synth::AddSineHarmonicGaussianProfile(tbl.Data, tbl.BaseLevelRatio, float(i), 1, 1.0f/float(i), 7*(1+i*0.3f));
		}
		ConvertAmplitudesToSamples(tbl);
		return tbl;
	};
	VibraphoneTables.AllowMipmaps = true;

	{
		auto& wt = Vibraphone.WaveTables.EmplaceLast();
		wt.Tables = &VibraphoneTables;
		wt.VolumeScale = 0.15f;
		wt.ExpCoeff = 4;
		wt.ADSR.AttackTime = 0.01f;
		wt.ADSR.ReleaseTime = 0.8f;
	}

	Crystal = Vibraphone;

	MarimbaTables.Generator = [](float freq, uint sampleRate)
	{
		WaveTable tbl;
		tbl.BaseLevelLength = 32768;
		tbl.Data.Reserve(tbl.BaseLevelLength * 2);
		tbl.Data.SetCount(tbl.BaseLevelLength / 2);
		tbl.BaseLevelRatio = freq/float(sampleRate);
		Synth::AddSineHarmonicGaussianProfile(tbl.Data, tbl.BaseLevelRatio, 1, 1.25f, 1.0f, 5);
		Synth::AddSineHarmonicGaussianProfile(tbl.Data, tbl.BaseLevelRatio, 4, 1.25f, 1.0f/4, 5);
		Synth::AddSineHarmonicGaussianProfile(tbl.Data, tbl.BaseLevelRatio, 9.2f, 1.25f, 1.0f/9.2f, 5);
		for(int i = 12; i<64; i *= 2)
		{
			Synth::AddSineHarmonicGaussianProfile(tbl.Data, tbl.BaseLevelRatio, float(i), 1.25f, 1.0f/float(i), 3);
		}
		ConvertAmplitudesToSamples(tbl);
		return tbl;
	};
	MarimbaTables.AllowMipmaps = true;

	{
		auto& wt = Marimba.WaveTables.EmplaceLast();
		wt.Tables = &MarimbaTables;
		wt.VolumeScale = 0.25f;
		wt.ExpCoeff = 8;
		wt.ADSR.AttackTime = 0.003f;
		wt.ADSR.ReleaseTime = 0.7f;
	}

	XylophoneTables.Generator = [](float freq, uint sampleRate)
	{
		WaveTable tbl;
		tbl.BaseLevelLength = 32768;
		tbl.Data.Reserve(tbl.BaseLevelLength * 2);
		tbl.Data.SetCount(tbl.BaseLevelLength / 2);
		tbl.BaseLevelRatio = freq/float(sampleRate);
		Synth::AddSineHarmonicGaussianProfile(tbl.Data, tbl.BaseLevelRatio, 1, 1, 1.0f, 30);
		Synth::AddSineHarmonicGaussianProfile(tbl.Data, tbl.BaseLevelRatio, 3, 1, 1.0f/3, 20);
		Synth::AddSineHarmonicGaussianProfile(tbl.Data, tbl.BaseLevelRatio, 9.2f, 1, 1.0f/9.2f, 10);
		for(float i = 13; i<64; i *= 2.3f)
		{
			Synth::AddSineHarmonicGaussianProfile(tbl.Data, tbl.BaseLevelRatio, i, 1, 1.0f/float(i), 10);
		}
		ConvertAmplitudesToSamples(tbl);
		return tbl;
	};
	XylophoneTables.AllowMipmaps = true;

	{
		auto& wt = Xylophone.WaveTables.EmplaceLast();
		wt.Tables = &XylophoneTables;
		wt.VolumeScale = 0.25f;
		wt.ExpCoeff = 10;

		wt.ADSR.AttackTime = 0.003f;
		wt.ADSR.ReleaseTime = 0.4f;
	}


	AtmosphereTables.Generator = [](float freq, uint sampleRate)
	{
		WaveTable tbl;
		tbl.BaseLevelLength = 32768;
		tbl.Data.Reserve(tbl.BaseLevelLength * 2);
		tbl.Data.SetCount(tbl.BaseLevelLength / 2);
		tbl.BaseLevelRatio = freq/float(sampleRate);
		for(int i = 1; i<64; i++)
		{
			Synth::AddSineHarmonicGaussianProfile(tbl.Data, tbl.BaseLevelRatio, float(i), 0.85f, 1.0f/float(i*i), 15);
		}
		ConvertAmplitudesToSamples(tbl);
		return tbl;
	};
	AtmosphereTables.AllowMipmaps = true;

	{
		auto& wt = Atmosphere.WaveTables.EmplaceLast();
		wt.Tables = &AtmosphereTables;
		wt.VolumeScale = 0.2f;

		wt.ADSR.AttackTime = 0.01f;
		wt.ADSR.DecayTime = 0.4f;
		wt.ADSR.SustainVolume = 0.3f;
		wt.ADSR.ReleaseTime = 0.1f;
	}

	NewAgeTables.Generator = [](float freq, uint sampleRate)
	{
		WaveTable tbl;
		tbl.BaseLevelLength = 32768;
		tbl.Data.Reserve(tbl.BaseLevelLength * 2);
		tbl.Data.SetCount(tbl.BaseLevelLength / 2);
		tbl.BaseLevelRatio = freq/float(sampleRate);
		Synth::AddSineHarmonicGaussianProfile(tbl.Data, tbl.BaseLevelRatio, 1, 2, 1.0f, 20);
		for(int i = 4; i<64; i *= 2)
		{
			Synth::AddSineHarmonicGaussianProfile(tbl.Data, tbl.BaseLevelRatio, float(i), 1, 2.0f/float(i), 15);
		}
		ConvertAmplitudesToSamples(tbl);
		return tbl;
	};
	NewAgeTables.AllowMipmaps = true;

	{
		//auto& wave = NewAge.Waves.EmplaceLast();
		//wave.Scale = 0.07f;
		//wave.Octaves = 5;
		auto& wt = NewAge.WaveTables.EmplaceLast();
		wt.Tables = &NewAgeTables;
		wt.ExpCoeff = 1;
		wt.VolumeScale = 0.2f;

		wt.ADSR.AttackTime = 0.005f;
		wt.ADSR.DecayTime = 0.5f;
		wt.ADSR.SustainVolume = 0.6f;
		wt.ADSR.ReleaseTime = 0.4f;
	}

	Pad5Bowed = NewAge;

	LeadSquareTables.Generator = [](float freq, uint sampleRate)
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
	};
	//LeadSquareTables.AllowMipmaps = true;

	{
		//LeadSquare.WaveTables.AddLast({&LeadSquareTables, 0, 0.3f});
		auto& wave = LeadSquare.Waves.EmplaceLast();
		wave.Scale = 0.2f;
		wave.VibratoFrequency = -2;
		wave.VibratoValue = 0.9f;

		wave.ADSR.AttackTime = 0.02f;
		wave.ADSR.DecayTime = 0.5f;
		wave.ADSR.SustainVolume = 0.8f;
		wave.ADSR.ReleaseTime = 0.1f;
	}

	LeadSawtoothTables.Generator = [](float freq, uint sampleRate)
	{
		WaveTable tbl;
		tbl.BaseLevelLength = 16384;
		tbl.Data.Reserve(tbl.BaseLevelLength * 2);
		tbl.Data.SetCount(tbl.BaseLevelLength / 2);
		tbl.BaseLevelRatio = freq/float(sampleRate);
		float sign = 1;
		for(int i = 1; i<64; i++)
		{
			Synth::AddSineHarmonicGaussianProfile(tbl.Data, tbl.BaseLevelRatio, float(i), 0.85f, sign/float(i), 15);
			sign = -sign;
		}
		ConvertAmplitudesToSamples(tbl);
		return tbl;
	};

	{
		/*auto& wave = LeadSawtooth.Waves.EmplaceLast();
		wave.Type = WaveType::Sawtooth;
		wave.Scale = 0.2f;
		wave.UpdownRatio = 25;*/

		auto& wt = LeadSawtooth.WaveTables.EmplaceLast();
		wt.VolumeScale = 0.2f;
		wt.Tables = &LeadSawtoothTables;

		wt.ADSR.AttackTime = 0.02f;
		wt.ADSR.DecayTime = 0.5f;
		wt.ADSR.SustainVolume = 0.7f;
		wt.ADSR.ReleaseTime = 0.15f;
	}


	{
		const float volume = 0.1f;
		const float  D = 0.75f, E = 0.93f, F = 0.5f, G = 0.8f, H = 0.7f;
		float scaleSum = 0;
		for(float k = 0; k < 10; k++)
		{
			auto& wave = Glockenspiel.Waves.EmplaceLast();
			wave.ExpCoeff = Math::Exp(5*(H-0.5f)+1.25f*G*k)+3;
			wave.Scale = Math::Exp(-0.625f*D*k)*Math::Sin((1+7*E)*k*k + 1);
			wave.FreqMultiplier = Math::Exp(1.25f*F*k) * 4;
			scaleSum += wave.Scale;
		}
		for(auto& wave: Glockenspiel.Waves)
			wave.Scale *= volume/scaleSum;
		Glockenspiel.ADSR.AttackTime = 0.003f;
		Glockenspiel.ADSR.ReleaseTime = 0.1f;
	}

	{
		const float volume = 0.33f;
		auto& harm1 = Piano.Waves.EmplaceLast();
		harm1.Scale = 0.15f*volume;
		harm1.ExpCoeff = 3;
		harm1.FreqMultiplier = 1;

		auto& harm2 = Piano.Waves.EmplaceLast();
		harm2.Scale = -0.075f*volume;
		harm2.ExpCoeff = 3;
		harm2.FreqMultiplier = 3;

		auto& harm3 = Piano.Waves.EmplaceLast();
		harm3.Scale = -0.075f*volume;
		harm3.ExpCoeff = 3;
		harm3.FreqMultiplier = 5;

		auto& harm4 = Piano.Waves.EmplaceLast();
		harm4.Scale = 0.0375f*volume;
		harm4.ExpCoeff = 3;
		harm4.FreqMultiplier = 7;

		auto& harm5 = Piano.Waves.EmplaceLast();
		harm5.Scale = -0.0375f*volume;
		harm5.ExpCoeff = 3;
		harm5.FreqMultiplier = 9;


		auto& harm6 = Piano.Waves.EmplaceLast();
		harm6.Scale = 0.15f*volume;
		harm6.ExpCoeff = 4;
		harm6.FreqMultiplier = 2;

		auto& harm7 = Piano.Waves.EmplaceLast();
		harm7.Scale = -0.15f*volume;
		harm7.ExpCoeff = 4;
		harm7.FreqMultiplier = 4;

		auto& harm8 = Piano.Waves.EmplaceLast();
		harm8.Scale = 0.075f*volume;
		harm8.ExpCoeff = 4;
		harm8.FreqMultiplier = 5;

		auto& harm9 = Piano.Waves.EmplaceLast();
		harm9.Scale = -0.075f*volume;
		harm9.ExpCoeff = 4;
		harm9.FreqMultiplier = 7;

		auto& harm10 = Piano.Waves.EmplaceLast();
		harm10.Scale = 0.0375f*volume;
		harm10.ExpCoeff = 4;
		harm10.FreqMultiplier = 11;

		auto& harm11 = Piano.Waves.EmplaceLast();
		harm11.Scale = -0.0375f*volume;
		harm11.ExpCoeff = 4;
		harm11.FreqMultiplier = 13;

		Piano.ADSR.AttackTime = 0.003f;
		Piano.ADSR.ReleaseTime = 0.2f;
	}

	{
		const float volume = 0.27f;
		auto& harm1 = ElectricPiano.Waves.EmplaceLast();
		harm1.Scale = 0.15f*volume;
		harm1.ExpCoeff = 3;
		harm1.FreqMultiplier =1;

		auto& harm2 = ElectricPiano.Waves.EmplaceLast();
		harm2.Scale = 0.15f*volume;
		harm2.ExpCoeff = 3.1f;
		harm2.FreqMultiplier = 2;

		auto& harm3 = ElectricPiano.Waves.EmplaceLast();
		harm3.Scale = -0.075f*volume;
		harm3.ExpCoeff = 3.2f;
		harm3.FreqMultiplier = 3;

		auto& harm4 = ElectricPiano.Waves.EmplaceLast();
		harm4.Scale = -0.15f*volume;
		harm4.ExpCoeff = 3.3f;
		harm4.FreqMultiplier = 4;

		auto& harm5 = ElectricPiano.Waves.EmplaceLast();
		harm5.Scale = -0.0375f*volume;
		harm5.ExpCoeff = 3.6f;
		harm5.FreqMultiplier = 7;

		auto& harm6 = ElectricPiano.Waves.EmplaceLast();
		harm6.Scale = -0.0375f*volume;
		harm6.ExpCoeff = 3.8f;
		harm6.FreqMultiplier = 9;

		auto& harm7 = ElectricPiano.Waves.EmplaceLast();
		harm7.Scale = 0.0375f*volume;
		harm7.ExpCoeff = 4;
		harm7.FreqMultiplier = 11;

		auto& harm8 = ElectricPiano.Waves.EmplaceLast();
		harm8.Scale = -0.0375f*volume;
		harm8.ExpCoeff = 4.2f;
		harm8.FreqMultiplier = 13;

		ElectricPiano.ADSR.AttackTime = 0.003f;
		ElectricPiano.ADSR.ReleaseTime = 0.2f;
	}

	ElectricPiano2 = ElectricPiano;

	{
		auto& wave = Bass1.Waves.EmplaceLast();
		wave.Scale = 0.3f;
		wave.ExpCoeff = 5;
		wave.FreqMultiplier = 2;

		Bass1.ADSR.AttackTime = 0.02f;
		Bass1.ADSR.ReleaseTime = 0.1f;
	}

	{
		auto& wave = ElectricBassFinger.Waves.EmplaceLast();
		wave.Scale = 0.4f;
		wave.ExpCoeff = 4;
		wave.FreqMultiplier = 2;

		wave.ADSR.AttackTime = 0.01f;
		wave.ADSR.ReleaseTime = 0.1f;
	}

	{
		auto& wave = Bass2.Waves.EmplaceLast();
		wave.Scale = 0.3f;
		wave.ExpCoeff = 4;
		wave.Octaves = 2;
		//wave.RateAcceleration = -0.15f;
		wave.FreqMultiplier = 2;

		wave.ADSR.AttackTime = 0.01f;
		wave.ADSR.ReleaseTime = 0.1f;
	}

	{
		auto& wave = SlapBass.Waves.EmplaceLast();
		wave.Type = WaveType::Sawtooth;
		wave.UpdownRatio = 0.25f;
		wave.Scale = 0.3f;
		wave.ExpCoeff = 4;

		wave.ADSR.AttackTime = 0.01f;
		wave.ADSR.ReleaseTime = 0.1f;
	}

	{
		auto& wave = Bass3.Waves.EmplaceLast();
		wave.Scale = 0.7f;
		wave.ExpCoeff = 4;
		//wave.RateAcceleration = -0.15f;
		wave.FreqMultiplier = 2;

		wave.ADSR.AttackTime = 0.01f;
		wave.ADSR.ReleaseTime = 0.1f;
	}

	{
		auto& wave = SynthBass1.Waves.EmplaceLast();
		wave.Type = WaveType::Sawtooth;
		wave.Scale = 0.4f;
		wave.ExpCoeff = 10;
		wave.UpdownRatio = 20;

		wave.ADSR.AttackTime = 0.01f;
		wave.ADSR.ReleaseTime = 0.1f;
	}

	{
		auto& wave = SynthBass2.Waves.EmplaceLast();
		wave.Scale = 0.5f;
		wave.ExpCoeff = 7;
		//wave.RateAcceleration = -0.12f;
		wave.FreqMultiplier = 2;

		wave.ADSR.AttackTime = 0.01f;
		wave.ADSR.ReleaseTime = 0.1f;
	}

	{
		auto& wave = Lead5Charang.Waves.EmplaceLast();
		wave.Type = WaveType::Sawtooth;
		wave.Scale = 0.2f;
		wave.UpdownRatio = 20;
		wave.Octaves = 1;

		wave.ADSR.AttackTime = 0.03f;
		wave.ADSR.ReleaseTime = 0.05f;
	}


	{
		/*MusicalInstrument flute1;
		TubeInstrument tube1;
		tube1.Scale = 1;
		tube1.DetuneValue = 0.999f;
		tube1.DetuneFactor = 0.00001f;
		tube1.RandomCoeff = 0.05f;
		flute1.GenericInstruments.AddLast(tube1);

		flute1.GenericModifiers.AddLast(DriveEffect(90));
		flute1.GenericModifiers.AddLast(ResonanceFilterFactory(-1, 0.95f));
		flute1.GenericModifiers.AddLast(ResonanceFilterFactory(-3, 0.95f));
		flute1.GenericModifiers.AddLast(SoftHighPassFilterFactory(7000));
		flute1.GenericModifiers.AddLast(NormalizeEffect(0.1f));

		MusicalInstrument flute2;
		TubeInstrument tube2;
		tube2.Scale = 1;
		tube2.DetuneValue = 1.001f;
		tube2.DetuneFactor = 0.00001f;
		tube2.RandomCoeff = 0.05f;
		flute2.GenericInstruments.AddLast(tube2);

		flute2.GenericModifiers.AddLast(DriveEffect(90));
		flute2.GenericModifiers.AddLast(ResonanceFilterFactory(-1, 0.95f));
		flute2.GenericModifiers.AddLast(ResonanceFilterFactory(-4, 0.95f));
		flute2.GenericModifiers.AddLast(SoftHighPassFilterFactory(7000));
		flute2.GenericModifiers.AddLast(NormalizeEffect(0.1f));

		Flute.GenericInstruments.AddLast(Cpp::Move(flute1));
		Flute.GenericInstruments.AddLast(Cpp::Move(flute2));*/

		auto& wt = Flute.WaveTables.EmplaceLast();
		wt.Tables = &FluteTables;
		wt.VolumeScale = 0.7f;
		wt.VibratoValue = 0.003f;
		wt.VibratoFrequency = 6;
		//auto& wave = Flute.Waves.EmplaceLast();
		//wave.Type = WaveType::Sawtooth;
		//wave.Scale = 0.5f;
		//wave.UpdownRatio = 0.5f;

		wt.ADSR.AttackTime = 0.04f;
		wt.ADSR.DecayTime = 0.3f;
		wt.ADSR.SustainVolume = 0.6f;
		wt.ADSR.ReleaseTime = 0.1f;
	}

	{
		//auto& wave = PanFlute.Waves.EmplaceLast();
		//wave.Type = WaveType::Sawtooth;
		//wave.Scale = 0.5f;
		//wave.UpdownRatio = 0.4f;

		//PanFlute.WaveTables.AddLast({&FluteTables, 0, 0.6f});

		/*TubeInstrument tube;
		tube.Scale = 1;
		tube.DetuneValue = 0.9999f;
		tube.DetuneFactor = 0.00001f;
		tube.RandomCoeff = 0.05f;
		PanFlute.GenericInstruments.AddLast(tube);

		PanFlute.GenericModifiers.AddLast(DriveEffect(900));
		PanFlute.GenericModifiers.AddLast(ResonanceFilterFactory(-1, 0.9f));
		PanFlute.GenericModifiers.AddLast(ResonanceFilterFactory(-3, 0.9f));
		PanFlute.GenericModifiers.AddLast(SoftHighPassFilterFactory(7500));
		PanFlute.GenericModifiers.AddLast(NormalizeEffect(0.2f));*/

		auto& wt = PanFlute.WaveTables.EmplaceLast();
		wt.Tables = &FluteTables;
		wt.VolumeScale = 0.7f;
		wt.VibratoValue = 0.003f;
		wt.VibratoFrequency = 6;

		wt.ADSR.AttackTime = 0.05f;
		wt.ADSR.DecayTime = 0.2f;
		wt.ADSR.SustainVolume = 0.6f;
		wt.ADSR.ReleaseTime = 0.1f;
	}

	{
		/*TubeInstrument tube;
		tube.Scale = 1;
		tube.DetuneValue = 0.9999f;
		tube.DetuneFactor = 0.00001f;
		tube.RandomCoeff = 0.05f;
		Piccolo.GenericInstruments.AddLast(tube);

		Piccolo.GenericModifiers.AddLast(DriveEffect(900));
		Piccolo.GenericModifiers.AddLast(ResonanceFilterFactory(-1, 0.9f));
		Piccolo.GenericModifiers.AddLast(ResonanceFilterFactory(-3, 0.9f));
		Piccolo.GenericModifiers.AddLast(SoftHighPassFilterFactory(7500));
		Piccolo.GenericModifiers.AddLast(NormalizeEffect(0.2f));*/

		//Piccolo = Flute;

		//auto& wave = Piccolo.Waves.EmplaceLast();
		//wave.Type = WaveType::Sawtooth;
		//wave.Scale = 0.5f;
		//wave.UpdownRatio = 0.5f;

		auto& wt = Piccolo.WaveTables.EmplaceLast();
		wt.Tables = &FluteTables;
		wt.VolumeScale = 0.7f;
		wt.VibratoValue = 0.003f;
		wt.VibratoFrequency = 6;

		wt.ADSR.AttackTime = 0.05f;
		wt.ADSR.DecayTime = 0.2f;
		wt.ADSR.SustainVolume = 0.6f;
		wt.ADSR.ReleaseTime = 0.1f;
	}

	{
		auto& wave = Birds.Waves.EmplaceLast();
		wave.Octaves = 2;

		wave.ADSR.AttackTime = 0.1f;
		wave.ADSR.ReleaseTime = 0.2f;
		Birds.Chorus = ChorusFactory(0.3f, 3, 0.75, 0.25);
	}

	{
		auto& wt = SoundTrackFX2.WaveTables.EmplaceLast();
		wt.Tables = &SynthStringTables;
		wt.VolumeScale = 0.1f;

		wt.ADSR.AttackTime = 0.7f;
		wt.ADSR.DecayTime = 0.3f;
		wt.ADSR.SustainVolume = 0.5f;
		wt.ADSR.ReleaseTime = 0.2f;
	}

	{
		ReverseCymbal.WhiteNoise.FreqMultiplier = 5;
		ReverseCymbal.WhiteNoise.VolumeScale = 0.2f;
		ReverseCymbal.ADSR.AttackTime = 1;
		ReverseCymbal.ADSR.ReleaseTime = 0.8f;
	}

	{
		auto& wave = Accordion.Waves.EmplaceLast();
		wave.Type = WaveType::Sawtooth;
		wave.Scale = 0.5f;
		wave.UpdownRatio = 5;
		//wave.Octaves = 3;

		//Accordion.WaveTables.AddLast({&AccordionTables, 0, 0.35f});

		wave.ADSR.AttackTime = 0.02f;
		wave.ADSR.ReleaseTime = 0.05f;
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

		wave.ADSR.AttackTime = 0.01f;
		wave.ADSR.ReleaseTime = 0.1f;
	}

	/*OverdrivenGuitar.Synth = CreateSawtoothSynthPass(9.5, 0.35f, 1, 1);
	
	OverdrivenGuitar.Modifiers.AddLast( CreateModifierPass(Modifiers::RelPulsator(6)) );
	OverdrivenGuitar.Attenuation = CreateTableAttenuationPass(
		{Norm8(0.1), 0.3, 0.7, 0.8, 0.75, 0.3, 0.25, 0.2, 0.16, 0.12, 0.09, 0.06, 0.03, 0.01, 0});*/


	{
		auto& wave = Trumpet.Waves.EmplaceLast();
		wave.Type = WaveType::Sawtooth;
		wave.Scale = 0.15f;
		wave.UpdownRatio = 8;

		wave.ADSR.AttackTime = 0.01f;
		wave.ADSR.ReleaseTime = 0.05f;
	}

	{
		auto& wave = Tuba.Waves.EmplaceLast();
		wave.Type = WaveType::Sawtooth;
		wave.Scale = 0.2f;
		wave.UpdownRatio = 8;

		wave.ADSR.AttackTime = 0.01f;
		wave.ADSR.ReleaseTime = 0.05f;
	}

	{
		auto& wave = FrenchHorn.Waves.EmplaceLast();
		wave.Type = WaveType::Sawtooth;
		wave.Scale = 0.15f;
		wave.UpdownRatio = 8;

		wave.ADSR.AttackTime = 0.05f;
		wave.ADSR.ReleaseTime = 0.05f;
	}

	{
		auto& wave = Oboe.Waves.EmplaceLast();
		wave.Type = WaveType::Sawtooth;
		wave.Scale = 0.45f;
		wave.UpdownRatio = 0.6f;
		wave.Octaves = 1;
		wave.FreqMultiplier = 1;

		//Oboe.WaveTables.AddLast({&FluteTables, 0, 0.5f});

		wave.ADSR.AttackTime = 0.07f;
		wave.ADSR.ReleaseTime = 0.07f;
	}

	{
		auto& wave = FretlessBass.Waves.EmplaceLast();
		wave.Type = WaveType::Sawtooth;
		wave.ExpCoeff = 5;
		wave.Scale = 0.4f;
		wave.UpdownRatio = 15;

		wave.ADSR.AttackTime = 0.01f;
		wave.ADSR.ReleaseTime = 0.1f;
	}

	{
		auto& wave = Sax.Waves.EmplaceLast();
		wave.Type = WaveType::Sawtooth;
		wave.Scale = 0.15f;
		wave.UpdownRatio = 0.2f;
		wave.Octaves = 2;

		wave.ADSR.AttackTime = 0.02f;
		wave.ADSR.ReleaseTime = 0.05f;
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

		wave.ADSR.AttackTime = 0.01f;
		wave.ADSR.ReleaseTime = 0.01f;
	}

	{
		auto& wave = Whistle.Waves.EmplaceLast();
		wave.Scale = 0.3f;
		wave.VibratoFrequency = 5;
		wave.VibratoValue = 0.003f;
		
		wave.ADSR.AttackTime = 0.03f;
		wave.ADSR.ReleaseTime = 0.25f;
	
		/*Whistle.Chorus.MaxDelay = 0.03f;
		Whistle.Chorus.MainVolume = 0.75f;
		Whistle.Chorus.SecondaryVolume = 0.25f;
		Whistle.Chorus.DelayFrequency = 10;*/
	}

	{
		auto& wave = Ocarina.Waves.EmplaceLast();
		wave.Scale = 0.3f;
		wave.VibratoFrequency = 5;
		wave.VibratoValue = 0.005f;

		wave.ADSR.AttackTime = 0.003f;
		wave.ADSR.ReleaseTime = 0.1f;

		/*Whistle.Chorus.MaxDelay = 0.03f;
		Whistle.Chorus.MainVolume = 0.75f;
		Whistle.Chorus.SecondaryVolume = 0.25f;
		Whistle.Chorus.DelayFrequency = 10;*/
	}

	{
		auto& wave = Sine2Exp.Waves.EmplaceLast();
		wave.ExpCoeff = 9;
		wave.Octaves = 2;
		wave.FreqMultiplier = 1;

		wave.ADSR.AttackTime = 0.01f;
		wave.ADSR.ReleaseTime = 0.1f;
	}


	Applause.WhiteNoise.FreqMultiplier = 20;
	Applause.WhiteNoise.VolumeScale = 0.15f;
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
	Seashore.WhiteNoise.VolumeScale = 0.04f;
	Seashore.ADSR.AttackTime = 1;
	Seashore.ADSR.ReleaseTime = 0.7f;

	{
		auto& wave = PhoneRing.Waves.EmplaceLast();
		wave.Scale = 0.5f;
		wave.VibratoValue = 0.03f;
		wave.VibratoFrequency = 5;

		wave.ADSR.AttackTime = 0.2f;
		wave.ADSR.ReleaseTime = 0.2f;
		//PhoneRing.Modifiers.AddLast( CreateModifierPass(Modifiers::AbsPulsator(5)) );
	}
	

	GunShot.WhiteNoise.FreqMultiplier = 40;
	GunShot.WhiteNoise.VolumeScale = 0.15f;
	GunShot.ExponentAttenuation.ExpCoeff = 4;
	GunShot.ADSR.AttackTime = 0.005f;
	GunShot.ADSR.ReleaseTime = 0.1f;

	Timpani.WhiteNoise.FreqMultiplier = 20;
	Timpani.WhiteNoise.VolumeScale = 0.15f;
	Timpani.ExponentAttenuation.ExpCoeff = 8;
	Timpani.ADSR.AttackTime = 0.005f;
	Timpani.ADSR.ReleaseTime = 0.04f;


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

		wave.ADSR.AttackTime = 0.01f;
		wave.ADSR.ReleaseTime = 0.1f;
	}
	
	UniDrum = CachedDrumInstrument(Generators::DrumPhysicalModel(2, 16, 16, 0.342f, 0.00026f, 0.20f), 44100, 0.02f);

	ClosedHiHat = CachedDrumInstrument(Generators::DrumPhysicalModel(2, 16, 16, 0.338f, 0.04928f, 0.10f), 44100, 0.02f);

	AcousticBassDrum = CachedDrumInstrument(Generators::DrumPhysicalModel(2, 8, 8, 0.092f, 0.0072f, 0.20f), 44100, 0.02f);
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
	result.ADSR.ReleaseTime = 0.2f;
	return result;
}

