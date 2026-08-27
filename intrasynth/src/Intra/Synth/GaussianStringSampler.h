#pragma once

#include <Math/Math.h>

#include "Intra/Range/Span.h"
#include "Utils/FixedArray.h"
#include "Random/FastUniform.h"
#include "Types.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

/// Экспериментальная альтернатива Karplus-Strong: струна-плектр без петли
/// обратной связи. Начальное возмущение (то же самое, что у KS) считается
/// периодическим буфером периода len; позиция чтения ходит по нему по кругу
/// с той же скоростью, что у KS (rate = len/precisePeriod), а каждый выходной
/// семпл — это свёртка буфера с тройным скользящим средним (квадратичный
/// B-сплайн, аппроксимирующий гаусс), радиус которого R растёт со временем:
///   R(t) = min(RadiusA * sqrt(t), RadiusCap * len)
/// Растущий радиус гасит высокие гармоники, как фильтр обратной связи KS.
/// Буфер иммутабелен (не считая кэшей средних) — это позволяет класть в него
/// любой сигнал (в т.ч. сгенерированный через обратное FFT), не меняя модель.
///
/// Спектральное сравнение с KS (scripts/_tmp-ma-ks-proto.cpp) показало, что
/// точной заменой он не является: фильтр KS имеет единичное усиление на DC,
/// поэтому фундаментальная частота живёт до конца огибающей, а любой
/// пространственный low-pass с радиусом >= len/5 заметно режет и её (на
/// лучшей подгонке R(n)=0.30*sqrt(n) фундаментал теряет 15-85 дБ сверх
/// огибающей). Лучшее соответствие — порядок затухания гармоник, не детали.
/// Как инструмент-технология (иммутабельный буфер + эволюция тембра) — да.
///
/// Реализация O(1) на семпл: box-среднее по круговому периодическому буферу
/// считается через префиксные суммы (2*len), а три прохода пересчитываются
/// за O(len) только когда R меняется (при R ~ sqrt(n) — раз в сотни семплов).
/// "Повтор возмущения до 16384 семплов" из исходной идеи эквивалентен учёту
/// по модулю len: буфер периодичен, окно усреднения не отличает линейную
/// развёртку от круговой.
class GaussianStringSampler: public IGenericSampler
{
	FixedArray<float> mLine;    // возмущение, период len
	FixedArray<float> mPrefix;  // префиксные суммы 2*len (box в O(1))
	FixedArray<float> mAvg1;    // кэш первого прохода (с префиксом в mAvgPref)
	FixedArray<float> mAvgPref;
	FixedArray<float> mAvg2;    // кэш второго прохода — читаем из него
	size_t mLen;
	float mPos;                 // позиция чтения (float-аккумулятор)
	float mRate;                // len/precisePeriod (как у KS)
	float mVolume;
	float mExpStep;
	float mRadiusA;
	float mRadiusCap;           // в долях len
	float mTime;                // счётчик семплов (float, чтобы не гонять uint64->float)
	long mLastR;
	float mInvWidth;

	float boxAt(long p, long R) const
	{
		// Круговой диапазон [p-R, p+R] периода len -> линейный в 2*len.
		// Деление на (2R+1) заменено умножением на обратную величину: на wasm
		// f32 деление дорогое, а это вызывается на каждый элемент кэша.
		const long w = long(mLen)*2;
		long lo = (p - R) % w; if(lo < 0) lo += w;
		long hi = (p + R) % w;
		const float s = (lo <= hi)
			? (mPrefix[hi+1] - mPrefix[lo])
			: (mPrefix[w] - mPrefix[lo] + mPrefix[hi+1]);
		return s*mInvWidth;
	}

	void buildPrefix(const FixedArray<float>& src)
	{
		const size_t w = mLen*2;
		mPrefix.SetCount(w + 1);
		mPrefix[0] = 0;
		for(size_t i = 0; i < w; i++) mPrefix[i+1] = mPrefix[i] + src[i % mLen];
	}

	// Пересчитывает три прохода box-среднего для радиуса R (R неизменен ->
	// вызов дорогой; вызывается только при смене R, см. RenderInto).
	void rebuildAverages(long R)
	{
		mInvWidth = 1.0f/float(2*R + 1);
		buildPrefix(mLine);
		mAvg1.SetCount(mLen);
		for(size_t q = 0; q < mLen; q++) mAvg1[q] = boxAt(long(q), R);
		buildPrefix(mAvg1);
		mAvg2.SetCount(mLen);
		for(size_t q = 0; q < mLen; q++) mAvg2[q] = boxAt(long(q), R);
		mLastR = R;
	}

public:
	GaussianStringSampler(float freq, float volume, unsigned sampleRate,
		float damping, float smoothFactorBase, float smoothFactorMul, float smoothFactorExp,
		float scale, float expCoeff, float radiusA, float radiusCap);

	/// Рендерит numSamples отсчётов. Лямбда-sink — как у KS: на wasm
	/// поинтер-инкремент в лямбде даёт лучший код, чем индексная запись.
	template<typename TSink> void RenderInto(size_t numSamples, TSink&& sink)
	{
		const float rate = mRate;
		const size_t len = mLen;
		const float expStep = mExpStep;
		const long capR = long(mRadiusCap*float(len));
		const float a = mRadiusA;
		float pos = mPos;
		float vol = mVolume;
		float t = mTime;
		// Блочная версия: пересчёт радиуса (и пересборка кэшей средних) вынесен
		// из пер-семплового цикла. Вызов rebuildAverages внутри цикла мешал LLVM
		// разворачивать/конвейеризовать петлю; с блоками по 64 семпла внутренний
		// цикл чистый (загрузка + фильтр + запись), а R обновляется раз в
		// сотни-тысячи семплов, поэтому пересборка дешевле. Цена: R переключается
		// на границе блока, т.е. закон R(t) квантован по времени (задержка до 64
		// семплов) — на слух незаметно, но вывод не бит-в-бит с пер-семпловой
		// версией и слегка зависит от границ вызова рендера.
		constexpr size_t BLOCK = 64;
		while(numSamples >= BLOCK)
		{
			// R(t) = RadiusA*sqrt(t) — диффузионный закон, подобранный спектральным
			// фитом (scripts/_tmp-ma-ks-proto.cpp). sqrt — одна инструкция wasm.
			long R = long(a*Math::Sqrt(t));
			if(R > capR) R = capR;
			if(R != mLastR) rebuildAverages(R);
			for(size_t k = 0; k < BLOCK; k++)
			{
				// pos всегда в [0, len): обёртка ниже гарантирует, поэтому % не нужен
				// (на wasm деление дорогое).
				const size_t ip = size_t(pos);
				sink(mAvg2[ip]*vol);
				pos += rate;
				if(pos >= float(len)) pos -= float(len);
				vol *= expStep;
				t += 1.0f;
			}
			numSamples -= BLOCK;
		}
		for(size_t k = 0; k < numSamples; k++)
		{
			long R = long(a*Math::Sqrt(t));
			if(R > capR) R = capR;
			if(R != mLastR) rebuildAverages(R);
			const size_t ip = size_t(pos);
			sink(mAvg2[ip]*vol);
			pos += rate;
			if(pos >= float(len)) pos -= float(len);
			vol *= expStep;
			t += 1.0f;
		}
		mPos = pos;
		mVolume = vol;
		mTime = t;
	}

	size_t GenerateMono(Span<float> ioDst) override;
	size_t GenerateStereo(Span<float> ioDstLeft, Span<float> ioDstRight) override;

private:
	static unsigned randGen(float freq, float volume, unsigned sampleRate);
	static void generateExcitation(Span<float> dst, float damping, Random::FastUniform<float>& noise);
};

/// Фабрика инструментов: параметры те же, что у KarplusStrongInstrument,
/// плюс закон роста радиуса R(t) = RadiusA*sqrt(t), кап RadiusCap*len.
struct GaussianStringInstrument
{
	float Damping;
	float SmoothFactorBase;
	float SmoothFactorMul;
	float SmoothFactorExp;
	float Scale;
	float ExpCoeff;
	float RadiusA;
	float RadiusCap;

	GenericSamplerRef operator()(float freq, float volume, unsigned sampleRate) const
	{
		return new GaussianStringSampler(freq, volume, sampleRate,
			Damping, SmoothFactorBase, SmoothFactorMul, SmoothFactorExp, Scale, ExpCoeff,
			RadiusA, RadiusCap);
	}
};

INTRA_WARNING_POP
