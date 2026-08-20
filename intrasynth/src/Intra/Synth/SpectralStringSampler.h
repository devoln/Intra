#pragma once

#include <Math/Math.h>

#include "Intra/Range/Span.h"
#include "Utils/FixedArray.h"
#include "Random/FastUniform.h"
#include "Types.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

/// Спектральная струна (модель FFT-1, вариант B из обсуждения замены KS).
///
/// Идея: у Karplus-Strong фундаментальная частота живёт потому, что петля
/// обратной связи = период ноты (усиление фильтра на DC = 1), а тембр задаёт
/// начальное возмущение. Здесь вместо петли — явная спектральная обработка:
///   1. начальное возмущение (тот же плектр, что у KS) раскладывается в FFT
///      один раз — получаем комплексные амплитуды гармоник X_k;
///   2. каждый период ноты спектр домножается на свой коэффициент затухания
///      g_k (разный для каждой гармоники), обратным FFT получается форма
///      нового периода, которая проигрывается по кругу (как вейвтейбл) с
///      линейной интерполяцией и кроссфейдом на границе периодов.
///
/// Фундаментал живёт по построению: гармоника k=1 имеет g_1 ≈ 1 в обоих
/// законах, т.е. ноту держит не фильтр, а сам период. При этом входом может
/// быть что угодно (сэмпл, FFT-синтезированный сигнал, любой вейвтейбл) —
/// в отличие от KS, где возмущение обязано быть коротким импульсом.
///
/// Законы затухания (выбираются в конструкторе):
///   - KS-закон (stiffness == 0): g_k = |(1-s) + s·e^{-j2πk/N}|^{P'}, где
///     s — per-sample smoothing фильтра KS, P' — точный период ноты в семплах.
///     Это в точности кривая затухания гармоник KS (фильтр, домноженный на
///     длину периода): при Brightness = 0 звук спектрально совпадает с KS.
///   - Stiffness-закон (stiffness > 0): g_k = exp(-stiffness·k²·P'/sampleRate) —
///     классическое затухание жёсткой струны (квадрат номера гармоники),
///     для фортепиано и т.п.
///
/// Скорость: IFFT раз в период (амортизированно ~2·log2(N) операций на
/// семпл), горячий цикл — чтение с интерполяцией + кроссфейд + огибающая.
/// IFFT полностью data-parallel (векторизуется), в отличие от последовательной
/// цепочки KS.
class SpectralStringSampler: public IGenericSampler
{
	// Спектр возмущения (N комплексных коэффициентов, эрмитово-симметричный,
	// т.к. это FFT реального сигнала) — домножается на mDecay каждый период.
	FixedArray<float> mSpecRe, mSpecIm;
	// Коэффициент затухания на период для каждого бина (g_k = g_{N-k}).
	FixedArray<float> mDecay;
	// Рабочие буферы обратного FFT.
	FixedArray<float> mFftRe, mFftIm;
	// Формы периодов: mWaveA — предыдущий период (хвост для кроссфейда),
	// mWaveB — текущий. Пинг-понг через Cpp::Swap в advancePeriod().
	FixedArray<float> mWaveA, mWaveB;
	size_t mN;             // размер FFT (степень двойки >= периода)
	size_t mHalf;          // mN/2
	size_t mPeriod;        // точный период ноты (длина линии задержки KS)
	float mRate;           // period / precisePeriod — скорость чтения формы
	float mPos;            // позиция чтения в форме периода [0, period)
	float mCrossfade;      // длина кроссфейда между периодами (в форме-координатах)
	float mVolume;
	float mExpStep;
	bool mHavePrev;        // есть предыдущий период (ложь только для первого)

	// Домножает спектр на затухание и пересчитывает форму текущего периода.
	void advancePeriod();

public:
	SpectralStringSampler(float freq, float volume, unsigned sampleRate,
		float damping, float smoothFactorBase, float smoothFactorMul, float smoothFactorExp,
		float brightness, float stiffness, float scale, float expCoeff, float cutoffRatio);

	/// Рендерит numSamples отсчётов. Лямбда-sink — как у KS: на wasm
	/// поинтер-инкремент в лямбде даёт лучший код, чем индексная запись.
	///
	/// На границе периода (pos оборачивается по mPeriod) вызывается
	/// advancePeriod(): спектр затухает, IFFT даёт новую форму, старый текущий
	/// период становится предыдущим. Голова нового периода (pos < mCrossfade)
	/// смешивается с хвостом предыдущего, чтобы скрыть скачок формы (KS это
	/// делает плавным фильтром в петле; здесь — явный кроссфейд).
	///
	/// ВАЖНО: читается только диапазон [0, mPeriod) формы (линия задержки KS),
	/// хвост [mPeriod, N) IFFT-буфера отбрасывается. Если читать весь N-буфер
	/// со скоростью N/precisePeriod, форма — это плектр в первых `period`
	/// сэмплах + нули до N: получается сжатый в ~N/period раз плектр и длинная
	/// пауза из нулей внутри каждого периода (артефакт «струна + шум», сильно
	/// зависящий от ноты). Скорость чтения — period/precisePeriod ≈ 1,
	/// ровно как KS циклирует свою линию задержки.
	template<typename TSink> void RenderInto(size_t numSamples, TSink&& sink)
	{
		const float rate = mRate;
		const size_t n = mPeriod;
		const float expStep = mExpStep;
		const float cf = mCrossfade;
		auto wA = mWaveA.AsRange();
		auto wB = mWaveB.AsRange();
		float pos = mPos;
		float vol = mVolume;
		bool havePrev = mHavePrev;

		while(numSamples)
		{
			if(pos >= float(n))
			{
				pos -= float(n);
				advancePeriod();
				havePrev = true;
				// После Cpp::Swap в advancePeriod() диапазоны указывают на
				// другие буферы — обновляем ссылки.
				wA = mWaveA.AsRange();
				wB = mWaveB.AsRange();
			}
			const size_t i = size_t(pos);
			const float frac = pos - float(i);
			const size_t j = i + 1 < n ? i + 1 : 0;
			float v;
			if(havePrev && pos < cf)
			{
				// Кроссфейд: голова нового периода с хвостом предыдущего.
				// Той же позиции времени в предыдущей форме соответствует
				// pos + (period - cf) — его последние cf семплов.
				const float prevPos = pos + float(n) - cf;
				const size_t pi = size_t(prevPos);
				const size_t pj = pi + 1 < n ? pi + 1 : 0;
				const float prevV = wA[pi] + (wA[pj] - wA[pi])*(prevPos - float(pi));
				const float curV = wB[i] + (wB[j] - wB[i])*frac;
				const float w = pos / cf;
				v = prevV*(1.0f - w) + curV*w;
			}
			else v = wB[i] + (wB[j] - wB[i])*frac;
			sink(v*vol);
			vol *= expStep;
			pos += rate;
			numSamples--;
		}
		mPos = pos;
		mVolume = vol;
		mHavePrev = havePrev;
	}

	size_t GenerateMono(Span<float> ioDst, Span<float> ioDstReverb) override;
	size_t GenerateStereo(Span<float> ioDstLeft, Span<float> ioDstRight, Span<float> ioDstReverb) override;

private:
	static unsigned randGen(float freq, float volume, unsigned sampleRate);
	static void generateExcitation(Span<float> dst, float damping, Random::FastUniform<float>& noise);
};

/// Фабрика инструментов. Параметры — как у KarplusStrongInstrument
/// (Damping/SmoothFactor* задают KS-закон), плюс:
///   - Brightness: 0 = в точности кривая затухания KS, ближе к 1 — слабее
///     демпфирование верхов (струна ярче и «поёт» дольше);
///   - Stiffness: > 0 — закон exp(-Stiffness·k²·P'/sr) вместо KS-закона.
struct SpectralStringInstrument
{
	float Damping;
	float SmoothFactorBase;
	float SmoothFactorMul;
	float SmoothFactorExp;
	float Brightness;
	float Stiffness;
	float Scale;
	float ExpCoeff;
	// Срез спектра возмущения (доля Найквиста, 0 — без среза). Эмулирует
	// звукосниматель электрогитары: убирает белую "пудру" плектра и главное —
	// продукты waveshaper'а выше ~2·fc, которые иначе алиасят и дают "физу"
	// (шум поверх струны). Нужен только инструментам с перегрузом.
	float CutoffRatio = 0;

	GenericSamplerRef operator()(float freq, float volume, unsigned sampleRate) const
	{
		return new SpectralStringSampler(freq, volume, sampleRate,
			Damping, SmoothFactorBase, SmoothFactorMul, SmoothFactorExp,
			Brightness, Stiffness, Scale, ExpCoeff, CutoffRatio);
	}
};

INTRA_WARNING_POP
