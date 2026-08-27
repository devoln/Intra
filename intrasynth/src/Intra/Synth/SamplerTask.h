#pragma once

#include "Intra/Core.h"
#include "Intra/Assert.h"
#include "Utils/Unique.h"
#include "Utils/FixedArray.h"
#include "Intra/Range/Mutation/Fill.h"
#include "Intra/Range/Mutation/Transform.h"
#include "Intra/Range/Mutation/Copy.h"
#include "Container/Utility/Blob.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

class SamplerTaskContext;

/// Задача синтезатора: голос (через указатель) + номер канала + диапазон кадра.
/// Одноканальная задача пишет только в буфер своего канала и модифицирует только
/// своё per-channel состояние голоса, поэтому задачи разных каналов можно
/// выполнять параллельно (архитектурная заметка: до 3 задач на голос за кадр).
class SamplerTask
{
public:
    enum Channel: byte
    {
        LeftChannel = 0, RightChannel = 1,
        AllChannels = 2   // задача пишет во все dry-каналы (выполняется главным потоком)
    };

    enum ChannelFlag: byte
    {
        LeftChannelFlag = 1, RightChannelFlag = 2,
        ChannelFlags = LeftChannelFlag|RightChannelFlag
    };

    uint16 OffsetInSamples, NumSamples;
    byte ChannelIndex;
    // Относительная стоимость выполнения задачи (для распределения нагрузки между
    // потоками). Линейно зависит от числа семплов, с коэффициентом на тип задачи:
    // дешёвые задачи (обычное наложение волновых форм) имеют коэффициент 1,
    // тяжёлые (синтез NoteSampler с Karplus-Strong и огибающими) — больше.
    uint16 Cost;

    SamplerTask(size_t offsetInSamples, size_t numSamples, byte channel = AllChannels):
        OffsetInSamples(uint16(offsetInSamples)), NumSamples(uint16(numSamples)),
        ChannelIndex(channel), Cost(uint16(numSamples)) {}

    virtual ~SamplerTask() {}
    virtual void MoveConstruct(void* dst) = 0;
    virtual void operator()(SamplerTaskContext& stc) = 0;
};

typedef Intra::Container::DynamicBlob<SamplerTask, alignof(SamplerTask), uint16> SamplerTaskContainer;

/// Диапазон задач одного семплера [Begin, End) с суммарной ценой (для
/// распределения нагрузки между потоками). Задачи одного семплера обязаны
/// выполняться последовательно и в одном потоке: они модифицируют состояние
/// этого семплера.
struct SamplerJob { uint16 Begin, End, Cost; };

/// Два dry-канала кадра + выполнение задач. Master reverb получает отдельный
/// mono-send после сведения этих каналов в MidiSynth.
class SamplerTaskContext
{
    FixedArray<float> allSamples;
public:
    uint16 UsedChannels;
    Span<float> Channels[2];

    SamplerTaskContext& operator=(const SamplerTaskContext&) = delete;

    SamplerTaskContext(size_t frameLength):
        allSamples(frameLength*2), UsedChannels(0),
        Channels{
            allSamples.AsRange().Take(frameLength),
            allSamples.AsRange().Drop(frameLength).Take(frameLength)
        }
    {}

    void ClearChannels()
    {
        for(int i = 0; i < 2; i++) FillZeros(Channels[i]);
        UsedChannels = 0;
    }

    // Обнуляет только первые count семплов каждого канала. Нужен для переиспользуемых
    // контекстов рабочих потоков: их буферы выделяются на максимальную длину кадра,
    // а задачи каждого кадра трогают только первые count семплов.
    void ClearChannels(size_t count)
    {
        for(int i = 0; i < 2; i++) FillZeros(Channels[i].Take(count));
        UsedChannels = 0;
    }

    /// Последовательное выполнение задач (однопоточный режим / WASM).
    void RunTasks(const SamplerTaskContainer& tasks)
    {
        ClearChannels();
        for(size_t i = 0; i < tasks.Length(); i++)
        {
            SamplerTask* task = tasks[i];
            if(task) (*task)(*this);
        }
    }

    void MergeTo(SamplerTaskContext& dst) const
    {
        for(int i = 0; i < 2; i++)
        {
            if((UsedChannels & (1 << i)) == 0) continue;
            if((dst.UsedChannels & (1 << i)) == 0) CopyTo(Channels[i], dst.Channels[i]);
            else Add(dst.Channels[i], Channels[i]);
        }
        dst.UsedChannels |= UsedChannels;
    }
};

INTRA_WARNING_POP
