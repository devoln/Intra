#ifndef INTRA_NO_MIDI_SYNTH

#include "Sound/InstrumentLibrary.h"
#include "Containers/Array2D.h"

namespace Intra {

using namespace Math;

static float DrumSample(float freq, float t)
{
	float e = 0.5f, f = 0.5f;
	float x = 2*float(PI)*t;
	float resonanse = 20*0.00390625f;
	float cutoff = 5*0.0019531f;
	auto exponent = Exp(-20*t);
	float a = Sin(218*x)*0.3051758f;
	a += Sin(343*x)*0.06103516f;
	a += Sin(570*x)*0.1831055f;
	a += Sin(625*x)*0.1831055f;
	a *= exponent;

	float k3 = float(PI)*cutoff;
	k3 = 1.0f/Tan(k3);
	resonanse *= k3;
	k3 *= k3;
	float km = 1.0f/(1.0f+resonanse+k3);
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



struct DrumPhysicalModel: Range::RangeMixin<DrumPhysicalModel, float, Range::TypeEnum::Forward, false>
{
	byte Cnt;
	byte DX, DY;
	float Frc, K1, K2;
	Array2D<float> P, S, F;
	float ampl, dt;
	Random<float> frandom;

	float NextSample() {PopFirst(); return First();}

	void PopFirst()
	{
		const float maxP = 0.3f;

		const uint maxX = DX-1u;
		const uint maxY = DY-1u;

		for(uint i=0; i<Cnt; i++)
		{
			for(uint y=0; y<DY; y++)
			{
				for(uint x=0; x<DX; x++)
				{
					S(x, y) += ((P((x-1) & maxX, y) + P((x+1) & maxX, y) +
								 P(x, (y-1) & maxY) + P(x, (y+1) & maxY))*0.25f - P(x, y)
						)*F(x, y);
				}
			}

			for(uint y=0; y<DY; y++)
			{
				for(uint x=0; x<DX; x++)
				{
					S(x, y) = S(x, y)*K1 +
						(S((x-1) & maxX, y) + S((x+1) & maxX, y) +
						S(x, (y-1) & maxY) + S(x, (y+1) & maxY))*K2;
				}
			}

			for(uint y=0; y<DY; y++)
				for(uint x=0; x<DX; x++)
					P(x, y) += S(x, y);

			for(uint x=0; x<DX; x+=4)
			{
				if((P(x, 0)>maxP && S(x, 0)>0) || (P(x, 0)<maxP && S(x, 0)<0))
						S(x, 0) *= -0.5f;
			}

			P(1, 1) *= 0.5f;
			S(0, 0) = sRand()*0.00001f;
		}
	}

	float First() const {return P(1, DY/2u)*ampl;}

	bool Empty() const {return P.Width()==0;}

	DrumPhysicalModel(null_t=null):
		Cnt(0), DX(0), DY(0),
		Frc(0), K1(0), K2(0),
		P(), S(), F(),
		ampl(1), dt(0), frandom() {}

	DrumPhysicalModel(byte count, byte dx, byte dy, float frc, float kDemp, float kRand):
		Cnt(count), DX(dx), DY(dy),
		Frc(frc), K1(0), K2(0),
		P(dx, dy), S(dx, dy), F(dx, dy),
		ampl(1), dt(1.0f/44100.0f), frandom()
	{
		K1 = 1.0f - kDemp*0.333f*frc;
		K2 = (1.0f-K1)*0.25f;

		for(uint y=0; y<dy; y++)
		{
			for(uint x=0; x<dx; x++)
			{
				float v = 1.0f+sRand()*kRand;
				F(x, y) = Frc*v;
			}
		}
		F(0, 0) = frc;
		F(dx/2u, dy/2u) = frc;
		F(1, dy/2u) = frc;
		F(dx/2u, 1) = frc;
		S(0, 0) = 10;
		S(dx/2u, dy/2u) = -10;
	}

	void SetParams(float frequency, float amplitude, double step)
	{
		(void)frequency;
		ampl = amplitude;
		dt = float(step);
	}

	float sRand()
	{
		static float RR=0;
		float R = frandom();
		float result = R - RR;
		RR = R;
		return result;
	}

};



static DrumInstrument CreateDrums()
{
	DrumInstrument result;

	auto UniDrum = new SynthesizedInstrument;
	UniDrum->SynthPass = SynthesizedInstrument::CreateSynthPass(DrumPhysicalModel(2, 16, 16, 0.342f, 0.00026f, 0.20f), 0.03f, 1, 0.35f);
	//UniDrum->SynthPass = SynthesizedInstrument::CreateSynthPass(SoundSamplers::SoundSampler<float(*)(float, float)>(DrumSample), 0.1f, 1, 0.35f);
	UniDrum->PostEffects.AddLast(SoundPostEffects::FilterQ(4000, 0.6f));
	//UniDrum->PostEffects.AddLast(SoundPostEffects::FilterHP(0.9f));
	UniDrum->PostEffects.AddLast(SoundPostEffects::Fade(0, 1000));
	UniDrum->MinNoteDuration = 0.25f;
	for(uint id=0; id<=127; id++)
		result.Generators[id]=UniDrum;


	/*auto ClosedHiHat = new SynthesizedInstrument;
	ClosedHiHat->SynthPass = SynthesizedInstrument::CreateSynthPass(DrumGenerator(2, 16, 16, 0.342f, 0.00026f, 0.20f), 0.03f, 1, 0.35f);
	ClosedHiHat->PostEffects.AddLast(SoundPostEffects::FilterQ(12000, 0.2f));
	//ClosedHiHat->PostEffects.AddLast(SoundPostEffects::FilterHP(0.9f));
	ClosedHiHat->PostEffects.AddLast(SoundPostEffects::Fade(0, 1000));
	ClosedHiHat->MinNoteDuration = 0.25f;*/

	auto ClosedHiHat = new SynthesizedInstrument;
	ClosedHiHat->SynthPass = SynthesizedInstrument::CreateSynthPass(DrumPhysicalModel(2, 16, 16, 0.338f, 0.04928f, 0.10f), 0.03f, 1, 0.35f);
	ClosedHiHat->PostEffects.AddLast(SoundPostEffects::FilterQ(5000, 0.9f));
	//ClosedHiHat->PostEffects.AddLast(SoundPostEffects::FilterHP(0.9f));
	ClosedHiHat->PostEffects.AddLast(SoundPostEffects::Fade(0, 1000));
	ClosedHiHat->MinNoteDuration = 0.25f;

	for(uint id: {41u}) result.Generators[id] = ClosedHiHat;

	auto AcousticBassDrum = new SynthesizedInstrument;
	AcousticBassDrum->SynthPass = SynthesizedInstrument::CreateSynthPass(DrumPhysicalModel(2, 8, 8, 0.092f, 0.0072f, 0.20f), 0.03f, 1, 0.35f);
	AcousticBassDrum->PostEffects.AddLast(SoundPostEffects::FilterQ(8500, 0.3f));
	AcousticBassDrum->PostEffects.AddLast(SoundPostEffects::FilterHP(0.4f));
	AcousticBassDrum->PostEffects.AddLast(SoundPostEffects::Fade(0, 1000));
	AcousticBassDrum->MinNoteDuration = 0.2f;

	for(uint id: {34u, 35u}) result.Generators[id]=AcousticBassDrum;

	//auto instr1 = new SynthesizedInstrument;
	//instr1->SynthPass = SynthesizedInstrument::CreateSynthPass(DrumPhysicalModel(2, 16, 16, 0.342f, 0.00026f, 0.20f), 0.05f, 1, 0.35f);
	//instr1->SynthPass = SynthesizedInstrument::CreateSynthPass(SoundSamplers::SoundSampler<float(*)(float, float)>(DrumSample), 0.4f, 1, 1);
	//instr1->MinNoteDuration = 0.3f;

	//for(auto id: {39}) result.Generators[id]=instr1;

	return result;
}


MusicalInstruments::MusicalInstruments()
{
	SynthesizedInstrument pianoInstr1;
	pianoInstr1.SynthPass = SynthesizedInstrument::CreateSineSynthPass(0.42f*0.25f, 3, 1);
	pianoInstr1.ModifierPasses.AddLast( SynthesizedInstrument::CreateModifierPass(SoundModifiers::RelPulsator(0.5f)) );
	pianoInstr1.AttenuationPass = SynthesizedInstrument::CreateExpAttenuationPass(4);
	pianoInstr1.MinNoteDuration = 1.2f;

	SynthesizedInstrument pianoInstr2;
	pianoInstr2.SynthPass = SynthesizedInstrument::CreateSineSynthPass(0.38f*0.25f, 3, 1.5f);
	pianoInstr2.ModifierPasses.AddLast( SynthesizedInstrument::CreateModifierPass(SoundModifiers::RelPulsator(0.5f)) );
	pianoInstr2.AttenuationPass = SynthesizedInstrument::CreateExpAttenuationPass(3);
	pianoInstr2.MinNoteDuration = 1.5f;

	Piano.Combination.AddLast(pianoInstr1);
	Piano.Combination.AddLast(pianoInstr2);


	//Piano=CreateGuitar(20, 1, 2.15f, 1.3f, 1.3f, 1.0f, 1.2f);

	

	/*SynthesizedInstrument epiano2Instr1;
	epiano2Instr1.SynthPass = SynthesizedInstrument::CreateSineSynthPass(0.52f*0.7f, 3, 1);
	epiano2Instr1.ModifierPasses.AddLast( SynthesizedInstrument::CreateModifierPass(SoundModifiers::RelPulsator(0.5f)) );
	epiano2Instr1.AttenuationPass = SynthesizedInstrument::CreateExpAttenuationPass(7);
	epiano2Instr1.MinNoteDuration = 0.5f;*/

	SynthesizedInstrument epiano2Instr2;
	epiano2Instr2.SynthPass = SynthesizedInstrument::CreateSineExpSynthPass({{0.48f*0.3f, 5, 1.0f, 1.0f}});
	epiano2Instr2.MinNoteDuration = 0.4f;

	ElectricPiano2 = Piano;//{{epiano2Instr1/*, epianoInstr2*/}};
	ElectricPiano.Combination.AddLast(epiano2Instr2);
	//ElectricPiano=/*ElectricPiano2;*/{{epiano2Instr2/*, epianoInstr2*/}};


	/*SynthesizedInstrument ElectricPiano={
		SynthesizedInstrument::CreateSynthPass(SoundSamplers::RelModSine(2.5, 1), 0.5f, 1),
		null,
		SynthesizedInstrument::CreateTableAttenuationPass(
			{0.7f, 1.0f, 1.0f, 0.9f, 0.8f, 0.7f, 0.65f, 0.6f, 0.5f, 0.42f, 0.37f, 0.33f, 0.28f, 0.24f, 0.2f, 0.15f, 0.11f, 0.0f})
	};*/

	/*SynthesizedInstrument ElectricPiano={
		SynthesizedInstrument::CreateSynthPass(SoundSamplers::RelModSine(0.75, 1), 0.25f, 1),
		null,
		//SynthesizedInstrument::CreateAttenuationPass(SoundAttenuators::Exponential(1.0f, 7.0))
		SynthesizedInstrument::CreateADPass(0.003, 0.08)
	};*/


	Bass1.SynthPass = SynthesizedInstrument::CreateSynthPass(SoundSamplers::RelModSine(0.25, 0.25), 0.2f, 1, 1.0f);
	Bass1.AttenuationPass = SynthesizedInstrument::CreateTableAttenuationPass(
			{norm8(0.7), 0.99, 0.99, 0.9, 0.8, 0.7, 0.65, 0.6, 0.5, 0.42, 0.28, 0.15, 0.11, 0.0});

	ElectricBassFinger.SynthPass = SynthesizedInstrument::CreateSynthPass(SoundSamplers::RelModSine(0.25, 0.25), 0.4f, 1, 1.0f);
	ElectricBassFinger.AttenuationPass = SynthesizedInstrument::CreateTableAttenuationPass(
			{norm8(0.7), 0.99, 0.99, 0.9, 0.8, 0.7, 0.65, 0.6, 0.5, 0.42, 0.28, 0.15, 0.11, 0.0});

	Bass2.SynthPass = SynthesizedInstrument::CreateSynthPass(SoundSamplers::RelModSine(0.75, 1), 0.3f, 2);
	Bass2.AttenuationPass = SynthesizedInstrument::CreateTableAttenuationPass(
			{norm8(0.7), 0.99, 0.99, 0.9, 0.8, 0.7, 0.65, 0.6, 0.5,
		        0.42, 0.37, 0.33, 0.28, 0.24, 0.2, 0.15, 0.11, 0.0});



	Bass3.SynthPass = SynthesizedInstrument::CreateSynthPass(SoundSamplers::RelModSine(0.75, 1), 1, 1);
	Bass3.AttenuationPass = SynthesizedInstrument::CreateTableAttenuationPass(
			{norm8(0.1), 0.4, 0.99, 0.9, 0.8, 0.6, 0.55, 0.48, 0.45, 0.42,
		        0.37, 0.33, 0.28, 0.24, 0.2, 0.15, 0.11, 0.0});

	
	
	SynthBass1.SynthPass = SynthesizedInstrument::CreateSawtoothSynthPass(20, 0.6f, 1, 0.5f);
	SynthBass1.AttenuationPass = SynthesizedInstrument::CreateExpAttenuationPass(10);

	SynthBass2.SynthPass = SynthesizedInstrument::CreateSynthPass(SoundSamplers::RelModSine(0.75, 0.5f), 1, 1);
	SynthBass2.AttenuationPass = SynthesizedInstrument::CreateExpAttenuationPass(7);



	SynthBrass.SynthPass = SynthesizedInstrument::CreateSawtoothSynthPass(20, 0.12f, 3, 0.5f);
	SynthBrass.AttenuationPass = SynthesizedInstrument::CreateADPass(0.05, 0.1);

	Lead5Charang.SynthPass = SynthesizedInstrument::CreateSawtoothSynthPass(20, 0.22f, 3, 0.5f);
	Lead5Charang.AttenuationPass = SynthesizedInstrument::CreateADPass(0.03, 0.05);

	/*SynthesizedInstrument InterestingInstrument1={ //Интересный звук типа скрипки. Может пригодится
		SynthesizedInstrument::CreateSawtoothSynthPass(0.75, 1, 3, 1),
		{SynthesizedInstrument::CreateModifierPass(SoundModifiers::RelPulsator(6))},
		//SynthesizedInstrument::CreateAttenuationPass(SoundAttenuators::Exponential(1.0f, 7.0))
		SynthesizedInstrument::CreateADPass(0.1, 0.2)
	};

	SynthesizedInstrument InterestingInstrument2={ //Аналогично
		SynthesizedInstrument::CreateSawtoothSynthPass(0.75, 1, 3, 1),
		{SynthesizedInstrument::CreateModifierPass(SoundModifiers::RelPulsator(1))},
		//SynthesizedInstrument::CreateAttenuationPass(SoundAttenuators::Exponential(1.0f, 7.0))
		SynthesizedInstrument::CreateADPass(0.1, 0.2)
	};*/


	SynthesizedInstrument fluteSine;
	fluteSine.SynthPass = SynthesizedInstrument::CreateSineSynthPass(0.075f, 2, 0.5f);
	fluteSine.AttenuationPass = SynthesizedInstrument::CreateADPass(0.15, 0.05);

	SynthesizedInstrument fluteSawtooth;
	fluteSawtooth.SynthPass = SynthesizedInstrument::CreateSawtoothSynthPass(0.5f, 0.15f, 1, 0.5f);
	fluteSawtooth.AttenuationPass = SynthesizedInstrument::CreateADPass(0.1, 0.05);

	Flute.Combination.AddLast(fluteSine);
	Flute.Combination.AddLast(fluteSawtooth);

	SynthesizedInstrument panFluteSine;
	panFluteSine.SynthPass = SynthesizedInstrument::CreateSineSynthPass(0.2f, 2, 0.5f);
	panFluteSine.AttenuationPass = SynthesizedInstrument::CreateADPass(0.2, 0.05);

	SynthesizedInstrument panFluteSawtooth;
	panFluteSawtooth.SynthPass = SynthesizedInstrument::CreateSawtoothSynthPass(0.4f, 0.3f, 2, 0.5f);
	panFluteSawtooth.AttenuationPass = SynthesizedInstrument::CreateADPass(0.25, 0.05);

	PanFlute.Combination.AddLast(panFluteSine);
	PanFlute.Combination.AddLast(panFluteSawtooth);


	Birds.SynthPass = SynthesizedInstrument::CreateSineSynthPass(1, 2);
	Birds.AttenuationPass = SynthesizedInstrument::CreateADPass(0.1, 0.2);
	Birds.PostEffects.AddLast(SoundPostEffects::Chorus(0.3f, 3, 0.75, 0.25));

	SynthVoice.SynthPass = SynthesizedInstrument::CreateSawtoothSynthPass(2, 0.35f, 1, 0.5f);
	SynthVoice.AttenuationPass = SynthesizedInstrument::CreateADPass(0.1, 0.2);
	//SynthVoice.PostEffects.AddLast(SoundPostEffects::FilterQ(6000, 0.6f));
	SynthVoice.PostEffects.AddLast(SoundPostEffects::Chorus(0.002f, 2, 0.8f, 0.2f));
	SynthVoice.PostEffects.AddLast(SoundPostEffects::FilterHP(0.8f));

	PadChoir.SynthPass = SynthesizedInstrument::CreateSawtoothSynthPass(1, 0.5f, 1, 0.5f);
	PadChoir.AttenuationPass = SynthesizedInstrument::CreateADPass(0.2, 0.3);

	SoundTrackFX2.SynthPass = SynthesizedInstrument::CreateSawtoothSynthPass(5, 0.15f, 2, 0.5f);
	SoundTrackFX2.AttenuationPass = SynthesizedInstrument::CreateTableAttenuationPass(
			{norm8(0.1), 0.27, 0.41, 0.55, 0.74, 0.90, 0.99, 0.81, 0.62, 0.44, 0.25, 0.05});

	

	Pad7Halo.SynthPass = SynthesizedInstrument::CreateSawtoothSynthPass(2, 0.2f, 1, 0.5f);
	Pad7Halo.AttenuationPass = SynthesizedInstrument::CreateADPass(0.08, 0.05);
	Pad7Halo.PostEffects.AddLast(SoundPostEffects::Chorus(0.002f, 2, 0.75, 0.25));
	Pad7Halo.MinNoteDuration=0.3f;


	Pad8Sweep.SynthPass = SynthesizedInstrument::CreateSawtoothSynthPass(2, 0.12f, 1, 0.5f);
	Pad8Sweep.AttenuationPass = SynthesizedInstrument::CreateADPass(0.08, 0.05);
	Pad8Sweep.PostEffects.AddLast(SoundPostEffects::Chorus(0.002f, 2, 0.75, 0.25));
	Pad8Sweep.MinNoteDuration=0.3f;

	StringEnsemble.SynthPass = SynthesizedInstrument::CreateSawtoothSynthPass(2, 0.15f, 1, 0.5f);
	StringEnsemble.AttenuationPass = SynthesizedInstrument::CreateADPass(0.08, 0.05);
	StringEnsemble.PostEffects.AddLast(SoundPostEffects::Chorus(0.002f, 2, 0.75, 0.25));
	StringEnsemble.MinNoteDuration=0.3f;

	ReverseCymbal.SynthPass = SynthesizedInstrument::CreateNoiseSynthPass(0.2f, 1, 20);//SynthesizedInstrument::CreateSawtoothSynthPass(1.0f, 0.5f, 1, 1.0f);
	ReverseCymbal.AttenuationPass = SynthesizedInstrument::CreateTableAttenuationPass(
			{norm8(0.1), 0.2, 0.45, 0.7, 0.99, 1.0, 0.32});
	ReverseCymbal.FadeOffTime = 1.0f;

	Sawtooth.SynthPass = SynthesizedInstrument::CreateSawtoothSynthPass(10, 0.25f, 1, 0.5f);
	Sawtooth.AttenuationPass = SynthesizedInstrument::CreateTableAttenuationPass(
			{norm8(0.5), 0.8, 0.996, 0.95, 0.92, 0.85, 0.72, 0.55, 0.42, 0.31, 0.17, 0.05});


	LeadSquare.SynthPass = SynthesizedInstrument::CreateSynthPass(SoundSamplers::RelModSine(1.5, 2), 0.2f, 1, 0.5f);
	LeadSquare.AttenuationPass = SynthesizedInstrument::CreateTableAttenuationPass(
			{norm8(0.4), 0.7, 0.995, 0.98, 0.95, 0.92, 0.90, 0.89, 0.79, 0.57, 0.41, 0.22, 0.05});

	Crystal.SynthPass = SynthesizedInstrument::CreateSynthPass(SoundSamplers::RelModSine(0.75, 0.5f), 0.4f, 3, 1);
	Crystal.AttenuationPass = SynthesizedInstrument::CreateExpAttenuationPass(7);

	

	SynthesizedInstrument bassLead;
	bassLead.SynthPass = SynthesizedInstrument::CreateSynthPass(SoundSamplers::RelModSine(0.75, 1), 0.45f, 1, 0.5f);
	bassLead.AttenuationPass = SynthesizedInstrument::CreateExpAttenuationPass(6.5f);
	bassLead.MinNoteDuration = 0.4f;

	SynthesizedInstrument sawtoothBassLead;
	sawtoothBassLead.SynthPass = SynthesizedInstrument::CreateSawtoothSynthPass(10, 0.35f, 1, 0.5f);
	sawtoothBassLead.AttenuationPass = SynthesizedInstrument::CreateExpAttenuationPass(8);
	sawtoothBassLead.MinNoteDuration = 0.4f;

	BassLead.Combination.AddLast(bassLead);
	BassLead.Combination.AddLast(sawtoothBassLead);

	SynthesizedInstrument sawtoothElectricBassPick;
	sawtoothElectricBassPick.SynthPass = SynthesizedInstrument::CreateSawtoothSynthPass(3, 0.25f, 1, 0.5f);
	sawtoothElectricBassPick.AttenuationPass = SynthesizedInstrument::CreateExpAttenuationPass(8);
	sawtoothElectricBassPick.MinNoteDuration = 0.4f;

	ElectricBassPick.Combination.AddLast(bassLead);
	ElectricBassPick.Combination.AddLast(sawtoothElectricBassPick);


	SynthesizedInstrument orchestraHit = {
		SynthesizedInstrument::CreateSynthPass(SoundSamplers::RelModSine(0.85f, 1), 0.5f, 1, 0.5f),
		null,
		SynthesizedInstrument::CreateExpAttenuationPass(7),
		//SynthesizedInstrument::CreateTableAttenuationPass({0.996n8, 0.9f, 0.8f, 0.6f, 0.25f, 0.1f})
		null,
		0.4f
	};

	SynthesizedInstrument sawtoothOrchestraHit;
	sawtoothOrchestraHit.SynthPass = SynthesizedInstrument::CreateSawtoothSynthPass(5, 0.4f, 1, 0.5f);
	sawtoothOrchestraHit.AttenuationPass = SynthesizedInstrument::CreateExpAttenuationPass(7);

	OrchestraHit.Combination.AddLast(orchestraHit);
	OrchestraHit.Combination.AddLast(sawtoothOrchestraHit);


	Atmosphere.SynthPass = SynthesizedInstrument::CreateSineSynthPass(0.6f, 4, 0.5f);
	Atmosphere.AttenuationPass = SynthesizedInstrument::CreateTableAttenuationPass(
		{norm8(0.995), 0.3, 0.6, 0.5, 0.45, 0.3, 0.2, 0.15, 0.11, 0.08, 0.06, 0.02, 0.00});

	Rain.SynthPass = SynthesizedInstrument::CreateSawtoothSynthPass(2, 0.5f, 1, 1);
	Rain.ModifierPasses.AddLast(SynthesizedInstrument::CreateModifierPass(SoundModifiers::AddPulsator(1, 0.75f, 0.25f)));
	Rain.AttenuationPass = SynthesizedInstrument::CreateTableAttenuationPass(
			{norm8(0.7), 0.96, 0.7, 0.6, 0.5, 0.45, 0.3, 0.2, 0.15, 0.11, 0.08, 0.06, 0.02, 0.00});

	Guitar = CreateGuitar(15, 3, 1.7f, 1.15f, 1, 0.5f, 3.0f, 0.35f);//CreateGuitar(15, 128, 3.5f, 1.1f);
	GuitarSteel = CreateGuitar(15, 5, 2.5f, 0.75f, 1.2f, 0.5f, 3.0f, 0.3f);//CreateGuitar(15, 224, 3.5f, 1.7f);

	OverdrivenGuitar.SynthPass = SynthesizedInstrument::CreateSawtoothSynthPass(9.5, 0.35f, 1, 1);
	
	OverdrivenGuitar.ModifierPasses.AddLast( SynthesizedInstrument::CreateModifierPass(SoundModifiers::RelPulsator(6)) );
	OverdrivenGuitar.AttenuationPass = SynthesizedInstrument::CreateTableAttenuationPass(
			{norm8(0.1), 0.3, 0.7, 0.8, 0.75, 0.3, 0.25, 0.2, 0.16, 0.12, 0.09, 0.06, 0.03, 0.01, 0});

	Trumpet.SynthPass = SynthesizedInstrument::CreateSawtoothSynthPass(8, 0.15f, 1, 0.5f);
	Trumpet.AttenuationPass = SynthesizedInstrument::CreateADPass(0.05, 0.7);

	Oboe.SynthPass = SynthesizedInstrument::CreateSawtoothSynthPass(0.6f, 0.45f, 2, 1.0f);
	Oboe.AttenuationPass = SynthesizedInstrument::CreateADPass(0.07, 0.07);
	Oboe.PostEffects.AddLast(SoundPostEffects::Chorus(0.002f, 1, 0.75, 0.25));

	FretlessBass.SynthPass = SynthesizedInstrument::CreateSawtoothSynthPass(15, 0.4f, 1, 0.5f);
	FretlessBass.AttenuationPass = SynthesizedInstrument::CreateExpAttenuationPass(8);

	Sax.SynthPass = SynthesizedInstrument::CreateSawtoothSynthPass(0.2f, 0.2f, 2, 0.5f);
	Sax.AttenuationPass = SynthesizedInstrument::CreateADPass(0.02, 0.05);


	Calliope.SynthPass = SynthesizedInstrument::CreateSynthPass(SoundSamplers::RelModSine(2.5, 1), 0.2f, 1, 0.5f);
	Calliope.AttenuationPass = SynthesizedInstrument::CreateTableAttenuationPass(
		{norm8(0.4), 0.9, 0.99, 0.9, 0.8, 0.7, 0.65, 0.6, 0.5, 0.42, 0.37, 0.33, 0.28, 0.24, 0.2, 0.15, 0.11, 0.0});


	/*static SynthesizedInstrument violinInstr1={
		SynthesizedInstrument::CreateSawtoothSynthPass(0.85f, 0.5f/2, 1, 1),
		//SynthesizedInstrument::CreateSynthPass(SoundSamplers::Sawtooth(0.85f), 0.5f/3, 1),
		null,
		SynthesizedInstrument::CreateADPass(0.1, 0.1)
	};
	static SynthesizedInstrument violinInstr2={
		SynthesizedInstrument::CreateSawtoothSynthPass(1.21f, 0.5f/2, 1, 1.01f),
		null,
		SynthesizedInstrument::CreateADPass(0.1, 0.1)
	};
	
	CombinedSynthesizedInstrument OldViolin={{violinInstr1, violinInstr2}};*/

	Violin.SynthPass = SynthesizedInstrument::CreateSynthPass(SoundSamplers::ViolinPhysicalModel(), 0.25f, 1, 0.375f);
	Violin.AttenuationPass = SynthesizedInstrument::CreateADPass(0.1, 0.1);

	Organ.SynthPass = SynthesizedInstrument::CreateSawtoothSynthPass(0.1f, 0.03f, 4);
	Organ.AttenuationPass = SynthesizedInstrument::CreateADPass(0.01, 0.01);

	PercussiveOrgan.SynthPass = SynthesizedInstrument::CreateSawtoothSynthPass(2, 0.2f, 3);
	PercussiveOrgan.AttenuationPass = SynthesizedInstrument::CreateADPass(0.2, 0.3);

	Whistle.SynthPass = SynthesizedInstrument::CreateSineSynthPass(0.3f, 1, 0.5f);
	Whistle.AttenuationPass = SynthesizedInstrument::CreateADPass(0.2, 0.25);

	Sine2Exp.SynthPass = SynthesizedInstrument::CreateSineSynthPass(1, 2);
	Sine2Exp.AttenuationPass = SynthesizedInstrument::CreateExpAttenuationPass(9);

	Vibraphone.SynthPass = SynthesizedInstrument::CreateSineSynthPass(0.35f, 5, 0.5f);
	Vibraphone.AttenuationPass = SynthesizedInstrument::CreateExpAttenuationPass(7);
	Vibraphone.MinNoteDuration = 0.8f;


	SynthesizedInstrument::SineHarmonic glockenspielHarmonics[]=
	{
		{0.125f, 0.75f},
		{1.0f, 3.0f},
		{0.03125f, 1.25f},
		{1.0f, 1.75f},
		{1.0f, 4.0f},
		{0.125f, 4.5f},
		{0.25f, 5.75f}
	};
	Glockenspiel.SynthPass = SynthesizedInstrument::CreateMultiSineSynthPass(glockenspielHarmonics);
	Glockenspiel.AttenuationPass = SynthesizedInstrument::CreateExpAttenuationPass(7);
	Glockenspiel.MinNoteDuration = 0.8f;


	NewAge.SynthPass = SynthesizedInstrument::CreateSineSynthPass(0.15f, 5, 0.5f);
	NewAge.AttenuationPass = SynthesizedInstrument::CreateTableAttenuationPass(
	    {norm8(0.8), 0.995, 0.7, 0.5, 0.4, 0.05});
	NewAge.MinNoteDuration = 0.55f;


	Applause.SynthPass = SynthesizedInstrument::CreateNoiseSynthPass(0.5f, 1, 10);
	Applause.ModifierPasses.AddLast( SynthesizedInstrument::CreateModifierPass(SoundModifiers::AbsPulsator(20, 0.7f, 0.3f)) );
	Applause.AttenuationPass = SynthesizedInstrument::CreateADPass(1, 0.5);

	Helicopter.SynthPass = SynthesizedInstrument::CreateNoiseSynthPass(4, 1, 0.1f);
	Helicopter.ModifierPasses.AddLast( SynthesizedInstrument::CreateModifierPass(SoundModifiers::AbsPulsator(10, 0, 1)) );
	Helicopter.AttenuationPass = SynthesizedInstrument::CreateADPass(0.4, 0.4);

	Seashore.SynthPass = SynthesizedInstrument::CreateNoiseSynthPass(0.07f, 1, 20);
	Seashore.AttenuationPass = SynthesizedInstrument::CreateADPass(1, 0.7);

	PhoneRing.SynthPass = SynthesizedInstrument::CreateSineSynthPass(0.5f, 1, 0.5f);
	PhoneRing.ModifierPasses.AddLast( SynthesizedInstrument::CreateModifierPass(SoundModifiers::AbsPulsator(5)) );
	PhoneRing.AttenuationPass = SynthesizedInstrument::CreateADPass(0.2, 0.2);
	

	GunShot.SynthPass = SynthesizedInstrument::CreateNoiseSynthPass(0.4f, 1, 16);
	GunShot.AttenuationPass = SynthesizedInstrument::CreateExpAttenuationPass(5);
	GunShot.MinNoteDuration = 0.7f;


	DrumSound2.SynthPass = SynthesizedInstrument::CreateSynthPass(SoundSamplers::SoundSampler<float(*)(float, float)>(DrumSample), 0.3f, 1, 0.01f);
	DrumSound2.MinNoteDuration = 0.3f;

	/*SynthesizedInstrument kalimba;
	kalimba.SynthPass = SynthesizedInstrument::CreateSineSynthPass(0.1f, 3, 1);
	kalimba.AttenuationPass = SynthesizedInstrument::CreateExpAttenuationPass(11);
	kalimba.MinNoteDuration = 0.3f;*/

	SynthesizedInstrument kalimba;
	kalimba.SynthPass = SynthesizedInstrument::CreateSineExpSynthPass({{0.48f*0.5f, 8, 0.5f, 1}});
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
	SynthesizedInstrument::SineExpHarmonic harmonics[20];
	for(size_t i=1; i<=n; i++)
	{
		auto scale = Abs( ((Mod(c*float(i*i)+37.0f*float(i), 397.0f)/200.0f)-1.0f) )*Pow(float(i), -f);
		harmonics[i-1] = {scale * 0.5f*volume,   d+e*float(i-1),   freqMult*float(i), 1.0f/(float(i)*2.0f-1.0f)};
	}
	result.SynthPass = SynthesizedInstrument::CreateSineExpSynthPass({harmonics, n});
	return result;
}

#endif

}
