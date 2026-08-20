#pragma once

#include <Cpp/Warnings.h>
#include <Utils/Span.h>
#include <Math/Math.h>

#if defined(__AVX2__) && !defined(INTRA_NO_SIMD_KERNELS)
#include <immintrin.h>
#endif

#if defined(__wasm_simd128__) && !defined(INTRA_NO_SIMD_KERNELS)
#include <wasm_simd128.h>
#endif

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

/// Низкоуровневые вычислительные ядра синтезатора (архитектурная заметка:
/// нужный низкоуровневый функционал выносится в отдельный файл с вычислительными
/// ядрами). Ядра не имеют состояния: всё изменяемое состояние живёт в per-channel
/// структурах голоса (VoiceChannelState, по 64 байта), поэтому разные каналы
/// одного голоса можно обрабатывать параллельно без точек синхронизации.
///
/// Типы ядер из заметки:
///   1. MultiplyAdd с постоянным коэффициентом
///   2. MultiplyAdd с линейно меняющимся коэффициентом
///   3. MultiplyAddLinearInterpolated - с постоянной скоростью движения по источнику
///   4. С обратной связью (линейный фильтр) и стандартной скоростью движения
///   5. С обратной связью и постоянной скоростью движения по источнику
namespace SynthKernels
{
    /// 1. dst[i] += src[i] * k
    forceinline void MultiplyAddConst(Span<float> dst, Span<const float> src, float k)
    {
        const size_t n = Min(dst.Length(), src.Length());
        for(size_t i = 0; i < n; i++) dst[i] += src[i]*k;
    }

    /// 2. dst[i] += src[i] * (a + b*i)
    forceinline void MultiplyAddLinear(Span<float> dst, Span<const float> src, float a, float b)
    {
        const size_t n = Min(dst.Length(), src.Length());
        float amp = a;
        for(size_t i = 0; i < n; i++)
        {
            dst[i] += src[i]*amp;
            amp += b;
        }
    }

    /// 3. Движение по источнику с постоянной скоростью и линейной интерполяцией
    ///    (ioOffset += rate, обёртка по периоду). На выход умножаются две огибающие:
    ///    exp*expStep^i (на семпл) и ioWrapExp*wrapStep^k (на каждую обёртку периода -
    ///    используется для предварительно затухших данных WaveFormSampler).
    forceinline void MultiplyAddLinearInterpolated(Span<float> dst, Span<const float> src,
        float& ioOffset, float rate,
        float exp, float expStep, float& ioWrapExp, float wrapStep,
        float lin, float linStep)
    {
        const size_t len = src.Length();
        if(len == 0) return;
        float wrapExp = ioWrapExp;
        for(float& out: dst)
        {
            ioOffset += rate;
            if(ioOffset >= float(len))
            {
                ioOffset -= float(len);
                wrapExp *= wrapStep;
            }
            const int ii = int(ioOffset);
            const float frac = ioOffset - float(ii);
            const size_t i = size_t(ii);
            const size_t j = i + 1 < len ? i + 1 : 0;
            const float s = src[i] + (src[j] - src[i])*frac;
            out += s*exp*wrapExp*lin;
            exp *= expStep;
            lin += linStep;
        }
        ioWrapExp = wrapExp;
    }

    /// 4. Линейный фильтр 1-го порядка с обратной связью и стандартной скоростью
    ///    движения по источнику: state = src[i] + k*state; dst[i] += state*amp.
    forceinline void MultiplyAddFeedbackLinear(Span<float> dst, Span<const float> src,
        float& ioState, float k, float amp)
    {
        const size_t n = Min(dst.Length(), src.Length());
        float state = ioState;
        for(size_t i = 0; i < n; i++)
        {
            state = src[i] + k*state;
            dst[i] += state*amp;
        }
        ioState = state;
    }

    /// 5. Линейный фильтр 1-го порядка с обратной связью и постоянной
    ///    (интерполированной) скоростью движения по источнику.
    forceinline void MultiplyAddFeedbackInterpolated(Span<float> dst, Span<const float> src,
        float& ioOffset, float rate, float& ioState, float k, float amp)
    {
        const size_t len = src.Length();
        if(len == 0) return;
        float state = ioState;
        for(float& out: dst)
        {
            ioOffset += rate;
            if(ioOffset >= float(len)) ioOffset -= float(len);
            const int ii = int(ioOffset);
            const float frac = ioOffset - float(ii);
            const size_t i = size_t(ii);
            const size_t j = i + 1 < len ? i + 1 : 0;
            state = src[i] + (src[j] - src[i])*frac + k*state;
            out += state*amp;
        }
        ioState = state;
    }

    /// Быстрый вариант ядра 3 для постоянной амплитуды (sustain-сегмент
    /// огибающей): без пошаговых exp/lin огибающих остаётся только
    /// интерполяция и умножение на константу. Позиция хранится как целая
    /// часть + дробный аккумулятор — в горячей цепочке нет float->int
    /// конверсии и пошагового float-сложения.
    forceinline void AddInterpolatedConst(Span<float> dst, Span<const float> src,
        float& ioOffset, float rate, float amp)
    {
        const size_t len = src.Length();
        if(len == 0) return;
        const size_t rateInt = size_t(rate);
        const float rateFrac = rate - float(rateInt);
        size_t i = size_t(ioOffset);
        float frac = ioOffset - float(i);
        for(float& out: dst)
        {
            frac += rateFrac;
            if(frac >= 1.0f)
            {
                frac -= 1.0f;
                i += rateInt + 1;
            }
            else i += rateInt;
            if(i >= len) i -= len;
            const size_t j = i + 1 < len ? i + 1 : 0;
            out += (src[i] + (src[j] - src[i])*frac)*amp;
        }
        ioOffset = float(i) + frac;
    }

    /// Стерео-вариант AddInterpolatedConst: общие коэффициенты, без
    /// промежуточных буферов. Правый канал читается со сдвигом channelDelta
    /// от левого (та же дробь: оба канала переносят дробь одновременно),
    /// поэтому позиция и дробь считаются один раз на семпл.
    forceinline void AddInterpolatedConstStereo(Span<float> dstL, Span<float> dstR,
        Span<const float> src, float& ioOffsetL, float& ioOffsetR, float rate,
        float ampL, float ampR, size_t channelDelta)
    {
        const size_t len = src.Length();
        if(len == 0) return;
        const size_t rateInt = size_t(rate);
        const float rateFrac = rate - float(rateInt);
        size_t i = size_t(ioOffsetL);
        float frac = ioOffsetL - float(i);
        size_t iR = i + channelDelta;
        if(iR >= len) iR -= len;
        const size_t n = Min(dstL.Length(), dstR.Length());
        for(size_t k = 0; k < n; k++)
        {
            frac += rateFrac;
            if(frac >= 1.0f)
            {
                frac -= 1.0f;
                i += rateInt + 1;
                iR += rateInt + 1;
            }
            else
            {
                i += rateInt;
                iR += rateInt;
            }
            if(i >= len) i -= len;
            if(iR >= len) iR -= len;
            const size_t j = i + 1 < len ? i + 1 : 0;
            const size_t jR = iR + 1 < len ? iR + 1 : 0;
            const float s = src[i] + (src[j] - src[i])*frac;
            dstL[k] += s*ampL;
            dstR[k] += (src[iR] + (src[jR] - src[iR])*frac)*ampR;
        }
        ioOffsetL = float(i) + frac;
        ioOffsetR = float(iR) + frac;
    }

#if defined(__AVX2__) && !defined(INTRA_NO_SIMD_KERNELS)
    /// AVX2-вариант AddInterpolatedConstStereo: 8 семплов за итерацию,
    /// позиции накапливаются в векторе, семплы берутся gather-ом.
    forceinline void AddInterpolatedConstStereo8(Span<float> dstL, Span<float> dstR,
        Span<const float> src, float& ioOffsetL, float& ioOffsetR, float rate,
        float ampL, float ampR, size_t channelDelta)
    {
        const size_t len = src.Length();
        if(len == 0) return;
        const size_t n = Min(dstL.Length(), dstR.Length());
        float offL = ioOffsetL;
        const float* base = src.Data();
        const __m256 rate8 = _mm256_set1_ps(rate);
        const __m256 len8 = _mm256_set1_ps(float(len));
        const __m256i leni = _mm256_set1_epi32(int(len));
        const __m256i deltai = _mm256_set1_epi32(int(channelDelta));
        const __m256 ampaL = _mm256_set1_ps(ampL);
        const __m256 ampaR = _mm256_set1_ps(ampR);
        const __m256i one = _mm256_set1_epi32(1);
        const __m256i zeroi = _mm256_setzero_si256();
        const __m256i lenMinusOne = _mm256_sub_epi32(leni, one);
        // Первый выходной семпл — на позиции ioOffsetL + rate (как в скалярном ядре).
        offL += rate;
        if(offL >= float(len)) offL -= float(len);
        const __m256 step = _mm256_mul_ps(rate8,
            _mm256_set_ps(7, 6, 5, 4, 3, 2, 1, 0));
        const __m256 step8 = _mm256_set1_ps(rate*8.0f);
        __m256 pos = _mm256_add_ps(_mm256_set1_ps(offL), step);

        size_t k = 0;
        while(k + 8 <= n)
        {
            // Обёртка по периоду: pos -= len там, где pos >= len (7*rate < len).
            __m256 over = _mm256_cmp_ps(pos, len8, _CMP_GE_OQ);
            pos = _mm256_sub_ps(pos, _mm256_and_ps(over, len8));

            const __m256i ii = _mm256_cvttps_epi32(pos);
            const __m256 frac = _mm256_sub_ps(pos, _mm256_cvtepi32_ps(ii));

            // Левый канал: i, i+1 (с обёрткой последнего индекса).
            __m256i j = _mm256_add_epi32(ii, one);
            __m256i overJ = _mm256_cmpgt_epi32(j, lenMinusOne);
            j = _mm256_blendv_epi8(j, zeroi, overJ);
            __m256 a = _mm256_i32gather_ps(base, ii, 4);
            __m256 b = _mm256_i32gather_ps(base, j, 4);
            __m256 s = _mm256_add_ps(a, _mm256_mul_ps(_mm256_sub_ps(b, a), frac));

            // Правый канал: iR = (i + channelDelta) mod len, та же дробь.
            __m256i iR = _mm256_add_epi32(ii, deltai);
            __m256i overR = _mm256_cmpgt_epi32(iR, lenMinusOne);
            iR = _mm256_blendv_epi8(iR, _mm256_sub_epi32(iR, leni), overR);
            __m256i jR = _mm256_add_epi32(iR, one);
            __m256i overJR = _mm256_cmpgt_epi32(jR, lenMinusOne);
            jR = _mm256_blendv_epi8(jR, zeroi, overJR);
            __m256 aR = _mm256_i32gather_ps(base, iR, 4);
            __m256 bR = _mm256_i32gather_ps(base, jR, 4);
            __m256 sR = _mm256_add_ps(aR, _mm256_mul_ps(_mm256_sub_ps(bR, aR), frac));

            __m256 dL = _mm256_loadu_ps(dstL.Data() + k);
            __m256 dR = _mm256_loadu_ps(dstR.Data() + k);
            dL = _mm256_add_ps(dL, _mm256_mul_ps(s, ampaL));
            dR = _mm256_add_ps(dR, _mm256_mul_ps(sR, ampaR));
            _mm256_storeu_ps(dstL.Data() + k, dL);
            _mm256_storeu_ps(dstR.Data() + k, dR);

            offL += rate*8.0f;
            while(offL >= float(len)) offL -= float(len);
            pos = _mm256_add_ps(_mm256_set1_ps(offL), step);
            k += 8;
        }
        // Откат к последнему обработанному семплу, чтобы хвост совпал со скалярным ядром.
        offL -= rate;
        if(offL < 0.0f) offL += float(len);
        // Хвост — скалярно.
        for(; k < n; k++)
        {
            offL += rate;
            if(offL >= float(len)) offL -= float(len);
            const int ii = int(offL);
            const float frac = offL - float(ii);
            const size_t i = size_t(ii);
            const size_t j = i + 1 < len ? i + 1 : 0;
            size_t iR = i + channelDelta;
            if(iR >= len) iR -= len;
            size_t jR = iR + 1 < len ? iR + 1 : 0;
            const float s = src[i] + (src[j] - src[i])*frac;
            dstL[k] += s*ampL;
            dstR[k] += (src[iR] + (src[jR] - src[iR])*frac)*ampR;
        }
        ioOffsetL = offL;
        ioOffsetR = offL + float(channelDelta);
        if(ioOffsetR >= float(len)) ioOffsetR -= float(len);
    }
#endif

#if defined(__AVX2__) && !defined(INTRA_NO_SIMD_KERNELS)
    /// AVX2-вариант MultiplyAddLinearInterpolatedStereo: 8 семплов за итерацию.
    /// Позиции и интерполяция — как в AddInterpolatedConstStereo8 (gather),
    /// а пошаговые огибающие exp (геометрическая) и lin (линейная) разложены
    /// по линиям: lane k использует exp*expStep^k и lin+linStep*k.
    forceinline void MultiplyAddLinearInterpolatedStereo8(Span<float> dstL, Span<float> dstR,
        Span<const float> src, float& ioOffsetL, float& ioOffsetR, float rate,
        float exp, float expStep, float lin, float linStep, float ampL, float ampR,
        size_t channelDelta)
    {
        const size_t len = src.Length();
        if(len == 0) return;
        const size_t n = Min(dstL.Length(), dstR.Length());
        float offL = ioOffsetL;
        const float* base = src.Data();

        const __m256 rate8 = _mm256_set1_ps(rate);
        const __m256 len8 = _mm256_set1_ps(float(len));
        const __m256i leni = _mm256_set1_epi32(int(len));
        const __m256i deltai = _mm256_set1_epi32(int(channelDelta));
        const __m256 ampaL = _mm256_set1_ps(ampL);
        const __m256 ampaR = _mm256_set1_ps(ampR);
        const __m256i one = _mm256_set1_epi32(1);
        const __m256i zeroi = _mm256_setzero_si256();
        const __m256i lenMinusOne = _mm256_sub_epi32(leni, one);
        // Первый выходной семпл — на позиции ioOffsetL + rate (как в скалярном ядре).
        offL += rate;
        if(offL >= float(len)) offL -= float(len);
        const __m256 step = _mm256_mul_ps(rate8,
            _mm256_set_ps(7, 6, 5, 4, 3, 2, 1, 0));
        const __m256 step8 = _mm256_set1_ps(rate*8.0f);
        __m256 pos = _mm256_add_ps(_mm256_set1_ps(offL), step);

        // Линейная огибающая по линиям: lin + linStep*lane.
        __m256 linv = _mm256_add_ps(_mm256_set1_ps(lin),
            _mm256_mul_ps(_mm256_set1_ps(linStep),
                _mm256_set_ps(7, 6, 5, 4, 3, 2, 1, 0)));
        // Геометрическая огибающая по линиям: exp*expStep^lane.
        float e[8];
        e[0] = exp;
        for(int t = 1; t < 8; t++) e[t] = e[t - 1]*expStep;
        __m256 expv = _mm256_loadu_ps(e);

        float expStep8 = expStep;
        for(int t = 1; t < 8; t++) expStep8 *= expStep; // expStep^8
        const __m256 expStep8v = _mm256_set1_ps(expStep8);
        const __m256 linStep8v = _mm256_set1_ps(linStep*8.0f);

        size_t k = 0;
        while(k + 8 <= n)
        {
            __m256 over = _mm256_cmp_ps(pos, len8, _CMP_GE_OQ);
            pos = _mm256_sub_ps(pos, _mm256_and_ps(over, len8));

            const __m256i ii = _mm256_cvttps_epi32(pos);
            const __m256 frac = _mm256_sub_ps(pos, _mm256_cvtepi32_ps(ii));

            __m256i j = _mm256_add_epi32(ii, one);
            __m256i overJ = _mm256_cmpgt_epi32(j, lenMinusOne);
            j = _mm256_blendv_epi8(j, zeroi, overJ);
            __m256 a = _mm256_i32gather_ps(base, ii, 4);
            __m256 b = _mm256_i32gather_ps(base, j, 4);
            __m256 s = _mm256_add_ps(a, _mm256_mul_ps(_mm256_sub_ps(b, a), frac));

            __m256i iR = _mm256_add_epi32(ii, deltai);
            __m256i overR = _mm256_cmpgt_epi32(iR, lenMinusOne);
            iR = _mm256_blendv_epi8(iR, _mm256_sub_epi32(iR, leni), overR);
            __m256i jR = _mm256_add_epi32(iR, one);
            __m256i overJR = _mm256_cmpgt_epi32(jR, lenMinusOne);
            jR = _mm256_blendv_epi8(jR, zeroi, overJR);
            __m256 aR = _mm256_i32gather_ps(base, iR, 4);
            __m256 bR = _mm256_i32gather_ps(base, jR, 4);
            __m256 sR = _mm256_add_ps(aR, _mm256_mul_ps(_mm256_sub_ps(bR, aR), frac));

            const __m256 amp = _mm256_mul_ps(expv, linv);
            __m256 dL = _mm256_loadu_ps(dstL.Data() + k);
            __m256 dR = _mm256_loadu_ps(dstR.Data() + k);
            dL = _mm256_add_ps(dL, _mm256_mul_ps(s, _mm256_mul_ps(amp, ampaL)));
            dR = _mm256_add_ps(dR, _mm256_mul_ps(sR, _mm256_mul_ps(amp, ampaR)));
            _mm256_storeu_ps(dstL.Data() + k, dL);
            _mm256_storeu_ps(dstR.Data() + k, dR);

            offL += rate*8.0f;
            while(offL >= float(len)) offL -= float(len);
            pos = _mm256_add_ps(_mm256_set1_ps(offL), step);
            expv = _mm256_mul_ps(expv, expStep8v);
            linv = _mm256_add_ps(linv, linStep8v);
            k += 8;
        }

        // Откат к последнему обработанному семплу, чтобы хвост совпал со скалярным ядром.
        offL -= rate;
        if(offL < 0.0f) offL += float(len);
        // Хвост — скалярно, продолжая огибающие с lane 0 последнего чанка.
        float eTail = _mm256_cvtss_f32(expv);
        float lTail = _mm256_cvtss_f32(linv);
        for(; k < n; k++)
        {
            offL += rate;
            if(offL >= float(len)) offL -= float(len);
            const int ii = int(offL);
            const float frac = offL - float(ii);
            const size_t i = size_t(ii);
            const size_t j = i + 1 < len ? i + 1 : 0;
            size_t iR = i + channelDelta;
            if(iR >= len) iR -= len;
            size_t jR = iR + 1 < len ? iR + 1 : 0;
            const float amp = eTail*lTail;
            const float s = src[i] + (src[j] - src[i])*frac;
            dstL[k] += s*amp*ampL;
            dstR[k] += (src[iR] + (src[jR] - src[iR])*frac)*amp*ampR;
            eTail *= expStep;
            lTail += linStep;
        }
        ioOffsetL = offL;
        ioOffsetR = offL + float(channelDelta);
        if(ioOffsetR >= float(len)) ioOffsetR -= float(len);
    }
#endif

#if defined(__wasm_simd128__) && !defined(INTRA_NO_SIMD_KERNELS)
    namespace Impl
    {
        /// Сборка вектора из 4 скалярных загрузок по вычисленным индексам
        /// (в wasm нет gather: компилятор сводит это к load32_lane/load32_zero).
        forceinline v128_t Gather4(const float* base, v128_t idx)
        {
            v128_t v = wasm_v128_load32_zero(base + wasm_i32x4_extract_lane(idx, 0));
            v = wasm_v128_load32_lane(base + wasm_i32x4_extract_lane(idx, 1), v, 1);
            v = wasm_v128_load32_lane(base + wasm_i32x4_extract_lane(idx, 2), v, 2);
            v = wasm_v128_load32_lane(base + wasm_i32x4_extract_lane(idx, 3), v, 3);
            return v;
        }
    }

    /// wasm128-вариант AddInterpolatedConstStereo: 4 семпла за итерацию,
    /// позиции накапливаются в векторе (модель как в AVX2-версии, без gather).
    forceinline void AddInterpolatedConstStereo4(Span<float> dstL, Span<float> dstR,
        Span<const float> src, float& ioOffsetL, float& ioOffsetR, float rate,
        float ampL, float ampR, size_t channelDelta)
    {
        const size_t len = src.Length();
        if(len == 0) return;
        const size_t n = Min(dstL.Length(), dstR.Length());
        float offL = ioOffsetL;
        const float* base = src.Data();

        const v128_t rate4 = wasm_f32x4_splat(rate);
        const v128_t len4 = wasm_f32x4_splat(float(len));
        const v128_t leni = wasm_i32x4_splat(int(len));
        const v128_t deltai = wasm_i32x4_splat(int(channelDelta));
        const v128_t ampaL = wasm_f32x4_splat(ampL);
        const v128_t ampaR = wasm_f32x4_splat(ampR);
        const v128_t one = wasm_i32x4_splat(1);
        const v128_t zeroi = wasm_i32x4_splat(0);
        const v128_t lenMinusOne = wasm_i32x4_sub(leni, one);
        // Первый выходной семпл — на позиции ioOffsetL + rate (как в скалярном ядре).
        offL += rate;
        if(offL >= float(len)) offL -= float(len);
        const v128_t step = wasm_f32x4_mul(rate4, wasm_f32x4_make(0.0f, 1.0f, 2.0f, 3.0f));
        v128_t pos = wasm_f32x4_add(wasm_f32x4_splat(offL), step);

        size_t k = 0;
        while(k + 4 <= n)
        {
            // Обёртка по периоду: pos -= len там, где pos >= len.
            const v128_t over = wasm_f32x4_ge(pos, len4);
            pos = wasm_f32x4_sub(pos, wasm_v128_and(over, len4));

            const v128_t ii = wasm_i32x4_trunc_sat_f32x4(pos);
            const v128_t frac = wasm_f32x4_sub(pos, wasm_f32x4_convert_i32x4(ii));

            // Левый канал: i, i+1 (с обёрткой последнего индекса).
            v128_t j = wasm_i32x4_add(ii, one);
            const v128_t overJ = wasm_i32x4_gt(j, lenMinusOne);
            j = wasm_v128_bitselect(zeroi, j, overJ);
            const v128_t a = Impl::Gather4(base, ii);
            const v128_t b = Impl::Gather4(base, j);
            const v128_t s = wasm_f32x4_add(a, wasm_f32x4_mul(wasm_f32x4_sub(b, a), frac));

            // Правый канал: iR = (i + channelDelta) mod len, та же дробь.
            v128_t iR = wasm_i32x4_add(ii, deltai);
            const v128_t overR = wasm_i32x4_gt(iR, lenMinusOne);
            iR = wasm_v128_bitselect(wasm_i32x4_sub(iR, leni), iR, overR);
            v128_t jR = wasm_i32x4_add(iR, one);
            const v128_t overJR = wasm_i32x4_gt(jR, lenMinusOne);
            jR = wasm_v128_bitselect(zeroi, jR, overJR);
            const v128_t aR = Impl::Gather4(base, iR);
            const v128_t bR = Impl::Gather4(base, jR);
            const v128_t sR = wasm_f32x4_add(aR, wasm_f32x4_mul(wasm_f32x4_sub(bR, aR), frac));

            v128_t dL = wasm_v128_load(dstL.Data() + k);
            v128_t dR = wasm_v128_load(dstR.Data() + k);
            dL = wasm_f32x4_add(dL, wasm_f32x4_mul(s, ampaL));
            dR = wasm_f32x4_add(dR, wasm_f32x4_mul(sR, ampaR));
            wasm_v128_store(dstL.Data() + k, dL);
            wasm_v128_store(dstR.Data() + k, dR);

            offL += rate*4.0f;
            while(offL >= float(len)) offL -= float(len);
            pos = wasm_f32x4_add(wasm_f32x4_splat(offL), step);
            k += 4;
        }
        // Откат к последнему обработанному семплу, чтобы хвост совпал со скалярным ядром.
        offL -= rate;
        if(offL < 0.0f) offL += float(len);
        // Хвост — скалярно.
        for(; k < n; k++)
        {
            offL += rate;
            if(offL >= float(len)) offL -= float(len);
            const int ii = int(offL);
            const float frac = offL - float(ii);
            const size_t i = size_t(ii);
            const size_t j = i + 1 < len ? i + 1 : 0;
            size_t iR = i + channelDelta;
            if(iR >= len) iR -= len;
            size_t jR = iR + 1 < len ? iR + 1 : 0;
            const float s = src[i] + (src[j] - src[i])*frac;
            dstL[k] += s*ampL;
            dstR[k] += (src[iR] + (src[jR] - src[iR])*frac)*ampR;
        }
        ioOffsetL = offL;
        ioOffsetR = offL + float(channelDelta);
        if(ioOffsetR >= float(len)) ioOffsetR -= float(len);
    }

    /// wasm128-вариант MultiplyAddLinearInterpolatedStereo: 4 семпла за итерацию.
    /// Позиции и интерполяция — как в AddInterpolatedConstStereo4, а пошаговые
    /// огибающие exp (геометрическая) и lin (линейная) разложены по линиям:
    /// lane k использует exp*expStep^k и lin+linStep*k.
    forceinline void MultiplyAddLinearInterpolatedStereo4(Span<float> dstL, Span<float> dstR,
        Span<const float> src, float& ioOffsetL, float& ioOffsetR, float rate,
        float exp, float expStep, float lin, float linStep, float ampL, float ampR,
        size_t channelDelta)
    {
        const size_t len = src.Length();
        if(len == 0) return;
        const size_t n = Min(dstL.Length(), dstR.Length());
        float offL = ioOffsetL;
        const float* base = src.Data();

        const v128_t rate4 = wasm_f32x4_splat(rate);
        const v128_t len4 = wasm_f32x4_splat(float(len));
        const v128_t leni = wasm_i32x4_splat(int(len));
        const v128_t deltai = wasm_i32x4_splat(int(channelDelta));
        const v128_t ampaL = wasm_f32x4_splat(ampL);
        const v128_t ampaR = wasm_f32x4_splat(ampR);
        const v128_t one = wasm_i32x4_splat(1);
        const v128_t zeroi = wasm_i32x4_splat(0);
        const v128_t lenMinusOne = wasm_i32x4_sub(leni, one);
        offL += rate;
        if(offL >= float(len)) offL -= float(len);
        const v128_t step = wasm_f32x4_mul(rate4, wasm_f32x4_make(0.0f, 1.0f, 2.0f, 3.0f));
        v128_t pos = wasm_f32x4_add(wasm_f32x4_splat(offL), step);

        // Линейная огибающая по линиям: lin + linStep*lane.
        v128_t linv = wasm_f32x4_add(wasm_f32x4_splat(lin),
            wasm_f32x4_mul(wasm_f32x4_splat(linStep),
                wasm_f32x4_make(0.0f, 1.0f, 2.0f, 3.0f)));
        // Геометрическая огибающая по линиям: exp*expStep^lane.
        float e[4];
        e[0] = exp;
        for(int t = 1; t < 4; t++) e[t] = e[t - 1]*expStep;
        v128_t expv = wasm_v128_load(e);

        float expStep4 = expStep;
        for(int t = 1; t < 4; t++) expStep4 *= expStep; // expStep^4
        const v128_t expStep4v = wasm_f32x4_splat(expStep4);
        const v128_t linStep4v = wasm_f32x4_splat(linStep*4.0f);

        size_t k = 0;
        while(k + 4 <= n)
        {
            const v128_t over = wasm_f32x4_ge(pos, len4);
            pos = wasm_f32x4_sub(pos, wasm_v128_and(over, len4));

            const v128_t ii = wasm_i32x4_trunc_sat_f32x4(pos);
            const v128_t frac = wasm_f32x4_sub(pos, wasm_f32x4_convert_i32x4(ii));

            v128_t j = wasm_i32x4_add(ii, one);
            const v128_t overJ = wasm_i32x4_gt(j, lenMinusOne);
            j = wasm_v128_bitselect(zeroi, j, overJ);
            const v128_t a = Impl::Gather4(base, ii);
            const v128_t b = Impl::Gather4(base, j);
            const v128_t s = wasm_f32x4_add(a, wasm_f32x4_mul(wasm_f32x4_sub(b, a), frac));

            v128_t iR = wasm_i32x4_add(ii, deltai);
            const v128_t overR = wasm_i32x4_gt(iR, lenMinusOne);
            iR = wasm_v128_bitselect(wasm_i32x4_sub(iR, leni), iR, overR);
            v128_t jR = wasm_i32x4_add(iR, one);
            const v128_t overJR = wasm_i32x4_gt(jR, lenMinusOne);
            jR = wasm_v128_bitselect(zeroi, jR, overJR);
            const v128_t aR = Impl::Gather4(base, iR);
            const v128_t bR = Impl::Gather4(base, jR);
            const v128_t sR = wasm_f32x4_add(aR, wasm_f32x4_mul(wasm_f32x4_sub(bR, aR), frac));

            const v128_t amp = wasm_f32x4_mul(expv, linv);
            v128_t dL = wasm_v128_load(dstL.Data() + k);
            v128_t dR = wasm_v128_load(dstR.Data() + k);
            dL = wasm_f32x4_add(dL, wasm_f32x4_mul(s, wasm_f32x4_mul(amp, ampaL)));
            dR = wasm_f32x4_add(dR, wasm_f32x4_mul(sR, wasm_f32x4_mul(amp, ampaR)));
            wasm_v128_store(dstL.Data() + k, dL);
            wasm_v128_store(dstR.Data() + k, dR);

            offL += rate*4.0f;
            while(offL >= float(len)) offL -= float(len);
            pos = wasm_f32x4_add(wasm_f32x4_splat(offL), step);
            expv = wasm_f32x4_mul(expv, expStep4v);
            linv = wasm_f32x4_add(linv, linStep4v);
            k += 4;
        }

        // Откат к последнему обработанному семплу, чтобы хвост совпал со скалярным ядром.
        offL -= rate;
        if(offL < 0.0f) offL += float(len);
        // Хвост — скалярно, продолжая огибающие с lane 0 последнего чанка.
        float eTail = wasm_f32x4_extract_lane(expv, 0);
        float lTail = wasm_f32x4_extract_lane(linv, 0);
        for(; k < n; k++)
        {
            offL += rate;
            if(offL >= float(len)) offL -= float(len);
            const int ii = int(offL);
            const float frac = offL - float(ii);
            const size_t i = size_t(ii);
            const size_t j = i + 1 < len ? i + 1 : 0;
            size_t iR = i + channelDelta;
            if(iR >= len) iR -= len;
            size_t jR = iR + 1 < len ? iR + 1 : 0;
            const float amp = eTail*lTail;
            const float s = src[i] + (src[j] - src[i])*frac;
            dstL[k] += s*amp*ampL;
            dstR[k] += (src[iR] + (src[jR] - src[iR])*frac)*amp*ampR;
            eTail *= expStep;
            lTail += linStep;
        }
        ioOffsetL = offL;
        ioOffsetR = offL + float(channelDelta);
        if(ioOffsetR >= float(len)) ioOffsetR -= float(len);
    }
#endif

    /// Стерео-вариант ядра 3: общие пошаговые огибающие exp/lin для обоих
    /// каналов, позиция (целая + дробь) считается один раз на семпл.
    forceinline void MultiplyAddLinearInterpolatedStereo(Span<float> dstL, Span<float> dstR,
        Span<const float> src, float& ioOffsetL, float& ioOffsetR, float rate,
        float exp, float expStep, float lin, float linStep, float ampL, float ampR,
        size_t channelDelta)
    {
        const size_t len = src.Length();
        if(len == 0) return;
        const size_t rateInt = size_t(rate);
        const float rateFrac = rate - float(rateInt);
        size_t i = size_t(ioOffsetL);
        float frac = ioOffsetL - float(i);
        size_t iR = i + channelDelta;
        if(iR >= len) iR -= len;
        const size_t n = Min(dstL.Length(), dstR.Length());
        for(size_t k = 0; k < n; k++)
        {
            frac += rateFrac;
            if(frac >= 1.0f)
            {
                frac -= 1.0f;
                i += rateInt + 1;
                iR += rateInt + 1;
            }
            else
            {
                i += rateInt;
                iR += rateInt;
            }
            if(i >= len) i -= len;
            if(iR >= len) iR -= len;
            const size_t j = i + 1 < len ? i + 1 : 0;
            const size_t jR = iR + 1 < len ? iR + 1 : 0;
            const float amp = exp*lin;
            const float s = src[i] + (src[j] - src[i])*frac;
            dstL[k] += s*amp*ampL;
            dstR[k] += (src[iR] + (src[jR] - src[iR])*frac)*amp*ampR;
            exp *= expStep;
            lin += linStep;
        }
        ioOffsetL = float(i) + frac;
        ioOffsetR = float(iR) + frac;
    }
}

INTRA_WARNING_POP
