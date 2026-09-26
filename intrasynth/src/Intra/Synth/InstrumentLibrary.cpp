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
#include "PianoRegions.h"
#include "Filter.h"
#include "Synth.h"

#include "Generators.hh"
#include "PostEffects.hh"

#include "WaveTableGeneration.h"

#ifdef INTRA_PROBE_NAN
#include <stdio.h>
#endif

using namespace Intra;

namespace
{
	// Кусочно-линейная интерполяция скаляра по точкам профиля. Определение —
	// ниже по файлу; объявление нужно профилю вибрато пан-флейты, который
	// записан ВЫШЕ определения.
	noinline float MixParam(const float* xs, const float* vals, size_t n, float x);

	// Формулы web-midisynth: один enum вместо десятков отдельных лямбд-функций.
	// Параметры: x = 1..numHarmonics, r = случайное число [0;1).
	enum HarmonicFunc : uint8
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

	noinline float ApplyHarmonicFunc(HarmonicFunc f, float x, float r)
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
	noinline HarmonicSet Series(size_t num, HarmonicFunc bw, HarmonicFunc amp, HarmonicFunc fm)
	{
		HarmonicSet set;
		Random::FastUniform<float> rnd(0x9e3779b9u);
		for(size_t i = 1; i <= num; i++)
		{
			const float x = float(i), r = rnd();
			set.Harmonics.AddLast(HarmonicDesc{ApplyHarmonicFunc(amp, x, r), ApplyHarmonicFunc(fm, x, r), ApplyHarmonicFunc(bw, x, r)});
		}
		return set;
	}

	noinline HarmonicSet Res(HarmonicSet s, Span<const ResonanceDesc> r, bool mult)
	{
		s.IsResonanceMultiplicative = mult;
		for(const auto& e: r) s.Resonances.AddLast(e);
		return s;
	}

	noinline HarmonicSet Harms(Span<const HarmonicDesc> h)
	{
		HarmonicSet s;
		for(const auto& e: h) s.Harmonics.AddLast(e);
		return s;
	}

	// Flute spectra are Q16 (0..2.0) measured coefficients; halves profile data.
	noinline HarmonicSet Harms16(Span<const uint16> amplitudes, float volumeScale = 1)
	{
		HarmonicSet s;
		s.VolumeScale = volumeScale;
		for(size_t i = 0; i < amplitudes.Length(); i++)
			s.Harmonics.AddLast({float(amplitudes[i])*(2.0f/65535.0f), float(i + 1), 0});
		return s;
	}

	// Юбка гармоники (Update 51): у живых духовых гармоника — не линия, а
	// горб резонанса трубы (Q≈3-5): в замере банка (.scratch/bins-near.mjs)
	// вокруг КАЖДОЙ гармоники подъём на 8-15 дБ над подложкой, шириной
	// ±12-15% её частоты (то есть в центах, а не в герцах). Здесь каждая
	// гармоника превращается в ядро (острая линия) + размытую копию с той же
	// амплитудой, умноженной на skirtGain. Нормализация BuildWaveTable
	// делит на сумму амплитуд, поэтому ядро при этом слабеет ровно так же,
	// как у банка (линия над горбом).
	noinline HarmonicSet WithSkirt(HarmonicSet s, float bandwidthCents, float skirtGain)
	{
		if(skirtGain <= 0.0f) return s;
		Array<HarmonicDesc> out;
		for(const auto& h: s.Harmonics)
		{
			out.AddLast(HarmonicDesc{h.Amplitude, h.FreqMultiplier, 0});
			out.AddLast(HarmonicDesc{h.Amplitude*skirtGain, h.FreqMultiplier, bandwidthCents});
		}
		s.Harmonics = Move(out);
		return s;
	}

	// Вибрато пан-флейты — ОДНО на все слои ноты (тело, дыхание N/W, блум).
	// Замер банка (.scratch/vib-track.mjs, полоса h1): rms 2.9/2.2/2.1 цента,
	// у нас после Update 58 было 3.5/3.2/2.3 — глубина не менялась, поменялась
	// только СОГЛАСОВАННОСТЬ: раньше вибрато было у одного тела, а юбки и
	// овершут стояли на месте и бились с ним (владелец: «гудение на C4, как
	// будто насос работает, но с юбкой»). 5.6 Гц, задержка 0.35 с, вход 0.30 с.
	Vibrato PanFluteVibrato(float freq)
	{
		const float x = Math::Clamp(Math::Log(freq/261.63f)/Math::Log(2.0f), 0.0f, 2.0f);
		Vibrato r;
		// Доставка вышла глубже банка на 15-60 % (замер 14.5/6.9/3.5 против
		// 12.6/4.2/2.2) — глубина срезана на 20 %.
		// Update 61: глубина ×2.8 и ранний вход. Замер той же полосой: у банка
		// когерентного вибрато нет вовсе (rms 2.2-3.1 ц при mag 0.5-1.1 — это
		// шероховатость), но владелец слышит «в сустейне не хватает лёгкого
		// вибрато» и «вибрато не слышно» — тон ±4 цента на слух не модулируется.
		// Update 63: глубина срезана до ЗАМЕРА БАНКА. Владелец: «в оригинале
		// две компоненты — одна вибрирует, другая ровная, а у нас всё
		// вибрирует». Замер pump-check (AMDF по всей ноте) и hnr-audit (полоса
		// h1): у банка rms глубины 2.9/2.2/2.1 цента (75:60/72/84), когерентной
		// линии вибрато нет вовсе (12.08 Гц mag 1.3); у нас было 7.8/6.3/4.2
		// при когерентных 6.04 Гц mag 10.1 — качалась вся нота. Множитель 0.35
		// даёт ±3.9/±2.7/±1.9 цента — ровно банковский уровень. Дыхание
		// (слои N/W) статично с Update 63, блум-слои по-прежнему на этом же
		// LFO: статичный когерентный обертон бился бы с уходящим тоном («насос»
		// Update 58).
		// Update 64: владелец — «вибрато именно у ОСНОВНОГО ТОНА после раскачки,
		// а воздух ровный в сустейне; наш рендер и оригинал противоположны».
		// Глубина 0.35× (Update 63) давала ±3.9…±1.9 цента — владелец её не
		// слышит («вибрато не слышно»), поэтому тону возвращена слышимая
		// глубина ±10.0 → ±6.6 цента, а вход отодвинут ЗА раздув (0.45+0.35 —
		// полная к 0.80 с). Дыхание (NoiseSampler) с Update 63 статично, так что
		// «воздух ровный» выполнено. Update 65: слышимое движение отдано
		// ДЫХАНИЮ (PanFluteBreathVibrato, Tremolo 0.24 @ 4.04 Гц), у самого тона
		// Tremolo по-прежнему 0 — он остаётся ровным, как в банке.
		// Update 66: владелец — «не слышу тремоло основного тембра; у оригинала
		// всё ровно, только тремоло основного тембра, воздух ровный». То есть
		// ±4 цента (Update 65) — слишком мало, а движение должно быть у ТОНА,
		// не у воздуха. Глубина возвращена к прежней ±10.0…±6.6 цента, к ней
		// добавлена АМ ±0.9 дБ, и вся модуляция получила дрожание глубины
		// (Jitter 0.5) — иначе жёсткий синус читается как механический период.
		// Update 67: владелец — «пан-флейта как будто стала слишком часто
		// вибрировать». Замер `mod` (когерентный пик огибающей банка, 1.5-4.0 с
		// от note-on, полоса 0.3-20 Гц): G3 3.00 | C4 4.00 | F4 5.40 | G4 6.00 |
		// C5 5.55 | F5 7.35 | G5 6.40 | C6 8.58 Гц. У нас было ПОСТОЯННОЕ
		// 6.00 Гц на всех нотах: на G3 вдвое, на C4 в 1.5 раза быстрее банка
		// (это и слышно как «слишком часто вибрирует»), а на C6 на 30 %
		// медленнее. Внутри зон банка отношение «частота вибрато / f0»
		// постоянно (0.0153 для G3-G4, 0.0106 для C5-F5, 0.0082 для G5-C6) —
		// вибрато записано в семпл и транспонируется вместе с ним, поэтому
		// границы зон видны как изломы кривой. Узлы — ровно замеренные клавиши
		// (G3/C4/F4/G4/C5/F5/G5/C6), x — полутоны от C4.
		static const float vibRateX[] = {
			-5.0f/12.0f, 0.0f, 5.0f/12.0f, 7.0f/12.0f,
			1.0f, 17.0f/12.0f, 19.0f/12.0f, 2.0f};
		// Update 70: вершина возвращена к ЗАМЕРУ банка (8.10 -> 8.50). Владелец:
		// «у C6 тоже редкое, оно в оригинале совсем частое», а замер даёт в банке
		// ровно 8.50 Гц — здесь просьба и замер согласны (в Update 68 срез был
		// сделан по двусмысленному «чуть чаще»).
		static const float vibRateV[] = {
			3.90f, 5.00f, 5.55f, 5.95f, 1.86208714f, 7.10f, 6.30f, 8.50f};
		const float rf = Math::Clamp(Math::Log(freq/261.63f)/Math::Log(2.0f),
			vibRateX[0], vibRateX[7]);
		r.Frequency = MixParam(vibRateX, vibRateV, 8, rf);
		// Update 70: ФОРМА модуляции. Владелец: «не слышу тремоло на C4 или оно
		// слишком редкое; у C6 тоже редкое, оно в оригинале совсем частое».
		// Замер `mod` (полоса 2-60 Гц, окно 0.9-4.2 с от note-on) показал, что
		// модулятор банка — НЕ синус: рядом с основной стоят её гармоники
		// сравнимой амплитуды (в скобках — номер гармоники и отношение амплитуд
		// к основной):
		//   G3 3.03 + 9.08 (3×0.82) + 15.14 (5×0.79)
		//   C4 4.04 + 12.11 (3×1.06) + 20.19 (5×0.89)
		//   F4 5.38 + 16.15 (3×1.10) + 26.92 (5×0.95)
		//   C5 5.55 + 11.02 (2×0.41) + 22.04 (4×0.79)
		//   C6 8.50 + 17.08 (2×0.52)
		// Все высокие линии — ЦЕЛЫЕ кратные основной (3.000 и 5.000 с точностью
		// 0.1 %), причём их ДВА разных: шов петли дал бы одну частоту, поэтому
		// это форма вибрато, запечённого в семпл банка, а не артефакт петли
		// (прежняя догадка Update 68 про «шов» неверна — см. worklog).
		// Отсюда «эффективная скорость» (спектральный центроид модуляции) у
		// банка 13.0…20.3 Гц при основной 3.0…8.5 Гц, а у нас было 6.9…9.1 Гц:
		// при ТОЙ ЖЕ основной частоте импульсная форма читается как заметно
		// более частое тремоло. Поэтому основная частота оставлена замеренной
		// (кривая vibRateV), а добавлены гармоники ФОРМЫ — по зонам семплов
		// (низ: 3-я и 5-я, середина: 2-я и 4-я, верх: 2-я).
		// Update 73: амплитуды формы срезаны ~15 %. Владелец: «сам сустейн
		// похож, правда наш вроде чаще осциллирует» — при той же основной
		// частоте (замер банка не трогаем) воспринимаемая скорость задаётся
		// именно гармониками формы, поэтому мягче форма — спокойнее качание.
		static const float h2V[] = {0, 0, 0, 0, 0.34f, 0.34f, 0.44f, 0.44f};
		static const float h3V[] = {0.85f, 0.85f, 0.85f, 0.85f, 0, 0, 0, 0};
		static const float h4V[] = {0, 0, 0, 0, 0.62f, 0.62f, 0, 0};
		static const float h5V[] = {0.70f, 0.70f, 0.70f, 0.70f, 0, 0, 0, 0};
		const float h2 = MixParam(vibRateX, h2V, 8, rf);
		const float h3 = MixParam(vibRateX, h3V, 8, rf);
		const float h4 = MixParam(vibRateX, h4V, 8, rf);
		const float h5 = MixParam(vibRateX, h5V, 8, rf);
		r.Harm2 = h2; r.Harm3 = h3; r.Harm4 = h4; r.Harm5 = h5;
		// Движок делит форму на (1+Σh) — компенсируем RMS-глубину, чтобы
		// форма меняла только ЧАСТОТУ восприятия, а не глубину (её подбирали
		// отдельно по замеру).
		const float k = (1.0f + h2 + h3 + h4 + h5)
			/Math::Sqrt(1.0f + h2*h2 + h3*h3 + h4*h4 + h5*h5);
		r.Value     = (0.0058f - 0.0020f*x)*k;   // ±10.0 (C4) … ±6.6 (C6) цента по RMS
		r.Tremolo   = 0.10f*k;                   // ±0.9 дБ по RMS — «тремоло тембра»
		// Update 68: 0.50 -> 0.20. Замер широкой полосы 300-1500 Гц, блоки 0.5 с:
		// у нас глубина гуляла 0.59…1.27 дБ (размах 2.2×) и в провалах уходила
		// НИЖЕ порога слышимости — отсюда «на C4 тремоло не слышу вообще», при
		// том что у банка она ровная (0.81-0.83 дБ, размах 1.5×). Множитель
		// глубины при Jitter 0.5 идёт 0.5…1.5 (размах 3×), при 0.2 — 0.8…1.2.
		r.Jitter    = 0.20f;
		// Update 61: задержка/вход оставлены как в v60 (0.35/0.30). Ранний вход
		// (0.22/0.22) ронял уровень атаки на 3-5 дБ: замер 75:72 по 1-мс rms
		// (click-zoom) давал 0.32-0.46 с на −28…−29 дБ вместо −24…−25, а
		// atk-bands — −6.6 дБ против банка в окне 120-250 мс. Причина —
		// когерентное сложение тела с гребёнкой дыхания: пока модуляция не
		// тронута, шумовая юбка стоит на тех же бинах, что и гармоника, и
		// складывается с ней по амплитуде; FM разносит её фазово, сложение
		// становится мощностным (до −3 дБ при равных вкладах). В сустейне
		// дыхание мало, поэтому там вибрато уровня не роняет.
		r.Delay     = 0.45f;
		r.Ramp      = 0.35f;
		return r;
	}

	// Update 66: АМ дыхания (Update 65) УБРАНА. Владелец: «какая-то неприятная
	// периодичность воздуха ... с каким-то неприятным подтоном, может алиасинг?
	// у оригинала всё ровно, только тремоло основного тембра, воздух ровный».
	// АМ на ГРЕБЁНКУ дыхания даёт боковые полосы f0±4 Гц вокруг каждой
	// гармоники — это и слышно как периодический «подтон» в воздухе. Воздух
	// снова статичен (LFO не задаём вовсе), а движение отдано ТОНУ.

	// Вибрато флейт Titanic (43/115) — ОДНО на все слои ноты (тело, дыхание,
	// блум), как у пан-флейты (Update 59). Глубина — как у FluteClean после
	// среза Update 42 (0.6× замера банка, rms ≈ 5.0/7.1/11.4 ц против банковских
	// 7.2/6.6/10.1). У FluteHybrid глубина была в 1.6× больше (копипаст до
	// среза) — владелец слышал это как «неприятную вибрирующую расстройку».
	Vibrato FluteTitanicVibrato(float freq)
	{
		const float x = Math::Clamp(Math::Log(freq/261.63f)/Math::Log(2.0f), 0.0f, 2.0f);
		Vibrato r;
		// Update 61: глубина приведена к замеру банка. hnr-audit (полоса h1,
		// 0.8-4.2 с): банк 21.6/22.0/23.1/24.6/25.3 цента rms (клавиши
		// 55/58/62/67/72) — то есть «вибрирующая расстройка», которую владелец
		// слышит в оригинале, это его собственная глубокая модуляция, а наша
		// была в 4 раза мельче (5.0-7.1). Берём 0.75 от замера банка, чтобы
		// расстройка не вернулась как «неприятная»: ±19.8 (C4) → ±24.1 (C5) →
		// ±32.7 (C6) цента. Частота растёт с регистром (у банка 4.4→7.3 Гц),
		// вход раньше: у банка вибрато слышно уже к 0.25 с.
		// Update 64: владелец — «завывание привидения… в оригинале основной
		// компонент ровный и не вибрирующий, но есть и вибрирующая часть, а у
		// нас вибрирует всё, особенно на C6». Замер .scratch/hvib.mjs (полоса
		// h1, полосовой детектор ФМ + АМ, окно сустейна): у банка ЧИСТОЙ ЧМ-линии
		// нет (значения в шуме), зато есть КОГЕРЕНТНАЯ АМ 2.75 дБ @ 3.5 Гц
		// (43:60) и 3.43 дБ @ 7.07 Гц (43:72), а у нас АМ 0.01 дБ при ЧМ
		// ±21…±33 цента. «Вибрирующая часть» оригинала — это ДЫХАНИЕ, а не
		// высота: ЧМ срезана в 2.9 раза (тон почти ровный, ±6.9 → ±11.5 цента,
		// «завывание» уходит), а АМ того же LFO отдана слою дыхания (Tremolo).
		// Update 68: было 4.3 + 0.85x (C4 4.3, C5 5.15, C6 6.0). Владелец:
		// «на C4 не слышу тремоло вообще, на C5/C6 вроде норм, но чуть чаще
		// оригинала». Кривая сжата и поднята снизу: 4.95 + 0.25x (C4 4.95,
		// C5 5.20, C6 5.45). Ниже ~5 Гц модуляция читается как медленное
		// качание, а не тремоло — это и есть «не слышу».
		// Update 73: 4.95 -> 4.60. Владелец: «тремоло чуть чаще, чем надо».
		// Update 74: 4.60 -> 4.15. Владелец: «У нас сейчас тремоло чуть чаще,
		// чем у банка». Замечание верное и вот почему: замеры Update 73 шли по
		// пресету 43 (Contra Bass!), и «среднее 3.7» было случайно похоже.
		// По верному эталону (пресет 73) `trem --wide 300,4000` (блоки 0.5 с)
		// даёт 3.36-4.04 Гц и на C4, и на C5; у нас — жёсткие 4.71.
		r.Frequency = 4.15f + 0.25f*x;
		r.Value     = x <= 1.0f ? 0.0040f + (0.0049f - 0.0040f)*x : 0.0049f + (0.00665f - 0.0049f)*(x - 1.0f);
		// Update 65: владелец — «тремоло слишком глубокое по сравнению с
		// оригиналом и почти сразу начинается, не хватает раскачки». Замер
		// .scratch-зондом, перенесённым в CLI (trem 43:60/43:72, блоки 0.25 с):
		// у нас АМ стояла ровными 2.5 дБ rms / 3.6 дБ когерентно уже с 0.4 с,
		// а у банка она РАЗГОНЯЕТСЯ — 1.0-1.6 дБ rms в 1.0-1.25 с, 3.6-4.9 в
		// 0.5-0.9 и 1.5-2.25 с, пик 6.8 dB на 0.75 с. Поэтому глубина срезана
		// (0.35 → 0.22) и вход отодвинут: полная к ~0.85 с вместо 0.35 с.
		// Update 66: владелец — «тремоло всё ещё слишком глубокое, у оригинала
		// мягче ... может оно быть другой формы, например не синусом?».
		// Замер широкой полосы 300-4000 Гц (2 с): глубина у нас и у банка
		// сравнима (rms 1.17-1.53 против 1.41-1.44), но ИНДЕКС ТОНАЛЬНОСТИ
		// (когерентный пик / (rms·√2)) у нас 0.98-1.01 — чистый синус, —
		// а у банка 0.66-0.70, и его когерентная частота гуляет 2.7-6.1 Гц.
		// Поэтому срезана не только глубина (0.22 → 0.13), но и добавлено
		// дрожание: жёсткий синус фиксированной глубины и звучит «глубже».
		// Update 68b: 0.13 -> 0.18. Замер широкой полосы 300-4000 Гц, блоки
		// 0.5 с: у банка глубина 1.12…1.71 дБ rms (в среднем 1.42) и НИКОГДА
		// не падает ниже 1.1; у нас при 0.13 было 0.75…1.02. 0.18 даёт
		// ≈1.2 дБ rms и когерентный пик ≈2.0 дБ — внутри банковского 1.6-2.6.
		// Update 69: 0.18 -> 0.22. Владелец: «тремоло так и не чувствуется,
		// не хватает глубины, как минимум на C4». Замер (широкая полоса
		// 300-4000 Гц, блоки 0.5 с, C4): банк 1.42 дБ rms, у нас при 0.18 —
		// 1.16. 0.22 даёт ≈1.4 дБ rms, то есть ровно банковский уровень.
		// Update 73: 0.22 -> 0.16. Владелец: «тремоло очень глубокое». Замер
		// `trem --wide 300,4000` даёт по rms 1.28-1.68 дБ против банковских
		// 1.12-1.71 — то есть глубина по полосе СОВПАДАЛА, но наш модулятор —
		// чистый синус (индекс тональности 0.98 против 0.66 у банка), и
		// боковые линии стоят ровно на гармониках (дыхание с гребёнкой), отчего
		// качается сам тон. 0.16 — шаг вниз по просьбе владельца.
		r.Tremolo   = 0.16f;                     // ±16 % ≈ +1.3/−1.5 дБ у дыхания
		// Update 68: 0.60 -> 0.20, как у пан-флейты. Замер широкой полосы
		// 300-4000 Гц, блоки 0.5 с: у нас глубина гуляла 0.53…1.42 дБ
		// (размах 2.7×), у банка 1.12…1.71 (1.5×) и НИКОГДА не падала ниже
		// 1.1 дБ. В провалах (Jitter 0.6 даёт множитель 0.4…1.6) тремоло
		// пропадало совсем — владелец: «на C4 не слышу тремоло вообще».
		r.Jitter    = 0.20f;
		r.Delay     = x <= 1.0f ? 0.42f - 0.03f*x : 0.39f - 0.03f*(x - 1.0f);
		r.Ramp      = x <= 1.0f ? 0.45f - 0.03f*x : 0.42f - 0.03f*(x - 1.0f);
		return r;
	}

	// Вибрато блокфлейты (Update 55/58h) — тоже одно на тело и дыхание.
	Vibrato RecorderVibrato(float freq)
	{
		const float u = Math::Clamp(Math::Log(freq/523.25f)/Math::Log(2.0f), -1.0f, 1.0f)*0.5f + 0.5f;
		Vibrato r;
		// Update 61: глубина ×1.46 (владелец: «вибрато по частоте в 2 раза
		// уступает оригиналу»). Замер hnr-audit (0.8-4.2 с) даёт у нас и у банка
		// одинаковые rms 7.7-9.2 цента и одну и ту же частоту 4.89 Гц, то есть
		// удвоения нет — но прежние ±10.9 цента это ровно тот случай, когда
		// вибрато читается как дрожание, а не как игра: ±15.9 … ±18.6 цента.
		// Update 62: вибрато приходит ПОСЛЕ раскачки (владелец: «оно такое должно
		// быть не сразу, а после раскачки»). Замер .scratch/vib-onset.mjs (rms FM
		// полосы h1 блоками 0.26 с): у банка до 0.35-0.4 с глубина стоит на полу
		// измерения (1.4-2.6 цента), рост идёт в 0.4-0.65 с, полная — к 0.6-0.7 с
		// на всех проверенных клавишах (62/72/84). У нас при 0.15+0.12 вибрато
		// было полным уже к 0.27 с — отсюда «не сразу, а после раскачки».
		// Вход 0.30, рамп 0.30 → полная глубина к 0.60 с. Глубина и частота
		// (Update 61) не тронуты: владелец принял их как совпадающие.
		// Update 63: владелец — «на C6 частота вибрато в сустейне
		// недостаточна». Замер hnr-audit: у банка частота вибрато 4.89 Гц на
		// ВСЕХ клавишах (60/72/84/86), но глубина ровная (rms 8.5/8.3/8.1 цента),
		// а у нас она растёт с регистром (±15.9 → ±18.6, rms 11.2 → 12.9):
		// глубокое медленное качание на C6 читается как «более редкое». Выше C5
		// частота поднята до 5.90 Гц, глубина к банковской (×0.68 на C6).
		const float t6 = Math::Clamp((u - 0.5f)*2.0f, 0.0f, 1.0f);   // 0 = C5, 1 = C6
		r.Frequency = 4.78f + 1.12f*t6;
		r.Value     = (0.0092f + 0.0016f*u)*(1.0f - 0.32f*t6);
		r.Delay     = 0.30f;
		r.Ramp      = 0.30f;
		return r;
	}

	noinline Array<HarmonicSet> Sets(HarmonicSet a)
	{
		Array<HarmonicSet> r;
		r.AddLast(Move(a));
		return r;
	}

	noinline Array<HarmonicSet> Sets(HarmonicSet a, HarmonicSet b)
	{
		Array<HarmonicSet> r;
		r.AddLast(Move(a));
		r.AddLast(Move(b));
		return r;
	}

	noinline Array<HarmonicSet> Sets(HarmonicSet a, HarmonicSet b, HarmonicSet c)
	{
		Array<HarmonicSet> r;
		r.AddLast(Move(a));
		r.AddLast(Move(b));
		r.AddLast(Move(c));
		return r;
	}

	noinline WaveTableInstrument Wt(WaveTableCache* tables, float volume, float exp, const EnvelopeDesc& env,
		float vibFreq = 0, float vibVal = 0)
	{
		WaveTableInstrument wt;
		wt.Tables = tables;
		wt.VolumeScale = volume;
		wt.ExpCoeff = exp;
		wt.Envelope = MakeEnvelope(env);
		wt.VibratoFrequency = vibFreq;
		wt.VibratoValue = vibVal;
		return wt;
	}

	// Линейная смесь профилей по зонам («ступенька» смены семпла); x — октавы над C4.
	noinline HarmonicSet MixZoneSets(const HarmonicSet* const* sets, const float* xs, size_t n, float x)
	{
		if(x <= xs[0]) return *sets[0];
		if(x >= xs[n - 1]) return *sets[n - 1];
		size_t s = 0;
		while(s + 1 < n && x > xs[s + 1]) s++;
		const float u = (x - xs[s])/(xs[s + 1] - xs[s]);
		HarmonicSet mix;
		mix.VolumeScale = sets[s]->VolumeScale + (sets[s + 1]->VolumeScale - sets[s]->VolumeScale)*u;
		const size_t harmonicCount = Math::Max(sets[s]->Harmonics.Length(), sets[s + 1]->Harmonics.Length());
		for(size_t i = 0; i < harmonicCount; i++)
		{
			const float av = i < sets[s]->Harmonics.Length() ? sets[s]->Harmonics[i].Amplitude : 0;
			const float bv = i < sets[s + 1]->Harmonics.Length() ? sets[s + 1]->Harmonics[i].Amplitude : 0;
			// Update 51: ширина тоже интерполируется (было жёстко 0 — юбки
			// пан-флейты не доезжали до зоны g4la на C5+).
			const float abw = i < sets[s]->Harmonics.Length() ? sets[s]->Harmonics[i].Bandwidth : 0;
			const float bbw = i < sets[s + 1]->Harmonics.Length() ? sets[s + 1]->Harmonics[i].Bandwidth : 0;
			mix.Harmonics.AddLast({av + (bv - av)*u, float(i + 1), abw + (bbw - abw)*u});
		}
		return mix;
	}

	// Кусочно-линейная интерполяция скаляра по точкам MixZoneSets (регистровые профили).
	noinline float MixParam(const float* xs, const float* vals, size_t n, float x)
	{
		if(x <= xs[0]) return vals[0];
		if(x >= xs[n - 1]) return vals[n - 1];
		size_t s = 0;
		while(s + 1 < n && x > xs[s + 1]) s++;
		const float u = (x - xs[s])/(xs[s + 1] - xs[s]);
		return vals[s] + (vals[s + 1] - vals[s])*u;
	}

	// Один гармонический ряд вейвтаблицы. Num — uint8 (LTO и так всё сворачивает).
	struct WtSeriesSpec
	{
		uint8 Num;
		HarmonicFunc Bw, Amp, Fm;
		const ResonanceDesc* Resonances;
		uint8 NumResonances;
		bool Multiplicative;
	};

	// Вейвтабличный инструмент: Reuse != null => таблица из Tables[Reuse], иначе из Series[].
	struct WtSpec
	{
		const char* Name;
		const char* Reuse;
		size_t TableSize;
		WtSeriesSpec Series[3];
		float Volume, ExpCoeff, VibFreq, VibVal;
		EnvelopeDesc Env;
	};
}

InstrumentLibrary::~InstrumentLibrary() {}

InstrumentLibrary::InstrumentLibrary()
{
	// === Струнные (Karplus-Strong) ===
	// GaussianStringSampler не взошёл (теряет фундаментал, медленнее KS); гитары на KS.
	{
		auto& g = Instruments["AcousticGuitarNylon"];
		g.GenericInstruments.EmplaceLast(KarplusStrongInstrument{0.15f, 0.15f, 0.8f, 0.5f, 0.55f, 1.5f});
		g.Envelope = MakeEnvelope({0, 0, 1, 0.7f, 1.5f, false, true});
	}
	{
		auto& g = Instruments["AcousticGuitarSteel"];
		g.GenericInstruments.EmplaceLast(KarplusStrongInstrument{0.05f, 0.25f, 0.8f, 0.5f, 0.4f, 1.0f});
		g.Envelope = MakeEnvelope({0, 0, 1, 0.7f, 1, false, true});
	}
	// === Фортепиано: аддитивная модель (SIMD SineRange), партиалы из PianoRegions.h ===
	// Параметры: Brightness, MaxPartials, Scale, Decay*, DetuneCents, Unison, VelBrightness, TrebleTilt.
	{
		// Громкость 2026-08-28; без ADSR (AdditiveSampler сам гасит ноту).
		auto& g = Instruments["AcousticPiano"];
		g.GenericInstruments.EmplaceLast([](){
				// Партиалы/атака/унисон (Detune 1.4) — из SF2-семпла.
				return AdditivePianoInstrument{0.25f, 40, 0.9f, 1.0f, 0.0f, 1.4f, 2, 0.0f, 0.0f, 2.18524432f, 1.0f, 0, 0.0f, 0};
			}());
	}
	{
		auto& g = Instruments["BrightAcousticPiano"];
		g.GenericInstruments.EmplaceLast([](){
			// Ярче; BeatScale=0 (региональный профиль давал AM 15-40 дБ, 2026-08-29).
			return AdditivePianoInstrument{0.25f, 40, 0.9f, 1.0f, 0.0f, 1.4f, 2, 0.0f, 0.0f, 2.28823171f, 1.0f, 0, 0.0f, 1};
		}());
		g.Envelope = MakeEnvelope({0, 0, 1, 0.5f, 0, false, true});
	}
	{
		auto& g = Instruments["ElectricGrandPiano"];
		g.GenericInstruments.EmplaceLast([](){
			// CP-80: BeatScale=0, TableId=XP50, Brightness 0.40 + DecayStiffness 0.02
			// (калибровки 2026-08-31..09-04, см. ворклог).
			return AdditivePianoInstrument{0.40f, 40, 0.42f, 0.8f, 0.02f, 2.5f, 2, 0.1f, 0.0f, 1.05803492f, 0.0f, PianoTableElectricGrand, 0.0f, 2};
		}());
		g.Envelope = MakeEnvelope({0, 0, 1, 0.9f, 0, false, true});
	}
	{
		auto& g = Instruments["HonkyTonkPiano"];
		g.GenericInstruments.EmplaceLast([](){
			// Honky-tonk: BeatScale=1 (характер в басу G2/требли C5+, середина
			// плоская), TableId=HonkyTonk (в Titanic алиасит acoustic, 2026-08-30).
			return AdditivePianoInstrument{0.3f, 40, 0.9f, 1.0f, 0.0f, 9.0f, 3, 0.4f, 0.0f, 1.88473372f, 1.0f, PianoTableHonkyTonk, 0.0f, 3};
		}());
		g.Envelope = MakeEnvelope({0, 0, 1, 0.5f, 0, false, true});
	}
	{
		auto& g = Instruments["ElectricPiano1"];
		g.GenericInstruments.EmplaceLast([](){
			// Родс EVP73: BeatScale=0, TableId=EP1; VolumeScale откалиброван относительно AGP по Titanic.
			return AdditivePianoInstrument{0.55f, 40, 0.42f, 0.7f, 0.0f, 0.8f, 2, 0.05f, 0.0f, 0.665196568f, 0.0f, PianoTableElectricPiano1, 0.0f, 4};
		}());
		g.Envelope = MakeEnvelope({0, 0, 1, 1.0f, 0, false, true});
	}	{
 		auto& g = Instruments["ElectricPiano2"];
 		g.GenericInstruments.EmplaceLast([](){
			// DX7 EP2: TableId=EP2 из слышимого слоя "* Soft" (14 зон, 2026-09-04),
			// 2 струны BeatCents=6.0 (биения ~2.2 Гц как в семпле); VolumeScale — линейная калибровка.
			return AdditivePianoInstrument{0.6f, 40, 0.42f, 0.6f, 0.0f, 6.0f, 2, 0.05f, 0.0f, 1.5324983f, 1.0f, PianoTableElectricPiano2, 6.0f, 5};
 		}());
 		g.Envelope = MakeEnvelope({0, 0, 1, 1.2f, 0, false, true});
	}
	{
		auto& g = Instruments["Harpsichord"];
		g.GenericInstruments.EmplaceLast([](){
			// Клавесин: TableId=Harpsichord 8'I (9 регионов), линейный VolumeScale.
			return AdditivePianoInstrument{0.5f, 40, 0.4f, 1.6f, 0.012f, 0.0f, 1, 0.0f, 0.0f, 2.3850638f, 1.0f, PianoTableHarpsichord, 0.0f, 6};
		}());
		g.Envelope = MakeEnvelope({0, 0, 1, 0.25f, 0, false, true});
	}
	{
		auto& g = Instruments["Clavinet"];
		g.GenericInstruments.EmplaceLast([](){
			// Клавинет: TableId=Clavinet (11 регионов, короткие семплы), линейный VolumeScale.
			return AdditivePianoInstrument{0.65f, 40, 0.4f, 2.8f, 0.015f, 0.6f, 2, 0.2f, 0.0f, 2.45244909f, 1.0f, PianoTableClavinet, 0.0f, 7};
		}());
		g.Envelope = MakeEnvelope({0, 0, 1, 0.15f, 0, false, true});
	}
	{
		auto& g = Instruments["ElectricGuitarJazz"];
		g.GenericInstruments.EmplaceLast(KarplusStrongInstrument{0.25f, 0.4f, 0.7f, 0.2f, 0.85f, 1.0f});
		g.Envelope = MakeEnvelope({0, 0, 1, 0.1f, 1, false, true});
	}
	{
		auto& g = Instruments["ElectricGuitarClean"];
		g.GenericInstruments.EmplaceLast(KarplusStrongInstrument{0.015f, 0.35f, 0.8f, 0.4f, 0.2f, 1.5f});
		g.Envelope = MakeEnvelope({0, 0, 1, 0.1f, 1.5f, false, true});
	}
	{
		auto& g = Instruments["ElectricGuitarMuted"];
		g.GenericInstruments.EmplaceLast(KarplusStrongInstrument{0.2f, 0.3f, 0.7f, 0.2f, 0.7f, 1.0f});
		g.Envelope = MakeEnvelope({0, 0, 1, 0.2f, 1, false, true});
	}
	{
		auto& g = Instruments["Sitar"];
		g.GenericInstruments.EmplaceLast(KarplusStrongInstrument{0.02f, 0.12f, 0.8f, 0.4f, 0.35f, 0.2f});
		g.Envelope = MakeEnvelope({0.003f, 0, 1, 0.4f, 0.2f, false, false});
	}
	{
		auto& g = Instruments["AcousticBass"];
		g.GenericInstruments.EmplaceLast(KarplusStrongInstrument{0.03f, 0.15f, 0.8f, 0.5f, 0.2f, 1.5f});
		g.Envelope = MakeEnvelope({0, 0, 1, 0.3f, 1.5f, false, true});
	}

	// >>> ГИТАРЫ: НАЧАЛО (сгенерировано guitar-apply.mjs) <<<
	// === Гитары с перегрузом (29) и дисторшном (30): МОДАЛЬНАЯ СТРУНА С ЗАТРАВКОЙ ОТ GUITAR STEEL (Updates 106-111) ===
	// Струна — сумма мод (SpectralStringSampler), но щипок и закон потерь —
	// ТЕ ЖЕ, что у KarplusStrongSampler (затравка AcousticGuitarSteel): форма
	// струны с шумом медиатора и |H(2πf_k/f_s)| однополюсной петли KS,
	// применённый к амплитудам гармоник. Поэтому в линейном режиме (Drive = 0)
	// струна звучит как наша steel-гитара (совпадение с KS проверено:
	// .scratch/diag106-modes.mjs — h2..h11 в пределах 1-2 дБ, огибающая 0.1 дБ).
	// ЭТАП 2 (Update 108) — ТРАКТ УСИЛИТЕЛЯ ПОВЕРХ ЭТОЙ ЖЕ СТРУНЫ.
	// Стадия 1 (Update 107: всё выключено, струна = guitar steel) подтверждена
	// владельцем на слух, поэтому струна НЕ менялась, а включены звенья тракта:
	// датчик (Pickup/PickupDepthMin/PickupHz) → клипер (Drive/Bias) → кабинет
	// (ToneCutoff/CabQ). Перегруз — нелинейность ПОСЛЕ струны, как в усилителе:
	// клипер сам создаёт гармоники и интермодуляцию, которых в спектре струны не
	// было, и заполняет провалы комба. Уровень в клипер даёт velocity (сила
	// удара), поэтому тембр зависит от велосити сам, без слоёв: слабый удар не
	// доходит до клипа (чистый звук), сильный режет. Именно поэтому профиль банка
	// (на E2 h2/h3 выше фундаментала на 15-30 дБ, на C4 фундаментал выше всех
	// гармоник на 25-45 дБ) — это РАЗНЫЙ ТЕМБР ПО ВЫСОТЕ, и это замерено, а не
	// предположено: прогон банка на велоситетах 40/70/100/127 (зонд
	// .scratch/bank-velocity-u120.mjs) даёт ОДИН И ТОТ ЖЕ ряд гармоник, меняется
	// только абсолютный уровень, то есть слоёв велосити у этих пресетов нет.
	// Прежнее объяснение «разные слоёвки велосити» (Updates 106-119) — ОШИБКА.
	// Следствие: дефицит среднего верха на низах — у самой струны (печатный
	// steel тоже держит сильный фундаментал), а не артефакт сравнения.
	// Нормировка спектра — по RMS слышимых мод (моды выше
	// Найквиста ноты зануляются), поэтому Scale — единый уровень всех нот.
	//
	// ШУМА в струне НЕТ (Update 101): отдельного щипок-шума (и слоя, и впрыска в
	// тракт) не существует — владелец забраковал его дважды. Шум в модели —
	// только часть затравки KS (шум медиатора в её возбуждении).
	//
	// Числа (таблиц гармоник нет): затравка — числа steel-гитары (damping,
	// sfBase/SfMul/SfExp — общая с KS формула петли), доводка — expCoeff (общий
	// спад) и rateCap (предел скорости потерь; 0 = закон KS без предела), тракт —
	// pickup/pickupHz/pickupDepth/pickupAperture (комб датчика ПО МОДАМ, полоса до
	// клипера, глубина нулей и апертура катушки), перегруз — drive/bias/tone/cabQ, уровень —
	// scale, нарастание — attack.
	//
	// РЕГИОНЫ ПО КЛАВИШАМ (Update 109). Шесть чисел тракта — pickup, scale,
	// pickupHz, drive, bias, tone — заданы НА ТРЁХ ЯКОРЯХ 40/64/88,
	// которые SpectralRegionValue считает от C0 и потому попадают на РЕАЛЬНЫЕ
	// клавиши 52/76/100 (E3/E5/E7), а не E2/E4/E6 (Update 120, см. его комментарий).
	// Интерполируются по номеру клавиши.
	// Почему: у банка низ и верх звучат разными трактами, и одной цепочкой это
	// не описывается (замер .scratch/guitar-diag109-bands.mjs: в атаке E2 у банка
	// нет ничего выше 800 Гц, а у нас вся энергия была в 1.6-3.2 кГц — «резкий
	// щипок»; на C6 наоборот, банк ярче нас). Интерполяция считается один раз
	// при создании голоса, в рендере региональных веток нет.
	// КОМБ ДАТЧИКА — ПО МОДАМ, С АПЕРТУРОЙ (Update 115). Раньше он был свёрткой
	// во временной области с глубиной 1 на C4 и ниже: при q = 0.5 (все клавиши
	// выше A#4) нули ложились точно на чётные гармоники и были бесконечными —
	// спектр становился «только нечётные», то есть прямоугольной волной.
	// Владелец: «в начале всех нот неприятное дребезжание или явный прямоугольник».
	// Замер (.scratch/guitar-harm115e.mjs): у нас на C4 чётные на 13-16 дБ ниже
	// нечётных (нечётность +10.4 дБ), у банка Titanic — −1.2 дБ, спектр ровный.
	// Сейчас глубина нулей кончится апертурой катушки: depth_k =
	// PickupDepth·sinc(π·Aperture·k) — высокие моды нулей не видят, спектр
	// заполняется. И это дешевле: нет свёртки на mN операций при создании голоса.
	// ДАТЧИК ОБЯЗАТЕЛЕН, хотя перебор по цене его выключает (цена 14.8 против
	// 17.1: без наклона +6 дБ/окт спектр темнее и ближе к измеренному профилю
	// банка). Цена сравнивает спектры, а не узнаваемость инструмента: без датчика
	// звук читается акустической гитарой — это и есть забракованное владельцем.
	// PickupHz (полоса ДО клипера) нужен ещё и против алиасинга: без него продукты
	// нелинейности уходят выше Найквиста и возвращаются — на слух владельца это
	// «очень яркий шум в спектре». Bias мал (0.08/0.15): у банка чётные гармоники
	// не доминируют, а bias 0.25 давал h2 на C4 на 19 дБ ярче банка («Overdriven C4
	// слишком яркая»).
	// ATTACK = 0 (Update 128). Было 0.02 (20 мс рампа — «нарастание банка 15-25 мс»),
	// и это оказалось главным отличием атаки от guitar steel: у программы 25 рампа НЕ
	// берётся (StartsAtFull), у 29/30 рампа 20 мс срезала щипок — владелец: «Атака не похожа
	// на guitar steel. Где щипок? …какая-то вата дребезжащая». Замер онсета
	// (.scratch/u128-onset.mjs): 29 steel — 50 % огибающей на 5-7 мс, 90 % на 10 мс;
	// 25 steel — 50/90 % на 0 мс (мгновенный старт). Attack 0 возвращает мгновенный старт.
	//
	// Почему rateCap = 0. Предел скорости потерь держал верхние гармоники живыми,
	// выравнивая их скорости по ОДНОЙ величине: спектр перестаёт меняться, форма
	// периода застывает, и нота превращается в незатухающую стационарную волну
	// («прямоугольная волна, которая не гаснет», Update 106). В зонде это ловит
	// крест-фактор (пик/RMS) по окнам: у замороженного спектра он высок и не падает.
	//
	//
	// СУСТЕЙН И РЕЖИМ КЛИПЕРА (Update 111). Замер собранной сборки показал две
	// причины того, что гитары читались как guitar steel, а не как перегруз:
	// у Overdrive клипер работал только первые 0.3 с (доля отсчётов за порогом
	// |x·Drive| > 1: 23 % → 0 % → 0 % → 0 % на 0.05/0.3/0.9/1.5 с), а нота умирала
	// на высоких клавишах: закон KS даёт скорость потерь ∝ f₀, поэтому на C6
	// фундаментал терял 26 дБ/с (29 на C6: −59 дБ к 2.5 с против банковских −21).
	// Drive поднят (у 29 0.9-1.6 → 1.8-3.2), а сустейн дан SfBase (0.25 → 0.06/0.12:
	// он масштабирует именно потери петли ∝ f₀ и щипок НЕ трогает — затравка от
	// SfBase не зависит) и ExpCoeff (1.0 → 0.8 ≈ 7 дБ/с общего спада). DampSlope
	// 1.0 → 0.6: на C6 он срезал весь верх (h4 терял 36 дБ/с), а банк там ярче нас.
	//
	// ЧТО ОСТАЛОСЬ: форма периода заморожена (моды стоят ровно на k·f₀ с
	// постоянными фазами — меняются только амплитуды), поэтому нота «просто
	// затухает», а не «переливается»: у реальной струны гармоники некратны и их
	// фазы расходятся. Предклиперного среза низа тоже нет.
	//
	// Правка — в guitar-modal.json + этот инструмент (руками числа не менять).
	{
		auto& g = Instruments["OverdrivenGuitar"];
		g.GenericInstruments.EmplaceLast(SpectralStringInstrument{
			0.05f, 0.25f, 0.8f, 0.5f,  // Damping (шум медиатора = тембр АТАКИ; у steel 0.05, у jazz 0.25: у 29 как у steel, у 30 смягчён втрое — Update 131) и SfBase/SfMul/SfExp (формула петли KS: sf = SfBase·(0.9 − (note/128)^SfExp·SfMul + rand·0.1))
			1.0f, 0.0f, 0.0f,  // ExpCoeff (общий спад, 1/с; у steel 1.0), RateCap (предел скорости потерь; 0 — чистый KS), DampSlope (трение: доп. потери гармоники в 1/с на кГц её частоты ВЫШЕ основного тона; 0 — чистый KS). СКАЛЯР: региональный вариант проверен в Update 120 — оценку не улучшил, см. .h
			{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, 8,  // TiltExp (E2/E4/E6, наклон датчика в единицах +6 дБ/окт: 1 — скорость струны, меньше — глаже; РЕГИОНАЛЬНЫЙ с Update 122 — низ/середина требуют глаже, верх наоборот), Inharm (E2/E4/E6, жёсткость струны B в f_k = k·f0·√(1+B·k²): 0 — моды строго кратны), InharmPeriods (сколько периодов в буфере; 1 — сетка буфера совпадает с гармониками и сдвига частот нет, 8 — сетка f0/8)
			0.0f, 0.0f,  // BeatSplit (относительная расстройка пары поляризаций: биения моды k идут со скоростью k·BeatSplit·f0), BeatDepth (глубина модуляции амплитуды мод; 0 — выключено)
			0.0f, 0.15f, {1.0f, 1.6f, 4.0f},  // PickupDepth (глубина нулей комба датчика на НИЗКИХ модах; 0 — без комба), PickupAperture (доля длины струны, усредняемая катушкой: depth_k = PickupDepth·sinc(π·Aperture·k) — апертура заполняет нули на высоких модах), SustainMax (E2/E4/E6, предел подъёма уровня на входе клипера по мере затухания струны; 1 — выключено)
			1.15f, {1.0f, 1.0f, 1.0f}, 30.0f,  // CabQ (добротность резонанса кабинета), CabPoles (E2/E4/E6, сколько секций кабинета: 1 — 12 дБ/окт, 2 — каскад 24 дБ/окт; ПОРОГ 1.5 — значение ≥ 1.5 включает вторую секцию, поэтому [1, 1, 2.5] даёт переключение РОВНО на C6. Update 116 включил 2 всюду, Update 123 вернул 1 — верх 10-16 кГц стал темнее банка; Update 141 сделал величину РЕГИОНАРНОЙ: 24 дБ/окт нужны только там, где ряд клипера лежит в 6-14 кГц, а это верхние клавиши), PostHpHz (срез низа ПОСЛЕ клипера, межкаскадный конденсатор; 0 — выключено)
			{0.0f, 0.0f, 0.0f}, {0.0851571f, 0.0650941f, 0.0205776f},  // Pickup (E2/E4/E6, точка датчика в долях длины струны), Scale (E2/E4/E6, уровень после клипера)
			{2080.0f, 2210.0f, 2730.0f}, {1.2f, 1.65f, 1.95f},  // PickupHz (E2/E4/E6, полоса до клипера, Гц), Drive (E2/E4/E6, глубина клипа)
			{0.12f, 0.06f, 0.02f}, {2960.0f, 3520.0f, 7200.0f},  // Bias (E2/E4/E6, асимметрия клипера), ToneCutoff (E2/E4/E6, срез верха после клипа, Гц)
			2500.0f, 12.0f,  // PresenceHz (частота полки тембр-стека ПОСЛЕ кабинета, Гц), PresenceDb (подъём, дБ; 0 — звено выключено). Update 130: подтверждено замером блочным путём, стало каноном
			120.0f, 0.0f, 2778.0f, 1.38f, 0.0f, 8963.0f, 0.0f  // ToneLowHz/Db (полка НИЗА: тише/громче под ней), TonePeakHz/Q/Db (резонансный ПИК присутствия), ToneHighHz/Db (полка ВЕРХА)
		});
		g.Envelope = MakeEnvelope({0, 0, 1, 0.70f, 1, false, true});
	}
	{
		auto& g = Instruments["DistortionGuitar"];
		g.GenericInstruments.EmplaceLast(SpectralStringInstrument{
			0.05f, 0.25f, 0.8f, 0.5f,  // Damping (шум медиатора = тембр АТАКИ; у steel 0.05, у jazz 0.25: у 29 как у steel, у 30 смягчён втрое — Update 131) и SfBase/SfMul/SfExp (формула петли KS: sf = SfBase·(0.9 − (note/128)^SfExp·SfMul + rand·0.1))
			1.0f, 0.0f, 0.0f,  // ExpCoeff (общий спад, 1/с; у steel 1.0), RateCap (предел скорости потерь; 0 — чистый KS), DampSlope (трение: доп. потери гармоники в 1/с на кГц её частоты ВЫШЕ основного тона; 0 — чистый KS). СКАЛЯР: региональный вариант проверен в Update 120 — оценку не улучшил, см. .h
			{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, 8,  // TiltExp (E2/E4/E6, наклон датчика в единицах +6 дБ/окт: 1 — скорость струны, меньше — глаже; РЕГИОНАЛЬНЫЙ с Update 122 — низ/середина требуют глаже, верх наоборот), Inharm (E2/E4/E6, жёсткость струны B в f_k = k·f0·√(1+B·k²): 0 — моды строго кратны), InharmPeriods (сколько периодов в буфере; 1 — сетка буфера совпадает с гармониками и сдвига частот нет, 8 — сетка f0/8)
			0.0f, 0.0f,  // BeatSplit (относительная расстройка пары поляризаций: биения моды k идут со скоростью k·BeatSplit·f0), BeatDepth (глубина модуляции амплитуды мод; 0 — выключено)
			0.0f, 0.15f, {1.0f, 1.2f, 1.6f},  // PickupDepth (глубина нулей комба датчика на НИЗКИХ модах; 0 — без комба), PickupAperture (доля длины струны, усредняемая катушкой: depth_k = PickupDepth·sinc(π·Aperture·k) — апертура заполняет нули на высоких модах), SustainMax (E2/E4/E6, предел подъёма уровня на входе клипера по мере затухания струны; 1 — выключено)
			1.15f, {1.0f, 1.0f, 2.5f}, 30.0f,  // CabQ (добротность резонанса кабинета), CabPoles (E2/E4/E6, сколько секций кабинета: 1 — 12 дБ/окт, 2 — каскад 24 дБ/окт; ПОРОГ 1.5 — значение ≥ 1.5 включает вторую секцию, поэтому [1, 1, 2.5] даёт переключение РОВНО на C6. Update 116 включил 2 всюду, Update 123 вернул 1 — верх 10-16 кГц стал темнее банка; Update 141 сделал величину РЕГИОНАРНОЙ: 24 дБ/окт нужны только там, где ряд клипера лежит в 6-14 кГц, а это верхние клавиши), PostHpHz (срез низа ПОСЛЕ клипера, межкаскадный конденсатор; 0 — выключено)
			{0.0f, 0.0f, 0.0f}, {0.0822262f, 0.0882348f, 0.0253282f},  // Pickup (E2/E4/E6, точка датчика в долях длины струны), Scale (E2/E4/E6, уровень после клипера)
			{2560.0f, 2720.0f, 3360.0f}, {0.9f, 1.2375f, 1.4625f},  // PickupHz (E2/E4/E6, полоса до клипера, Гц), Drive (E2/E4/E6, глубина клипа)
			{0.225f, 0.03f, 0.0f}, {3600.0f, 5400.0f, 3300.0f},  // Bias (E2/E4/E6, асимметрия клипера), ToneCutoff (E2/E4/E6, срез верха после клипа, Гц)
			2500.0f, 12.0f,  // PresenceHz (частота полки тембр-стека ПОСЛЕ кабинета, Гц), PresenceDb (подъём, дБ; 0 — звено выключено). Update 130: подтверждено замером блочным путём, стало каноном
			134.0f, 0.0f, 2063.0f, 1.98f, 0.0f, 7421.0f, 0.0f  // ToneLowHz/Db (полка НИЗА: тише/громче под ней), TonePeakHz/Q/Db (резонансный ПИК присутствия), ToneHighHz/Db (полка ВЕРХА)
		});
		g.Envelope = MakeEnvelope({0, 0, 1, 0.70f, 1, false, true});
	}
	// >>> ГИТАРЫ: КОНЕЦ <<<



	// === Формантные/аддитивные вейвтаблицы ===

	// Повторяющиеся наборы резонансов.
	static const ResonanceDesc choirRes[] = {
		{600, 100, 1.25f}, {900, 175, 1.95f}, {2200, 150, 3.5f}, {2600, 175, 4.4f}, {0, 2000, 7.5f}};
	static const ResonanceDesc voiceRes[] = {
		{600, 100, 1.25f}, {900, 230, 1.95f}, {2200, 840, 3.5f}, {2600, 175, 4.4f}, {0, 2000, 0.75f}};
	static const ResonanceDesc padRes[] = {
		{500, 140, 1}, {900, 540, 4}, {2100, 1400, 10}, {3700, 2100, 15}, {4700, 2800, 20}};
	static const ResonanceDesc str3Res[] = {
		{400, 35, 87}, {900, 115, 195}, {1400, 115, 60}, {2200, 345, 1320}, {3500, 140, 35}, {6000, 2500, 460}, {0, 7, 2}};
	static const ResonanceDesc pizziRes[] = {
		{500, 50, 0.35f}, {800, 315, 2.5f}, {2100, 1400, 10}, {3700, 2100, 15}, {0, 0.1f, 0.001f}};
	static const ResonanceDesc violinRes[] = {
		{400, 85, 8.7f}, {900, 115, 19.5f}, {1400, 115, 6}, {2200, 300, 7}, {3500, 240, 2}, {10000, 4500, 20}};
	static const ResonanceDesc violinOldRes[] = {
		{500, 50, 12.5f}, {800, 315, 76}, {2100, 1400, 420}, {3700, 1750, 440}, {0, 7, 1.75f}};
	static const ResonanceDesc orchRes[] = {
		{275, 700, 175}, {1150, 1400, 420}, {2500, 700, 175}, {4100, 175, 44}, {0, 2100, 525}};
	static const ResonanceDesc callRes[] = {
		{275, 700, 175}, {650, 1400, 420}, {1100, 700, 175}, {2700, 250, 62.5f}, {0, 2100, 52.5f}};
	static const ResonanceDesc trumpetRes[] = {
		{2500, 1000, 500}, {6500, 1000, 100}};
	static const ResonanceDesc accRes[] = {
		{1700, 400, 150}, {4200, 400, 100}, {7300, 700, 15}, {11200, 700, 10}};
	static const ResonanceDesc fluteNewRes[] = {{650, 140, 350}, {1400, 80, 200}};
	static const ResonanceDesc flute2Res[] = {{650, 140, 85}, {1400, 80, 50}};

	// Вейвтаблицы со статическими гармониками (уже data-driven, оставлены как есть).
	{
		static const HarmonicDesc vibraphoneH[] = {{1, 1, 2}, {0.25f, 4, 15}, {0.125f, 8, 25}, {0.0625f, 16, 45}, {0.03125f, 32, 58}};
		auto& t = Tables["Vibraphone"] = CreateWaveTables(Sets(Harms(SpanOf(vibraphoneH))), 16384);
		auto& wt = Instruments["Vibraphone"].WaveTables.EmplaceLast();
		wt = Wt(&t, 0.15f, 2, {0.004f, 0.05f, 0.3f, 0.25f, 2, false, false}, 5, 0.0015f);
	}
	{
		static const HarmonicDesc musicBoxH[] = {{1, 1, 20}, {0.5f, 4, 15}, {0.25f, 8, 15}, {0.125f, 16, 15}, {0.0625f, 32, 15}};
		auto& t = Tables["MusicBox"] = CreateWaveTables(Sets(Harms(SpanOf(musicBoxH))), 16384);
		auto& wt = Instruments["MusicBox"].WaveTables.EmplaceLast();
		wt = Wt(&t, 0.1f, 4, {0.01f, 0.08f, 0.5f, 0.4f, 4, false, false});
	}
	{
		static const HarmonicDesc marimbaH[] = {{1, 1, 8}, {0.25f, 4, 9}, {0.108f, 9.2f, 6}, {0.0835f, 12, 6}, {0.04175f, 24, 6}, {0.021f, 48, 6}};
		auto& t = Tables["Marimba"] = CreateWaveTables(Sets(Harms(SpanOf(marimbaH))), 16384);
		auto& wt = Instruments["Marimba"].WaveTables.EmplaceLast();
		wt = Wt(&t, 0.25f, 5, {0.005f, 0.05f, 0.3f, 0.3f, 5, false, false});
	}
	{
		static const HarmonicDesc xylophoneH[] = {{1, 1, 20}, {0.333f, 3, 60}, {0.108f, 9.2f, 184}, {0.0769f, 13, 260}, {0.033f, 30, 600}};
		auto& t = Tables["Xylophone"] = CreateWaveTables(Sets(Harms(SpanOf(xylophoneH))), 16384);
		auto& wt = Instruments["Xylophone"].WaveTables.EmplaceLast();
		wt = Wt(&t, 0.25f, 5, {0.006f, 0.015f, 0.2f, 0.2f, 5, true, false});
	}
	{
		static const HarmonicDesc newAgeH[] = {{1, 1, 20}, {0.5f, 4, 46}, {0.25f, 8, 94}, {0.125f, 16, 190}, {0.0625f, 32, 380}};
		auto& t = Tables["NewAge"] = CreateWaveTables(Sets(Harms(SpanOf(newAgeH))), 8192);
		auto& wt = Instruments["NewAge"].WaveTables.EmplaceLast();
		wt = Wt(&t, 0.17f, 3, {0.015f, 0.04f, 0.5f, 0.3f, 3, false, false});
	}
	{
		static const HarmonicDesc glockenspielH[] = {{0.33f, 1, 7}, {0.19f, 6.7f, 30}, {0.15f, 6.1f, 40}, {0.12f, 8.4f, 17}, {0.15f, 12.7f, 37}, {0.12f, 23.2f, 28}};
		auto& t = Tables["Glockenspiel"] = CreateWaveTables(Sets(Harms(SpanOf(glockenspielH))), 16384);
		auto& wt = Instruments["Glockenspiel"].WaveTables.EmplaceLast();
		wt = Wt(&t, 0.15f, 8, {0.011f, 0.08f, 0.6f, 0.7f, 8, false, false});
	}
	{
		static const HarmonicDesc clarinetH[] = {{1, 1, 15}, {0.275f, 3, 45}, {0.2f, 5, 55}, {0.1f, 7, 105}, {0.05f, 9, 135}, {0.03f, 11, 165}, {0.08f, 13, 195}};
		auto& t = Tables["Clarinet"] = CreateWaveTables(Sets(Harms(SpanOf(clarinetH))), 16384);
		auto& wt = Instruments["Clarinet"].WaveTables.EmplaceLast();
		wt = Wt(&t, 0.35f, 0, {0.03f, 0.05f, 0.75f, 0.1f, 0, false, false}, 0.5f, 0.005f);
	}
	{
		// Флейта «FluteHybrid» (115, Update 17): атака DLS, тело Titanic (таблица FluteClean), релиз 0.25 с.
		auto& t = Tables["FluteClean"]; // общая таблица — тело Titanic
		auto& wt = Instruments["FluteHybrid"].WaveTables.EmplaceLast();
		wt = Wt(&t, 0.24f, 0, {0.03f, 0, 1, 0.25f, 0, true, false});
		// Атака — как у FluteDLS: подъём 0→1 за ~20-30 мс по регистру; релиз 0.25 с.
		wt.EnvelopeProfile = [](float freq)
		{
			const float x = Math::Clamp(Math::Log(freq/261.63f)/Math::Log(2.0f), 0.0f, 2.0f);
			static const float xs[] = {0.0f, 1.0f, 2.0f};
			static const float d0v[] = {0.003f, 0.004f, 0.003f};
			static const float t1v[] = {0.030f, 0.030f, 0.020f};
			EnvelopeFactory f;
			f.StartVolume = 0;
			f.Segments[0] = {false, 0, MixParam(xs, d0v, 3, x)};
			f.Segments[1] = {false, 1, MixParam(xs, t1v, 3, x)};
			f.Segments[2] = {false, 1, Intra::Infinity};
			f.Segments[3] = {false, 1, Intra::Infinity};
			f.Segments[4] = {true, 0, 0.25f};
			return f;
		};				// Вибрато — Titanic (как у FluteClean): 3.8→3.7→4.35 Гц, ±6.6→±5.2→±10.6 ц,
		// включается через 0.28 с.
		// Update 60: один профиль на все слои ноты (тело/дыхание/блум).
		wt.VibratoProfile = [](float freq) { return FluteTitanicVibrato(freq); };
		// Дуновение — Titanic (Update 28): полоса струи 0.7·f0…2200-350·x Гц, 2 каскада ФНЧ, уровень растёт к C5/C6.
		Instruments["FluteHybrid"].GenericInstruments.EmplaceLast(
			[](float freq, float volume, unsigned sampleRate) -> GenericSamplerRef
			{
				// Update 74: пшик убран (то же, что у FluteClean — числа совпадают:
				// атака 150 мс, полка 0.72, уровень ×0.20/0.72).
				// Update 73: выдох ВСПЫХИВАЕТ В НАЧАЛЕ. Владелец: «нет в атаке выдоха,
				// сразу общий тембр, как в сустейне». Замер `timbre` (окно 60 мс, дБ
				// отн. h1 сустейна, 43:60, банк → наш v72): 600-1500 Гц в 0-60 мс
				// −37.0 → −54.4, 1500-3500 −43.7 → −54.4, а пик у банка в 40 мс
				// против наших 120. У банка выдох гаснет за ~0.2 с с динамикой
				// 17 дБ (от −33.8 до −51), у нас динамика была 5 дБ (1/0.55).
				// Update 72 (атака 100/спад 450/полка 0.55) убрала «низкий пшик»,
				// но вместе с ним и выдох: медленный подъём читается как «сразу
				// тембр сустейна». Теперь атака 8 мс, спад 0.20 с до полки 0.28
				// (пик/полка 11 дБ), уровень ×0.20/0.28 — сустейн не сдвинут.
				static const EnvelopeDesc breathEnv = {0.15f, 0.40f, 0.72f, 0.10f, 0, true, false};
				const float x = Math::Clamp(Math::Log(freq/261.63f)/Math::Log(2.0f), 0.0f, 2.0f);
				static const float lvlX[] = {0.0f, 1.0f, 2.0f};
				// Update 75: ЭТОТ ПОДЪЁМ БЫЛ ПО НЕВЕРНОМУ ЭТАЛОНУ (Update 74).
				// Замер по верному банку (пресет 73) показал, что наш межгармонический
				// пол на 12 дБ выше файла вкладки: `floor 43:60` 1000-1400 мс — наш
				// −54.4 дБ, банк −66.5. Изоляция сборкой с level = 0
				// (/.scratch/wasm-cmp/nonoise) даёт −81.6, то есть весь пол — это
				// наше дыхание, а не грязь вейвтаблицы. Множители снижены по замеру
				// `bands` (400-4800 Гц): C4 −6 дБ, C5 −8 дБ, C6 −3 дБ
				// (2.41/6.46/2.16 → 1.21/2.57/1.53).
				// Update 69: дыхание поднято по замеру bands (было 0.50/1.15/0.45 при
				// 0.0325). Множители взяты из отношения «наш/банк» по полосам
				// 400-4800 Гц: C4 ×4.81 (13.6 дБ), F4 ×4.03 (12.1), C5 ×5.62 (15.0),
				// C6 — как C4 (у банка семпла на C6 нет). Абсолютные уровни стали
				// 0.078/0.131/0.210/0.070 при уровне пан-флейты 0.0855 на C4 — то
				// есть воздух флейты теперь того же порядка, что у пан-флейты
				// (раньше он был в 5.3 раза тише, и тремоло по дыханию не читалось).
				static const float lvlK[] = {1.21f, 2.57f, 1.53f};
								const float level = 0.0325f*(0.20f/0.72f)*MixParam(lvlX, lvlK, 3, x);
				// Форма воздуха — полоса струи: низ режет ФВЧ на ~0.7·f0 (NoiseSampler),
				// верх — плато до ~2.2 кГц (C4) → 1.5 кГц (C6), потом КРУТОЙ спад —
				// второй каскад ФНЧ (12 дБ/окт), у банка выше ~2.5 кГц шума почти нет.
				// Гребёнка на периоде ноты (combGain 0.40) делает воздух ТОНО-коррелированным —
				// «дыхание привязано к гармонике»: на слух это мягкое дуновение, а не шип
				// (Update 31, .scratch/c6-prof.cjs: некоррелированный слой был +6..+10 дБ в атаке).
				// Update 75: ФНЧ воздуха был 2200-350x — по замеру шумового пола банка
				// (пресет 73) он режет рано: у банка воздух ровный до ~6.8 кГц, у нас
				// падал на 5-9 дБ выше 2.2 кГц. 3800-600x даёт C4 3800 / C5 3200 / C6 2600.
				// ФВЧ: 0.90f0 отрезало шум ниже 0.9·f0, у банка он есть и на 134-168 Гц
				// (был ниже на 9 дБ) — 0.55f0.
				const float cutoffHz = 3800.0f - 600.0f*x;
				const float hpHz = Math::Min(0.55f*freq, 1200.0f);
				return new NoiseSampler(freq, volume, sampleRate, 32768, level, 442115003u,
					cutoffHz/freq, true, breathEnv, hpHz/freq, 2, 0.40f, 1u, FluteTitanicVibrato(freq));
			});
	}
	// Атака-блум флейт Titanic (Update 27): вспышка h4/h5 и тёмное начало h2/h3, спад к сустейну.
	// Общий для 43/115; зонная сетка — как у тела.
	{
		static const float bloomFlashC4[12] = {0, 0, 0, 0.370f, 0, 0, 0.012f, 0, 0, 0, 0, 0};
		static const float bloomFlashE4[12] = {0, 0.26f, 0.11f, 0.21f, 0.20f, 0.043f, 0.040f, 0, 0.017f, 0, 0, 0};
		static const float bloomFlashB4[12] = {0, 0.18f, 0.077f, 0.081f, 0.041f, 0.017f, 0, 0, 0, 0, 0, 0};
		static const float bloomFlashD5[12] = {0, 0.13f, 0, 0.091f, 0, 0, 0, 0, 0, 0, 0, 0};
		static const float bloomFlashF5[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		static const float bloomFlashG5[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		static const float bloomFlashC6[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		static const float bloomFlashF6[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		static const float bloomSwellC4[12] = {0, -0.47f, -0.32f, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		static const float bloomSwellE4[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		static const float bloomSwellB4[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		static const float bloomSwellD5[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		static const float bloomSwellF5[12] = {0, -0.10f, -0.025f, -0.016f, 0, 0, 0, 0, 0, 0, 0, 0};
		static const float bloomSwellG5[12] = {0, -0.030f, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		static const float bloomSwellC6[12] = {0, -0.096f, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		static const float bloomSwellF6[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		static const float* const flashZones[] = { bloomFlashC4, bloomFlashE4, bloomFlashE4, bloomFlashB4, bloomFlashB4,
			bloomFlashD5, bloomFlashD5, bloomFlashF5, bloomFlashF5, bloomFlashG5, bloomFlashG5,
			bloomFlashC6, bloomFlashC6, bloomFlashF6};
		static const float* const swellZones[] = {bloomSwellC4, bloomSwellE4, bloomSwellE4, bloomSwellB4, bloomSwellB4,
			bloomSwellD5, bloomSwellD5, bloomSwellF5, bloomSwellF5, bloomSwellG5, bloomSwellG5,
			bloomSwellC6, bloomSwellC6, bloomSwellF6};
		static const float bloomZoneX[] = {0.0f, 0.25f, 0.4167f, 0.5f, 0.9167f,
			1.0f, 1.25f, 1.333f, 1.5f, 1.583f, 1.75f, 1.833f, 2.0f, 2.042f};
		// Тайминг вспышки/всплытия/гейта по регистру (окна замеров банка, C4→C6 быстрее).
		static const float bloomRiseT[] = {0.10f, 0.09f, 0.08f, 0.08f, 0.06f, 0.06f, 0.06f, 0.06f, 0.06f, 0.05f, 0.05f, 0.05f, 0.05f, 0.04f};
		static const float bloomDecayT[] = {0.13f, 0.12f, 0.11f, 0.11f, 0.09f, 0.09f, 0.09f, 0.09f, 0.09f, 0.09f, 0.10f, 0.10f, 0.10f, 0.09f};
		static const float bloomSwellT[] = {0.10f, 0.09f, 0.08f, 0.08f, 0.08f, 0.08f, 0.08f, 0.08f, 0.09f, 0.09f, 0.10f, 0.10f, 0.10f, 0.10f};
		static const float bloomGateT[] = {0.22f, 0.18f, 0.15f, 0.15f, 0.12f, 0.12f, 0.12f, 0.11f, 0.11f, 0.10f, 0.10f, 0.09f, 0.09f, 0.08f};
		static auto Interp12 = [](const float* const* tables, const float* xs, size_t n, float x, float* out)
		{
			size_t s = 0;
			if(x <= xs[0])
			{
				for(int k = 0; k < 12; k++) out[k] = tables[0][k];
				return;
			}
			if(x >= xs[n - 1])
			{
				for(int k = 0; k < 12; k++) out[k] = tables[n - 1][k];
				return;
			}
			while(s + 1 < n && x > xs[s + 1]) s++;
			const float u = (x - xs[s])/(xs[s + 1] - xs[s]);
			for(int k = 0; k < 12; k++) out[k] = tables[s][k] + (tables[s + 1][k] - tables[s][k])*u;
		};
		auto MakeFluteBloom = [](float freq, float volume, unsigned sampleRate) -> GenericSamplerRef
		{
			const float x = Math::Clamp(Math::Log(freq/261.63f)/Math::Log(2.0f), 0.0f, 2.0f);
			float flash[12], swell[12];
			Interp12(flashZones, bloomZoneX, 14, x, flash);
			Interp12(swellZones, bloomZoneX, 14, x, swell);
			return new BloomSampler(freq, volume, sampleRate, SpanOf(flash), 12, SpanOf(swell), 12,
				MixParam(bloomZoneX, bloomRiseT, 14, x), MixParam(bloomZoneX, bloomDecayT, 14, x),
				MixParam(bloomZoneX, bloomSwellT, 14, x), MixParam(bloomZoneX, bloomGateT, 14, x),
				0.24f, FluteTitanicVibrato(freq));
		};
		Instruments["FluteClean"].GenericInstruments.EmplaceLast(MakeFluteBloom);
		Instruments["FluteHybrid"].GenericInstruments.EmplaceLast(MakeFluteBloom);
	}
	{
		// Флейта «FluteClean» (43) — эталон Titanic. Зоны перетюнены по sustain-петле банка
		// (Update 26); дуновение — полоса струи 0.7·f0…2700-400·x Гц (Update 28), релиз 0.50 с.
		// Legacy-блок «Flute» ниже не замаплен (MidiInstrumentMapping).
		static const uint16 fluteCleanC4[] = {32768, 16095, 16065, 4507, 5505, 19890, 2993, 3729, 706, 2082, 848, 355, 568, 733, 599, 395}; // C#4(R): keys < 63 (Update 74: пересчёт по пресету банка 73)
		static const uint16 fluteCleanE4[] = {32768, 50439, 35961, 9604, 34961, 16334, 3242, 1601, 9072, 2331, 487, 1020, 1595, 258, 224, 926}; // E4(R): keys 63-65 (Update 74: пересчёт по пресету 73)
		static const uint16 fluteCleanB4[] = {32768, 21323, 15019, 14662, 10737, 2237, 1303, 1800, 288, 356, 475, 150, 175, 200, 154, 140}; // B4(R): keys 66-71 (Update 74: пересчёт по пресету 73)
		static const uint16 fluteCleanD5[] = {32768, 8417, 32646, 1628, 6051, 656, 1013, 873, 555, 809, 490, 246, 103, 97, 133, 117}; // D5(R): keys 72-75 (Update 74: пересчёт по пресету 73)
		static const uint16 fluteCleanF5[] = {32768, 8506, 6831, 1551, 2724, 592, 448, 254, 157, 137, 109, 167, 221, 269, 82, 73}; // F#5(R): keys 76-78

		static const uint16 fluteCleanG5[] = {32768, 5546, 11689, 1060, 841, 201, 333, 155, 39, 59, 58, 41, 64, 41, 16, 8}; // G#5(R): keys 79-81
		static const uint16 fluteCleanC6[] = {32768, 6121, 6531, 974, 881, 889, 348, 118, 138, 306, 128, 40, 16, 7, 1, 1}; // C6(R): keys 82-84 (Update 74: пересчёт по пресету 73)
		static const uint16 fluteCleanF6[] = {32768, 1594, 3003, 944, 135, 114, 118, 83, 26, 6, 1, 1, 1, 1, 1, 1}; // F#6(R): keys 85+
		static const HarmonicSet setCleanC4 = Harms16(SpanOf(fluteCleanC4), 0.9425f); // выравнивание зон по регрессии уровня регистра
		static const HarmonicSet setCleanE4 = Harms16(SpanOf(fluteCleanE4), 1.0466f); // выравнивание зон по регрессии уровня регистра
		static const HarmonicSet setCleanB4 = Harms16(SpanOf(fluteCleanB4), 1.0821f); // выравнивание зон по регрессии уровня регистра
		static const HarmonicSet setCleanD5 = Harms16(SpanOf(fluteCleanD5), 1.0337f); // выравнивание зон по регрессии уровня регистра
		static const HarmonicSet setCleanF5 = Harms16(SpanOf(fluteCleanF5), 0.9038f); // выравнивание зон по регрессии уровня регистра
		static const HarmonicSet setCleanG5 = Harms16(SpanOf(fluteCleanG5), 0.9728f); // выравнивание зон по регрессии уровня регистра
		static const HarmonicSet setCleanC6 = Harms16(SpanOf(fluteCleanC6), 0.9650f); // выравнивание зон по регрессии уровня регистра
		static const HarmonicSet setCleanF6 = Harms16(SpanOf(fluteCleanF6), 1.0683f); // выравнивание зон по регрессии уровня регистра
		auto& t = Tables["FluteClean"];
		t.Generator = [](float freq, unsigned sampleRate)
		{
			// x = октавы над C4; зоны C#4/E4/B4/D5/F#5/G#5/C6/F#6 (плоские «ступеньки», как в банке).
			static const HarmonicSet* const zoneSets[] = {&setCleanC4, &setCleanE4, &setCleanE4, &setCleanB4, &setCleanB4,
				&setCleanD5, &setCleanD5, &setCleanF5, &setCleanF5, &setCleanG5, &setCleanG5,
				&setCleanC6, &setCleanC6, &setCleanF6};
			static const float zoneX[] = {0.0f, 0.25f, 0.4167f, 0.5f, 0.9167f,
				1.0f, 1.25f, 1.333f, 1.5f, 1.583f, 1.75f, 1.833f, 2.0f, 2.042f};
			const float x = Math::Log(freq/261.63f)/Math::Log(2.0f);
			return BuildWaveTable(MixZoneSets(zoneSets, zoneX, 14, x), 16384, freq, sampleRate);
		};
		t.AllowMipmaps = false;
		auto& wt = Instruments["FluteClean"].WaveTables.EmplaceLast();
		wt = Wt(&t, 0.24f, 0, {0.06f, 0, 1, 0.50f, 0, true, false});
		// Огибающая тона — регистровый профиль: D0 → подъём к V1 за T1 → «раздув» к 1 за T2, релиз 0.10 с.
		wt.EnvelopeProfile = [](float freq)
		{
			const float x = Math::Clamp(Math::Log(freq/261.63f)/Math::Log(2.0f), 0.0f, 2.0f);
			// Опорные точки C4 → C5 → C6 (кусочно-линейно по x, октавы над C4).
			const float d0 = x <= 1.0f ? 0.008f + 0.005f*x : 0.013f - 0.005f*(x - 1.0f);
			const float v1 = x <= 1.0f ? 0.42f + 0.28f*x : 0.70f + 0.10f*(x - 1.0f);
			const float t1 = x <= 1.0f ? 0.050f - 0.022f*x : 0.028f - 0.004f*(x - 1.0f);
			const float t2 = x <= 1.0f ? 0.16f - 0.120f*x : 0.040f - 0.020f*(x - 1.0f);
			EnvelopeFactory f;
			f.StartVolume = 0;
			f.Segments[0] = {false, 0, d0};
			f.Segments[1] = {false, v1, t1};
			f.Segments[2] = {true, 1, t2};
			f.Segments[3] = {false, 1, Intra::Infinity};
			f.Segments[4] = {true, 0, 0.50f};
			return f;
		};
		// Вибрато — по замеру банка Titanic (Update 28, .scratch/vib-period.mjs):
		// 3.8 Гц на C4 → 3.7 на C5 → 4.35 на C6, глубина ±6.6 → ±5.2 → ±10.6 ц (пик).
		// Update 60: один профиль на все слои ноты (тело/дыхание/блум).
		wt.VibratoProfile = [](float freq) { return FluteTitanicVibrato(freq); };
		// Дуновение — как у FluteHybrid (Update 28, замер банка Titanic): воздух
		// это ПОЛОСА СТРУИ, а не белый шум. Уровень растёт к C5/C6 (у банка воздух
		// Воздух живёт ТОЛЬКО в атаке (шифф ~25 мс, спад за ~0.3 с; в сустейне шума нет —
		// в оригинале дыхание коррелировано с тоном, отдельного шумового слоя нет); низ режет
		// ФВЧ на ~0.7·f0, верх — плато до ~2.2 кГц (C4) → 1.5 кГц (C6), крутой спад.
		Instruments["FluteClean"].GenericInstruments.EmplaceLast(
			[](float freq, float volume, unsigned sampleRate) -> GenericSamplerRef
			{
				// Update 74: пшик убран. Владелец: «У флейты противный пшик!». Замер
				// `timbre 43:60` по ВЕРНОМУ эталону (пресет банка 73, а не 43 — см.
				// BANK_PROGRAM в tools/analysis/lib/render.mjs): у банка шум в атаке
				// НАРАСТАЕТ с −72 дБ (0 мс) до −54 (80 мс) и держит −51…−54, то есть
				// вспышки на note-on нет вообще. Прежняя формула (пик/полка 1/0.16 =
				// 6.25× = +16 дБ, подъём 8 мс) давала −34.5 дБ уже в 0-20 мс — это и
				// был пшик. Стало: атака 150 мс, спад 0.40 с до полки 0.72 (пик/полка
				// 1.39× = +2.8 дБ), уровень ×0.20/0.72 — сустейн не сдвинут
				// (проверено `timbre`: шум теперь НАРАСТАЕТ — −59.7 дБ в 0-60 мс →
				// −45.6 в 100 мс → полка −46; выше полки вспышки нет).
				// Update 73: выдох ВСПЫХИВАЕТ В НАЧАЛЕ. Владелец: «нет в атаке выдоха,
				// сразу общий тембр, как в сустейне». Замер `timbre` (окно 60 мс, дБ
				// отн. h1 сустейна, 43:60, банк → наш v72): 600-1500 Гц в 0-60 мс
				// −37.0 → −54.4, 1500-3500 −43.7 → −54.4, а пик у банка в 40 мс
				// против наших 120. У банка выдох гаснет за ~0.2 с с динамикой
				// 17 дБ (от −33.8 до −51), у нас динамика была 5 дБ (1/0.55).
				// Update 72 (атака 100/спад 450/полка 0.55) убрала «низкий пшик»,
				// но вместе с ним и выдох: медленный подъём читается как «сразу
				// тембр сустейна». Теперь атака 8 мс, спад 0.20 с до полки 0.28
				// (пик/полка 11 дБ), уровень ×0.20/0.28 — сустейн не сдвинут.
				static const EnvelopeDesc breathEnv = {0.15f, 0.40f, 0.72f, 0.10f, 0, true, false};
				const float x = Math::Clamp(Math::Log(freq/261.63f)/Math::Log(2.0f), 0.0f, 2.0f);
				static const float lvlX[] = {0.0f, 1.0f, 2.0f};
				// Update 75: ЭТОТ ПОДЪЁМ БЫЛ ПО НЕВЕРНОМУ ЭТАЛОНУ (Update 74).
				// Замер по верному банку (пресет 73) показал, что наш межгармонический
				// пол на 12 дБ выше файла вкладки: `floor 43:60` 1000-1400 мс — наш
				// −54.4 дБ, банк −66.5. Изоляция сборкой с level = 0
				// (/.scratch/wasm-cmp/nonoise) даёт −81.6, то есть весь пол — это
				// наше дыхание, а не грязь вейвтаблицы. Множители снижены по замеру
				// `bands` (400-4800 Гц): C4 −6 дБ, C5 −8 дБ, C6 −3 дБ
				// (2.41/6.46/2.16 → 1.21/2.57/1.53).
				// Update 69: дыхание поднято по замеру bands (было 0.50/1.15/0.45 при
				// 0.0325). Множители взяты из отношения «наш/банк» по полосам
				// 400-4800 Гц: C4 ×4.81 (13.6 дБ), F4 ×4.03 (12.1), C5 ×5.62 (15.0),
				// C6 — как C4 (у банка семпла на C6 нет). Абсолютные уровни стали
				// 0.078/0.131/0.210/0.070 при уровне пан-флейты 0.0855 на C4 — то
				// есть воздух флейты теперь того же порядка, что у пан-флейты
				// (раньше он был в 5.3 раза тише, и тремоло по дыханию не читалось).
				static const float lvlK[] = {1.21f, 2.57f, 1.53f};
								const float level = 0.0325f*(0.20f/0.72f)*MixParam(lvlX, lvlK, 3, x);
				// Форма воздуха — полоса струи: низ режет ФВЧ на ~0.7·f0 (NoiseSampler),
				// верх — плато до ~2.2 кГц (C4) → 1.5 кГц (C6), потом КРУТОЙ спад —
				// второй каскад ФНЧ (12 дБ/окт), у банка выше ~2.5 кГц шума почти нет.
				// Гребёнка на периоде ноты (combGain 0.40) — тоно-коррелированный воздух
				// (Update 31): атака звучит как дуновение у гармоник, а не белый шип.
				// Update 75: ФНЧ воздуха был 2200-350x — по замеру шумового пола банка
				// (пресет 73) он режет рано: у банка воздух ровный до ~6.8 кГц, у нас
				// падал на 5-9 дБ выше 2.2 кГц. 3800-600x даёт C4 3800 / C5 3200 / C6 2600.
				// ФВЧ: 0.90f0 отрезало шум ниже 0.9·f0, у банка он есть и на 134-168 Гц
				// (был ниже на 9 дБ) — 0.55f0.
				const float cutoffHz = 3800.0f - 600.0f*x;
				const float hpHz = Math::Min(0.55f*freq, 1200.0f);
				return new NoiseSampler(freq, volume, sampleRate, 32768, level, 150000003u,
					cutoffHz/freq, true, breathEnv, hpHz/freq, 2, 0.40f, 1u, FluteTitanicVibrato(freq));
			});
		// «Открывающийся» срез атаки (то же, что у legacy «Flute»), но регистровый:
		// C4..C#5 — как было (600 Гц → полный за ~35 мс, живой вход). C5+ — ТЁМНЫЙ
		// СТАРТ банка (Update 31, .scratch/c6-prof.cjs): у банка на C6 атака на h3/h4
		// темнее сустейна на ~7-8 дБ и распускается ~0.3-0.5 с («нежная» эволюция
		// спектра), а у нас партиалы были сразу на полной яркости — отсюда «писклявая
		// и пронзительная». ФНЧ на ~2.3·f0 пропускает h1/h2 и режет h3+, распускаясь
		// до полного за 0.18 с (C#5) → 0.31 с (C6). Релиз не моделируется.
		Instruments["FluteClean"].GenericModifiers.EmplaceLast(
			[](float freq, float volume, unsigned sampleRate) -> GenericModifier
			{
				const float x = Math::Clamp(Math::Log(freq/261.63f)/Math::Log(2.0f), 0.0f, 2.0f);
				if(x <= 1.4f)
					return CutoffFactory(600, 20000, 20000, 100, {0.035f, 0.05f, 0.92f, 0.06f, 0, true, false})(freq, volume, sampleRate);
				const float c0 = Math::Min(1.6f*freq, 2000.0f);
				// Держим тёмный срез ~0.12-0.16 с (h3/h4 приглушены, как у банка),
				// потом открываем за ~0.22-0.28 с — банк C6 «распускается» ~0.3-0.5 с.
				const float holdSamples = (0.12f + 0.06f*(x - 1.4f))*float(sampleRate);
				const float openSamples = (0.22f + 0.10f*(x - 1.4f))*float(sampleRate);
				auto alpha = [sampleRate](float cutoff) -> float
				{
					const float f = Math::Max(cutoff, 1.0f);
					return 1.0f - Math::Exp(-2.0f*float(Math::PI)*f/float(sampleRate));
				};
				return GenericModifier(CutoffFilter(alpha(c0), holdSamples, alpha(c0), openSamples, alpha(20000.0f)));
			});
	}
	{
		// FluteDLS (GM 73): профиль GM-банка macOS gs_instruments.dls, 6 зон
		// (A 60-69 … F 98+), проверен по-клавишно ±0.2 дБ (Update 15).
		// Подробности — Update 14/15/16 (worklog, .scratch/appledls-flute.mjs).
		static const uint16 fluteDLS_A[] = {32768, 46202, 20676, 20676, 11632, 1304, 2068, 2068, 183, 583, 147, 233}; // зона A: ключи 60-69
		static const uint16 fluteDLS_B[] = {32768, 36700, 26017, 3277, 1163, 1465, 822, 206, 164, 105, 115, 131}; // зона B: ключи 70-77
		static const uint16 fluteDLS_C[] = {32768, 14647, 1642, 1842, 292, 233, 82, 52, 72, 52, 52, 33}; // зона C: ключи 78-84
		static const uint16 fluteDLS_D[] = {32768, 13041, 3670, 292, 259, 115, 46, 59, 46, 33, 36, 43}; // зона D: ключи 85-91
		static const uint16 fluteDLS_E[] = {32768, 3277, 367, 164, 52, 33, 52, 46, 72, 72, 16, 16}; // зона E: ключи 92-97
		static const uint16 fluteDLS_F[] = {32768, 1304, 92, 33, 26, 23, 20, 20, 20, 23, 23, 46}; // зона F: ключи 98+
		static const HarmonicSet setA = Harms16(SpanOf(fluteDLS_A), 0.9981f); // выравнивание зон по регрессии уровня регистра
		static const HarmonicSet setB = Harms16(SpanOf(fluteDLS_B), 1.0355f); // выравнивание зон по регрессии уровня регистра
		static const HarmonicSet setC = Harms16(SpanOf(fluteDLS_C), 0.9311f); // выравнивание зон по регрессии уровня регистра
		static const HarmonicSet setD = Harms16(SpanOf(fluteDLS_D), 1.0613f); // выравнивание зон по регрессии уровня регистра
		static const HarmonicSet setE = Harms16(SpanOf(fluteDLS_E), 0.9642f); // выравнивание зон по регрессии уровня регистра
		static const HarmonicSet setF = Harms16(SpanOf(fluteDLS_F), 1.0155f); // выравнивание зон по регрессии уровня регистра
		auto& t = Tables["FluteDLS"];
		t.Generator = [](float freq, unsigned sampleRate)
		{
			// x = октавы над C4; пары одинаковых наборов на краях зон держат
			// зоны плоскими — «ступенька» смены семпла на границах.
			static const HarmonicSet* const zoneSets[] = {&setA, &setA, &setB, &setB, &setC, &setC,
				&setD, &setD, &setE, &setE, &setF};
			static const float zoneX[] = {0.0f, 0.75f, 0.8333f, 1.4167f, 1.5f, 2.0f,
				2.0833f, 2.5833f, 2.6667f, 3.0833f, 3.1667f};
			const float x = Math::Log(freq/261.63f)/Math::Log(2.0f);
			return BuildWaveTable(MixZoneSets(zoneSets, zoneX, 11, x), 16384, freq, sampleRate);
		};
		t.AllowMipmaps = false;
		auto& wt = Instruments["FluteDLS"].WaveTables.EmplaceLast();
		wt = Wt(&t, 0.24f, 0, {0.03f, 0, 1, 0.015f, 0, true, false});
		// Огибающая: быстрый подъём 0→1 за T1 ~12-32 мс по зонам (атака банка
		// почти мгновенная), релиз 0.015 с (у банка тишина через ~20-40 мс).
		wt.EnvelopeProfile = [](float freq)
		{
			// d0 — плоский ноль 2-4 мс (тон не «щёлкает»), T1 — подъём 0→1 по зонам.
			static const float xs[] = {0.0f, 0.75f, 0.8333f, 1.4167f, 1.5f, 2.0f,
				2.0833f, 2.5833f, 2.6667f, 3.0833f, 3.1667f};
			static const float d0v[] = {0.003f, 0.003f, 0.004f, 0.004f, 0.004f, 0.004f,
				0.004f, 0.004f, 0.003f, 0.003f, 0.003f};
			static const float t1v[] = {0.012f, 0.012f, 0.030f, 0.030f, 0.032f, 0.032f,
				0.028f, 0.028f, 0.022f, 0.022f, 0.018f};
			const float x = Math::Log(freq/261.63f)/Math::Log(2.0f);
			EnvelopeFactory f;
			f.StartVolume = 0;
			f.Segments[0] = {false, 0, MixParam(xs, d0v, 11, x)};
			f.Segments[1] = {false, 1, MixParam(xs, t1v, 11, x)};
			f.Segments[2] = {false, 1, Intra::Infinity};
			f.Segments[3] = {false, 1, Intra::Infinity};
			f.Segments[4] = {true, 0, 0.015f};
			return f;
		};
		// Вибрато: в банке его нет (≤3 ц) — VibratoProfile не задан.
		// Дуновение — по чистым замерам банка (Update 16): у Apple DLS воздуха
		// в удержании почти нет (1-2 кГц ≈ −52…−55 дБ отн. h1 на C5-G5, ниже
		// C5 и выше G5 — −70…−80 дБ, практически тишина) и он темнее белого
		// шума. Срез опущен: min(2.5·f0, 1600 Гц) вместо min(5·f0, 1900 Гц) —
		// «пшиканье» ушло, остался тёмный воздух. Уровень — кусочно-линейный
		// множитель m(x) к базовой кривой 0.0325·0.70^x: пик на C5-G5 (×0.43),
		// спад к краям регистра (×0.028 внизу, ×0.08-0.22 наверху), как в банке.
		Instruments["FluteDLS"].GenericInstruments.EmplaceLast(
			[](float freq, float volume, unsigned sampleRate) -> GenericSamplerRef
			{
				static const EnvelopeDesc breathEnv = {0.025f, 0.28f, 0.60f, 0.07f, 0, true, false};
				const float x = Math::Clamp(Math::Log(freq/261.63f)/Math::Log(2.0f), 0.0f, 3.2f);
				static const float xs[] = {0.0f, 0.5f, 1.0f, 1.41f, 2.0f, 2.67f, 3.17f};
				static const float mv[] = {0.028f, 0.035f, 0.43f, 0.44f, 0.083f, 0.117f, 0.219f};
				const float level = 0.0325f*Math::Pow(0.70f, x)*MixParam(xs, mv, 7, x);
				const float cutoffHz = Math::Min(2.5f*freq, 1600.0f);
				return new NoiseSampler(freq, volume, sampleRate, 32768, level, 150000003u,
					cutoffHz/freq, true, breathEnv);
			});
	}
	{
		// Флейта (GM 73) — регистровые профили по ЗОНАМ реального SF2-банка
		// (Titanic, Roland-сэмплы), обновлено 2026-09-06 по справедливому A/B
		// СУХИХ рендеров (наш синтезатор против fluidsynth тех же банков без
		// реверба/хоруса, cherilady-flute первые 15 с + 3-секундные ноты):
		//   - C5/D5 (зона D5(R), ключи 72-75): совпадали по h3 (−1 дБ), но были
		//     темнее банка на h4/h5 (наши −30/−23 против −24/−19 дБ) и ярче на
		//     нечётном хвосте h7/h9 (замеренные у банка h7 −37, h9 −43 дБ);
		//   - F#5/G#5 (ключи 76-81): банк «уходит в чистоту» (h3 ≈ −22…−20 дБ),
		//     а наша интерполяция C5→C6 держала h3 ≈ −5 дБ — на +17 дБ ярче;
		//   - ключи 85+ (зона F#6(R)): банк снова флейтовый (h2 −31, h3 −20,
		//     h4 −33 дБ на короткой ноте), а наш «почти чистый» C6-профиль давал
		//     h3 −46…−59 дБ — на ~25 дБ темнее.
		// Опорные точки стоят на границах реальных зон (C#4 — D5 — F#5 — G#5 —
		// C6 — F#6), амплитуды взяты из рендеров банка и целых петель образцов;
		// между соседними точками — линейная по амплитуде смесь (у банка зоны
		// переключаются дискретно, поэтому точки на смежных клавишах границы
		// дают почти «ступеньку», как смена семпла). Зоны 63-71 (E4/G4/B4) в
		// пьесе не звучат и остаются интерполяцией C#4→D5.
		static const uint16 fluteHC4[] = {32768, 19005, 19005, 3604, 10490, 16384, 1966, 2949, 1638, 1966, 983, 655}; // C#4(R): ключи < 63
		static const uint16 fluteHD5[] = {32768, 11468, 29127, 2062, 3277, 360, 524, 459, 262, 459, 262, 105}; // D5(R): ключи 72-75
		static const uint16 fluteHF5[] = {32768, 6554, 2588, 917, 1311, 233, 164, 111, 72, 56, 52, 39}; // F#5(R): ключи 76-78
		static const uint16 fluteHG5[] = {32768, 4588, 3277, 524, 590, 147, 206, 72, 59, 52, 43, 36}; // G#5(R): ключи 79-81
		static const uint16 fluteHC6[] = {32768, 4125, 1049, 183, 52, 92, 59, 72, 33, 33, 33, 33}; // C6(R): ключи 82-84
		static const uint16 fluteHF6[] = {32768, 917, 3277, 721, 183, 115, 164, 92, 52, 43, 36, 33}; // F#6(R): ключи 85+
		static const HarmonicSet setC4 = Harms16(SpanOf(fluteHC4), 0.9425f); // выравнивание зон по регрессии уровня регистра
		static const HarmonicSet setD5 = Harms16(SpanOf(fluteHD5), 1.0337f); // выравнивание зон по регрессии уровня регистра
		static const HarmonicSet setF5 = Harms16(SpanOf(fluteHF5), 0.9038f); // выравнивание зон по регрессии уровня регистра
		static const HarmonicSet setG5 = Harms16(SpanOf(fluteHG5), 0.9728f); // выравнивание зон по регрессии уровня регистра
		static const HarmonicSet setC6 = Harms16(SpanOf(fluteHC6), 0.9650f); // выравнивание зон по регрессии уровня регистра
		static const HarmonicSet setF6 = Harms16(SpanOf(fluteHF6), 1.0683f); // выравнивание зон по регрессии уровня регистра
		auto& t = Tables["Flute"];
		t.Generator = [](float freq, unsigned sampleRate)
		{
			// x = октавы над C4; опорные точки на ключах 60/72/75/76/79/84/85
			// (границы зон C#4/D5/F#5/G#5/C6/F#6; точка 75 держит зону D5
			// плоской на ключах 72-75 — иначе рамп C#4→D5 «размазывает» сильную
			// h6 зоны C#4 (0.50) на клавиши 72-75). Ниже C4 — профиль C#4,
			// выше клавиши 85 (зона F#6) — профиль F#6.
			static const HarmonicSet* const zoneSets[] = {&setC4, &setD5, &setD5, &setF5, &setG5, &setC6, &setF6};
			static const float zoneX[] = {0.0f, 1.0f, 1.25f, 1.333f, 1.583f, 2.0f, 2.042f};
			const float x = Math::Log(freq/261.63f)/Math::Log(2.0f);
			return BuildWaveTable(MixZoneSets(zoneSets, zoneX, 7, x), 16384, freq, sampleRate);
		};
		t.AllowMipmaps = false;
		auto& wt = Instruments["Flute"].WaveTables.EmplaceLast();
		wt = Wt(&t, 0.24f, 0, {0.06f, 0, 1, 0.10f, 0, true, false});
		// Огибающая тона — регистровый профиль, «записанное начало ноты»:
		// простая ADSR-рампа не даёт живого входа. Оконный замер траекторий
		// табов (Titanic C4/C5/C6, RMS и партиалы по 40-мс окнам) показал:
		// первые ~10-15 мс тон почти беззвучен (исполнитель «дует» в инструмент,
		// тон вступает позже дыхания), затем громкость быстро доходит лишь до
		// ~половины, и дальше идёт экспоненциальный «раздув» до полной, чья
		// длительность регистровая — полный вход в петлю ~0.2 с на C4, ~0.1 с
		// на C5, ~0.05 с на C6 (чем выше октава, тем короче, как в сэмплах).
		// Поэтому EnvelopeProfile строит посегментную огибающую: плоский ноль
		// D0 (тон не «щёлкает» раньше дыхания), быстрый линейный подъём
		// 0 → V1 за T1 (короткие ноты по-прежнему набирают громкость —
		// артикуляция 16-х сохранена), затем экспоненциальный (линейный в дБ)
		// V1 → 1 за T2 — «дыхание» в начале удержанной ноты. Сустейн 1
		// (SF2-петля держится, пока нота нажата); релиз 0.10 с
		// экспоненциальный, как раньше.
		wt.EnvelopeProfile = [](float freq)
		{
			const float x = Math::Clamp(Math::Log(freq/261.63f)/Math::Log(2.0f), 0.0f, 2.0f);
			// Опорные точки C4 → C5 → C6 (кусочно-линейно по x, октавы над C4).
			const float d0 = x <= 1.0f ? 0.008f + 0.005f*x : 0.013f - 0.005f*(x - 1.0f);
			const float v1 = x <= 1.0f ? 0.42f + 0.28f*x : 0.70f + 0.10f*(x - 1.0f);
			const float t1 = x <= 1.0f ? 0.050f - 0.022f*x : 0.028f - 0.004f*(x - 1.0f);
			const float t2 = x <= 1.0f ? 0.16f - 0.120f*x : 0.040f - 0.020f*(x - 1.0f);
			EnvelopeFactory f;
			f.StartVolume = 0;
			f.Segments[0] = {false, 0, d0};
			f.Segments[1] = {false, v1, t1};
			f.Segments[2] = {true, 1, t2};
			f.Segments[3] = {false, 1, Intra::Infinity};
			f.Segments[4] = {true, 0, 0.10f};
			return f;
		};
		// Вибрато — регистровый профиль, а не фиксированный синус: слушатель
		// отметил, что фикс 6.5 Гц с первой миллисекунды «слишком частый» и
		// механический, и что в образцах оно заметно на C4, слабо на C5 и почти
		// нет выше. Поэтому: 5.2 Гц на C4 → 4.6 Гц на C6; глубина ±13 центов на
		// C4, ±6 на C5, ±3 на C6 (замер по h1 табов даёт именно малое, ~±5 ц,
		// качание фундаментала); появляется с задержкой 0.28 с после атаки и
		// входит плавно за ~0.15 с (VibratoGate в сэмплере) — как живое
		// исполнение, где вибрато включается после установки тона.
		wt.VibratoProfile = [](float freq)
		{
			// Опорные точки: C4 (261.6 Гц), C5 (523.3), C6 (1046.5); ниже C4 и
			// выше C6 параметры крайних точек. Кусочно-линейная интерполяция
			// по x (октавам над C4).
			const float x = Math::Clamp(Math::Log(freq/261.63f)/Math::Log(2.0f), 0.0f, 2.0f);
			Vibrato r;
			r.Frequency = x <= 1.0f ? 5.2f - 0.4f*x : 4.8f - 0.2f*(x - 1.0f);
			r.Value     = x <= 1.0f ? 0.0075f + (0.0035f - 0.0075f)*x : 0.0035f + (0.0018f - 0.0035f)*(x - 1.0f);
			r.Delay     = x <= 1.0f ? 0.28f - 0.04f*x : 0.24f - 0.06f*(x - 1.0f);
			r.Ramp      = x <= 1.0f ? 0.16f - 0.03f*x : 0.13f - 0.03f*(x - 1.0f);
			return r;
		};
		// Дуновение: шумовой слой с собственной ADSR. В образцах воздух сильнее
		// всего в первые ~100-200 мс (атака-шифф), затем ложится тихой
		// «воздушной подушкой» удержания. По A/B-спектру шум в 4-16 кГц у нас
		// был на ~10-15 дБ выше, чем у табов (−59..−63 против −70..−78 дБ отн.
		// h1) и читался как белый «целлофан»: срез опущен до ×5 от f0 (воздух
		// темнее, привязан к тону), уровень снижен 0.21 → 0.13. На верхних
		// октавах (C5/C6) тот же срез ×5·f0 раскрывался до 2.6-5.2 кГц белого
		// шума поверх почти чистого тона — «шуршит пакетом»: срез ограничен
		// ~1.9 кГц по абсолютной частоте (воздух флейты — полоса ~1-2 кГц, не
		// растёт с тоном), а уровень спадает с регистром (×0.70 на октаву от
		// C4). Шифф атаки остаётся громче подушки — он и даёт «дыхание» в
		// начале ноты.
		Instruments["Flute"].GenericInstruments.EmplaceLast(
			[](float freq, float volume, unsigned sampleRate) -> GenericSamplerRef
			{
				// Шифф атаки чуть опережает тон (25 мс vs 60 мс), релиз
				// экспоненциальный — воздух не «висит» после отпускания клавиши.
				static const EnvelopeDesc breathEnv = {0.025f, 0.28f, 0.60f, 0.07f, 0, true, false};
				const float x = Math::Clamp(Math::Log(freq/261.63f)/Math::Log(2.0f), 0.0f, 2.0f);
				// Слушатель: «в атаке слишком громкий шум — в быстрых мелких нотах
				// кроме него ничего не слышно, в оригинале его почти нет или он
				// очень тонкий». Атака шумового слоя ослаблена в 4 раза от исходного
				// (0.13 → 0.0325 у C4), а «воздушная подушка» сустейна — лишь в 2 раза
				// (Sustain 0.30 → 0.60 в breathEnv); регистровый спад ×0.70/октаву
				// сохранён. Подушка остаётся слышимой на долгих нотах, шифф атаки
				// больше не перекрывает быстрые мелкие ноты.
				const float level = 0.0325f*Math::Pow(0.70f, x);
				const float cutoffHz = Math::Min(5.0f*freq, 1900.0f);
				return new NoiseSampler(freq, volume, sampleRate, 32768, level, 150000003u,
					cutoffHz/freq, true, breathEnv);
			});
	}
	{
		// Пан-флейта (GM 75) — отдельный инструмент, НЕ клон флейты. Трубки
		// закрыты снизу → резонируют почти только по нечётным партиалам
		// (полая нота). Физика (Fletcher, «Stopped-pipe wind instruments», 2005):
		// струя даёт и заметные чётные партиалы, поэтому h2/h4 в банке не
		// нулевые. Update 24: профиль перетюнен по ЧИСТЫМ замерам Titanic SF2
		// (.scratch/panflute-probe.mjs, program 75, удержания 2 с, 0.6-2.0 с):
		// у банка сильная h3 (−5..−7 дБ отн. h1 на C4-G4 — «полый» тембр
		// остановленной трубы), выше по регистру тон чище (D5: h3 −15.5, G5:
		// h3 −27.5 — почти чистый тон). Четыре опорные точки C4/G4/D5/G5,
		// лог-интерполяция, вне диапазона — крайние профили.
		//
		// АТАКА: у банка фундаментал набирается ~150-250 мс, дыхание струи
		// приходит сразу и «привязано» к ноте. Update 36: дыхание — это НЕ
		// отдельный шумовой слой, а ШИРИНА ГАРМОНИК. Update 37: чистая юбка
		// (44% мощности партиалы в случайных фазах) билась с периодом таблицы
		// (16384 семпла = 2.69 Гц при 44.1 к) — слушатель слышал это как
		// «агрессивное вибрато» (детектор: ±110…1140 центов ровно на 2.69 Гц,
		// у банка ±3…7 ц = пол квантования). Теперь каждая партиала = ЯДРО
		// (острая линия, 75% мощности, когерентно) + ЮБКА (гауссова ширина
		// Update 36, 25% мощности) — биения юбки больше не качают высоту,
		// воздух остаётся вшитым в тон. Вибрато у пан-флейты нехарактерно —
		// его и нет (жёсткий тон теперь стабилен, как у банка). Атака —
		// регистровый профиль огибающей (тёмный раздув C4/C5, овершут C6) +
		// блум-вспышка h4 (замер банка: +14…16 дБ над сустейном, τ≈0.08-0.12 с).
		// Update 40: таблицы зон ПРИВЕДЕНЫ к замеру банка (L-канал, 0.6-2.0 с,
		// .scratch/pf-fit.mjs) — прежний файл был темнее банка на 3…8 дБ в C4/G5
		// и на 4…19 дБ в G4/D5 (именно это слушатель слышал как «слишком
		// рафинированный» тембр); заявленный в Update 37 перетюн в таблицы так и
		// не попал. Значения = прежние × 10^(Δ/20), Δ = банк − наш рендер на
		// опорном ключе зоны (60/67/74/79).
		// Update 47: профили = рендер банка по L-каналу (.scratch/skirt-v47.mjs,
		// пик гармоники отн. h1) в опорных клавишах ФАКТИЧЕСКИХ семплов
		// (.scratch/oc-rec-sf2map.js): panflutea3la 0..70 (опоры 60/67/70),
		// panfluted4la 71..78 (72/74/78), panfluteg4la 79..108 (79 и 91).
		// Прежние 4 «октавные» опоры мешали профили через границы семплов, а
		// значения снимались усреднением L+R — гребёнка L+R занижает чётные
		// гармоники, поэтому h4/h6/h7/h8 на C6 стояли на 11-24 дБ выше банка
		// (слушатель: «C6 писклявее оригинала»).
		// A3-семпл (≤70): −28.9/−6.6/−42.9/−19.2/−50.1/−37.5/−61.8/−51.0/−68.1/−59.8/−72.9.
		static const uint16 panA3[] = {32768, 1172, 17141, 235, 4540, 183, 694, 48, 208, 37, 108, 28};
		// D4-семпл (71..78): −38.2/−15.8/−48.4/−35.2/−61.3/−55.2/−76.2/−65.8/−77.3/−74.4/−84.9.
		static const uint16 panD4[] = {32768, 402, 5964, 197, 1012, 63, 161, 14, 68, 13, 19, 4};
		// G4-семпл, низ зоны (клавиша 79): −30.1/−29.8/−53.0/−59.1/−64.0/−64.4/−84.4/−75.9/−91.5/−87.2/−91.6.
		static const uint16 panG4lo[] = {32768, 1289, 1680, 116, 72, 47, 50, 4, 7, 1, 1, 1};
		// G4-семпл, верх зоны (клавиша 91): −34.9/−39.3/−65.2/−71.9/−76.9/−78.9/−97.5/−92.3/−105.5/−107.0/−106.2.
		static const uint16 panG4hi[] = {32768, 742, 563, 29, 16, 11, 10, 2, 1, 1, 1, 1};
		// Update 50: ширина гармоник 200 центов (≈±12% частоты) — замер банка
		// (.scratch/bins-near.mjs): горб вокруг гармоники на 10-15 дБ выше
		// подложки, ширина ±15% её частоты.
		// A3 и D4 — без юбки (см. Update 52: на C4 статичная юбка даёт 1.7 дБ
		// медленного шиммера, который владелец уже отбраковывал).
		static const HarmonicSet setPanA3 = Harms16(SpanOf(panA3));
		static const HarmonicSet setPanD4 = Harms16(SpanOf(panD4));
		static const HarmonicSet setPanG4lo = Harms16(SpanOf(panG4lo));
		static const HarmonicSet setPanG4hi = Harms16(SpanOf(panG4hi));
		auto& t = Tables["PanFlute"];
		t.Generator = [](float freq, unsigned sampleRate)
		{
			// Update 47: жёсткий выбор зоны по клавише (банк переключает семпл
			// мгновенно), как у блокфлейты/окарины. Внутри g4la профиль
			// сползает от транспозиции — опоры 79/91.
			const float key = 69.0f + 12.0f*Math::Log(freq/440.0f)/Math::Log(2.0f);
			const HarmonicSet* setPtr = key < 70.5f ? &setPanA3 : (key < 78.5f ? &setPanD4 : nullptr);
			HarmonicSet mixed;
			if(!setPtr)
			{
				const HarmonicSet* const zoneSets[] = {&setPanG4lo, &setPanG4hi};
				static const float zoneX[] = {19.0f/12.0f, 31.0f/12.0f};
				mixed = MixZoneSets(zoneSets, zoneX, 2, Math::Log(freq/261.63f)/Math::Log(2.0f));
				setPtr = &mixed;
			}
			const HarmonicSet& set = *setPtr;
			// Update 37 (v38): СТАТИЧЕСКИЕ ЮБКИ запрещены. Любая много-линейная
			// юбка со случайными фазами бьётся с ядром на периоде сетки таблицы
			// fs/16384 = 2.69 Гц (0.37 с): провалы до (1−a)/(1+a) (a=0.8 → −19 дБ,
			// a=0.40 → −10.5 дБ — обе мерялись, pf-vib37 v37d…f) — h3 перехватывает
			// пересечения нуля в провалах, детектор читает «агрессивное вибрато»
			// на сотни центов на C4. В банке ширина h1 — это ЧАСТОТНАЯ МОДУЛЯЦИЯ:
			// pf-vib37 меряет у Titanic настоящую FM-траекторию h1: C4 10.25 Гц
			// ±13 ц, C5 5 Гц ±7 ц, C6 3.5 Гц ±7 ц. Боковые полосы FM на f0±rate
			// и есть «широкая гармоника» — воздух вшит в тон без статичных биений.
			// Итог: партиалы — острые линии (таблица без юбок). Дрожания высоты
			// нет: FM-вибрато v38 убрано (Update 40, комментарий у wt ниже) —
			// тон ровный, как у банка.
			// Update 42: период таблицы 16384 -> 32768. Слушатель: «тембр у C4
			// стал вибрирующий, как будто зациклили очень короткий период...
			// единственный недостаток был в периоде 16к семплов, но ради
			// качества можно и побольше поставить». Сетка ДПФ таблицы — fs/N:
			// при N=16384 это 2.93 Гц, то есть любая широкополосная структура
			// партиала билась с периодом 0.34 с. Точную высоту даёт скорость
			// чтения (BuildWaveTable), от N она не зависит — меняется шаг сети
			// (1.46 Гц) и цена памяти/кэша на ноту.
			return BuildWaveTable(set, 32768, freq, sampleRate);
		};
		t.AllowMipmaps = false;
		auto& wt = Instruments["PanFlute"].WaveTables.EmplaceLast();
		// Update 61d: 0.10 → 0.112 пробовалось, но +1 дБ тела поднимал сустейн
		// всего на 0.8 дБ, а атаку (0.22-0.32 с) ронял на 2.7 дБ — тело в атаке
		// складывается с блум-овершутом в противофазе (замер click-zoom 75:72).
		wt = Wt(&t, 0.10f, 0, {0.15f, 0, 1, 0.08f, 0, true, false});
		// Update 40: FM-вибрато (v38) УБРАНО. Слушатель: «тембр у C4 стал
		// какой-то вибрирующий, как будто зациклили очень короткий период».
		// Замер h1 банка по L-каналу не подтверждает линии 10 Гц: у Titanic
		// 4.3-7.3 Гц на ≤7 ц, то есть на полу разрешения зонда pf-vib37 —
		// принятая тогда «настоящая FM-траектория» была артефактом детектора
		// (см. Update 37, где то же самое уже случилось с юбками). 		// Пан-флейта играет ровным тоном, вибрато ей не задаём.
		// Update 58: владелец — «в сустейне не хватает лёгкого вибрато
		// оригинала». Замер банка: 5.5-8.5 Гц при ±2.2…±12.6 цента (слабеет
		// к верху). Ставим 5.6 Гц и ±5.0…±2.5 цента, задержка 0.35 с:
		// прежняя жалоба «тембр вибрирующий, как зациклили короткий период»
		// была на 10 Гц и глубокой модуляции.
		// Update 59: параметры вынесены в PanFluteVibrato — тот же LFO уходит и в
		// слои дыхания/блума, иначе статичные юбки бьются с уходящим тоном.
		wt.VibratoProfile = [](float freq) { return PanFluteVibrato(freq); };
		// Update 40: атака по окнам банка (L-канал, 0-40/40-120/120-300 мс от
		// note-on, дБ отн. сустейна; .scratch/pf-fit.mjs): C4 −19/−7/+3,
		// G4 −17/−2/+4.5, D5 −39/−4/+3.5, G5 −25/+7/+3.4, C6 −14/+8/+4.8.
		// Прежний профиль поднимался 0.2-0.5 с и всё это время был тёмным, из-за
		// чего слушатель не слышал атаки вовсе («атака так и не появилась»).
		// Теперь: D0 (плоский 0) → быстрый линейный подъём к V1 за T1 →
		// экспоненциальное добирание до 1 за T2 → сустейн. Овершут +3…+5 дБ
		// даёт блум-слой ниже (h1), а не огибающая: её 8-битная упаковка
		// (Envelope::Point::Volume) не умеет > 1.0.
		wt.EnvelopeProfile = [](float freq)
		{
			const float x = Math::Clamp(Math::Log(freq/261.63f)/Math::Log(2.0f), 0.0f, 2.0f);
			// Update 42d: 4 опорные точки зон (C4/G4/D5/G5), а не 3 «октавных».
			// Замер банка по окнам 0-15/15-30/30-60/60-120 мс (.scratch/pf-atk.mjs,
			// L-канал, дБ отн. сустейна h1): C4 −21.7/−18.0/−13.9/−5.6,
			// G4 −21.3/−16.9/−10.6/−1.2, D5 −27.1/−25.3/−13.8/−1.2,
			// G5 −35.6/−22.3/+1.9/+8.5. Верхние зоны банка стартуют заметно
			// тише и темнее, а v42c давал им такой же быстрый подъём, как C4
			// (наши G5 0-15 −20.4 против −35.6, D5 15-30 −9.5 против −25.3).
			static const float xs[] = {0.0f, 7.0f/12.0f, 14.0f/12.0f, 21.0f/12.0f};
			// Плоский ноль (тон не должен щёлкать до «дыхания»): 4 мс внизу,
			// 8-10 мс в верхних зонах (там банк первые 15 мс почти пуст).
			// Update 42e: нижние зоны — плоский ноль 6 мс (v42d дал 0-15 на 4.5 дБ
			// громче банка), верхние оставлены тёмными.
			// Update 44: верх входит как у банка — пауза, затем быстрый подъём.
			// Замер (.scratch/wind-attack.mjs, дБ отн. сустейна h1, G5):
			// банк h1 −38.7/−22.8/+1.3/+8.0 (окна 0-15/15-30/30-60/60-120 мс),
			// у нас было −35.2/−31.7/−28.0/−3.7 — линейный раздув без атаки.
			static const float d0v[] = {0.006f, 0.006f, 0.012f, 0.022f};
			// Update 42c: V1/T1/T2 подобраны под замер банка (L-канал, окна
			// 0-15/15-30/30-60/60-120 мс, дБ отн. сустейна h1): 0.082/0.126/0.202/0.525.
			// Отсюда короткий быстрый подъём (T1), затем экспоненциальный «раздув»
			// до 1.0 к ~180 мс (T2). Прежние 0.80 за 70 мс давали +7…8 дБ в окнах
			// 30-120 мс — слушатель слышит это как «атака мимо».
			// V1/T1 — «полка» подъёма, T2 — экспоненциальное добирание до 1.0.
			// Подобраны так, чтобы средние по окнам повторили банк: T1 заканчивается
			// на ~25-35 мс (0-15 ≈ половина V1), T2 добирает до 1.0 к ~120-150 мс
			// (банк 60-120 мс: C4 0.53, D5 0.87 от сустейна). Выше 1.0 огибающая
			// не умеет (8-битная упаковка Envelope::Point::Volume) — овершут даёт
			// блум-слой ниже.
			// Update 58: опоры D5/G5 пересобраны по замеру bank-env (C5 был на
			// +5…+7 дБ в окнах 0-100 мс, C6 — на −4…−8 дБ в 25-500 мс).
			static const float v1v[] = {0.105f, 0.105f, 0.032f, 0.075f};
			static const float t1v[] = {0.020f, 0.022f, 0.012f, 0.006f};
			static const float t2v[] = {0.100f, 0.090f, 0.070f, 0.014f};
			EnvelopeFactory f;
			f.StartVolume = 0;
			f.Segments[0] = {false, 0, MixParam(xs, d0v, 4, x)};
			f.Segments[1] = {false, MixParam(xs, v1v, 4, x), MixParam(xs, t1v, 4, x)};
			f.Segments[2] = {true, 1, MixParam(xs, t2v, 4, x)};
			f.Segments[3] = {false, 1, Intra::Infinity};
			f.Segments[4] = {true, 0, 0.08f};
			return f;
		};
		// Блум-слой атаки (механизм флейт, Update 27): h4-вспышка (замер банка:
		// h4 в атаке +14…16 дБ над своим сустейном — C4 −11.9 дБ отн. h1,
		// G4 −14.1, выше C6 вспышка не нужна, спад τ≈0.10 с) плюс новый
		// h1-овершут +3…+5 дБ (окно банка 120-300 мс; амплитуда = 0.5·доля h1 в
		// сумме зоны — 0.55/0.51/0.73/0.91 для C4/G4/D5/G5). Масштаб = volume Wt
		// (0.10), амплитуды — отн. h1 таблицы (тот же контракт, что у флейт).
		Instruments["PanFlute"].GenericInstruments.EmplaceLast(
			[](float freq, float volume, unsigned sampleRate) -> GenericSamplerRef
			{
				const float x = Math::Clamp(Math::Log(freq/261.63f)/Math::Log(2.0f), 0.0f, 2.0f);
// Update 42b: атака пан-флейты = короткий ТОНАЛЬНЫЙ всплеск h3/h4/h5
				// (амплитуда = доля сустейна h1; банк в первые 60 мс держит эти три
				// гармоники почти на сустейне, а h1 приглушён — см. замер в комментарии
				// у слоёв ниже). Белого/полосового шума нет.
				// Update 42c: амплитуды подняты по замеру v42b (в 0-15 мс не хватало
				// 6…19 дБ), подъём/гейт сокращены 12 -> 6/5 мс: у банка h3/h4/h5
				// стоят на сустейне УЖЕ в первом окне, а не приходят позже.
				// Update 42d: амплитуды пересчитаны по замеру v42c (0-15 мс, дБ
				// «наши − банк»): v42c пересветил атаку на 5-13 дБ RMS и до +20 дБ
				// по h4/h5 в верхних зонах. Множители 10^(−Δ/20) по зонам.
				// Update 42e: амплитуды интерполированы по двум замерам (v42c/v42d).
				// h3 (доминанта атаки банка) теперь в ±5 дБ во всех зонах; h4/h5
				// в D5/G5 убраны — там их фазовый бой с телом неустойчив (сетка
				// наклона до 3.8 дБ на 1 дБ амплитуды), а банк там держит их на
				// −42…−50 дБ отн. h1, т.е. практически не слышно.
				// Update 44: в D5/G5 вспышка h3/h4/h5 возвращена. В Update 42e её
				// убрали из-за фазовой неустойчивости оверлея, но замер банка
				// показывает, что именно она делает атаку: у банка в G5 сумма
				// гармоник 0-15 мс на 6 дБ громче нашей при h1 на 3 дБ тише
				// (tone −26.4 при h1 −38.7 против наших −32.3 при −35.2).
				// Update 45: в G5 вспышка усилена (h1 там почти не должен звучать
				// в первые 15 мс), в C5-зоне срезана — там наша сумма гармоник была
				// на 6.7 дБ громче банка при совпавшем h1.
				static const float splashA3[] = {0.46f, 0.45f, 0.05f, 0.90f};
				// Update 54: в верхней зоне (G5+) h4/h5 срезаны — в окне 0-40 мс
				// на C6 мы были на +16 дБ громче банка на 4 кГц, а он держит
				// там −27. h3 усилена (у банка в атаке она почти на сустейне).
				static const float splashA4[] = {0.30f, 0.20f, 0.03f, 0.26f};
				static const float splashA5[] = {0.50f, 0.40f, 0.02f, 0.20f};
				static const float zoneX[] = {0.0f, 7.0f/12.0f, 14.0f/12.0f, 21.0f/12.0f};
// Замер банка (.scratch/pf-atk.mjs, L-канал, окна 0-15/15-30/30-60/60-120 мс,
				// дБ отн. сустейна h1): банк держит h3/h4/h5 уже в атаке почти на
				// сустейне, тогда как h1 поднят на 20+ дБ слабее — «атака с другим
				// тембром», а не просто тише. У нас же всё пряталось под общей
				// огибающей (0-15 мс: h1 −30.0, h3 −33.8 против банка −21.7/−14.2) —
				// слушатель: «атаки так и нет, у атаки совсем другой тембр, это не
				// про огибающую». Слой быстрый: подъём 12 мс, спад τ≈55 мс.
				float splash[12] = {};
				splash[2] = MixParam(zoneX, splashA3, 4, x);
				splash[3] = MixParam(zoneX, splashA4, 4, x);
				splash[4] = MixParam(zoneX, splashA5, 4, x);
				return new BloomSampler(freq, volume, sampleRate, SpanOf(splash), 5,
					SpanOf(splash), 0, 0.006f, 0.030f, 0.05f, 0.005f, 0.10f,
					PanFluteVibrato(freq));
			});
// Медленный овершут h1 (Update 42b). Банк: +2.1 дБ в окне 120-250 мс и
		// +3.6 дБ в 250-500 мс отн. собственного сустейна, т.е. после быстрого
		// всплеска громкость чуть перелетает сустейн и садится к ~1 с. Прежний
		// flash[0] был в неверных единицах (0.274 от h1 таблицы = −11 дБ, а не
		// +3.5) и не читался вовсе — отсюда «никакого блум-слоя не слышу».
		// Подъём 150 мс (в первых 60 мс вклад ≈0, как у банка), спад τ 0.45 с.
		// Update 42c: пик овершута сдвинут на ~300 мс и амплитуда снижена — банк
		// даёт +2.1 дБ в 120-250 мс и +3.6 в 250-500, а не +7 в начале (v42b).
		// Update 42d: амплитуда и время зоны — под пик банка (C4 250-500 мс,
		// G4/D5 120-250 мс, G5 60-120 мс). v42c с плоскими 0.70/0.30 не читался.
		// Update 58: C5-зона была пересвечена на входе (+5 дБ в 0-100 мс) и
		// недобирала пик (−3 дБ в 100-500), C6 наоборот — недобирал атаку на
		// 4-8 дБ во всей первой половине секунды. Пик сдвинут раньше и поднят.
		static const float overA1[] = {1.05f, 0.70f, 1.15f, 1.55f};
		static const float overRise[] = {0.30f, 0.16f, 0.10f, 0.05f};
		// Update 58b: хвост овершута длиннее — на C5/C6 он был на 3-4 дБ ниже
		// банка в 200-700 мс (замер bank-env).
		static const float overTau[] = {0.50f, 0.40f, 0.62f, 0.58f};
		Instruments["PanFlute"].GenericInstruments.EmplaceLast(
			[](float freq, float volume, unsigned sampleRate) -> GenericSamplerRef
			{
				const float x = Math::Clamp(Math::Log(freq/261.63f)/Math::Log(2.0f), 0.0f, 2.0f);
				static const float zoneX[] = {0.0f, 7.0f/12.0f, 14.0f/12.0f, 21.0f/12.0f};
				float over[12] = {};
				over[0] = MixParam(zoneX, overA1, 4, x);
				return new BloomSampler(freq, volume, sampleRate, SpanOf(over), 12,
					SpanOf(over), 0, MixParam(zoneX, overRise, 4, x),
					MixParam(zoneX, overTau, 4, x), 0.05f, 0.010f, 0.10f,
					PanFluteVibrato(freq));
			});
		// Дыхание пан-флейты — ВОЗВРАЩЕНО (Update 42). Слушатель: «Дыхания тоже
		// никакого нет, хотя, когда я говорил, что тембр похож, оно было!
		// Хорошее звучание наверное давали широкие гармоники, но ты их убрал
		// что ли?» Именно так: Update 36/37 убрали последний шумовой слой, а
		// Update 40 довёл таблицу до острых линий — осталось «обычное гудение».
		// Здесь ровно v35-конфигурация (та, что слушатель отметил как похожую):
		// воздух ВШИТ в партиалы — гребёнка на периоде ноты (combGain 0.40)
		// превращает шум в «юбки» вокруг гармоник (долины −47…−40 дБ отн. h1,
		// как у банка), а не в отдельный шипящий слой. Полоса: ФВЧ 0.6·f0,
		// ФНЧ min(2·f0, 1.5 кГц). Период шумовой таблицы 32768 -> 65536
		// (повтор 1.37 с вместо 0.68 с — та же жалоба на «короткий период»).
		Instruments["PanFlute"].GenericInstruments.EmplaceLast(
			[](float freq, float volume, unsigned sampleRate) -> GenericSamplerRef
			{
				// Update 42c: уровень и подушка дыхания подняты по замеру (наши полосы
				// 0.5-1.5к/1.5-3к были на 8-11 дБ тише банка, отсюда «дыхания нет»);
				// подушка 0.15 -> 0.35 даёт ту самую «полку» первых 60 мс, которую
				// банк держит на −10…−12 дБ. Шум по-прежнему тоно-коррелированный
				// (гребёнка), не белый.
				// Update 42g: уровень приведён к банку. Замер (.scratch/noise-v41.mjs,
				// Hann, L-канал, дБ отн. сустейна h1): v42c давал 0-40 мс −37.8 при
				// банке −50.6, сустейн −56.8 при −68.7 — ровно +12.5 дБ во всех окнах
				// и межгармоническая подложка на 30-50 дБ выше банка (это и есть
				// «пшик»). Форма (спад 19 дБ за ноту против 18 у банка) уже совпадала,
				// поэтому масштабирован только уровень: 0.22 → 0.052.
				// Update 43: выше C5 воздух банка практически исчезает (6-12 кГц у
				// банка −93 дБ, у нас было −73; сустейн на C6 был на +17.8 дБ, атака
				// на C5 на +8.3). Уровень теперь падает по регистру (C4/G4 1.0,
				// D5 0.60, G5 0.25), ФНЧ — вторым порядком и ниже (1.8·f0 / 1.6 кГц),
				// подушка 0.35 → 0.12, чтобы хвост затухал как у банка.
				const float x = Math::Clamp(Math::Log(freq/261.63f)/Math::Log(2.0f), 0.0f, 2.0f);
				// Update 56b: опорные точки — РОВНО измеренные клавиши C4/C5/C6
				// (до этого были C4/G4/D5/G5, а C5 попадал в интерполяцию, и
				// поправки из замера нельзя было применять один к одному).
				static const float zoneX[] = {0.0f, 1.0f, 2.0f};
				// Update 43c: «пшик» выше C5 давала ФОРМА дыхания, а не уровень.
				// Огибающая давала 30-мс всплеск в 8 раз громче подушки на
				// каждом note-on, а гребёнка 0.40 была мелкой — энергия шла в пол
				// МЕЖДУ гармониками (10-й процентиль на C5 −102.6 против банковских
				// −114.2), тогда как юбки у гармоник были на 20+ дБ тише банка.
				// Теперь рост медленный (120 мс) без всплеска, гребёнка глубокая
				// (0.85) — энергия уходит В юбки гармоник (те самые «широкие
				// гармоники»), а не в шипящий пол. Полоса ФНЧ 2-го порядка 2.2 кГц.
				// Update 43d: уровни под замер банка (.scratch/noise-v41.mjs,
				// полосы 0.5-1.5к/1.5-3к/3-6к, дБ отн. сустейна h1): банк сам
				// теряет ~10 дБ воздуха от C4 к C5 (C4 −55.2/−63.1, C5 −64.9/−62.1,
				// G5 −68.7/−67.6, C6 −66.4/−71.1), а v43c держал воздух ровным и
				// был на 9-10 дБ громче банка выше C4.
				// Update 43e: опорные точки уровня — свои (C4/G4/C5/G5): у банка
				// воздух стоит на месте от C4 до G4 и падает только к C5.
				// Update 47: уровень воздуха почти ровный по регистру. Замер
				// (.scratch/spec-bands.mjs EXCL=1, дБ отн. h1): подложка банка
				// на C4/C6 стоит на одном уровне (600-2400 Гц: −53…−70), а v43
				// роняла уровень вчетверо выше C5 — отсюда «не хватает юбки».
				static const float lvlX[] = {0.0f, 1.0f, 2.0f};
				// Update 48: стартовый уровень 0.030 — по замеру v47 (при 0.085
				// с новым ФНЧ подложка на C4 была бы −52 против банковских −61).
				// Update 54: уровень 0.033 → 0.030 при гребёнке 0.12 → 0.85. Сам
				// уровень поднимать нельзя: у ВЫХОДА гребёнки пик на гармонике
				// усилен 1/(1−g) = +16 дБ, поэтому банковские пики даёт уже
				// «тихий» вход 0.030 (замер atk-spec 0-40 мс: C4 в ±10 дБ,
				// C5/C6 — в ±15). Плоский вход 0.070 при той же гребёнке
				// пересвечивал атаку на +12…+24 дБ.
				// Update 55: уровень поднят 0.030 → 0.045: под новую форму
				// (пик держится дольше, полка ниже) сустейн банка на C4/C5 был
				// на 3.5-4.5 дБ выше нашего, а окна 40-200 мс — на 10-18 дБ.
				// Update 56d: каскад из двух проходов (v56b/v56c) срезал ФЛАНГ юбки
				// (полосу 1.1-1.4·k·f0) на 10 дБ сильнее, чем нужно: у банка фланг
				// всего на 16-20 дБ ниже своего пика, то есть юбка ШИРОКАЯ, а
				// каскад сужал её. Снова ОДИН проход 0.85, уровни — по замеру v55
				// (совпадали с банком в ±5 дБ). C5 поднят на 3 дБ: там банк держит
				// фланг −54…−56, а v55 давал −63…−66. C6 не поднимаем — его уровень
				// задаёт слой W (см. ниже).
				// Update 56e: по замеру v56d C5 пересветил (+9.5/+10.3 дБ в окнах
				// 60-260 мс, +5.8 на сустейне) — 2.60 → 1.60.
				// Update 71: −4 дБ по всем полосам. Замер `bands` (75:60, 1.4-4.0 с, медиана
				// бинов, дБ отн. h1) — наш воздух против банка: 400-600 +7.9, 600-1200 −0.3,
				// 1200-2400 +1.4, 2400-4800 +4.1, 4800-9600 +6.0, 9600-16000 +1.0. В пяти
				// полосах из шести мы громче, сильнее всего в 400-600 Гц — ровно там, где
				// у C4 стоит его h2. Это тот самый открытый пункт «воздух пан-флейты на
				// 5-13 дБ громче банка», и он же маскирует тремоло ТОНА на C4 (владелец:
				// «не слышу тремоло на C4»). Срез ровно ×0.63 (−4 дБ).
				static const float lvlK[] = {1.20f, 1.01f, 0.79f};
				// (Каскад из двух проходов пробовался в v56b/v56c: пол между
				// гармониками он опускает на 19 дБ, но вместе с ним на 10 дБ падает
				// и фланг юбки — уровень приходилось поднимать обратно, и выигрыша
				// не оставалось. Механизм combPasses в движке сохранён.)
				// Update 73: уровень воздуха оставлен (гребёнка после снижения
				// 0.85 → 0.60 даёт в долинах +1.8 дБ, а опущенный ФНЧ столько же
				// снимает — замер `bands` 75:60/75:72 держится в банковских ±3 дБ).
				const float level = 0.045f*MixParam(lvlX, lvlK, 3, x);
				// Update 44: вход 120 → 8 мс. Банк в первых 15 мс держит юбку на
				// −23.9 дБ при h1 −38.7 (G5) и −26.2 при −22.0 (C4) — воздух
				// слышен РАНЬШЕ тона; у нас же в 0-15 мс юбки не было вовсе.
				// Update 54: вспышка дыхания на атаке — вход 6 мс, пик 1.0,
				// спад до 0.40 за 100 мс (τ≈22 мс): это слышимый «чифф»,
				// а не плавное вползание. Банк в 0-40 мс держит юбки на 5-20 дБ
				// громче своего же сустейна.
				// Update 55: «чифф» стал ДОЛГИМ. Слушатель: «C5 уже начала какая-то
				// похожая атака вырисовываться, но слишком короткая по сравнению
				// с банком. У C4 её не слышно». Замер (.scratch/noise-v41.mjs
				// ATK=1 — межгармонический пол по окнам, Hann, дБ отн. сустейна h1):
				//   key 60  банк −50.6/−43.9/−45.0/−52.1/−56.8/−62.7/−64.7, сустейн −68.7
				//   key 72  банк −60.1/−50.4/−54.5/−61.9/−62.1/−65.8/−68.4, сустейн −74.8
				//   key 84  банк −54.2/−57.0/−56.2/−65.7/−68.5/−72.5/−78.4, сустейн −87.6
				// (окна 0-40/40-80/80-120/120-200/200-300/300-500/500-800 мс).
				// У банка подложка ГРОМЧЕ всего в 40-200 мс и держится там
				// 10-25 дБ выше сустейна, а у нас спад заканчивался к 120 мс.
				// Теперь: вход 45 мс, экспоненциальный спад 0.42-0.65 с до полки
				// 0.15-0.26 (чем выше зона — тем короче и ниже «чифф»: на C6
				// у банка спад в 33 дБ против 25 дБ на C4).
				// Update 55b: спад замедлен (0.65/0.68/0.62/0.50 с), вход на G5+
				// укорочен (у банка на C6 пик подложки уже в первом окне),
				// полка поднята в средних зонах — по замеру v55.
				static const float atkv[] = {0.045f, 0.040f, 0.014f};
				// Update 56: на верхних зонах спад и полка подняты (было 0.50/0.19).
				// Владелец: «в C6 у Pan Flute в оригинале атака вроде дольше, чем у
				// нас. У нас очень быстро она затухает». Замер банка на C6 (ключ 84,
				// .scratch/skirt-time.mjs, окна 0-60…1200-1800 мс): юбка
				// −43/−41/−47/−49/−53/−56/−58 — держится до 800 мс и падает всего на
				// 15 дБ, а у нас гасла к 300 (450-800: −61 против банковских −53).
				// Дыхание нужно и в сустейне: «юбка частично должна остаться в
				// сустейне, сейчас недостаточно дыхания в нём, оно всё
				// сконцентрировано только в атаке».
				// Update 56e: спад C5 укорочен (0.62 → 0.45): у банка на C5 фланг
				// падает с −47 до −51 за 140-450 мс, а наш держался на −45.
				static const float decv[] = {0.68f, 0.45f, 0.95f};
				// Update 56d: полка в сустейне поднята (0.26 → 0.30, 0.42 → 0.45) —
				// просьба владельца «юбка частично должна остаться в сустейне,
				// сейчас недостаточно дыхания в нём, оно всё сконцентрировано
				// только в атаке».
				// Update 56e: полка C6 0.45 → 0.35 (замер v56d: +7/+9 дБ на 800-1800 мс).
				static const float susv[] = {0.30f, 0.30f, 0.25f};
				const EnvelopeDesc breathEnv = {MixParam(zoneX, atkv, 3, x),
					MixParam(zoneX, decv, 3, x), MixParam(zoneX, susv, 3, x),
					0.15f, 0, true, false};
				// Потолок ФНЧ 1.8 кГц: замер v43d — на G5/C6/D6 наши 3-6к и 6-12к
				// были на 8-16 дБ громче банка (воздух банка режется круче выше 3 кГц).
				// Update 47: полоса воздуха — АБСОЛЮТНАЯ (струйная полоса банка
				// ~0.6-3.5 кГц, независимо от ноты): прежние ФВЧ/ФНЧ ехали за f0,
				// из-за чего на C6 воздух уходил выше 4 кГц (+15…+25 дБ) и не
				// доходил до юбки вокруг h1 (−5…−10 дБ). Гребёнка 0.85 → 0.25:
				// узкие пики на гармониках читались как «писклявые» линии, а банк
				// держит широкую подложку. ФНЧ — 4 полюса (24 дБ/окт).
				// Update 48: полоса ФИКСИРОВАННАЯ — 550..2400 Гц (струйная полоса
				// банка), а не масштабируемая от f0: у банка она стоит на месте
				// от C4 до C6, и именно она даёт юбку у гармоник на верхних нотах.
				// Update 54: полоса ФИКСИРОВАННАЯ по абсолютной частоте — ФВЧ
				// 420 Гц (прежние 0.9·f0 на C6 давали 941 Гц, то есть глушили
				// ровно тот участок 800-1000 Гц, где у банка самая громкая
				// атака) и ФНЧ 2000..2600 Гц (не 4200: на C6 мы были на +22 дБ
				// громче банка на 3.8 кГц). Гребёнка 0.12 → 0.85: шум снова
				// липнет к гармоникам (юбки), а не ложится ровной шипящей
				// подложкой — именно гребёнка даёт банковскую форму «пики на
				// гармониках, провалы между ними» (C5: пики 445-500 и 1260-1587
				// совпали с банком за 1-4 дБ).
				// Update 63: у дыхания БОЛЬШЕ НЕТ вибрато. Владелец: «в оригинале есть
				// две компоненты — одна вибрирует, другая ровная, а у нас всё
				// вибрирует». Замер pf-am (АМ по полосам, 75:72): у банка тон ровный
				// (АМ 2.7 дБ rms при 0.16 когерентной линии на 5.9 Гц), а когерентные
				// 5.9 Гц живут в ПОЛОСАХ ДЫХАНИЯ (долины mag 1.5-2.8). То есть
				// «вибрирует» воздух, а не высота тона. Общий LFO у тона и дыхания
				// тащил за тоном всю гребёнку — это и есть «всё вибрирует».
				const float hpHz = 420.0f;
				// Update 73: 2000-2600 → 1600-2200 Гц. Замер `spec` (сустейн):
				// у банка юбки падают от h4 к h8 на 17 дБ (C4 −42.2 → −59.1), у нас
				// всего на 7 (−36.8 → −44.2) — верхняя граница воздуха стояла
				// слишком высоко и не давала«грязноватого» спада выше 1.5 кГц.
				const float cutoffHz = Math::Min(Math::Max(5.0f*freq, 1600.0f), 2200.0f);
				// Update 66: 5 полюсов ФНЧ и никакого LFO. Владелец: «дыхание
				// очень сильное и низкое, напоминает белый шум при частоте
				// дискретизации 8 кГц», «неприятная периодичность воздуха».
				// Периодичность давал АМ-слой Update 65 — он снят (у шумовых
				// слоёв LFO больше нет вовсе).
				// Замер CLI bands (сустейн 1.4-4.0 с, дБ отн. пика h1, 75:72,
				// наш → банк): 400-600 −42.2/−48.1, 600-1200 −48.7/−58.5,
				// 1200-2400 −52.8/−56.5, 2400-4800 −62.0/−68.2,
				// 4800-9600 −82.2/−95.5, 9600-16000 −100.1/−108.0. Пятый полюс
				// снял 4.8-9.6 кГц на 4.8 дБ и 9.6-16 на 8.2 (замер v65 → v66),
				// но воздух ВСЁ ЕЩЁ на 5-13 дБ громче банка и наклонён вниз — это
				// ОТКРЫТЫЙ пункт, этим апдейтом не закрыт.
				// (floor для сверки не годится: при f0 = 523 Гц допуск 0.15·f
				// исключает почти все бины выше 1 кГц и срезы выходят пустыми —
				// ровно для этого и добавлена команда bands.)
				return new NoiseSampler(freq, volume, sampleRate, 65536, level, 150000003u,
					cutoffHz/freq, true, breathEnv, hpHz/freq, 5, 0.60f, 1u);
			});
		// Слой W — «широкий вход» дыхания (Update 56). Владелец: «мне кажется,
		// что как будто ширина юбки по мере атаки должна спадать, иначе слегка
		// чувствуется что-то типа шума».
		// У банка контраст «юбка/пол» НАРАСТАЕТ по ходу ноты: на C4 в первых двух
		// окнах пол стоит ВМЕСТЕ с юбкой (−30…−32 против −34…−36), а в сустейне
		// уходит на 10-20 дБ ниже (−64 против −54). Одним фильтром это не
		// описать: гребёнка не может опустить пол НИЖЕ своей полки, а её
		// контраст задан на всю ноту. Поэтому дыхание — сумма двух слоёв: этот
		// почти без гребёнки (0.35) и с самым коротким спадом, он отвечает
		// только за первые 100-200 мс, а юбки в сустейне держит слой N выше.
		// Свой seed: таблицы не должны складываться когерентно.
		Instruments["PanFlute"].GenericInstruments.EmplaceLast(
			[](float freq, float volume, unsigned sampleRate) -> GenericSamplerRef
			{
				const float x = Math::Clamp(Math::Log(freq/261.63f)/Math::Log(2.0f), 0.0f, 2.0f);
				static const float zoneX[] = {0.0f, 1.0f, 2.0f};
				// Уровень по замеру пола банка (.scratch/skirt-time.mjs): C4 −30…−32
				// в первых двух окнах, C5 −54/−55, C6 −47/−48.
				// Update 56b: по замеру v56a Δ «наши−банк» на входе: C4 −0.3 дБ
				// (уровень оставлен), C5 +3.8 (снято 3 дБ), C6 +4.3 (снято 4).
				static const float wlvlK[] = {1.00f, 0.42f, 0.56f};
				const float level = 0.20f*MixParam(zoneX, wlvlK, 3, x);
				// Спад быстрый (100…300 мс) — «широкий» шум банка уходит раньше
				// юбок. На верхних зонах он живёт дольше: банк на C6 держит пол
				// почти на уровне юбки всю ноту.
				// Update 56b: полка на C6 снята с 0.30 до 0.09 — замер v56a дал там
				// +16 дБ на сустейне (широкий слой перекрывал узкий и снова давал
				// ровный пол, то есть «шум»).
				static const float wdecv[] = {0.10f, 0.10f, 0.30f};
				static const float wsusv[] = {0.04f, 0.04f, 0.05f};
				const EnvelopeDesc wideEnv = {0.006f, MixParam(zoneX, wdecv, 3, x),
					MixParam(zoneX, wsusv, 3, x), 0.10f, 0, true, false};
				const float hpHz = 420.0f;
				const float cutoffHz = Math::Min(Math::Max(6.0f*freq, 2000.0f), 2600.0f);
				return new NoiseSampler(freq, volume, sampleRate, 32768, level, 150000009u,
					cutoffHz/freq, true, wideEnv, hpHz/freq, 5, 0.35f, 1u);
			});
	}
	{
		// Блокфлейта (GM 74) — замерено по петлям семплов Roland "Recorder" из
		// Titanic (#258-264). Банк меняет семпл каждые 2-3 клавиши (D3 #264,
		// A4 #263, B4 #262, C#5 #261, D5 #260, A5 #259, A#5 #258), поэтому
		// профиль — семь зон по границам семплов, выбор жёсткий по клавише.
		// До Update 43 было 4 опорные точки с лог-интерполяцией — там, где банк
		// уже переключился, тембр уезжал до 14 дБ (клавиши 70-73 и 78-80).
		// Update 43c: зоны = фактические границы семплов банка
		// (.scratch/oc-rec-sf2map.js): Recorder-D3 0..66, A4 67..69, B4 70..71,
		// C#5 72..73, D5 74..77, A5 78..80, A#5 81..105. Значения — рендер банка
		// (L-канал) в нижней клавише зоны, дБ отн. h1. Прежние 4 опорные точки
		// с лог-интерполяцией давали до 14 дБ ошибки там, где банк уже
		// переключил семпл (клавиши 70-73 и 78-80).
		// D3 (клавиша 62): −20.2/−24.2/−34.0/−43.2/−46.9/−45.6/−49.7.
		static const HarmonicDesc recD3[] = {
			{1.00f, 1, 0}, {0.08708f, 2, 0}, {0.07768f, 3, 0}, {0.056374f, 4, 0},
			{0.034579f, 5, 0}, {0.022558f, 6, 0}, {0.02657f, 7, 0}, {0.020821f, 8, 0},
			{0.0141f, 9, 0}, {0.012584f, 10, 0}, {0.007938f, 11, 0}, {0.007078f, 12, 0},
			{0.00355f, 13, 0}, {0.002518f, 14, 0}, {0.001775f, 15, 0}, {0.001259f, 16, 0}};
		// A4 (клавиша 67): −25.9/−32.2/−42.7/−41.1/−44.7/−47.9/−47.9.
		static const HarmonicDesc recA4[] = {
			{1.00f, 1, 0}, {0.08035f, 2, 0}, {0.06905f, 3, 0}, {0.036585f, 4, 0},
			{0.055527f, 5, 0}, {0.046077f, 6, 0}, {0.035652f, 7, 0}, {0.031768f, 8, 0},
			{0.025129f, 9, 0}, {0.022375f, 10, 0}, {0.01414f, 11, 0}, {0.01123f, 12, 0},
			{0.005622f, 13, 0}, {0.003981f, 14, 0}, {0.00282f, 15, 0}, {0.001994f, 16, 0}};
		// B4 (клавиша 70): −35.7/−40.5/−57.5/−38.6/−40.9/−36.4/−49.2.
		static const HarmonicDesc recB4[] = {
			{1.00f, 1, 0}, {0.02317f, 2, 0}, {0.02973f, 3, 0}, {0.013f, 4, 0},
			{0.052264f, 5, 0}, {0.028463f, 6, 0}, {0.053578f, 7, 0}, {0.022078f, 8, 0},
			{0.022375f, 9, 0}, {0.015835f, 10, 0}, {0.007941f, 11, 0}, {0.005626f, 12, 0},
			{0.002825f, 13, 0}, {0.001992f, 14, 0}, {0.001413f, 15, 0}, {0.001f, 16, 0}};
		// C#5 (клавиша 72): −37.3/−37.7/−45.7/−31.1/−39.8/−36.0/−62.2.
		static const HarmonicDesc recCs5[] = {
			{1.00f, 1, 0}, {0.02171f, 2, 0}, {0.08202f, 3, 0}, {0.041303f, 4, 0},
			{0.088227f, 5, 0}, {0.028745f, 6, 0}, {0.071022f, 7, 0}, {0.022544f, 8, 0},
			{0.035511f, 9, 0}, {0.008906f, 10, 0}, {0.011227f, 11, 0}, {0.003537f, 12, 0},
			{0.001775f, 13, 0}, {0.001258f, 14, 0}, {0.0008907f, 15, 0}, {0.0006308f, 16, 0}};
		// D5 (клавиша 74): −36.7/−47.5/−48.3/−37.2/−53.3/−54.1/−62.5.
		static const HarmonicDesc recD5[] = {
			{1.00f, 1, 0}, {0.02913f, 2, 0}, {0.09403f, 3, 0}, {0.013483f, 4, 0},
			{0.034664f, 5, 0}, {0.007806f, 6, 0}, {0.020002f, 7, 0}, {0.005663f, 8, 0},
			{0.006315f, 9, 0}, {0.002514f, 10, 0}, {0.001778f, 11, 0}, {0.000794f, 12, 0},
			{0.0003984f, 13, 0}, {0.0002816f, 14, 0}, {0.0001996f, 15, 0}, {0.0001408f, 16, 0}};
		// A5 (клавиша 79): −44.7/−25.7/−48.8/−42.0/−57.7/−47.3/−64.1.
		static const HarmonicDesc recA5[] = {
			{1.00f, 1, 0}, {0.02591f, 2, 0}, {0.1162f, 3, 0}, {0.009043f, 4, 0},
			{0.039593f, 5, 0}, {0.00731f, 6, 0}, {0.01212f, 7, 0}, {0.003007f, 8, 0},
			{0.001586f, 9, 0}, {0.001414f, 10, 0}, {0.000447f, 11, 0}, {0.000501f, 12, 0},
			{0.0002514f, 13, 0}, {0.0001777f, 14, 0}, {0.0001411f, 15, 0}, {0.0001411f, 16, 0}};
		// Update 46: A#5 (клавиши 81..105) — среднее по амплитуде от замеров
		// клавиш 83/84/85/86/88/90, а не одна клавиша 82: внутри зоны h2
		// меняется от −59.7 (81) до −36.5 (85), и опора на «холодную» точку
		// давала полый тон (h2 −57 при h3 −32) — на C6 это слышно как
		// «неприятный шум, как будто алиасинг». Средние: −39.7/−33.7/−54.7/
		// −42.0/−62.5/−62.9/−77.1.
		static const HarmonicDesc recAs5[] = {
			{1.00f, 1, 0}, {0.0164f, 2, 0}, {0.03672f, 3, 0}, {0.009222f, 4, 0},
			{0.035469f, 5, 0}, {0.003759f, 6, 0}, {0.0072f, 7, 0}, {0.001112f, 8, 0},
			{0.001122f, 9, 0}, {0.000446f, 10, 0}, {0.000447f, 11, 0}, {0.000158f, 12, 0},
			{0.0000794f, 13, 0}, {0.00005626f, 14, 0}, {0.00003978f, 15, 0}, {0.00003978f, 16, 0}};
		// Update 50: ширина гармоник 150 центов — у банка на C5 полоса вокруг
		// h1 (530 Гц при f0 523) всего на 9-26 дБ ниже самого пика h1, то есть
		// гармоника тоже горб резонанса, а не линия.
		static const HarmonicSet setRecD3 = Harms(SpanOf(recD3));
		static const HarmonicSet setRecA4 = Harms(SpanOf(recA4));
		static const HarmonicSet setRecB4 = Harms(SpanOf(recB4));
		static const HarmonicSet setRecCs5 = Harms(SpanOf(recCs5));
		static const HarmonicSet setRecD5 = Harms(SpanOf(recD5));
		static const HarmonicSet setRecA5 = Harms(SpanOf(recA5));
		static const HarmonicSet setRecAs5 = Harms(SpanOf(recAs5));
		auto& t = Tables["Recorder"];
		t.Generator = [](float freq, unsigned sampleRate)
		{
			// Жёсткий выбор зоны по клавише — банк переключает семпл мгновенно.
			const float key = 69.0f + 12.0f*Math::Log(freq/440.0f)/Math::Log(2.0f);
			const HarmonicSet* set = key < 66.5f ? &setRecD3
				: (key < 69.5f ? &setRecA4
				: (key < 71.5f ? &setRecB4
				: (key < 73.5f ? &setRecCs5
				: (key < 77.5f ? &setRecD5
				: (key < 80.5f ? &setRecA5 : &setRecAs5)))));
			return BuildWaveTable(*set, 16384, freq, sampleRate);
		};
		t.AllowMipmaps = false;
		auto& wt = Instruments["Recorder"].WaveTables.EmplaceLast();
		// Update 43: атака 35 → 75 мс — банк набирает уровень ~90 мс.
		// Update 58b: затухание банка (6.2 дБ/с) и вход по замеру
		// .scratch/bank-env.mjs: на C4 первые 25 мс почти тишина, полная
		// громкость только к 0.5 с; на C6 — 20 % за 12 мс и полная к 60 мс.
		wt = Wt(&t, 0.16f, 0.71f, {0.075f, 0, 1, 0.06f, 0, true, false});
		wt.EnvelopeProfile = [](float freq)
		{
			const float u = Math::Clamp(Math::Log(freq/261.63f)/Math::Log(4.0f), 0.0f, 1.0f);
			const float key = 69.0f + 12.0f*Math::Log(freq/440.0f)/Math::Log(2.0f);
			// Update 60b: стартовый сегмент — ПО ЗОНАМ банка (те же границы, что у
			// таблицы). Замер банка суммой гармоник в окне 0-30 мс, дБ отн. сустейна
			// (.scratch/atk-harms.mjs): D3 −39.9, A4 −13.5, B4 −19.1, C#5 −26.8,
			// D5 −2.8, A5 −4.7, A#5 −1.6. Плавный профиль по u давал на A4/B4/D5/A5
			// старт на 6-16 дБ тише и позже банка — нота «запаздывала».
			// Update 65: форма атаки ПОДОБРАНА ЧИСЛЕННО (scripts/analysis/fit-attack.mjs)
			// по замеру банка окнами 2 мс (4-96 мс, дБ отн. полки) на опорной
			// клавише КАЖДОЙ зоны: D3 62, A4 67, B4 70, C#5 72, D5 74, A5 79,
			// A#5 84. Прежние zV0/zT0 с множителями 1.70/1.05 давали пик на 2 дБ
			// выше банка и на 2 мс раньше, спад с пика вдвое короче, а к 44 мс
			// мы уже стояли на полке, тогда как банк подходит к ней к ~70 мс
			// (владелец: «бугорок у оригинала длиннее, у нас быстро спадает»).
			// Теперь: тихий старт -> экспоненциальный подъём к пику -> спад до
			// провала -> экспоненциальный раздув на полку. Остаточная ошибка
			// фита 1.1-1.7 дБ (D3 2.8 — банк там сам ступает).
			static const float recAtkV0[7] = {0.0010f, 0.0005f, 0.0015f, 0.0005f, 0.0005f, 0.0005f, 0.0015f};
			// Update 65b: уровни сдвинуты по замеру петли Δ(наш−банк) в 8-16 мс
			// (D3 пересвечивал на 4.7 дБ — снято, остальные в ±2 дБ), t1 9-10 → 11 мс (все
			// клавиши пересвечивали на 4-9 дБ в 6 мс).
			static const float recAtkPk[7] = {0.0082f, 0.0499f, 0.0177f, 0.0300f, 0.0698f, 0.0849f, 0.0842f};
			static const float recAtkDip[7] = {0.0078f, 0.0474f, 0.0168f, 0.0285f, 0.0663f, 0.0807f, 0.0800f};
			static const float recAtkT1[7] = {0.0200f, 0.0110f, 0.0110f, 0.0110f, 0.0110f, 0.0110f, 0.0110f};
			static const float recAtkT2[7] = {0.0060f, 0.0040f, 0.0040f, 0.0060f, 0.0040f, 0.0040f, 0.0040f};
			static const float recAtkT3[7] = {0.0400f, 0.0250f, 0.0250f, 0.0300f, 0.0150f, 0.0150f, 0.0150f};
			const int zone = key < 66.5f ? 0 : (key < 69.5f ? 1 : (key < 71.5f ? 2 :
				(key < 73.5f ? 3 : (key < 77.5f ? 4 : (key < 80.5f ? 5 : 6)))));
			EnvelopeFactory f;
			// Update 58c: вход ещё мягче в начале: замер bank-env показывал на
			// C4 +11 дБ в окне 0-25 мс и −4 дБ в 50-75 мс — то есть наш подъём
			// приходил раньше банковского. Банк выходит на полную к 0.5 с.
			// Update 58f: финальная подгонка входа по сетке 10 мс.
			// Целевые значения (из банка, дБ отн. уровня 2 с):
			//   C4: −41 (0-10 мс), −19.6 (20-30), −6.9 (40-50), +1.4 (50-60);
			//   C6: −18.4, +7.8 (20-30), +11.6 (30-40), +13.5 в полке.
			// То есть на C4 тон приходит с задержкой ~30 мс, на C6 — сразу.
			// Update 64: атака — быстрый «бугорок» и провал после него (владелец:
			// «какая-то резкая, округлая… растёт быстро, а потом спад, как будто
			// бугорок такой в начале», у нас же монотонный плавный раздув).
			// Замер .scratch/rec-atk.mjs (окна 2 мс, дБ отн. 2.0-2.5 с, 74:72):
			// банк −30.0 (4 мс), −17.1 (10), −15.5 (12), −17.6 (18 — ПРОВАЛ),
			// −7.4 (24), +8.9 (40); у нас −30.4 (4), −23.2 (10), −21.8 (12),
			// −19.5 (18), −12.7 (24) — без пика и провала, на 3-9 дБ тише.
			// Теперь пик ×1.7 прежнего конца первого сегмента за 0.55·T0, провал
			// до ×1.05 за 0.35·T0: для 74:72 (zV0 0.0265, zT0 21 мс) это 0.045
			// на 11.6 мс и 0.028 на 18.9 мс — форма банка (0.036/0.028). Третий
			// сегмент — экспоненциальный раздув до 1.0 (прежние два слиты).
			f.StartVolume = recAtkV0[zone];
			f.Segments[0] = {true, recAtkPk[zone], recAtkT1[zone]};
			f.Segments[1] = {true, recAtkDip[zone], recAtkT2[zone]};
			// Длительность раздува — по зонам: D3/A4/B4 у банка входят медленно
			// (сумма гармоник 0-30 мс −39.9/−13.5/−19.1 дБ), C#5+ быстро
			// (D5 −2.8, A5 −4.7, A#5 −1.6); нотные замеры 74:62 и 74:72 дали
			// T ≈ 57 и 25 мс соответственно.
			f.Segments[2] = {true, 1, recAtkT3[zone]};
			f.Segments[3] = {false, 1, Intra::Infinity};
			f.Segments[4] = {true, 0, 0.06f};
			return f;
		};
		// Update 55: вибрато банка (замер .scratch/rec-vib.mjs).
		// Update 58: глубина возвращена к замеру банка. В v56 я решил, что
		// FM-вибрато «съедает» партиалы (замер давал −8..−16 дБ), но это был
		// артефакт узкого зонда: он мерил только несущую линию гармоники, а
		// банк меряется тем же зондом и держит свои партиалы. Замер
		// .scratch/rec-vib.mjs (клавиши 62..96): банк 4.96..4.99 Гц и
		// ±14.2..14.9 цента ровно на всех клавишах — «быстрее на C6» не
		// подтвердилось, поэтому частота постоянная.
		// Update 60: профиль вынесен в RecorderVibrato — тот же LFO уходит и в
		// слой дыхания, иначе гребёнка юбок стоит на месте и бьётся с тоном.
		wt.VibratoProfile = [](float freq) { return RecorderVibrato(freq); };
		// Дыхание блокфлейты (переделано в Update 41). Прежний слой был
		// коротким (15 мс) белым «пшиком»: за первые 15 мс после note-on он
		// звучал на 27 дБ ГРОМЧЕ банка в 0.3-0.7 кГц, а дальше — на 5..20 дБ
		// тише. В банке наоборот: почти тихий старт, подъём к максимуму к
		// ~100 мс и медленный спад к подушке. Теперь — медленный подъём
		// (0.10 с) и длинный спад; полоса струи сужена (ФВЧ 0.9·f0, крутой
		// ФНЧ min(2.8·f0, 2.4 кГц)) и привязана к периоду ноты гребёнкой
		// (combGain 0.40) — так шум читается как дыхание, а не как шип
		// (тот же приём, что у flute-семьи, Update 31).
		Instruments["Recorder"].GenericInstruments.EmplaceLast(
			[](float freq, float volume, unsigned sampleRate) -> GenericSamplerRef
			{
				// Update 44: вход 100 → 30 мс (банк даёт воздух уже в первые 40 мс),
				// подушка 0.70 → 0.62.
				// Update 58d: вход дыхания 30 → 115 мс. Замер bank-env: у банка в
				// окнах 0-25 и 25-50 мс мы были на +10 дБ громче (а с 50 мс уже
				// совпадали) — это чифф, он приходит позже тона. Затухание после
				// пика 0.60 → 0.40 с (подушка та же 0.62).
				// Update 66: владелец — «у Recorder многовато шума в сустейне».
				// Замер (CLI floor, дБ отн. h1): наш межгармонический пол РОС
				// −65.5 (0.3-0.6 с) → −51.1 (3.0-3.4 с), у банка он ровный
				// (−67.0/−67.2/−66.6/−67.6). Причина: у тона есть экспоненциальное
				// затухание (6.2 дБ/с), а дыхание жило на ПОСТОЯННОЙ подушке 0.62 —
				// шум оставался на месте, пока тон падал. Затухание дыхания теперь
				// такое же: 1.0 → 0.12 за 3 с (≈16.9 дБ за 2.75 с против 17 дБ
				// у тона). Уровень домножен на 0.786, чтобы в 0.45 с осталось
				// прежнее значение (прежняя подушка 0.62; 0.12^0.1117 = 0.789).
				static const EnvelopeDesc breathEnv = {0.115f, 3.0f, 0.12f, 0.06f, 0, true, false};
				// Update 43c: полоса шире (ФНЧ 2-го порядка 2.4 кГц вместо
				// 2.8·f0), уровень 0.019 → 0.030, гребёнка 0.40 → 0.80: у банка
				// между гармониками тише, а юбки шире (замер .scratch/noise-v41.mjs,
				// клавиши 67-86: 1.5-3к/3-6к у нас были на 6-12 дБ тише банка).
				// Update 47: уровень 0.030 → 0.018, гребёнка 0.80 → 0.30, ФВЧ
				// 0.75·f0, ФНЧ 3 полюса 2.2 кГц — по замеру (.scratch/spec-bands.mjs
				// EXCL=1): на C5 мы были на +3…+11 дБ громче банка по всей полосе
				// 0.3-2.4 кГц, на C6 — на +6…+12 дБ выше 7 кГц. «Широкая юбка,
				// которой быть не должно, ещё и осциллирует очень быстро» — это
				// гребёнка 0.80: она превращала воздух в модуляцию на f0.
				// Update 55: ФНЧ больше НЕ едет за нотой. Было
				// min(4.5*f0, 2200) — на C4 это 1322 Гц (4.5 гармоники),
				// и всё выше 2.7 кГц оказывалось на 13-20 дБ тише банка
				// (.scratch/spec-bands.mjs EXCL=1 T0=1.4: банк держит
				// подложку −78…−89 дБ отн. h1 вплоть до 4.8 кГц и −104 на 9.6).
				// Слушатель: «Recorder всё ещё тёмный, оригинал ярче и богаче».
				const float cutoffHz = 2600.0f;
				// Update 44: ФВЧ 0.90·f0 (потолок 1.2 кГц) → 0.55·f0 (потолок
				// 700 Гц). У банка на C6 широкая подложка НИЖЕ f0 (400-700 Гц на
				// −44…−53 дБ, .scratch/spectrum-dump.mjs), а наша обрезалась —
				// оставалась тонкая полоса, которую слушатель слышит как
				// «неприятный шум, как будто алиасинг». Гребёнка (0.80) держит
				// энергию у гармоник, белого шума не появляется.
				// Update 48: уровень растёт с регистром (+3.5 дБ на C6): замер v47
				// — на C5 мы совпали с банком, а на C6 были на 5-10 дБ тише в
				// 1.5-3.5 кГц.
				// Update 54: владелец — «на высоких октавах ненавистный белый шум,
				// а там вообще нет шума и дыхание не нужно». Шум размазан ровной
				// подложкой (гребёнка 0.15) и на C5/C6 читается как белый. Теперь
				// он гаснет по регистру (C4 1.0 / C5 0.55 / C6 0.12) и липнет к
				// гармоникам (гребёнка 0.55 ниже) — так он читается как юбка.
				// Update 55: гашение по регистру 1.0/0.55/0.12 снято почти
				// полностью (1.0/0.90/0.55). Замер (ATK=1): на C5 мы были
				// на 11-15 дБ, на C6 — на 24-42 дБ тише банка. «Шума нет»
				// (слушатель) — спад оставлен только как страховка на верху.
				// Update 55c: кривая по регистру — из замера. Банк делает
				// блокфлейту ТЕМ ШУМНЕЕ, чем выше нота (относительно h1):
				// в 600 Гц..10 кГц у него −70.5 дБ на C4 и −57.1 на C6.
				// Опора: 62/67/72/79/84/88 → +3.1/−2.4/−3.9/−4.3/−9.5/−8.8 дБ.
				static const float recLvlX[] = {0.0f, 7.0f/12.0f, 14.0f/12.0f};
				static const float recLvlK[] = {0.60f, 1.05f, 1.00f};
				const float recX = Math::Log(freq/261.63f)/Math::Log(2.0f);
				const float hpHz = Math::Min(0.75f*freq, 800.0f);
				return new NoiseSampler(freq, volume, sampleRate, 32768,
					0.00747f*MixParam(recLvlX, recLvlK, 3, recX), 150000003u,
					cutoffHz/freq, true, breathEnv, hpHz/freq, 3, 0.68f, 1u, RecorderVibrato(freq));
			});
	}
	{
		// Пикколо (GM 72) — отдельный инструмент, НЕ клон флейты. Замерено по
		// семплам Roland "piccolo" (#956-959) из того же SF2: пронзительный
		// тембр с очень сильной 2-й гармоникой (h2 −4..−6 дБ отн. h1 на E5),
		// на E6 тон чище (h2 −17, h3 −41), на верху снова яркий (D7: h2 −3).
		// Три опорные точки E5/E6/D7, лог-интерполяция, вне диапазона —
		// крайние профили.
		// (Восстановлено в Update 38: блок был потерян при поточных правках
		// flute-семьи — GM 72 молчал в маппинге, см. worklog.)
		static const HarmonicDesc piccE5[] = {
			{1.00f, 1, 0}, {0.56f, 2, 0}, {0.126f, 3, 0}, {0.100f, 4, 0},
			{0.025f, 5, 0}, {0.016f, 6, 0}, {0.010f, 7, 0}, {0.010f, 8, 0}};
		static const HarmonicDesc piccE6[] = {
			{1.00f, 1, 0}, {0.133f, 2, 0}, {0.0092f, 3, 0}, {0.0079f, 4, 0},
			{0.0010f, 5, 0}, {0.0007f, 6, 0}, {0.0004f, 7, 0}, {0.0003f, 8, 0}};
		static const HarmonicDesc piccD7[] = {
			{1.00f, 1, 0}, {0.68f, 2, 0}, {0.188f, 3, 0}, {0.209f, 4, 0},
			{0.030f, 5, 0}, {0.020f, 6, 0}, {0.012f, 7, 0}, {0.008f, 8, 0}};
		static const HarmonicSet setPiccE5 = Harms(SpanOf(piccE5));
		static const HarmonicSet setPiccE6 = Harms(SpanOf(piccE6));
		static const HarmonicSet setPiccD7 = Harms(SpanOf(piccD7));
		auto& t = Tables["Piccolo"];
		t.Generator = [](float freq, unsigned sampleRate)
		{
			// 0..1.6 октавы над E5 (659.3 Гц): сегмент 0 — E5→E6, сегмент 1 — E6→D7.
			const float x = Math::Clamp(Math::Log(freq/659.26f)/Math::Log(2.0f), 0.0f, 1.6f);
			const int seg = x >= 1.0f ? 1 : 0;
			const float v = seg == 0 ? x : (x - 1.0f)/0.6f;
			const HarmonicSet& a = seg == 0 ? setPiccE5 : setPiccE6;
			const HarmonicSet& b = seg == 0 ? setPiccE6 : setPiccD7;
			HarmonicSet mix;
			const size_t n = Math::Max(a.Harmonics.Length(), b.Harmonics.Length());
			for(size_t i = 0; i < n; i++)
			{
				const float av = i < a.Harmonics.Length() ? a.Harmonics[i].Amplitude : 0;
				const float bv = i < b.Harmonics.Length() ? b.Harmonics[i].Amplitude : 0;
				mix.Harmonics.AddLast({av + (bv - av)*v, float(i + 1), 0});
			}
			return BuildWaveTable(mix, 16384, freq, sampleRate);
		};
		t.AllowMipmaps = false;
		auto& wt = Instruments["Piccolo"].WaveTables.EmplaceLast();
		wt = Wt(&t, 0.12f, 0, {0.02f, 0, 1, 0.05f, 0, true, false});
	}
	{
		// Дутьё в бутылку (GM 76) — полый нечётно-гармонический тембр. Замерено
		// по семплам Roland "Bottle Blow" (#233, #952-955): C4..C6 почти
		// неизменен — h3 ≈ −19..−20 дБ, h5 ≈ −31..−34, h7 ≈ −31..−39, чётные
		// −37..−53 (очень слабые); низ (F#3, #233) почти чистый; верх (C7,
		// #952) ярче (h2 −20, h3 −12). Опорные точки F#3/C4/C6/C7 с
		// лог-интерполяцией по трём сегментам.
		static const HarmonicDesc botF3[] = {
			{1.00f, 1, 0}, {0.014f, 2, 0}, {0.012f, 3, 0}, {0.003f, 4, 0},
			{0.003f, 5, 0}, {0.001f, 6, 0}, {0.002f, 7, 0}, {0.001f, 8, 0}};
		static const HarmonicDesc botC4[] = {
			{1.00f, 1, 0}, {0.015f, 2, 0}, {0.116f, 3, 0}, {0.0033f, 4, 0},
			{0.030f, 5, 0}, {0.0022f, 6, 0}, {0.030f, 7, 0}, {0.0039f, 8, 0}};
		static const HarmonicDesc botC6[] = {
			{1.00f, 1, 0}, {0.011f, 2, 0}, {0.099f, 3, 0}, {0.0032f, 4, 0},
			{0.019f, 5, 0}, {0.0023f, 6, 0}, {0.011f, 7, 0}, {0.0013f, 8, 0}};
		static const HarmonicDesc botC7[] = {
			{1.00f, 1, 0}, {0.104f, 2, 0}, {0.243f, 3, 0}, {0.146f, 4, 0},
			{0.030f, 5, 0}, {0.020f, 6, 0}, {0.012f, 7, 0}, {0.008f, 8, 0}};
		static const HarmonicSet setBotF3 = Harms(SpanOf(botF3));
		static const HarmonicSet setBotC4 = Harms(SpanOf(botC4));
		static const HarmonicSet setBotC6 = Harms(SpanOf(botC6));
		static const HarmonicSet setBotC7 = Harms(SpanOf(botC7));
		auto& t = Tables["Bottle"];
		t.Generator = [](float freq, unsigned sampleRate)
		{
			// Октавы над F#3 (185.7 Гц): 0..0.491 — F#3→C4, 0.491..2.498 — C4→C6,
			// 2.498..3.407 — C6→C7; вне диапазона — крайние профили.
			const float x = Math::Clamp(Math::Log(freq/185.7f)/Math::Log(2.0f), 0.0f, 3.407f);
			float v; const HarmonicSet* a; const HarmonicSet* b;
			if(x < 0.491f) { v = x/0.491f; a = &setBotF3; b = &setBotC4; }
			else if(x < 2.498f) { v = (x - 0.491f)/(2.498f - 0.491f); a = &setBotC4; b = &setBotC6; }
			else { v = (x - 2.498f)/(3.407f - 2.498f); a = &setBotC6; b = &setBotC7; }
			HarmonicSet mix;
			const size_t n = Math::Max(a->Harmonics.Length(), b->Harmonics.Length());
			for(size_t i = 0; i < n; i++)
			{
				const float av = i < a->Harmonics.Length() ? a->Harmonics[i].Amplitude : 0;
				const float bv = i < b->Harmonics.Length() ? b->Harmonics[i].Amplitude : 0;
				mix.Harmonics.AddLast({av + (bv - av)*v, float(i + 1), 0});
			}
			return BuildWaveTable(mix, 16384, freq, sampleRate);
		};
		t.AllowMipmaps = false;
		auto& wt = Instruments["Bottle"].WaveTables.EmplaceLast();
		wt = Wt(&t, 0.14f, 0, {0.05f, 0, 1, 0.06f, 0, true, false});
		// Выдоха-шума здесь БОЛЬШЕ НЕТ (Update 42). Слушатель: «В Bottle тоже
		// не надо этого шума. Белый шум и полосатый запрещены! Они всегда дают
		// ужасный результат!» Этот слой был последним в семье духовых на СТАРОМ
		// рецепте — плоская белая подложка (NoiseSampler без ФВЧ/ФНЧ/гребёнки),
		// которая в банке на 20-25 дБ тише (Update 41, open item). Тембр бутылки
		// держится на нечётных гармониках таблицы; воздух вшивать не стали —
		// у слушателя он читается как шип.
	}
	{
		// Свисток (GM 78) — профиль Titanic/FluidR3 (Update 24; слушатель:
		// «Titanic нравится больше, FluidR3 звучит идентично, DLS — нет»;
		// Update 23 мерил/тюнил на Apple DLS — откат). Чистые замеры
		// (.scratch/whistle-dls-probe.mjs, БАНКИ с program 78 — в GM это
		// Whistle, а 77 — Shakuhachi): Titanic-свист — почти чистый тон с еле
		// заметной гребёнкой (h2 −55..−64 дБ отн. h1, выше нота — слабее),
		// настоящая шумовая подложка (−57..−64 дБ, плоская 2-8 кГц) и вибрато
		// ~5 Гц / ±7 центов (.scratch/whistle-vib-check.mjs). У DLS гребёнка
		// заметно сильнее (h2 ≈ −45) и нет ни шума, ни вибрато — из-за этого
		// он и звучит «дешевле». Три зоны: C4 (<65), G4 (65-85), C6 (86+).
		// Уровень зон в банке плоский — volumeScale 1.
		static const uint16 whistleC4[] = {32768, 55, 36, 17, 15, 11, 10, 9, 7, 6};  // C4: keys < 65
		static const uint16 whistleG4[] = {32768, 25, 15, 10, 7, 6, 5, 4, 4, 3};     // G4-D5: keys 65-85
		static const uint16 whistleC6[] = {32768, 27, 10, 9, 5, 4, 3, 3, 2, 2};      // C6: keys 86+
		static const HarmonicSet setWhistleC4 = Harms16(SpanOf(whistleC4));
		static const HarmonicSet setWhistleG4 = Harms16(SpanOf(whistleG4));
		static const HarmonicSet setWhistleC6 = Harms16(SpanOf(whistleC6));
		auto& t = Tables["Whistle"];
		t.Generator = [](float freq, unsigned sampleRate)
		{
			// x = октавы над C4; зоны C4/G4/C6.
			static const HarmonicSet* const zoneSets[] = {&setWhistleC4, &setWhistleG4, &setWhistleC6};
			static const float zoneX[] = {0.0f, 7.0f/12.0f, 2.0f};
			const float x = Math::Log(freq/261.63f)/Math::Log(2.0f);
			return BuildWaveTable(MixZoneSets(zoneSets, zoneX, 3, x), 16384, freq, sampleRate);
		};
		t.AllowMipmaps = false;
		auto& wt = Instruments["Whistle"].WaveTables.EmplaceLast();
		// Вибрато Titanic: 5 Гц / ±7 центов (0.004 ≈ ±0.4 %).
		// Update 47: атака 10 → 70 мс. Замер (.scratch/wind-attack.mjs, дБ отн.
		// сустейна h1): у нас 0-15 мс уже −2.3/−1.8 (мгновенный фронт), у банка
		// −19.0/−18.5, затем подъём до +4.8 к 60-120 мс. Слушатель: «атака
		// более заметная, у оригинала она плавнее».
		wt = Wt(&t, 0.14f, 0, {0.07f, 0, 1, 0.04f, 0, true, false}, 5, 0.004f);
		// Воздух свистка (переделано в Update 41). Прежний слой был белым
		// (срез 30·f0 ≈ без фильтра) и плоским до 18 кГц, а по уровню — на
		// 16..35 дБ ГРОМЧЕ банка в полосе 1.5-6 кГц; отсюда «свист ужасно
		// шумит». Замер банка (.scratch/noise-v41.mjs): воздух узкой полосой
		// ≈ −90 дБ отн. h1 на 1.5-3 кГц со спадом к 6-12 кГц (−104). Уровень
		// и полоса приведены к банку, воздух привязан к периоду ноты
		// гребёнкой (combGain 0.40), как у flute-семьи (Update 31).
		Instruments["Whistle"].GenericInstruments.EmplaceLast(
			[](float freq, float volume, unsigned sampleRate) -> GenericSamplerRef
			{
				static const EnvelopeDesc airEnv = {0.05f, 0, 1, 0.04f, 0, true, false};
				const float cutoffHz = Math::Min(3.0f*freq, 2800.0f);
				const float hpHz = Math::Min(0.90f*freq, 1200.0f);
				return new NoiseSampler(freq, volume, sampleRate, 32768, 0.0015f, 150000004u,
					cutoffHz/freq, true, airEnv, hpHz/freq, true, 0.40f);
			});
	}
	{
		// Окарина (GM 79) — замерено по петлям семплов Roland "Ocarina" из
		// Titanic (#2013 ocd3la, #2014 ocfs3la, #2015 ocgs3la, #2018 bnaflb5la;
		// банк переключает семпл на клавишах 62/63/68/93). Опорные точки
		// D4/F#4/G#4/B5, лог-интерполяция (h2..h8, дБ отн. h1):
		//   D4  −21.1/−22.3/−29.2/−34.6/−37.0/−37.1/−38.2
		//   F#4 −26.9/−36.1/−36.4/−43.4/−44.1/−47.8/−45.0
		//   G#4 −24.3/−23.0/−36.0/−35.6/−37.9/−41.4/−41.5
		//   B5  −30.2/−43.7/−36.0/−46.3/−57.9/−64.0/−57.3
		// До Update 39 окарина была почти чистым тоном (h2 −40 дБ, без h5..h8)
		// и не имела ничего общего с банком. Полосы (Bandwidth) не используются —
		// как у остальной flute-семьи.
		// Update 43: значения = измеренный рендер банка в опорных клавишах
		// (L-канал only). Клавиша 62: −34.4/−25.2/−35.3/−37.3/−41.8/−49.3/−50.3.
		static const HarmonicDesc ocarinaD4[] = {
			{1.00f, 1, 0}, {0.0191f, 2, 0}, {0.0549f, 3, 0}, {0.0172f, 4, 0},
			{0.0136f, 5, 0}, {0.0081f, 6, 0}, {0.0034f, 7, 0}, {0.0031f, 8, 0}};
		// Клавиша 66: −26.6/−39.8/−36.0/−51.9/−50.1/−53.3/−51.3 (Update 43).
		static const HarmonicDesc ocarinaFs4[] = {
			{1.00f, 1, 0}, {0.0468f, 2, 0}, {0.0102f, 3, 0}, {0.0159f, 4, 0},
			{0.0025f, 5, 0}, {0.0031f, 6, 0}, {0.0022f, 7, 0}, {0.0027f, 8, 0}};
		// Клавиша 80: −39.3/−27.1/−54.6/−45.2/−46.9/−65.1/−57.4 (Update 43).
		static const HarmonicDesc ocarinaGs4[] = {
			{1.00f, 1, 0}, {0.0108f, 2, 0}, {0.0442f, 3, 0}, {0.0019f, 4, 0},
			{0.0055f, 5, 0}, {0.0045f, 6, 0}, {0.0006f, 7, 0}, {0.0014f, 8, 0}};
		// Клавиша 95: −30.5/−41.9/−37.6/−47.6/−58.5/−66.8/−63.0 (Update 43).
		static const HarmonicDesc ocarinaB5[] = {
			{1.00f, 1, 0}, {0.0299f, 2, 0}, {0.0080f, 3, 0}, {0.0132f, 4, 0},
			{0.0042f, 5, 0}, {0.0012f, 6, 0}, {0.0005f, 7, 0}, {0.0007f, 8, 0}};
		static const HarmonicSet setOcarinaD4 = Harms(SpanOf(ocarinaD4));
		static const HarmonicSet setOcarinaFs4 = Harms(SpanOf(ocarinaFs4));
		static const HarmonicSet setOcarinaGs4 = Harms(SpanOf(ocarinaGs4));
		static const HarmonicSet setOcarinaB5 = Harms(SpanOf(ocarinaB5));
		auto& t = Tables["Ocarina"];
		t.Generator = [](float freq, unsigned sampleRate)
		{
			// Update 43c: зоны = фактические границы семплов банка
			// (ocd3la 0..62, ocfs3la 63..67, ocgs3la 68..92, bnaflb5la 93+).
			// Прежние 4 точки с интерполяцией ставили G#4-семпл только на
			// клавишу 80, и клавиши 68-79 (включая C5) уезжали до 23 дБ.
			const float key = 69.0f + 12.0f*Math::Log(freq/440.0f)/Math::Log(2.0f);
			const HarmonicSet* set = key < 62.5f ? &setOcarinaD4
				: (key < 67.5f ? &setOcarinaFs4
				: (key < 92.5f ? &setOcarinaGs4 : &setOcarinaB5));
			return BuildWaveTable(*set, 16384, freq, sampleRate);
		};
		t.AllowMipmaps = false;
		auto& wt = Instruments["Ocarina"].WaveTables.EmplaceLast();
		// Update 43: атака 3 → 45 мс (банк набирает уровень ~40-50 мс).
		// Update 43c: вибрато СНЯТО. 0.3 % — это относительное отклонение
		// скорости, поэтому FM-индекс k-й гармоники β = k·0.5 и несущая
		// падает как J0(β): h4 (β=2) −13 дБ, h5 (β=2.5) −26 дБ. Замер совпал:
		// наши h4/h5 на клавише 80 читались −71.0/−62.0 при собственной
		// таблице −54.6/−45.2, то есть инструмент терял верхние гармоники.
		// В рендере банка линии 5 Гц нет — вибрато ему не задаём.
		// Update 47: уровень 0.30 → 0.085. Замер (.scratch/skirt-v47.mjs, ключ
		// 74): наш сустейн −16.1 dBFS при банковских −34.5, а наша же пан-
		// флейта на той же клавише −27.5, то есть окарина была на 11.4 дБ
		// громче пан-флейты, тогда как в банке они равны. Слушатель:
		// «Ocarina вроде по громкости не та, наверное слишком громкая».
		// Update 54: владелец — «в 10 раз тише, а надо было в 2 раза»:
		// 0.30 → 0.15 (было 0.085, то есть 0.30/3.5).
		wt = Wt(&t, 0.15f, 0, {0.045f, 0, 1, 0.1f, 0, false, false});
	}
	{
		static const HarmonicDesc phoneRingH[] = {{1, 1, 3}};
		auto& t = Tables["PhoneRing"] = CreateWaveTables(Sets(Harms(SpanOf(phoneRingH))), 16384);
		auto& wt = Instruments["PhoneRing"].WaveTables.EmplaceLast();
		wt = Wt(&t, 0.5f, 0, {0, 0, 1, 0.5f, 0, true, true});
	}
	{
		static const HarmonicDesc kalimbaH[] = {{1, 1, 20}, {0.5f, 4, 46}, {0.25f, 8, 94}, {0.125f, 16, 190}, {0.0625f, 32, 380}};
		auto& t = Tables["Kalimba"] = CreateWaveTables(Sets(Harms(SpanOf(kalimbaH))), 32768);
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
			// NB: "Flute" (GM 73) and "Recorder" (GM 74) are NOT registered here -
			// they are defined by their own explicit blocks above (measured
			// SF2-sample spectrum + breath-noise layer); a spec entry would ADD a
			// second legacy wavetable layer on top of them.
			{"Flute2", nullptr, 32768, {
				{64, BW_3px, A_0_8_0_1rSqrt_pow, F_x, flute2Res, 2, false},
				{64, F_1200, A_0_01_0_02r, F_x, flute2Res, 2, false},
				{64, F_1200, A_0_02_0_01r, F_x, nullptr, 0, false}},
				0.1f, 0, 0.5f, 0.003f, {0.03f, 0.07f, 0.92f, 0.3f, 0, false, false}},
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
				Array<HarmonicSet> sets;
				for(const auto& se: sp.Series)
				{
					if(se.Num == 0) continue;
					HarmonicSet s = Series(se.Num, se.Bw, se.Amp, se.Fm);
					if(se.Resonances)
						s = Res(Move(s), Span<const ResonanceDesc>(se.Resonances, se.NumResonances), se.Multiplicative);
					sets.AddLast(Move(s));
				}
				t = &(Tables[sp.Name] = CreateWaveTables(sets, sp.TableSize));
			}
			auto& wt = Instruments[sp.Name].WaveTables.EmplaceLast();
			wt = Wt(t, sp.Volume, sp.ExpCoeff, sp.Env, sp.VibFreq, sp.VibVal);
		}

		// Инструменты с дополнительными модификаторами.
		Instruments["SynthBass1"].GenericModifiers.EmplaceLast(ExpExpModifierFactory(0.00003f, 5));
		Instruments["Flute"].GenericModifiers.EmplaceLast(CutoffFactory(600, 20000, 20000, 100, {0.035f, 0.05f, 0.92f, 0.06f, 0, true, false}));
	}

	// === Шумовые ===

	{
		auto& g = Instruments["ReverseCymbal"];
		g.GenericInstruments.EmplaceLast(NoiseInstrument{32768, 0.2f, 157898685});
		g.Envelope = MakeEnvelope({1, 0.1f, 1, 2, 0, true, false});
	}
	{
		auto& g = Instruments["Applause"];
		g.GenericInstruments.EmplaceLast(NoiseInstrument{32768, 0.05f, 157898685});
		g.Envelope = MakeEnvelope({0, 0, 1, 0.5f, 0, true, true});
		g.GenericModifiers.EmplaceLast(CutoffFactory(500, 5000, 5000, 500, {0, 0, 1, 0.5f, 0, true, true}));
	}
	{
		auto& g = Instruments["Helicopter"];
		g.GenericInstruments.EmplaceLast(NoiseInstrument{32768, 0.5f, 157898685});
		g.Envelope = MakeEnvelope({0, 0, 1, 0.5f, 0, true, true});
		g.GenericModifiers.EmplaceLast(CutoffFactory(500, 2000, 2000, 500, {0, 0, 1, 0.5f, 0, true, true}));
	}
	{
		auto& g = Instruments["Seashore"];
		g.GenericInstruments.EmplaceLast(NoiseInstrument{32768, 0.04f, 157898685});
		g.Envelope = MakeEnvelope({0, 0, 1, 0.5f, 0, true, true});
		g.GenericModifiers.EmplaceLast(CutoffFactory(500, 2000, 2000, 500, {0, 0, 1, 0.5f, 0, true, true}));
	}
	{
		auto& g = Instruments["Gunshot"];
		g.GenericInstruments.EmplaceLast(NoiseInstrument{32768, 0.4f, 157898685});
		g.Envelope = MakeEnvelope({0, 0, 1, 4, 0, true, true});
		g.GenericModifiers.EmplaceLast(ExpExpModifierFactory(0.00003f, 7));
	}
	{
		auto& g = Instruments["Timpani"];
		g.GenericInstruments.EmplaceLast(NoiseInstrument{32768, 0.2f, 157898685});
		g.Envelope = MakeEnvelope({0, 0, 1, 4, 0, true, true});
		g.GenericModifiers.EmplaceLast(ExpExpModifierFactory(0.00005f, 15));
	}

	// === Ударные, которых нет в web-midisynth (остаются) ===

	UniDrum = CachedDrumInstrument(Generators::DrumPhysicalModel(2, 16, 16, 0.342f, 0.00026f, 0.20f), 44100, 0.015f);
	ClosedHiHat = CachedDrumInstrument(Generators::DrumPhysicalModel(2, 16, 16, 0.338f, 0.04928f, 0.10f), 44100, 0.015f);
	AcousticBassDrum = CachedDrumInstrument(Generators::DrumPhysicalModel(2, 8, 8, 0.092f, 0.0072f, 0.20f), 88200, 0.015f);
	AcousticSnare = CachedDrumInstrument(SnarePhysicalModel(), 22000, 0.05f);
}
