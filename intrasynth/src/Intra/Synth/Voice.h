#pragma once

#include <Cpp/Warnings.h>
#include <Cpp/Fundamental.h>
#include <Utils/Span.h>

#include "Envelope.h"
#include "ComputeKernels.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

/// Состояние ядра одного канала голоса: ровно 64 байта. Каналы разных голосов
/// обрабатываются разными потоками, поэтому состояние канала не должно разделяться
/// между потоками, а сами структуры выравниваются по 64 байта, чтобы избежать
/// false sharing (архитектурная заметка). Включает текущее состояние огибающей.
struct VoiceChannelState
{
    float Offset = 0;            // дробная позиция чтения в источнике
    float Rate = 0;              // скорость воспроизведения
    Envelope Env;                // огибающая канала (включая точки сегментов)
    float ExpFactor = 1;         // экспоненциальное затухание ноты
    float ExpStep = 1;           // его шаг: на семпл, либо на обёртку периода (предзатухание)
    float ChannelMultiplier = 0; // множитель канала (панорама / реверберация)
    byte Padding[12] = {0};      // до ровно 64 байт
};
static_assert(sizeof(VoiceChannelState) == 64, "VoiceChannelState must be exactly 64 bytes");

/// Общие константные данные голоса (не более 64 байт): источник и флаги.
struct VoiceSharedData
{
    const float* Fragment = nullptr; // период (волновая таблица / предзатухший фрагмент)
    unsigned FragmentLength = 0;
    bool PreAttenuated = false;      // экспоненциальное затухание вшито в данные фрагмента
    byte Padding[7] = {0};
};
static_assert(sizeof(VoiceSharedData) <= 64, "VoiceSharedData must fit in 64 bytes");

/// Голос: общие константные данные + по одному состоянию ядра на каждый канал
/// (левый, правый, реверберация). Голоса аллоцируются с выравниванием 64 байта.
/// Задача синтезатора = {голос, канал}: до 3 задач на голос за кадр.
struct Voice
{
    VoiceSharedData Shared;
    alignas(64) VoiceChannelState Channels[3];

    /// Голос закончен, когда закончилась огибающая левого канала (все каналы
    /// идут в ногу, левый рендерится всегда).
    bool Finished() const {return Channels[0].Env.CurrentSegment.SamplesLeft == 0;}

    void MultiplyPitch(float freqMultiplier)
    {
        for(auto& ch: Channels)
        {
            ch.Rate *= freqMultiplier;
            if(Abs(ch.Rate - 1) < 0.0001f) ch.Rate = 1;
        }
    }

    void MultiplyVolume(float volumeMultiplier)
    {for(auto& ch: Channels) ch.ExpFactor *= volumeMultiplier;}

    void SetPan(float pan)
    {
        Channels[1].ChannelMultiplier = (pan + 1) / 2.0f;
        Channels[0].ChannelMultiplier = 1 - Channels[1].ChannelMultiplier;
    }

    void SetReverbCoeff(float reverbCoeff)
    {Channels[2].ChannelMultiplier = reverbCoeff;}

    void NoteRelease()
    {for(auto& ch: Channels) ch.Env.StartLastSegment();}

    /// Рендер одного канала (без учёта ChannelMultiplier — масштабирование
    /// выполняет потребитель). Возвращает число обработанных семплов.
    size_t RenderChannel(size_t channel, Span<float> dst);

    /// Рендер всех активных каналов с учётом их множителей (L, R, реверберация).
    size_t RenderAll(Span<float> dstLeft, Span<float> dstRight, Span<float> dstReverb);
};

INTRA_WARNING_POP
