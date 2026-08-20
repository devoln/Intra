INTRA_DISABLE_REDUNDANT_WARNINGS

#include "Random/FastUniformNoise.h"
#include "Random/FastUniform.h"

#include "Container/Sequential/Array.h"
#include "Container/Utility/Array2D.h"

#include "InstrumentLibrary.h"
#include "DrumPhysicalModel.h"
#include "SnarePhysicalModel.h"

#include "WaveTableSampler.h"
#include "MusicalInstrument.h"
#include "RecordedSampler.h"
#include "KarplusStrongSampler.h"
#include "GaussianStringSampler.h"
#include "SpectralStringSampler.h"
#include "AdditiveSampler.h"
#include "Filter.h"
#include "WebSynth.h"

#include "Generators.hh"
#include "PostEffects.hh"

#include "WaveTableGeneration.h"

#ifdef INTRA_PROBE_NAN
#include <stdio.h>
#endif

using namespace Intra;

namespace
{
	// Формулы web-midisynth: один enum вместо десятков отдельных лямбд-функций.
	// Параметры: x = 1..numHarmonics, r = случайное число [0;1).
	enum WebFunc : uint8
	{
		F_1overx, F_1overx2, F_1oversqrtx, F_xm1_5, F_xm2_4, F_x, F_1,
		F_0_001, F_0_1, F_1200,
		// bandwidth
		BW_70p70x, BW_60p20x, BW_40p40x, BW_1px, BW_4p4x, BW_35p30x,
		BW_50p15x, BW_7_1p0_8x, BW_2p5x, BW_1_2x, BW_52x_20, BW_24x_14,
		BW_6p14x, BW_1_7x_0_7, BW_1_7x, BW_3p5x, BW_0_5x, BW_6x_1, BW_3px,
		BW_0_2x_0_1, BW_0_4x, BW_3_4p1_6x, BW_12_2xp2_8, BW_15p10x, BW_10_1pPow,
		// amplitude
		A_0_8_0_8r_div_x2, A_0_8_0_1rSqrt_div_x2, A_0_8_1_0_5r_div_x,
		A_0_2_1_0_7r_div_x, A_cos_x26_div_x2, A_pow_m0_4, A_mod2_pow_m1_3,
		A_mod2_div_x, A_cos_x21_div_x2, A_cos_x16_div_x, A_0_8_0_1rSqrt_pow,
		A_0_025_0_05r_div_Sqrt, A_pow_m2_5, A_0_01_0_02r, A_0_02_0_01r,
		A_cos_x2_25_pow, A_cos_x6_pow1_8, A_0_8_div_x2, A_cos_x9_div_x2,
		A_cos_x1_6_div_x2, A_cos_x6_pow1_5, A_0_06_0_3r_pow, A_0_08_0_4r_div_x2,
		A_cos_x16_div_x2, A_sax, A_abs_mod_div_x,
		// frequency multiplier
		FM_2x_1, FM_1_007x, FM_0_9955x, FM_pow2_x_2
	};

	noinline float ApplyWebFunc(WebFunc f, float x, float r)
	{
		switch(f)
		{
		case F_1overx: return 1.0f/x;
		case F_1overx2: return 1.0f/(x*x);
		case F_1oversqrtx: return 1.0f/Math::Sqrt(x);
		case F_xm1_5: return Math::Pow(x, -1.5f);
		case F_xm2_4: return Math::Pow(x, -2.4f);
		case F_x: return x;
		case F_1: return 1.0f;
		case F_0_001: return 0.001f;
		case F_0_1: return 0.1f;
		case F_1200: return 1200.0f;
		case BW_70p70x: return 70+70*x;
		case BW_60p20x: return 60+20*x;
		case BW_40p40x: return 40+40*x;
		case BW_1px: return 1+x;
		case BW_4p4x: return 4+4*x;
		case BW_35p30x: return 35+30*x;
		case BW_50p15x: return 50+15*x;
		case BW_7_1p0_8x: return 7*(1+0.8f*x);
		case BW_2p5x: return 2+5*x;
		case BW_1_2x: return 1.2f*x;
		case BW_52x_20: return 52*x - 20;
		case BW_24x_14: return 24*x - 14;
		case BW_6p14x: return 6+14*x;
		case BW_1_7x_0_7: return 1.7f*x - 0.7f;
		case BW_1_7x: return 1.7f*x;
		case BW_3p5x: return 3+5*x;
		case BW_0_5x: return 0.5f*x;
		case BW_6x_1: return 6*x - 1;
		case BW_3px: return 3+x;
		case BW_0_2x_0_1: return 0.2f*x - 0.1f;
		case BW_0_4x: return 0.4f*x;
		case BW_3_4p1_6x: return 3.4f + 1.6f*x;
		case BW_12_2xp2_8: return 12.2f*x + 2.8f;
		case BW_15p10x: return 15+10*x;
		case BW_10_1pPow: return 10.0f*(1.0f + Math::Pow(x, 1.25f));
		case A_0_8_0_8r_div_x2: return (0.8f - 0.8f*r*x/100.0f)/(x*x);
		case A_0_8_0_1rSqrt_div_x2: return (0.8f - 0.1f*r*Math::Sqrt(x))/(x*x);
		case A_0_8_1_0_5r_div_x: return 0.8f*(1.0f - r*0.5f)/x;
		case A_0_2_1_0_7r_div_x: return 0.2f*(1.0f - r*0.7f)/x;
		case A_cos_x26_div_x2: return Math::Cos(float(Math::PI)*(x/26.0f - 0.5f))/(x*x);
		case A_pow_m0_4: return Math::Pow(x, -0.4f);
		case A_mod2_pow_m1_3: return (Math::Mod(x, 2.0f)*2.0f - 1.0f)*Math::Pow(x, -1.3f);
		case A_mod2_div_x: return (Math::Mod(x, 2.0f)*2.0f - 1.0f)/x;
		case A_cos_x21_div_x2: return Math::Cos(float(Math::PI)*(x/21.0f - 0.5f))/(x*x);
		case A_cos_x16_div_x: return Math::Cos(float(Math::PI)*(x/16.0f - 0.5f))/x;
		case A_0_8_0_1rSqrt_pow: return (0.8f - 0.1f*r*Math::Sqrt(x))*Math::Pow(x, -1.5f);
		case A_0_025_0_05r_div_Sqrt: return (0.025f + 0.05f*r)/Math::Sqrt(x);
		case A_pow_m2_5: return Math::Pow(x, -2.5f);
		case A_0_01_0_02r: return 0.01f + 0.02f*r;
		case A_0_02_0_01r: return 0.02f + 0.01f*r;
		case A_cos_x2_25_pow: return Math::Cos(float(Math::PI)*(x/2.25f - 0.5f))/Math::Pow(x, 2.3f);
		case A_cos_x6_pow1_8: return Math::Cos(float(Math::PI)*(x/6.0f - 0.5f))/Math::Pow(x, 1.8f);
		case A_0_8_div_x2: return 0.8f/(x*x);
		case A_cos_x9_div_x2: return Math::Cos(float(Math::PI)*(x/9.0f - 0.5f))/(x*x);
		case A_cos_x1_6_div_x2: return Math::Cos(float(Math::PI)*(x/1.6f - 0.5f))/(x*x);
		case A_cos_x6_pow1_5: return Math::Cos(float(Math::PI)*(x/6.0f - 0.5f))/Math::Pow(x, 1.5f);
		case A_0_06_0_3r_pow: return (0.06f + 0.3f*r)*Math::Pow(x, -1.5f);
		case A_0_08_0_4r_div_x2: return (0.08f + 0.4f*r)/(x*x);
		case A_cos_x16_div_x2: return Math::Cos(float(Math::PI)*(x/16.0f - 0.5f))/(x*x);
		case A_sax:
			return Math::Cos(float(Math::PI)*(x/9.0f - 0.5f))*Math::Pow(x, -2.4f)
				+ 0.5f*Math::Mod(x + 1.0f, 2.0f)*Math::Cos(float(Math::PI)*(x/18.0f - 0.5f))*Math::Pow(x/2.0f, -2.5f);
		case A_abs_mod_div_x: return Math::Abs(Math::Mod(43*x*x + 37*x, 397.0f)/200.0f - 1.0f)/x;
		case FM_2x_1: return 2*x - 1;
		case FM_1_007x: return 1.007f*x;
		case FM_0_9955x: return 0.9955f*x;
		case FM_pow2_x_2: return Math::Pow(2.0f, x - 2.0f);
		}
		return 0.0f;
	}

	// Гармонический ряд: x = 1..numHarmonics, r = случайное число [0;1).
	noinline WebHarmonicSet Series(size_t num, WebFunc bw, WebFunc amp, WebFunc fm)
	{
		WebHarmonicSet set;
		Random::FastUniform<float> rnd(0x9e3779b9u);
		for(size_t i = 1; i <= num; i++)
		{
			const float x = float(i), r = rnd();
			set.Harmonics.AddLast(WebHarmonicDesc{ApplyWebFunc(amp, x, r), ApplyWebFunc(fm, x, r), ApplyWebFunc(bw, x, r)});
		}
		return set;
	}

	noinline WebHarmonicSet Res(WebHarmonicSet s, Span<const WebResonanceDesc> r, bool mult)
	{
		s.IsResonanceMultiplicative = mult;
		for(const auto& e: r) s.Resonances.AddLast(e);
		return s;
	}

	noinline WebHarmonicSet Harms(Span<const WebHarmonicDesc> h)
	{
		WebHarmonicSet s;
		for(const auto& e: h) s.Harmonics.AddLast(e);
		return s;
	}

	noinline Array<WebHarmonicSet> Sets(WebHarmonicSet a)
	{
		Array<WebHarmonicSet> r;
		r.AddLast(Move(a));
		return r;
	}

	noinline Array<WebHarmonicSet> Sets(WebHarmonicSet a, WebHarmonicSet b)
	{
		Array<WebHarmonicSet> r;
		r.AddLast(Move(a));
		r.AddLast(Move(b));
		return r;
	}

	noinline Array<WebHarmonicSet> Sets(WebHarmonicSet a, WebHarmonicSet b, WebHarmonicSet c)
	{
		Array<WebHarmonicSet> r;
		r.AddLast(Move(a));
		r.AddLast(Move(b));
		r.AddLast(Move(c));
		return r;
	}

	noinline WaveTableInstrument Wt(WaveTableCache* tables, float volume, float exp, const WebEnvelope& env,
		float vibFreq = 0, float vibVal = 0)
	{
		WaveTableInstrument wt;
		wt.Tables = tables;
		wt.VolumeScale = volume;
		wt.ExpCoeff = exp;
		wt.Envelope = MakeWebEnvelope(env);
		wt.VibratoFrequency = vibFreq;
		wt.VibratoValue = vibVal;
		return wt;
	}

	// Описание одного гармонического ряда вейвтаблицы.
	// Num хранится как uint8 (макс. 128) вместо uint16. Заметного выигрыша в
	// итоговом wasm это не даёт (~94 байта): LTO и так сворачивает static const
	// таблицу в константы конструктора и устраняет padding.
	struct WtSeriesSpec
	{
		uint8 Num;
		WebFunc Bw, Amp, Fm;
		const WebResonanceDesc* Resonances;
		uint8 NumResonances;
		bool Multiplicative;
	};

	// Описание вейвтабличного инструмента. Reuse != null => таблица берётся
	// из Tables[Reuse], иначе строится заново из рядов Series[].
	struct WtSpec
	{
		const char* Name;
		const char* Reuse;
		size_t TableSize;
		WtSeriesSpec Series[3];
		float Volume, ExpCoeff, VibFreq, VibVal;
		WebEnvelope Env;
	};
}

InstrumentLibrary::~InstrumentLibrary() {}

InstrumentLibrary::InstrumentLibrary()
{
	// === Струнные (Karplus-Strong) ===
	// Эксперимент с GaussianStringSampler (тройное скользящее среднее по
	// иммутабельному буферу) на гитарах закончился неудачей: спектрально он не
	// сохраняет фундаментал (см. scripts/_tmp-ma-ks-proto.cpp), на wasm ~1.6-1.8x
	// медленнее KS и звучит глухо. Гитары возвращены на KS; сам семплер остался
	// в коде (GaussianStringSampler.h/.cpp) как технология для инструментов без
	// живущего фундаментала. Подключение — одна строка ниже: заменить
	// KarplusStrongInstrument на GaussianStringInstrument.
	{
		auto& g = Instruments["AcousticGuitarNylon"];
		g.GenericInstruments.EmplaceLast(KarplusStrongInstrument{0.15f, 0.15f, 0.8f, 0.5f, 0.55f, 1.5f});
		g.Envelope = MakeWebEnvelope({0, 0, 1, 0.7f, 1.5f, false, true});
	}
	{
		auto& g = Instruments["AcousticGuitarSteel"];
		g.GenericInstruments.EmplaceLast(KarplusStrongInstrument{0.05f, 0.25f, 0.8f, 0.5f, 0.4f, 1.0f});
		g.Envelope = MakeWebEnvelope({0, 0, 1, 0.7f, 1, false, true});
	}
	// === Фортепиано: аддитивная модель (SIMD SineRange) ===
	// Каждая партиала — независимый осциллятор со своей частотой (измеренный
	// ratio f_k = f0·k·ratio_k из региона — растяжка/негармоничность) и своим
	// затуханием.
	// Амплитуды/фазы/частоты/затухания партиал — из таблицы PianoRegions.h:
	// реальные семплы Clavinova Grand (анализ SF2). Поверх — атака-рамп (по
	// AttackT региона), унисон-расстройка (только у программ, которым она
	// нужна — у Acoustic лёгкий двухструнный унисон, основные биения уже записаны в семпле) и
	// velocity→яркость. DecayScale=1 значит «затухание ровно как в семпле».
	// Параметры: Brightness, MaxPartials, Scale, DecayScale, DecayStiffness,
	// DetuneCents, AttackBoost, UnisonVoices, VelBrightness, HammerLevel,
	// TrebleTilt.
	{
		auto& g = Instruments["AcousticPiano"];
		g.GenericInstruments.EmplaceLast([](){
			// Эталонная модель — один измеренный голос: амплитуды, фазы и
			// затухание уже извлечены из полного SF2-семпла. Случайные голоса
			// унисона не добавляем: их биения не подтверждены анализом семпла
			// (стабильный участок — одна струна). Ударная атака задаётся
			// измеренной кривой PianoEnvelopeCorrections (подъём из тишины +
			// strike-выброс, см. PianoEnvelope.h); отдельный тональный
			// «молоточек» (f0/2+f0) не подтверждён анализом семпла и убран —
			// он добавлял звонкий тон вместо глухого удара.
			return AdditivePianoInstrument{0.25f, 40, 0.9f, 1.0f, 0.0f, 0.0f, 0.0f, 1, 0.35f, 0.0f, 0.0f};
		}());
		g.Envelope = MakeWebEnvelope({0, 0, 1, 0.6f, 0, false, true});
	}
	{
		auto& g = Instruments["BrightAcousticPiano"];
		g.GenericInstruments.EmplaceLast([](){
			// Ярче (k^0.12), 3 струны с расстройкой, сильнее молоточек.
			return AdditivePianoInstrument{0.4f, 40, 0.84f, 1.0f, 0.0f, 0.7f, 0.0f, 3, 0.4f, 0.45f, 0.0f};
		}());
		g.Envelope = MakeWebEnvelope({0, 0, 1, 0.5f, 0, false, true});
	}
	{
		auto& g = Instruments["ElectricGrandPiano"];
		g.GenericInstruments.EmplaceLast([](){
			// CP-80: ярче, длинный сустейн, хорус из расстройки, удар мягче.
			return AdditivePianoInstrument{0.45f, 40, 0.42f, 0.8f, 0.0f, 2.5f, 0.0f, 2, 0.1f, 0.1f, 0.0f};
		}());
		g.Envelope = MakeWebEnvelope({0, 0, 1, 0.9f, 0, false, true});
	}
	{
		auto& g = Instruments["HonkyTonkPiano"];
		g.GenericInstruments.EmplaceLast([](){
			// Honky-tonk: широкая расстройка → сильные биения (сам характер), 3
			// струны, затухание по семплу.
			return AdditivePianoInstrument{0.3f, 40, 0.9f, 1.0f, 0.0f, 9.0f, 0.0f, 3, 0.4f, 0.1f, 0.0f};
		}());
		g.Envelope = MakeWebEnvelope({0, 0, 1, 0.5f, 0, false, true});
	}
	{
		auto& g = Instruments["ElectricPiano1"];
		g.GenericInstruments.EmplaceLast([](){
			// Родс: колокольный верх, длинный сустейн, две струны с малой
			// расстройкой, без ударной атаки.
			return AdditivePianoInstrument{0.55f, 40, 0.42f, 0.7f, 0.0f, 0.8f, 0.0f, 2, 0.05f, 0.05f, 0.0f};
		}());
		g.Envelope = MakeWebEnvelope({0, 0, 1, 1.0f, 0, false, true});
	}
	{
		auto& g = Instruments["ElectricPiano2"];
		g.GenericInstruments.EmplaceLast([](){
			// DX7-подобный: статичный FM-тембр, одна струна.
			return AdditivePianoInstrument{0.6f, 40, 0.42f, 0.6f, 0.0f, 0.4f, 0.0f, 1, 0.05f, 0.05f, 0.0f};
		}());
		g.Envelope = MakeWebEnvelope({0, 0, 1, 1.2f, 0, false, true});
	}
	{
		auto& g = Instruments["Harpsichord"];
		g.GenericInstruments.EmplaceLast([](){
			// Клавесин: яркий, короткий (щипок), без velocity-динамики, одна
			// струна, резкая атака.
			return AdditivePianoInstrument{0.5f, 40, 0.4f, 1.6f, 0.012f, 0.0f, 0.0f, 1, 0.0f, 0.15f, 0.0f};
		}());
		g.Envelope = MakeWebEnvelope({0, 0, 1, 0.25f, 0, false, true});
	}
	{
		auto& g = Instruments["Clavinet"];
		g.GenericInstruments.EmplaceLast([](){
			// Клавинет: очень короткий, яркий, перкуссионный.
			return AdditivePianoInstrument{0.65f, 40, 0.4f, 2.8f, 0.015f, 0.6f, 0.0f, 2, 0.2f, 0.2f, 0.0f};
		}());
		g.Envelope = MakeWebEnvelope({0, 0, 1, 0.15f, 0, false, true});
	}
	{
		auto& g = Instruments["ElectricGuitarJazz"];
		g.GenericInstruments.EmplaceLast(KarplusStrongInstrument{0.25f, 0.4f, 0.7f, 0.2f, 0.85f, 1.0f});
		g.Envelope = MakeWebEnvelope({0, 0, 1, 0.1f, 1, false, true});
	}
	{
		auto& g = Instruments["ElectricGuitarClean"];
		g.GenericInstruments.EmplaceLast(KarplusStrongInstrument{0.015f, 0.35f, 0.8f, 0.4f, 0.2f, 1.5f});
		g.Envelope = MakeWebEnvelope({0, 0, 1, 0.1f, 1.5f, false, true});
	}
	{
		auto& g = Instruments["ElectricGuitarMuted"];
		g.GenericInstruments.EmplaceLast(KarplusStrongInstrument{0.2f, 0.3f, 0.7f, 0.2f, 0.7f, 1.0f});
		g.Envelope = MakeWebEnvelope({0, 0, 1, 0.2f, 1, false, true});
	}
	{
		auto& g = Instruments["Sitar"];
		g.GenericInstruments.EmplaceLast(KarplusStrongInstrument{0.02f, 0.12f, 0.8f, 0.4f, 0.35f, 0.2f});
		g.Envelope = MakeWebEnvelope({0.003f, 0, 1, 0.4f, 0.2f, false, false});
	}
	{
		auto& g = Instruments["AcousticBass"];
		g.GenericInstruments.EmplaceLast(KarplusStrongInstrument{0.03f, 0.15f, 0.8f, 0.5f, 0.2f, 1.5f});
		g.Envelope = MakeWebEnvelope({0, 0, 1, 0.3f, 1.5f, false, true});
	}

	// === Перегруженная гитара: спектральная струна (FFT-1) + перегруз ===
	// Вариант B: начальное возмущение то же, что у KS, но вместо петли —
	// спектральное затухание per-harmonic (см. SpectralStringSampler.h).
	// Brightness 0.6 ослабляет демпфирование KS в 2.5 раза: верха живут
	// дольше, тембр богаче — именно то, чего не хватает KS для перегруза.
	// Дальше цепочка перегруза: драйв (мягкий клиппинг) -> убрать DC/гул ->
	// тон (тёмный "кабинет") -> уровень.
	{
		auto& g = Instruments["OverdrivenGuitar"];
		g.GenericInstruments.EmplaceLast([](){
			SpectralStringInstrument s{0.15f, 0.3f, 0.8f, 0.4f, 0.6f, 0.0f, 0.55f, 1.5f};
			s.CutoffRatio = 0.15f; // ~3.6 кГц — звукосниматель, без алиасинга
			return s;
		}());
		g.Envelope = MakeWebEnvelope({0.001f, 0, 1, 1.2f, 1.5f, false, true});
		g.GenericModifiers.EmplaceLast(DriveEffect(4.5f));
		g.GenericModifiers.EmplaceLast(SoftHighPassFilterFactory(80));
		// «Кабинет» двумя каскадами (12 дБ/окт): одиночный one-pole пропускал
		// физа-гармоники (продукты драйва) 4-10 кГц — слышный «шум».
		g.GenericModifiers.EmplaceLast(FilterFactory::FromCutoffRatio(0.7f, 0.07f, FilterType::LowPass));
		g.GenericModifiers.EmplaceLast(FilterFactory::FromCutoffRatio(0.7f, 0.07f, FilterType::LowPass));
		g.GenericModifiers.EmplaceLast(FilterFactory(FilterCoeffs{0, 0, 0, 0, 0.35f}));
	}

	// === Дисторшн-гитара (программа 30): тот же спектральный источник, но
	// жёстче. Ярче источник (0.75 вместо 0.6 — больше верхов доходит до
	// клиппинга), ниже демпфирование (дольше сустейн), двухкаскадный драйв
	// (6 + 3 — сильнее компрессия и гармоники, чем одиночный 4.5 у Overdriven),
	// выше срез HPF (100 Гц — плотнее низ, без гула) и темнее "кабинет"
	// (0.055 вместо 0.07). Уровень ниже, т.к. жёсткий клиппинг сам по себе громче.
	{
		auto& g = Instruments["DistortionGuitar"];
		g.GenericInstruments.EmplaceLast([](){
			SpectralStringInstrument s{0.12f, 0.3f, 0.8f, 0.4f, 0.75f, 0.0f, 0.5f, 1.5f};
			s.CutoffRatio = 0.12f; // ~2.9 кГц — темнее, без алиасинга драйва
			return s;
		}());
		g.Envelope = MakeWebEnvelope({0.001f, 0, 1, 1.2f, 1.5f, false, true});
		g.GenericModifiers.EmplaceLast(DriveEffect(6.0f));
		g.GenericModifiers.EmplaceLast(DriveEffect(3.0f));
		g.GenericModifiers.EmplaceLast(SoftHighPassFilterFactory(100));
		// «Кабинет» двумя каскадами (12 дБ/окт) — глушит физа-гармоники драйва.
		g.GenericModifiers.EmplaceLast(FilterFactory::FromCutoffRatio(0.7f, 0.055f, FilterType::LowPass));
		g.GenericModifiers.EmplaceLast(FilterFactory::FromCutoffRatio(0.7f, 0.055f, FilterType::LowPass));
		g.GenericModifiers.EmplaceLast(FilterFactory(FilterCoeffs{0, 0, 0, 0, 0.22f}));
	}

	// === Формантные/аддитивные вейвтаблицы ===

	// Повторяющиеся наборы резонансов.
	static const WebResonanceDesc choirRes[] = {
		{600, 100, 1.25f}, {900, 175, 1.95f}, {2200, 150, 3.5f}, {2600, 175, 4.4f}, {0, 2000, 7.5f}};
	static const WebResonanceDesc voiceRes[] = {
		{600, 100, 1.25f}, {900, 230, 1.95f}, {2200, 840, 3.5f}, {2600, 175, 4.4f}, {0, 2000, 0.75f}};
	static const WebResonanceDesc padRes[] = {
		{500, 140, 1}, {900, 540, 4}, {2100, 1400, 10}, {3700, 2100, 15}, {4700, 2800, 20}};
	static const WebResonanceDesc str3Res[] = {
		{400, 35, 87}, {900, 115, 195}, {1400, 115, 60}, {2200, 345, 1320}, {3500, 140, 35}, {6000, 2500, 460}, {0, 7, 2}};
	static const WebResonanceDesc pizziRes[] = {
		{500, 50, 0.35f}, {800, 315, 2.5f}, {2100, 1400, 10}, {3700, 2100, 15}, {0, 0.1f, 0.001f}};
	static const WebResonanceDesc violinRes[] = {
		{400, 85, 8.7f}, {900, 115, 19.5f}, {1400, 115, 6}, {2200, 300, 7}, {3500, 240, 2}, {10000, 4500, 20}};
	static const WebResonanceDesc violinOldRes[] = {
		{500, 50, 12.5f}, {800, 315, 76}, {2100, 1400, 420}, {3700, 1750, 440}, {0, 7, 1.75f}};
	static const WebResonanceDesc orchRes[] = {
		{275, 700, 175}, {1150, 1400, 420}, {2500, 700, 175}, {4100, 175, 44}, {0, 2100, 525}};
	static const WebResonanceDesc callRes[] = {
		{275, 700, 175}, {650, 1400, 420}, {1100, 700, 175}, {2700, 250, 62.5f}, {0, 2100, 52.5f}};
	static const WebResonanceDesc trumpetRes[] = {
		{2500, 1000, 500}, {6500, 1000, 100}};
	static const WebResonanceDesc accRes[] = {
		{1700, 400, 150}, {4200, 400, 100}, {7300, 700, 15}, {11200, 700, 10}};
	static const WebResonanceDesc fluteNewRes[] = {{650, 140, 350}, {1400, 80, 200}};
	static const WebResonanceDesc flute2Res[] = {{650, 140, 85}, {1400, 80, 50}};

	// Вейвтаблицы со статическими гармониками (уже data-driven, оставлены как есть).
	{
		static const WebHarmonicDesc vibraphoneH[] = {{1, 1, 2}, {0.25f, 4, 15}, {0.125f, 8, 25}, {0.0625f, 16, 45}, {0.03125f, 32, 58}};
		auto& t = Tables["Vibraphone"] = CreateWebWaveTables(Sets(Harms(SpanOf(vibraphoneH))), 16384);
		auto& wt = Instruments["Vibraphone"].WaveTables.EmplaceLast();
		wt = Wt(&t, 0.15f, 2, {0.004f, 0.05f, 0.3f, 0.25f, 2, false, false}, 5, 0.0015f);
	}
	{
		static const WebHarmonicDesc musicBoxH[] = {{1, 1, 20}, {0.5f, 4, 15}, {0.25f, 8, 15}, {0.125f, 16, 15}, {0.0625f, 32, 15}};
		auto& t = Tables["MusicBox"] = CreateWebWaveTables(Sets(Harms(SpanOf(musicBoxH))), 16384);
		auto& wt = Instruments["MusicBox"].WaveTables.EmplaceLast();
		wt = Wt(&t, 0.1f, 4, {0.01f, 0.08f, 0.5f, 0.4f, 4, false, false});
	}
	{
		static const WebHarmonicDesc marimbaH[] = {{1, 1, 8}, {0.25f, 4, 9}, {0.108f, 9.2f, 6}, {0.0835f, 12, 6}, {0.04175f, 24, 6}, {0.021f, 48, 6}};
		auto& t = Tables["Marimba"] = CreateWebWaveTables(Sets(Harms(SpanOf(marimbaH))), 16384);
		auto& wt = Instruments["Marimba"].WaveTables.EmplaceLast();
		wt = Wt(&t, 0.25f, 5, {0.005f, 0.05f, 0.3f, 0.3f, 5, false, false});
	}
	{
		static const WebHarmonicDesc xylophoneH[] = {{1, 1, 20}, {0.333f, 3, 60}, {0.108f, 9.2f, 184}, {0.0769f, 13, 260}, {0.033f, 30, 600}};
		auto& t = Tables["Xylophone"] = CreateWebWaveTables(Sets(Harms(SpanOf(xylophoneH))), 16384);
		auto& wt = Instruments["Xylophone"].WaveTables.EmplaceLast();
		wt = Wt(&t, 0.25f, 5, {0.006f, 0.015f, 0.2f, 0.2f, 5, true, false});
	}
	{
		static const WebHarmonicDesc newAgeH[] = {{1, 1, 20}, {0.5f, 4, 46}, {0.25f, 8, 94}, {0.125f, 16, 190}, {0.0625f, 32, 380}};
		auto& t = Tables["NewAge"] = CreateWebWaveTables(Sets(Harms(SpanOf(newAgeH))), 8192);
		auto& wt = Instruments["NewAge"].WaveTables.EmplaceLast();
		wt = Wt(&t, 0.17f, 3, {0.015f, 0.04f, 0.5f, 0.3f, 3, false, false});
	}
	{
		static const WebHarmonicDesc glockenspielH[] = {{0.33f, 1, 7}, {0.19f, 6.7f, 30}, {0.15f, 6.1f, 40}, {0.12f, 8.4f, 17}, {0.15f, 12.7f, 37}, {0.12f, 23.2f, 28}};
		auto& t = Tables["Glockenspiel"] = CreateWebWaveTables(Sets(Harms(SpanOf(glockenspielH))), 16384);
		auto& wt = Instruments["Glockenspiel"].WaveTables.EmplaceLast();
		wt = Wt(&t, 0.15f, 8, {0.011f, 0.08f, 0.6f, 0.7f, 8, false, false});
	}
	{
		static const WebHarmonicDesc clarinetH[] = {{1, 1, 15}, {0.275f, 3, 45}, {0.2f, 5, 55}, {0.1f, 7, 105}, {0.05f, 9, 135}, {0.03f, 11, 165}, {0.08f, 13, 195}};
		auto& t = Tables["Clarinet"] = CreateWebWaveTables(Sets(Harms(SpanOf(clarinetH))), 16384);
		auto& wt = Instruments["Clarinet"].WaveTables.EmplaceLast();
		wt = Wt(&t, 0.35f, 0, {0.03f, 0.05f, 0.75f, 0.1f, 0, false, false}, 0.5f, 0.005f);
	}
	{
		static const WebHarmonicDesc whistleH[] = {{1, 1, 5}, {0.2f, 1, 1000}, {0.02f, 2, 10}, {0.1f, 2, 1000}, {0.01f, 3, 15}, {0.05f, 3, 1000}};
		auto& t = Tables["Whistle"] = CreateWebWaveTables(Sets(Harms(SpanOf(whistleH))), 16384);
		auto& wt = Instruments["Whistle"].WaveTables.EmplaceLast();
		wt = Wt(&t, 0.15f, 0, {0.01f, 0, 1, 0.01f, 0, false, false}, 5, 0.003f);
	}
	{
		static const WebHarmonicDesc ocarinaH[] = {{1, 1, 3}, {0.01f, 2, 7}, {0.1f, 3, 7}, {0.03f, 4, 7}};
		auto& t = Tables["Ocarina"] = CreateWebWaveTables(Sets(Harms(SpanOf(ocarinaH))), 16384);
		auto& wt = Instruments["Ocarina"].WaveTables.EmplaceLast();
		wt = Wt(&t, 0.3f, 0, {0.003f, 0, 1, 0.1f, 0, false, false}, 5, 0.003f);
	}
	{
		static const WebHarmonicDesc phoneRingH[] = {{1, 1, 3}};
		auto& t = Tables["PhoneRing"] = CreateWebWaveTables(Sets(Harms(SpanOf(phoneRingH))), 16384);
		auto& wt = Instruments["PhoneRing"].WaveTables.EmplaceLast();
		wt = Wt(&t, 0.5f, 0, {0, 0, 1, 0.5f, 0, true, true});
	}
	{
		static const WebHarmonicDesc kalimbaH[] = {{1, 1, 20}, {0.5f, 4, 46}, {0.25f, 8, 94}, {0.125f, 16, 190}, {0.0625f, 32, 380}};
		auto& t = Tables["Kalimba"] = CreateWebWaveTables(Sets(Harms(SpanOf(kalimbaH))), 32768);
		auto& wt = Instruments["Kalimba"].WaveTables.EmplaceLast();
		wt = Wt(&t, 0.2f, 5, {0.004f, 0.05f, 0.3f, 0.1f, 5, false, false});
	}

	// Прочие вейвтабличные инструменты — table-driven.
	{
		static const WtSpec specs[] =
		{
			{"ChoirAahs", nullptr, 16384, {
				{64, BW_70p70x, F_1overx2, F_x, choirRes, 5, true},
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false}},
				0.4f, 0, 0, 0, {0.1f, 0.1f, 0.7f, 0.4f, 0, false, false}},
			{"ChoirA", nullptr, 16384, {
				{100, BW_60p20x, A_0_8_0_8r_div_x2, F_x, choirRes, 5, false},
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false}},
				0.4f, 0, 0, 0, {0.1f, 0.1f, 0.7f, 0.4f, 0, false, false}},
			{"SynthVoice", nullptr, 32768, {
				{64, BW_40p40x, F_1overx2, F_x, voiceRes, 5, true},
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false}},
				0.2f, 0, 0, 0, {0.04f, 0, 1, 0.1f, 0, true, false}},
			{"VoiceOohs", "SynthVoice", 0, {
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false}},
				0.25f, 0, 0, 0, {0.005f, 0.3f, 0.6f, 0.2f, 0, true, false}},
			{"Pad4Choir", "SynthVoice", 0, {
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false}},
				0.65f, 0, 0, 0, {0.1f, 0.1f, 0.4f, 0.4f, 0, false, false}},
			{"Pad7Halo", "SynthVoice", 0, {
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false}},
				0.6f, 0, 0, 0, {0.03f, 0.5f, 0.7f, 0.3f, 0, false, false}},
			{"Pad8Sweep", nullptr, 32768, {
				{100, BW_40p40x, F_1overx, F_x, padRes, 5, true},
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false}},
				0.15f, 0, 0, 0, {0.015f, 0, 1, 0.25f, 0, false, false}},
			{"StringEnsemble2", "Pad8Sweep", 0, {
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false}},
				0.15f, 0, 0, 0, {0.007f, 0, 1, 0.15f, 0, false, false}},
			{"SynthStrings", "Pad8Sweep", 0, {
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false}},
				0.2f, 0, 0, 0, {0.3f, 0, 1, 0.2f, 0, false, false}},
			{"SynthStrings3", nullptr, 32768, {
				{100, BW_50p15x, A_0_8_0_1rSqrt_div_x2, F_x, str3Res, 7, false},
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false}},
				0.1f, 0, 0, 0, {0.3f, 0, 1, 0.3f, 0, false, false}},
			{"StringEnsemble", nullptr, 32768, {
				{64, BW_35p30x, F_1overx, F_x, padRes, 5, true},
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false}},
				0.25f, 0, 0, 0, {0.3f, 0, 1, 0.2f, 0, false, false}},
			{"TremoloStrings", "Pad8Sweep", 0, {
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false}},
				0.25f, 0, 0, 0, {0.01f, 0, 1, 0.07f, 0, false, false}},
			{"RockOrgan", "Pad8Sweep", 0, {
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false}},
				0.4f, 0, 5, 0.004f, {0.001f, 0.6f, 0.1f, 0.05f, 0, true, false}},
			{"Harmonica", nullptr, 16384, {
				{16, BW_1px, F_1overx, F_x, padRes, 5, true},
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false}},
				0.15f, 0, 0, 0, {0.01f, 0, 1, 0.01f, 0, false, false}},
			{"Fiddle", nullptr, 16384, {
				{16, BW_4p4x, F_1oversqrtx, F_x, padRes, 5, true},
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false}},
				0.15f, 0, 0, 0, {0.03f, 0, 1, 0.04f, 0, false, false}},
			{"PizzicatoStrings", nullptr, 16384, {
				{16, BW_4p4x, F_1oversqrtx, F_x, pizziRes, 5, true},
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false}},
				0.2f, 6, 0, 0, {0.02f, 0.2f, 0.3f, 0.07f, 6, false, false}},
			{"PercussiveOrgan", "SynthVoice", 0, {
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false}},
				0.25f, 0, 0, 0, {0.008f, 0.2f, 0.6f, 0.05f, 0, true, false}},
			{"Violin", nullptr, 32768, {
				{100, F_0_1, A_0_8_1_0_5r_div_x, F_x, violinRes, 6, true},
				{100, F_1200, A_0_2_1_0_7r_div_x, F_x, violinRes, 6, true},
				{0, F_1, F_1, F_1, nullptr, 0, false}},
				0.2f, 0, 6, 0.002f, {0.05f, 0.03f, 0.8f, 0.05f, 0, false, false}},
			{"ViolinOld", nullptr, 32768, {
				{100, F_0_1, A_0_8_1_0_5r_div_x, F_x, violinOldRes, 5, true},
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false}},
				0.2f, 0, 6, 0.002f, {0.05f, 0.03f, 0.8f, 0.05f, 0, false, false}},
			{"SynthBrass", nullptr, 16384, {
				{40, BW_7_1p0_8x, F_xm1_5, F_x, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false}},
				0.3f, 0, 12, 0.0015f, {0.01f, 0.4f, 0.3f, 0.1f, 0, false, false}},
			{"BrassSection", "SynthBrass", 0, {
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false}},
				0.25f, 0, 5, 0.003f, {0.02f, 0, 1, 0.03f, 0, false, false}},
			{"Pad3Polysynth", "SynthBrass", 0, {
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false}},
				0.1f, 0, 0, 0, {0.01f, 0, 1, 0.15f, 0, true, false}},
			{"Fx1Rain", "SynthBrass", 0, {
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false}},
				0.45f, 1, 25, 0.007f, {0.003f, 0.05f, 0.3f, 0.2f, 1, false, false}},
			{"SteelDrums", nullptr, 16384, {
				{40, BW_2p5x, F_xm1_5, F_x, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false}},
				0.3f, 0, 0, 0, {0.005f, 0.3f, 0.7f, 0.2f, 0, true, false}},
			{"Fx6Goblins", "SteelDrums", 0, {
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false}},
				0.07f, 0, 0, 0, {0.05f, 0, 1, 0.15f, 0, true, false}},
			{"Clav", nullptr, 16384, {
				{64, BW_1_2x, A_cos_x26_div_x2, F_x, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false}},
				0.12f, 6, 0, 0, {0.008f, 0, 1, 0.01f, 6, true, false}},
			{"OrchestraHit", nullptr, 32768, {
				{100, BW_52x_20, A_pow_m0_4, F_x, orchRes, 5, true},
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false}},
				0.2f, 3, 0, 0, {0.015f, 0.15f, 0.4f, 0.1f, 3, true, false}},
			{"Calliope", nullptr, 16384, {
				{64, BW_24x_14, F_1overx, F_x, callRes, 5, false},
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false}},
				0.25f, 0, 0, 0, {0.015f, 0, 1, 0.03f, 0, false, false}},
			{"Celesta", "Marimba", 0, {
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false}},
				0.15f, 5, 0, 0, {0.005f, 0, 1, 0.3f, 5, false, false}},
			{"Fx4Atmosphere", nullptr, 16384, {
				{64, BW_6p14x, F_xm2_4, F_x, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false}},
				0.5f, 3, 0, 0, {0.015f, 0.2f, 0.4f, 0.2f, 3, false, false}},
			{"Pad5Bowed", "NewAge", 0, {
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false}},
				0.17f, 3, 0, 0, {0.015f, 0.04f, 0.5f, 0.3f, 3, false, false}},
			{"Lead1Square", nullptr, 16384, {
				{64, BW_1_7x_0_7, A_mod2_pow_m1_3, FM_2x_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false}},
				0.1f, 0, 0, 0, {0.007f, 0.01f, 0.62f, 0.05f, 0, true, false}},
			{"Lead2Sawtooth", nullptr, 32768, {
				{64, BW_1_7x, A_mod2_div_x, F_x, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false}},
				0.05f, 0, 0, 0, {0.02f, 0.01f, 0.7f, 0.15f, 0, true, false}},
			{"SynthBass1", nullptr, 16384, {
				{64, BW_3p5x, A_cos_x21_div_x2, F_x, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false}},
				0.15f, 4, 7, 0.002f, {0.01f, 0, 1, 0.4f, 4, true, false}},
			{"Lead5Charang", nullptr, 16384, {
				{16, BW_0_5x, A_cos_x16_div_x, F_x, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false}},
				0.2f, 0, 0, 0, {0.01f, 0, 1, 0.05f, 0, true, false}},
			{"FluteNew", nullptr, 32768, {
				{64, BW_3p5x, A_0_8_0_1rSqrt_pow, F_x, fluteNewRes, 2, false},
				{64, F_1200, A_0_025_0_05r_div_Sqrt, F_x, fluteNewRes, 2, false},
				{0, F_1, F_1, F_1, nullptr, 0, false}},
				0.3f, 0, 0.5f, 0.003f, {0.02f, 0.07f, 0.92f, 0.23f, 0, false, false}},
			{"Flute", nullptr, 16384, {
				{16, BW_6x_1, A_pow_m2_5, FM_2x_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false}},
				0.2f, 0, 0.5f, 0.003f, {0.07f, 0.07f, 0.92f, 0.1f, 0, false, false}},
			{"Flute2", nullptr, 32768, {
				{64, BW_3px, A_0_8_0_1rSqrt_pow, F_x, flute2Res, 2, false},
				{64, F_1200, A_0_01_0_02r, F_x, flute2Res, 2, false},
				{64, F_1200, A_0_02_0_01r, F_x, nullptr, 0, false}},
				0.1f, 0, 0.5f, 0.003f, {0.03f, 0.07f, 0.92f, 0.3f, 0, false, false}},
			{"Recorder", nullptr, 16384, {
				{64, BW_0_2x_0_1, A_cos_x2_25_pow, F_x, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false}},
				0.25f, 0, 0, 0, {0.007f, 0.07f, 0.92f, 0.01f, 0, true, false}},
			{"Fx2SoundTrack", "Pad8Sweep", 0, {
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false}},
				0.1f, 0, 0, 0, {0.7f, 0.3f, 0.5f, 0.6f, 0, true, false}},
			{"BassLead", nullptr, 16384, {
				{16, F_x, A_cos_x6_pow1_8, F_x, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false}},
				0.2f, 0, 0, 0, {0.01f, 0.1f, 0.6f, 0.3f, 0, true, false}},
			{"SynthBass2", "BassLead", 0, {
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false}},
				0.2f, 0, 0, 0, {0.01f, 0.3f, 0.6f, 0.1f, 0, true, false}},
			{"ElectricBassPick", "BassLead", 0, {
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false}},
				0.25f, 0, 0, 0, {0.008f, 0.05f, 0.3f, 0.03f, 0, true, false}},
			{"SlapBass", "BassLead", 0, {
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false}},
				0.15f, 0, 0, 0, {0.006f, 0.3f, 0.6f, 0.1f, 0, true, false}},
			{"Trumpet", nullptr, 16384, {
				{64, F_1, A_0_8_div_x2, F_x, trumpetRes, 2, false},
				{64, F_1200, A_0_01_0_02r, F_x, trumpetRes, 2, false},
				{0, F_1, F_1, F_1, nullptr, 0, false}},
				0.35f, 0, 0, 0, {0.02f, 0.02f, 0.7f, 0.1f, 0, false, false}},
			{"TrumpetOld", nullptr, 16384, {
				{20, BW_0_4x, A_cos_x9_div_x2, F_x, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false}},
				0.35f, 0, 0, 0, {0.02f, 0.02f, 0.7f, 0.1f, 0, false, false}},
			{"EnglishHorn", "TrumpetOld", 0, {
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false}},
				0.35f, 0, 0.5f, 0.005f, {0.1f, 0, 1, 0.1f, 0, true, false}},
			{"FrenchHorn", "TrumpetOld", 0, {
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false}},
				0.2f, 0, 0, 0, {0.05f, 0, 1, 0.05f, 0, false, false}},
			{"Oboe", nullptr, 16384, {
				{16, BW_3_4p1_6x, A_cos_x1_6_div_x2, F_x, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false}},
				0.2f, 0, 0, 0, {0.015f, 0, 1, 0.03f, 0, false, false}},
			{"Accordion", nullptr, 16384, {
				{24, BW_12_2xp2_8, A_cos_x6_pow1_5, F_x, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false}},
				0.5f, 0, 0, 0, {0.025f, 0.02f, 0.7f, 0.05f, 0, true, false}},
			{"Accordion1", nullptr, 32768, {
				{64, F_1, A_0_06_0_3r_pow, FM_1_007x, accRes, 4, false},
				{64, BW_15p10x, A_0_08_0_4r_div_x2, FM_0_9955x, accRes, 4, false},
				{0, F_1, F_1, F_1, nullptr, 0, false}},
				0.1f, 0, 0, 0, {0.025f, 0.02f, 0.7f, 0.05f, 0, true, false}},
			{"Tuba", nullptr, 16384, {
				{24, F_0_1, A_cos_x9_div_x2, F_x, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false}},
				0.2f, 0, 0, 0, {0.01f, 0, 1, 0.05f, 0, false, false}},
			{"FretlessBass", nullptr, 16384, {
				{24, F_0_1, A_cos_x16_div_x2, F_x, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false}},
				0.2f, 5, 0, 0, {0.01f, 0, 1, 0.1f, 5, true, false}},
			{"Sax", nullptr, 16384, {
				{64, BW_0_5x, A_sax, F_x, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false}},
				0.3f, 0, 0, 0, {0.012f, 0, 1, 0.02f, 0, false, false}},
			{"SynthOrgan", nullptr, 32768, {
				{20, BW_10_1pPow, F_1overx, FM_pow2_x_2, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false},
				{0, F_1, F_1, F_1, nullptr, 0, false}},
				0.1f, 0, 0, 0, {0.01f, 0, 1, 0.01f, 0, false, false}},
		};

		for(const auto& sp: specs)
		{
			WaveTableCache* t = nullptr;
			if(sp.Reuse)
				t = &Tables[sp.Reuse];
			else
			{
				Array<WebHarmonicSet> sets;
				for(const auto& se: sp.Series)
				{
					if(se.Num == 0) continue;
					WebHarmonicSet s = Series(se.Num, se.Bw, se.Amp, se.Fm);
					if(se.Resonances)
						s = Res(Move(s), Span<const WebResonanceDesc>(se.Resonances, se.NumResonances), se.Multiplicative);
					sets.AddLast(Move(s));
				}
				t = &(Tables[sp.Name] = CreateWebWaveTables(sets, sp.TableSize));
			}
			auto& wt = Instruments[sp.Name].WaveTables.EmplaceLast();
			wt = Wt(t, sp.Volume, sp.ExpCoeff, sp.Env, sp.VibFreq, sp.VibVal);
		}

		// Инструменты с дополнительными модификаторами.
		Instruments["SynthBass1"].GenericModifiers.EmplaceLast(WebExpExpModifierFactory(0.00003f, 5));
		Instruments["Flute"].GenericModifiers.EmplaceLast(WebCutoffFactory(200, 20000, 20000, 100, {0.07f, 0.07f, 0.92f, 0.1f, 0, false, false}));
	}

	// === Шумовые ===

	{
		auto& g = Instruments["ReverseCymbal"];
		g.GenericInstruments.EmplaceLast(WebNoiseInstrument{32768, 0.2f, 157898685});
		g.Envelope = MakeWebEnvelope({1, 0.1f, 1, 2, 0, true, false});
	}
	{
		auto& g = Instruments["Applause"];
		g.GenericInstruments.EmplaceLast(WebNoiseInstrument{32768, 0.05f, 157898685});
		g.Envelope = MakeWebEnvelope({0, 0, 1, 0.5f, 0, true, true});
		g.GenericModifiers.EmplaceLast(WebCutoffFactory(500, 5000, 5000, 500, {0, 0, 1, 0.5f, 0, true, true}));
	}
	{
		auto& g = Instruments["Helicopter"];
		g.GenericInstruments.EmplaceLast(WebNoiseInstrument{32768, 0.5f, 157898685});
		g.Envelope = MakeWebEnvelope({0, 0, 1, 0.5f, 0, true, true});
		g.GenericModifiers.EmplaceLast(WebCutoffFactory(500, 2000, 2000, 500, {0, 0, 1, 0.5f, 0, true, true}));
	}
	{
		auto& g = Instruments["Seashore"];
		g.GenericInstruments.EmplaceLast(WebNoiseInstrument{32768, 0.04f, 157898685});
		g.Envelope = MakeWebEnvelope({0, 0, 1, 0.5f, 0, true, true});
		g.GenericModifiers.EmplaceLast(WebCutoffFactory(500, 2000, 2000, 500, {0, 0, 1, 0.5f, 0, true, true}));
	}
	{
		auto& g = Instruments["Gunshot"];
		g.GenericInstruments.EmplaceLast(WebNoiseInstrument{32768, 0.4f, 157898685});
		g.Envelope = MakeWebEnvelope({0, 0, 1, 4, 0, true, true});
		g.GenericModifiers.EmplaceLast(WebExpExpModifierFactory(0.00003f, 7));
	}
	{
		auto& g = Instruments["Timpani"];
		g.GenericInstruments.EmplaceLast(WebNoiseInstrument{32768, 0.2f, 157898685});
		g.Envelope = MakeWebEnvelope({0, 0, 1, 4, 0, true, true});
		g.GenericModifiers.EmplaceLast(WebExpExpModifierFactory(0.00005f, 15));
	}

	// === Ударные, которых нет в web-midisynth (остаются) ===

	UniDrum = CachedDrumInstrument(Generators::DrumPhysicalModel(2, 16, 16, 0.342f, 0.00026f, 0.20f), 44100, 0.015f);
	ClosedHiHat = CachedDrumInstrument(Generators::DrumPhysicalModel(2, 16, 16, 0.338f, 0.04928f, 0.10f), 44100, 0.015f);
	AcousticBassDrum = CachedDrumInstrument(Generators::DrumPhysicalModel(2, 8, 8, 0.092f, 0.0072f, 0.20f), 88200, 0.015f);
	AcousticSnare = CachedDrumInstrument(SnarePhysicalModel(), 22000, 0.05f);
}
