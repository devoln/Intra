#pragma once

#ifndef INTRA_NO_MIDI_SYNTH

#include "Core/Core.h"
#include "Math/Random.h"
#include "Utils/Callback.h"
#include "Music.h"
#include "SoundBuilder.h"
#include "Containers/HashMap.h"
#include "Math/Fixed.h"
#include "Math/MathRanges.h"

namespace Intra {

namespace SoundSamplers
{
	struct Sawtooth
	{
		Sawtooth(float updownRatio):
			p(0), dp(0), freq(0),
			updown_value(updownRatio/(updownRatio+1)),
			c1(0), c2(0), amplitude(0) {}

		void SetParams(float newFreq, float newAmplitude, double step)
		{
			freq = newFreq;
			amplitude = newAmplitude;
			c1 = 2.0f*amplitude/updown_value;
			c2 = 2.0f*amplitude/(1.0f-updown_value);
			p = updown_value*0.5f;
			dp = (step*freq);
		}

		forceinline float NextSample() {PopFirst(); return First();}

		forceinline void PopFirst() {p+=dp;}

		float First() const
		{
			float sawPos = float(Math::Fract(p));
			return sawPos<updown_value?
				sawPos*c1-amplitude:
				amplitude-(sawPos-updown_value)*c2;
		}

		bool Empty() const {return false;}

	private:
		double p, dp;
		float freq;
		float updown_value, c1, c2, amplitude;
	};

	struct Rectangular
	{
		Rectangular(): twoFreqs(0), amplitude(0), t(0), dt(0) {}

		void SetParams(float frequency, float ampl, double step)
		{
			twoFreqs = frequency*2;
			amplitude = ampl;
			dt = float(step);
		}

		forceinline float NextSample() {PopFirst(); return First();}
		void PopFirst() {t+=dt;}
		float First() const {return (float(int(t*twoFreqs)%2)*2.0f - 1.0f)*amplitude;}
		bool Empty() const {return false;}

	private:
		float twoFreqs, amplitude;
		float t, dt;
	};

	struct ViolinPhysicalModel
	{
		ViolinPhysicalModel():
			amplitude(0), dt(0), sineRange(0,0,0),
			Len(0), Frc(0), K1(0), K2(0),
			P(), S(), bowIndex(0), soundIndex(0) {}

		void SetParams(float frequency, float ampl, double step)
		{
			float note = float(Math::Log(frequency/MusicNote::BasicFrequencies[0]*44100.0*step)*12/0.6931471805599453)-14;
			float kDemp;
			if(note<13)
			{
				Frc = 0.375f*Math::Pow(4.0f, note/12);
				Len = 230;
				kDemp = 0.005f;
			}
			else
			{
				float s = 230/Math::Pow(2.0f, (note-12)/12);
				Len = Math::Max(4u, uint(s));
				Frc = 1.5f*(float(Len)/s)*(float(Len)/s);
				kDemp = 0.12f/(note+12);
			}
			K1 = 1.0f - kDemp*0.333f*Frc;
			K2 = (1.0f-K1)*0.5f;
			bowIndex = Len*9/10-3;
			soundIndex = Len/100+1;
			
			sineRange = Math::SineRange<float>(0.9f, 0, float(2*Math::PI*7*dt)); //Частота вибратто 7 Гц

			P.SetCount(Len+1u);
			S.SetCount(Len+1u);

			amplitude = ampl*0.025f;
			dt = float(step);

			for(int i=0; i<0.15/step; i++) PopFirst();
		}

		float NextSample()
		{
			PopFirst();
			return First();
		}

		float First() const
		{
			return P[soundIndex]*amplitude;
		}

		void PopFirst()
		{
			float f = Frc;//*(1.0f+*++sineIter);

			float* sptr = S.begin()+1;
			float* send = S.begin()+Len;
			float* pptr = P.begin()+1;
			for(; sptr<send; pptr++) *sptr++ += f*(( *(pptr-1) + *(pptr+1))*0.5f - *pptr);

			sptr = S.begin()+1;
			pptr = P.begin()+1;
			while(sptr<send)
			{
				*sptr = (*sptr)*K1 + ( *(sptr-1) + *(sptr+1) )*K2;
				*pptr++ += *sptr++;
			}

			S[bowIndex] += 1.0f / ((S[bowIndex]-1)*(S[bowIndex]-1)+1); //Сила действия смычка на струну
		}

	private:
		float amplitude;
		float dt;

		Math::SineRange<float> sineRange;

		uint Len;   //Длина струны
		float Frc; //Натяжение струны
		float K1, K2; //Коэффициенты, определяющие затухание звука в струне

		//TODO: Здесь могут быть проблемы с многопоточностью
		Array<float> P; //Позиция участка струны
		Array<float> S; //Скорость участка струны

		uint bowIndex;   //Участок струны, где работает смычок
		uint soundIndex; //Участок струны, где снимается звук
	};

	struct Noise
	{
		Noise(): amplitude(0), ds(0), s(0) {}

		void SetParams(float frequency, float ampl, double step)
		{
			amplitude = ampl;
			ds = float(frequency*step);
		}

		forceinline float NextSample() {PopFirst(); return First();}
		forceinline void PopFirst() {s+=ds;}
		forceinline float First() const {return amplitude*Math::RandomNoise::Linear(s);}
		bool Empty() const {return false;}

	private:
		float amplitude;
		float ds;
		float s;
	};

	class AbsModSine
	{
		float modOmega, k, amplitude;
		float phi, dphi;
		Math::SineRange<float> sine_range;
	public:
		AbsModSine(float koeff, float modFrequency):
			modOmega(2*float(Math::PI)*modFrequency), k(koeff),
			amplitude(0), phi(0), dphi(0), sine_range(0,0,0) {}

		void SetParams(float frequency, float ampl, double step)
		{
			amplitude = ampl;
			dphi = float(2*Math::PI*frequency*step);
			sine_range = Math::SineRange<float>(k, 0, float(modOmega*step));
			phi = 0;
		}

		forceinline float NextSample()
		{
			PopFirst();
			return First();
		}

		forceinline void PopFirst()
		{
			sine_range.PopFirst();
			phi += dphi;
		}

		forceinline float First() const
		{
			return amplitude*Math::Sin(phi + sine_range.First());
		}
	};

	class RelModSine
	{
		float modK, k, ampl;
		float phi, dphi;
		Math::SineRange<float> sine_range;

	public:
		RelModSine(float koeff, float modKoeff):
			modK(modKoeff), k(koeff),
			ampl(0), phi(0), dphi(0), sine_range(0,0,0) {}

		void SetParams(float frequency, float amplitude, double step)
		{
			ampl = amplitude;
			const double radFreq = 2*Math::PI*frequency;
			dphi = float(radFreq*step);
			phi = 0;
			sine_range = Math::SineRange<float>(k, 0, float(radFreq*modK*step));
		}

		forceinline float NextSample()
		{
			PopFirst();
			return First();
		}

		forceinline float First() const
		{
			return ampl*Math::Sin(phi + sine_range.First());
		}

		forceinline void PopFirst()
		{
			sine_range.PopFirst();
			phi += dphi;
		}
	};

	class AbsModTimeSine
	{
		float modOmega, k, t0, t1, dt, amplitude;
		float phi, dphi;
		Math::SineRange<float> sine_range;
	public:
		AbsModTimeSine() = default;

		AbsModTimeSine(float modFrequency, float tstart, float koeff):
			modOmega(float(2*Math::PI*modFrequency)), k(koeff), t0(tstart),
			t1(t0), dt(0), amplitude(0), phi(0), dphi(0), sine_range(0,0,0) {}
		
		void SetParams(float frequency, float ampl, double step)
		{
			amplitude = ampl;
			dphi = float(2*Math::PI*frequency*step);
			sine_range = Math::SineRange<float>(k, 0, float(modOmega*step));
			phi = 0;
			t1 = t0;
			dt = float(step);
		}

		forceinline float NextSample()
		{
			PopFirst();
			return First();
		}

		forceinline void PopFirst()
		{
			sine_range.PopFirst();
			phi += dphi;
			t1 -= dt;
		}

		forceinline float First() const
		{
			return amplitude*Math::Sin(phi + t1 * sine_range.First());
		}
	};

	class RelModTimeSine
	{
		float modK, k, t0, t1, dt, ampl;
		float phi, dphi;
		Math::SineRange<float> sine_range;
	public:
		RelModTimeSine(float modKoeff, float tstart, float koeff):
			modK(modKoeff), k(koeff), t0(tstart),
			t1(0), dt(0), ampl(0), phi(0), dphi(0), sine_range() {}

		void SetParams(float frequency, float amplitude, double step)
		{
			ampl = amplitude;
			const double omega = 2*Math::PI*frequency;
			dphi = float(omega*step);
			phi = 0;
			t1 = t0;
			dt = float(step);
			sine_range = Math::SineRange<float>(k, 0, float(omega*modK*step));
		}

		forceinline float NextSample() {PopFirst(); return First();}
		forceinline float First() const {return ampl*Math::Sin(phi + t1 * sine_range.First());}
		forceinline void PopFirst() {phi += dphi; t1 -= dt;}
	};



	template<typename T> class SoundSampler
	{
		float freq, ampl;
		float time, dt;
		T generator;
	public:
		SoundSampler(const SoundSampler&) = default;
		SoundSampler(T myGenerator):
			freq(0), ampl(0), time(0), dt(0), generator(myGenerator) {}

		void SetParams(float frequency, float amplitude, double step)
		{
			freq = frequency;
			ampl = amplitude;
			dt = float(step);
		}

		forceinline float NextSample()
		{
			const float result = operator()(time);
			time += dt;
			return result;
		}

		forceinline float operator()(float t) {return generator(freq, t)*ampl;}
	};

	template<typename T> SoundSampler<T> CreateSampler(T generator) {return SoundSampler<T>(generator);}
}

namespace SoundModifiers
{
	struct AbsPulsator
	{
		AbsPulsator() = default;
		AbsPulsator(float frequency, float baseAmplitude=0, float sinAmplitude=1): oscillator(),
			freq(frequency), base_amplitude(baseAmplitude), sin_amplitude(sinAmplitude) {}

		void SetParams(float, double step)
		{
			oscillator = Math::SineRange<float>(sin_amplitude, 0, float(2*Math::PI*freq*step));
		}

		forceinline float NextSample(float sample)
		{
			const float result = sample*(base_amplitude+sin_amplitude * oscillator.First());
			oscillator.PopFirst();
			return result;
		}

	private:
		Math::SineRange<float> oscillator;
		float freq;
		float base_amplitude, sin_amplitude;
	};

	struct AddPulsator
	{
		AddPulsator(float frequency, float basicVolume, float ampl): oscillator(),
			freq(frequency), basic_volume(basicVolume), amplitude(ampl) {}

		void SetParams(float, double step)
		{
			oscillator = Math::SineRange<float>(amplitude, 0, float(2*Math::PI*freq*step));
		}

		forceinline float NextSample(float sample)
		{
			const float result = sample*(basic_volume + oscillator.First());
			oscillator.PopFirst();
			return result;
		}

	private:
		Math::SineRange<float> oscillator;
		float freq, basic_volume;
		float amplitude;
	};

	struct RelPulsator
	{
		RelPulsator(float koeff): k(koeff), oscillator() {}

		void SetParams(float baseFrequency, double step)
		{
			oscillator = Math::SineRange<float>(1, 0, float(2*Math::PI*baseFrequency*k*step));
		}

		forceinline float NextSample(float sample)
		{
			const float result = sample * oscillator.First();
			oscillator.PopFirst();
			return result;
		}

	private:
		float k;
		Math::SineRange<float> oscillator;
	};
}

namespace SoundPostEffects
{
	struct Chorus
	{
		float MaxDelay;
		float Frequency;
		float MainVolume, SecondaryVolume;

		Chorus(float maxDelay=0.03f, float frequency=3, float mainVolume=0.5f, float secondaryVolume=0.5f):
			MaxDelay(maxDelay), Frequency(frequency), MainVolume(mainVolume), SecondaryVolume(secondaryVolume) {}

		void operator()(ArrayRange<float> inOutSamples, uint sampleRate) const;
	};

	struct Echo
	{
		float Delay;
		float MainVolume, SecondaryVolume;

		Echo(float delay=0.03f, float mainVolume=0.5f, float secondaryVolume=0.5f):
			Delay(delay), MainVolume(mainVolume), SecondaryVolume(secondaryVolume) {}

		void operator()(ArrayRange<float> inOutSamples, uint sampleRate) const;
	};

	struct FilterDrive
	{
		float K;
		FilterDrive(float k): K(k) {}

		void operator()(ArrayRange<float> inOutSamples, uint sampleRate) const;
	};

	struct FilterHP
	{
		float K;
		FilterHP(float k): K(k) {}

		void operator()(ArrayRange<float> inOutSamples, uint sampleRate) const;
	};


	struct FilterQ
	{
		float Frq, K;
		FilterQ(float frq, float k): Frq(frq), K(k) {}

		void operator()(ArrayRange<float> samples, uint sampleRate) const;
	};

	struct Fade
	{
		uint FadeIn, FadeOut;

		Fade(uint fadeIn=0, uint fadeOut=0):
			FadeIn(fadeIn), FadeOut(fadeOut) {}

		void operator()(ArrayRange<float> inOutSamples, uint sampleRate) const;
	};
}

typedef Utils::Delegate<void(float freq, float volume, ArrayRange<float> outSamples, uint sampleRate, bool add)> SoundSynthFunction;
typedef Utils::Delegate<void(float freq, ArrayRange<float> inOutSamples, uint sampleRate)> SoundModifierFunction;
typedef Utils::Delegate<void(float noteDuration, ArrayRange<float> inOutSamples, uint sampleRate)> SoundAttenuationFunction;
typedef Utils::Delegate<void(ArrayRange<float> inOutSamples, uint sampleRate)> SoundPostEffectFunction;


class SynthesizedInstrument: public IMusicalInstrument
{
public:
	struct SineHarmonic
	{
		FixedPoint<byte, 512> scale;
		FixedPoint<byte, 16> freqMultiplyer;
	};

	struct SineExpHarmonic
	{
		FixedPoint<byte, 512> scale;
		FixedPoint<byte, 8> attenCoeff;
		FixedPoint<byte, 16> freqMultiplyer;
		norm8s lengthMultiplyer;
	};

	template<typename T> struct impl_SamplerPassParams
	{
		impl_SamplerPassParams(const T& sampler_, ushort harmonics_, float scale_, float freqMultiplyer_):
			sampler(sampler_), harmonics(harmonics_), scale(scale_), freqMultiplyer(freqMultiplyer_) {}

		impl_SamplerPassParams(T&& sampler_, ushort harmonics_, float scale_, float freqMultiplyer_):
			sampler(Meta::Move(sampler_)), harmonics(harmonics_), scale(scale_), freqMultiplyer(freqMultiplyer_) {}

		T sampler;
		ushort harmonics;
		float scale;
		float freqMultiplyer;
	};
private:
	struct ADParams {double attackTime, decayTime;};
	static void functionADPass(const ADParams& params, float noteDuration, ArrayRange<float> inOutSamples, uint sampleRate);
	static void functionExpAttenuationPass(const float& coeff, float noteDuration, ArrayRange<float> inOutSamples, uint sampleRate);
	struct TableAttenuatorParams {byte len; norm8 table[23];};
	static void functionTableAttenuationPass(const TableAttenuatorParams& table, float noteDuration, ArrayRange<float> inOutSamples, uint sampleRate);
	
	struct SawtoothParams {float updownRatio; ushort harmonics; float scale; float freqMultiplyer;};
	static void functionSawtoothSynthPass(const SawtoothParams& params,
		float freq, float volume, ArrayRange<float> inOutSamples, uint sampleRate, bool add);

	struct SineParams {float scale; ushort harmonics; float freqMultiplyer;};
	static void functionSineSynthPass(const SineParams& params,
		float freq, float volume, ArrayRange<float> inOutSamples, uint sampleRate, bool add);

	struct MultiSineParams {byte len; SineHarmonic harmonics[20];};
	static void functionMultiSineSynthPass(const MultiSineParams& params,
		float freq, float volume, ArrayRange<float> inOutSamples, uint sampleRate, bool add);

	struct SineExpParams
	{
		byte len;
		SineExpHarmonic harmonics[20]; //Требуется, чтобы harmonics[0] имела наибольший lengthMultiplyer
	};
	static void functionSineExpSynthPass(const SineExpParams& params,
		float freq, float volume, ArrayRange<float> inOutSamples, uint sampleRate, bool add);

	
	template<typename T> static void functionSynthPass(const impl_SamplerPassParams<T>& params,
		float freq, float volume, ArrayRange<float> outSamples, uint sampleRate, bool add)
	{
		const float dt = 1.0f/float(sampleRate);

		//Считаем громкость основной гармоники так, чтобы суммарная громкость по всем гармоникам не превышала scale
		float maxValue=1, harmVal=1;
		for(ushort h=1; h<params.harmonics; h++) maxValue+=(harmVal/=2);
		float newVolume = volume*params.scale/maxValue;

		float frequency = freq*params.freqMultiplyer;

		Math::Random<float> frandom(988959283);
		for(ushort h=0; h<params.harmonics; h++)
		{
			auto samplerCopy = params.sampler;
			samplerCopy.SetParams(frequency, newVolume, dt);
			if(h==0 && !add) for(auto& sample: outSamples)
				sample = samplerCopy.NextSample();
			else for(auto& sample: outSamples)
				sample += samplerCopy.NextSample();
			newVolume /= 2;
			frequency *= 2+frandom(0.0005f);
		}
	}

	struct NoisePassParams {ushort harmonics; float scale; float freqMultiplyer;};
	static void functionNoiseSynthPass(const NoisePassParams& params,
		float freq, float volume, ArrayRange<float> outSamples, uint sampleRate, bool add)
	{
		impl_SamplerPassParams<SoundSamplers::Noise> p(SoundSamplers::Noise(), params.harmonics, params.scale, params.freqMultiplyer);
		functionSynthPass(p, freq, volume, outSamples, sampleRate, add);
	}

	template<typename T> static void functionModifierPass(const T& modifier,
		float freq, ArrayRange<float> inOutSamples, uint sampleRate)
	{
		auto modifierCopy = modifier;
		float t = 0.0f, dt = 1.0f/float(sampleRate);
		modifierCopy.SetParams(freq, dt);
		for(size_t i=0; i<inOutSamples.Length(); i++)
		{
			inOutSamples[i] = modifierCopy.NextSample(inOutSamples[i]);
			t+=dt;
		}
	}

	//r = rez amount, from Sqrt(2) to ~0.1
	//f = cutoff frequency
	//(from ~0 Hz to SampleRate/2 - though many synths seem to filter only up to SampleRate/4)
	struct HLPassParams {float rezAmount, cutoffFreq; bool highPass;};
	static void functionHLPass(const HLPassParams& params,
		float /*freq*/, ArrayRange<float> inOutSamples, uint sampleRate)
	{
		double t = 0.0, dt = 1.0/sampleRate;
		Array<float> in(inOutSamples);
		auto out = inOutSamples;

		float c = Math::Tan(float(Math::PI)*params.cutoffFreq/float(sampleRate));
		float a1, a2, a3, b1, b2;
		if(params.highPass)
		{
			a1 = 1.0f/(1 + params.rezAmount*c + c*c);
			a2 = -2*a1;
			a3 = a1;
			b1 = 2*(c*c-1)*a1;
			b2 = (1.0f - params.rezAmount*c + c*c)*a1;
		}
		else
		{
			c = 1/c;
			a1 = 1.0f/(1 + params.rezAmount*c + c*c);
			a2 = 2*a1;
			a3 = a1;
			b1 = 2*(1-c*c)*a1;
			b2 = (1.0f - params.rezAmount*c + c*c)*a1;
		}
		

		for(size_t i=2; i<inOutSamples.Length(); i++)
		{
			out[i] *= a1;
			out[i] += a2*in[i-1] + a3*in[i-2];
			out[i] -= b1*out[i-1] + b2*out[i-2];
			t+=dt;
		}
	}
public:
	SynthesizedInstrument():
		SynthPass(), ModifierPasses(), AttenuationPass(), PostEffects(),
		MinNoteDuration(0), FadeOffTime(0) {}

	SynthesizedInstrument(SoundSynthFunction synth,
		ArrayRange<const SoundModifierFunction> modifiers=null,
		SoundAttenuationFunction attenuator=null,
		ArrayRange<const SoundPostEffectFunction> postEffects=null,
		float minNoteDuration=0, float fadeOffTime=0);

	SynthesizedInstrument(SynthesizedInstrument&& rhs):
		SynthPass(Meta::Move(rhs.SynthPass)),
		ModifierPasses(Meta::Move(rhs.ModifierPasses)),
		AttenuationPass(Meta::Move(rhs.AttenuationPass)),
		PostEffects(Meta::Move(rhs.PostEffects)),
		MinNoteDuration(rhs.MinNoteDuration),
		FadeOffTime(rhs.FadeOffTime) {}

	SynthesizedInstrument(const SynthesizedInstrument& rhs):
		SynthPass(rhs.SynthPass),
		ModifierPasses(rhs.ModifierPasses),
		AttenuationPass(rhs.AttenuationPass),
		PostEffects(rhs.PostEffects),
		MinNoteDuration(rhs.MinNoteDuration),
		FadeOffTime(rhs.FadeOffTime) {}

	SynthesizedInstrument& operator=(const SynthesizedInstrument& rhs);
	SynthesizedInstrument& operator=(SynthesizedInstrument&& rhs);

	void GetNoteSamples(ArrayRange<float> dst, MusicNote note, float tempo, float volume=1, uint sampleRate=44100, bool add=false) const override;

	uint GetNoteSampleCount(MusicNote note, float tempo, uint sampleRate=44100) const override
	{
		auto noteDuration = note.AbsDuration(tempo);
		if(noteDuration<0.00001 || note.IsPause()) return 0;
		noteDuration = Math::Max(noteDuration, MinNoteDuration)+FadeOffTime;
		return uint(noteDuration*float(sampleRate));
	}

	static SoundSynthFunction CreateSawtoothSynthPass(float updownRatio=1, float scale=1, ushort harmonics=1, float freqMultiplyer=1);
	static SoundSynthFunction CreateSineSynthPass(float scale=1, ushort harmonics=1, float freqMultiplyer=1);
	
	static SoundSynthFunction CreateMultiSineSynthPass(ArrayRange<const SineHarmonic> harmonics);
	static SoundSynthFunction CreateSineExpSynthPass(ArrayRange<const SineExpHarmonic> harmonics);

	template<typename T> static SoundSynthFunction CreateSynthPass(T sampler, float scale=1, ushort harmonics=1, float freqMultiplyer=1)
	{
		impl_SamplerPassParams<T> params = {sampler, harmonics, scale, freqMultiplyer};
		return SoundSynthFunction(functionSynthPass<T>, params);
	}

	static SoundSynthFunction CreateNoiseSynthPass(float scale=1, ushort harmonics=1, float freqMultiplyer=1)
	{
		return SoundSynthFunction(functionNoiseSynthPass, NoisePassParams{harmonics, scale, freqMultiplyer});
	}

	template<typename T> static SoundModifierFunction CreateModifierPass(T modifier)
	{
		return SoundModifierFunction(functionModifierPass, modifier);
	}

	template<typename T> static SoundAttenuationFunction CreateAttenuationPass(T attenuator)
	{
		return [attenuator](float noteDuration, ArrayRange<float> inOutSamples, uint sampleRate)
		{
			auto attenuatorCopy=attenuator;
			attenuatorCopy.SetAttenuationStep(1.0f/sampleRate);
			for(size_t i=0; i<inOutSamples.Length(); i++)
			{
				inOutSamples[i]*=attenuatorCopy.NextSample();
			}
			(void)noteDuration;
		};
	}

	static SoundAttenuationFunction CreateADPass(double attackTime, double decayTime);
	static SoundAttenuationFunction CreateExpAttenuationPass(float coeff);
	static SoundAttenuationFunction CreateTableAttenuationPass(ArrayRange<const norm8> table);

	SoundSynthFunction SynthPass;
	Array<SoundModifierFunction> ModifierPasses;
	SoundAttenuationFunction AttenuationPass;
	Array<SoundPostEffectFunction> PostEffects;
	float MinNoteDuration, FadeOffTime;
};
//DEFINE_AS_POD1(SynthesizedInstrument::impl_SamplerPassParams<T1>);

class CombinedSynthesizedInstrument: public IMusicalInstrument
{
public:
	Array<SynthesizedInstrument> Combination;
	SoundAttenuationFunction AttenuationPass; //Not implemented yet
	SoundPostEffectFunction PostEffectPass; //Not implemented yet

	CombinedSynthesizedInstrument(ArrayRange<const SynthesizedInstrument> instruments=null):
		Combination(instruments), AttenuationPass(), PostEffectPass() {}

	void GetNoteSamples(ArrayRange<float> dst, MusicNote note, float tempo,
		float volume=1, uint sampleRate=44100, bool add=false) const override;

	uint GetNoteSampleCount(MusicNote note, float tempo, uint sampleRate=44100) const override
	{
		uint noteSampleCount=0;
		for(auto& instr: Combination)
			noteSampleCount = Math::Max(noteSampleCount, instr.GetNoteSampleCount(note, tempo, sampleRate));
		return noteSampleCount;
	}
};



class DrumInstrument: public IMusicalInstrument
{
public:
	DrumInstrument():
		SamplesCache(), Generators() {}

	DrumInstrument(DrumInstrument&& rhs):
		SamplesCache(Meta::Move(rhs.SamplesCache)), Generators(Meta::Move(rhs.Generators)) {}

	DrumInstrument(const DrumInstrument& rhs) = default;
	DrumInstrument& operator=(const DrumInstrument& rhs) = default;

	DrumInstrument& operator=(DrumInstrument&& rhs)
	{
		SamplesCache = Meta::Move(rhs.SamplesCache);
		Generators = Meta::Move(rhs.Generators);
		return *this;
	}

	void GetNoteSamples(ArrayRange<float> dst, MusicNote note, float tempo,
		float volume=1, uint sampleRate=44100, bool add=false) const override;

	uint GetNoteSampleCount(MusicNote note, float tempo, uint sampleRate=44100) const override
	{
		if(note.IsPause()) return 0;
		uint id = note.Octave*12u+uint(note.Note);
		auto gen = Generators.Get(id);
		if(gen==null) return 0;
		auto result = SamplesCache.Get(gen).Samples.Count();
		if(result!=0) return uint(result);
		return gen->GetNoteSampleCount(MusicNote(4, MusicNote::NoteType::C, ushort(note.Duration*tempo)), 1, sampleRate);
	}

	void PrepareToPlay(const MusicTrack& track, uint sampleRate) const override;

	mutable HashMap<IMusicalInstrument*, SoundBuffer> SamplesCache;
	HashMap<uint, IMusicalInstrument*> Generators;

private:
	//Убедиться, что в кеше присутствуют семплы для указанной ноты длины не менее указанной
	SoundBuffer& cache_note(MusicNote note, float tempo, uint sampleRate) const;
};


#endif

}
