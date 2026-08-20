#pragma once

#ifndef __EMSCRIPTEN__

#include "SamplerTask.h"

#include "Concurrency/Thread.h"
#include "Concurrency/Atomic.h"
#include "Concurrency/CondVar.h"
#include "System/ProcessorInfo.h"
#include "Utils/Unique.h"
#include "Container/Sequential/Array.h"
#include "Cpp/Fundamental.h"
#include "Math/Math.h"

#include "Range/Mutation/Fill.h"
#include "Range/Mutation/Transform.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

/// Параллельный исполнитель задач синтезатора (только нативные сборки).
///
/// Реализует схему распределения нагрузки из архитектурной заметки:
///   1. Задачи (сгруппированные по голосам-семплерам) имеют заранее посчитанную
///      цену (SamplerTask::Cost), линейно зависящую от числа семплов.
///   2. Задачи раскладываются по слотам жадным алгоритмом «наименее загруженный
///      слот», чтобы суммарная цена была как можно ровнее.
///   3. Главный поток берёт себе самую нагруженную очередь, остальные очереди
///      раздаются рабочим потокам.
///   4. Главный поток выполняет свою очередь и ждёт завершения рабочих через
///      Condition Variable (без busy-wait): каждый рабочий уменьшает общий
///      счётчик завершения и сигналит главному.
///
/// Рабочие потоки живут постоянно (создаются один раз при построении синтезатора).
/// Каждый рабочий пишет только в свой приватный буфер каналов, поэтому при записи
/// синхронизация не нужна; после завершения всех рабочих буферы суммируются в
/// выходной контекст главным потоком.
class ParallelSynthExecutor
{
public:
	static const uint16 MaxWorkers = 8;        // предел числа рабочих потоков (не считая главного)
	static const uint16 MaxFrameSamples = 8192; // максимальная длина кадра в приватных буферах
	static const uint16 MaxJobs = 512;          // максимум голосов-задач на кадр (иначе последовательно)
	static const uint16 MinParallelFrameSamples = 1024; // кадры короче этого рендерятся последовательно
	static const uint16 MinJobsPerWorker = 4;   // задач на поток минимум, чтобы окупить суммирование буферов

	explicit ParallelSynthExecutor(uint16 numWorkers = 0);
	~ParallelSynthExecutor();

	ParallelSynthExecutor(const ParallelSynthExecutor&) = delete;
	ParallelSynthExecutor& operator=(const ParallelSynthExecutor&) = delete;

	uint16 WorkerCount() const {return uint16(mWorkers.Length());}

	/// Выполняет задачи (сгруппированные в семплер-атомарные диапазоны jobs)
	/// параллельно и накапливает результат в frame. При малой нагрузке или
	/// превышении лимитов выполняется последовательный путь (frame.RunTasks).
	void Run(SamplerTaskContext& frame, const SamplerTaskContainer& tasks, Span<const SamplerJob> jobs, size_t frameLength);

private:
	struct Range { uint16 Begin, End; };

	/// Общее состояние, разделяемое между главным потоком и рабочими.
	struct Shared
	{
		Intra::Concurrency::AtomicInteger<uint32> Generation; // увеличивается на каждом кадре
		Intra::Concurrency::AtomicInteger<int32> DoneCount;   // число ещё не завершившихся рабочих
		Intra::Concurrency::Mutex WorkMutex;                  // защищает раздачу задач + Generation
		Intra::Concurrency::SeparateCondVar WorkCV;           // пробуждение рабочих о новой работе
		Intra::Concurrency::Mutex DoneMutex;                  // защищает DoneCount
		Intra::Concurrency::SeparateCondVar DoneCV;           // сигнал главному о завершении рабочих
		Shared(): Generation(0), DoneCount(0) {}
	};

	/// Один слот распределения: суммарная цена и диапазон задач в ordered[].
	struct Slot { uint32 Load; uint16 Count; uint16 Start; uint16 Pos; };

	struct Worker
	{
		Shared* S;
		const SamplerTaskContainer* Tasks;
		uint16 FrameLength;
		uint16 RangeCount;
		Range Ranges[MaxJobs];
		SamplerTaskContext Context; // приватные буферы (3 канала x MaxFrameSamples)
		uint32 LocalGeneration;
		bool Shutdown;
		Intra::Concurrency::Thread Thread; // последний: запускает Loop, захватив this

		Worker(Shared* s):
			S(s), Tasks(nullptr), FrameLength(0), RangeCount(0),
			Context(MaxFrameSamples), LocalGeneration(0), Shutdown(false),
			Thread([this](){Loop(this);}) {}
	};

	static void Loop(Worker* w);

	Shared mShared;
	Array<Unique<Worker>> mWorkers;
};

INTRA_WARNING_POP

#endif // !__EMSCRIPTEN__
