#pragma once

#include <Cpp/Warnings.h>

#include <Math/Math.h>
#include <Random/FastUniform.h>
#include <Container/Sequential/Array.h>

#include <Range/Mutation/Copy.h>
#include <Range/Mutation/Transform.h>

#include "Types.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

/// Одноразовый генератор малого барабана (snare) — порт алгоритма GenSnare +
/// FilterQ + Fade + Norm из devoln/web-midisynth (исходно VB-синтезатор).
///
/// Алгоритм: затухающий синусоидальный «удар» (щелчок) плюс два слоя шума
/// (белый и сглаженный с инерцией 0.99):
///   Ar(i) = Sin(2900000/(i+9000)) / (i*i*0.00015 + 200)
///         + Rand / (i*i*0.01 + 1000)
///         + RandR / (i*i*0.005 + 5000)
/// затем резонансный ФНЧ (FilterQ):
///   F = 2*pi*5000/sr,  p = p + s*F + x;  s = (s - p*F)*0.8;  out = p,
/// линейный fade-вход 50 и fade-выход 5000 семплов, нормализация к пику 1.
///
/// Все величины алгоритма заданы на «44100-часах» (22000 семплов = ~0.5 с).
/// Класс масштабирует их под фактический sample rate, который восстанавливается
/// из длины буфера: кеш ударника (CachedDrumInstrument) выделяется из расчёта
/// 22000 семплов на 44100 Гц, поэтому одна и та же волна звучит одинаково на
/// любом sample rate. Шум — через Random::FastUniform (как в остальных
/// инструментах), с фиксированным зерном, чтобы кеш пересобирался
/// детерминированно.
class SnarePhysicalModel
{
	Random::FastUniform<float> mRandom;
	float mSmoothedRand = 0;
	Array<float> mBuffer;

public:
	explicit SnarePhysicalModel(uint seed = 3735928559u): mRandom(seed) {}

	/// Сглаженный шум (порт RandR: (RR + Rand - 0.5) * 0.99).
	INTRA_FORCEINLINE float NextSmoothedRand()
	{
		mSmoothedRand = (mSmoothedRand + mRandom() - 0.5f)*0.99f;
		return mSmoothedRand;
	}

	/// Генерирует сэмпл малого барабана в dst (добавляет, если add == true).
	/// Буфер заполняется целиком (после конца сэмпла — тишина), возвращается
	/// необработанный остаток dst (пустой).
	Span<float> operator()(Span<float> dst, bool add);
};

Span<float> SnarePhysicalModel::operator()(Span<float> dst, bool add)
{
	const size_t n = dst.Length();
	if(n == 0) return dst;

	// Фактический sample rate из длины буфера кеша (22000 семплов на 44100 Гц).
	const float sampleRate = 44100.0f*float(n)/22000.0f;
	const float i44 = 44100.0f/sampleRate; // сколько «44100-семплов» в одном выходном

	if(mBuffer.Length() != n) mBuffer.SetCount(n);
	auto buf = mBuffer.AsRange();

	// 1) Сырой сэмпл: затухающий щелчок + два слоя шума.
	for(size_t i = 0; i < n; i++)
	{
		const float t = float(i)*i44;
		float sample = Math::Sin(2900000.0f/(t + 9000.0f))/(t*t*0.00015f + 200.0f);
		sample += mRandom()/(t*t*0.01f + 1000.0f);
		sample += NextSmoothedRand()/(t*t*0.005f + 5000.0f);
		buf[i] = sample;
	}

	// 2) Резонансный ФНЧ (FilterQ): F = 2*pi*5000/sr (в оригинале Frq/7019,
	//    7019 = 44100/2*pi), k = 0.8.
	{
		const float F = 5000.0f*(2.0f*float(Math::PI))/sampleRate;
		const float k = 0.8f;
		float p = 0, s = 0;
		for(size_t i = 0; i < n; i++)
		{
			const float x = buf[i];
			p = p + s*F + x;
			s = (s - p*F)*k;
			buf[i] = p;
		}
	}

	// 3) Fade-вход 50 и fade-выход 5000 семплов (на 44100 Гц).
	{
		const size_t fadeIn = size_t(50.0f*i44 + 0.5f);
		const size_t fadeOut = size_t(5000.0f*i44 + 0.5f);
		const size_t fadeInEnd = Math::Min(fadeIn, n);
		for(size_t i = 0; i < fadeInEnd; i++) buf[i] *= float(i)/float(fadeInEnd);
		const size_t fadeOutStart = fadeOut < n? n - fadeOut: 0;
		const float fadeOutLen = float(n - 1 - fadeOutStart);
		if(fadeOutLen > 0)
		{
			for(size_t i = fadeOutStart; i < n; i++)
				buf[i] *= float(n - 1 - i)/fadeOutLen;
		}
	}

	// 4) Нормализация к пику 1 (Norm 1).
	{
		float peak = 0;
		for(size_t i = 0; i < n; i++)
		{
			const float ax = Math::Abs(buf[i]);
			if(ax > peak) peak = ax;
		}
		if(peak > 1e-9f)
		{
			const float inv = 1.0f/peak;
			for(size_t i = 0; i < n; i++) buf[i] *= inv;
		}
	}

	if(add) Add(dst, buf);
	else CopyTo(buf, dst);
	return dst.Drop(n);
}

INTRA_WARNING_POP
