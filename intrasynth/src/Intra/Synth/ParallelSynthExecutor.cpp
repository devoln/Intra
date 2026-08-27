#include "ParallelSynthExecutor.h"

#ifndef __EMSCRIPTEN__

#ifdef __linux__
#include <stdio.h>
#include <unistd.h>
#endif
#include <stdlib.h>

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

// Определяет реально доступное число ядер (не полагаясь на /proc/cpuinfo, который
// в контейнерах показывает ядра хоста). Учитывает cgroup-квоту CPU (v2 и v1),
// затем число онлайн-процессоров, разрешённых маской affinity. Возвращает число
// рабочих потоков (без главного).
static uint16 DetectWorkerCount()
{
	uint32 cpus = 0;
#ifdef __linux__
	// cgroup v2: файл "<quota> <period>" (quota == "max" означает без ограничения).
	FILE* f = fopen("/sys/fs/cgroup/cpu.max", "rb");
	if(f)
	{
		long long quota = -1, period = 100000;
		if(fscanf(f, "%lld %lld", &quota, &period) == 2 && quota > 0 && period > 0)
			cpus = uint32((quota + period - 1) / period);
		fclose(f);
		if(cpus > 1) return uint16(cpus - 1);
	}
	// cgroup v1: cpu.cfs_quota_us / cpu.cfs_period_us.
	f = fopen("/sys/fs/cgroup/cpu/cpu.cfs_quota_us", "rb");
	if(f)
	{
		long long quota = -1;
		if(fscanf(f, "%lld", &quota) == 1)
		{
			long long period = 100000;
			FILE* fp = fopen("/sys/fs/cgroup/cpu/cpu.cfs_period_us", "rb");
			if(fp) {fscanf(fp, "%lld", &period); fclose(fp);}
			if(quota > 0 && period > 0)
				cpus = uint32((quota + period - 1) / period);
		}
		fclose(f);
		if(cpus > 1) return uint16(cpus - 1);
	}
	// Иначе — число онлайн-процессоров с учётом affinity.
	const long online = sysconf(_SC_NPROCESSORS_ONLN);
	if(online > 1) cpus = uint32(online);
#else
	cpus = Intra::System::ProcessorInfo::Get().LogicalProcessorNumber;
#endif
	return uint16(cpus > 1 ? cpus - 1 : 0);
}

ParallelSynthExecutor::ParallelSynthExecutor(uint16 numWorkers)
{
	// Явное управление числом рабочих потоков для бенчмарков/диагностики:
	// INTRASYNTH_WORKERS=0 — строго однопоточно, N — ровно N рабочих.
	const char* env = getenv("INTRASYNTH_WORKERS");
	if(env && env[0]) numWorkers = uint16(atoi(env));
	if(numWorkers == 0)
		numWorkers = DetectWorkerCount();
	if(numWorkers > MaxWorkers) numWorkers = MaxWorkers;

	for(uint16 i = 0; i < numWorkers; i++)
		mWorkers.AddLast(Unique<Worker>::New(&mShared));
}

ParallelSynthExecutor::~ParallelSynthExecutor()
{
	{
		Intra::Concurrency::Lock<Intra::Concurrency::Mutex> lock(mShared.WorkMutex);
		for(size_t i = 0; i < mWorkers.Length(); i++)
			mWorkers[i]->Shutdown = true;
		mShared.Generation.GetAdd(1);
		mShared.WorkCV.NotifyAll();
	}
	for(size_t i = 0; i < mWorkers.Length(); i++)
		mWorkers[i]->Thread.Join();
}

void ParallelSynthExecutor::Loop(Worker* w)
{
	for(;;)
	{
		{
			Intra::Concurrency::Lock<Intra::Concurrency::Mutex> lock(w->S->WorkMutex);
			w->S->WorkCV.Wait(lock, [w]()
			{
				return w->Shutdown || w->S->Generation.Get() != w->LocalGeneration;
			});
		}
		w->LocalGeneration = w->S->Generation.Get();
		if(w->Shutdown) return;
		if(w->RangeCount == 0) continue; // неактивный в этом кадре рабочий

		SamplerTaskContext& context = w->Context;
		context.ClearChannels(w->FrameLength);
		for(uint16 r = 0; r < w->RangeCount; r++)
		{
			for(uint16 t = w->Ranges[r].Begin; t < w->Ranges[r].End; t++)
			{
				SamplerTask* task = (*w->Tasks)[t];
				if(task) (*task)(context);
			}
		}

		// Уменьшаем счётчик завершения под мьютексом (предикат главного потока
		// проверяется под тем же мьютексом) и будим главный поток.
		{
			Intra::Concurrency::Lock<Intra::Concurrency::Mutex> lock(w->S->DoneMutex);
			const int32 prev = w->S->DoneCount.GetSub(1);
			if(prev == 1) w->S->DoneCV.NotifyAll();
		}
	}
}

void ParallelSynthExecutor::Run(SamplerTaskContext& frame, const SamplerTaskContainer& tasks, Span<const SamplerJob> jobs, size_t frameLength)
{
	const size_t numJobs = jobs.Length();
	const size_t numWorkers = mWorkers.Length();

	// Последовательный путь: нет рабочих, слишком мало/много задач, кадр больше
	// фиксированных приватных буферов или слишком короткий, чтобы окупить
	// накладные расходы на диспетчеризацию и суммирование приватных буферов.
	if(numJobs <= 1 || numWorkers == 0 || numJobs > MaxJobs ||
		frameLength > MaxFrameSamples || frameLength < MinParallelFrameSamples)
	{
		frame.RunTasks(tasks);
		return;
	}

	// Каждому рабочему должно достаться хотя бы несколько задач, иначе суммирование
	// его приватного буфера съедает выигрыш от параллелизма.
	const size_t usefulWorkers = Min<size_t>(numWorkers, numJobs / MinJobsPerWorker);
	if(usefulWorkers == 0)
	{
		frame.RunTasks(tasks);
		return;
	}
	const size_t numSlots = Min<size_t>(usefulWorkers + 1, numJobs);

	// Жадная укладка задач по слотам: каждую задачу кладём в наименее загруженный слот.
	Slot slots[MaxWorkers + 1];
	byte jobSlot[MaxJobs];
	for(size_t s = 0; s < numSlots; s++)
	{
		slots[s].Load = 0;
		slots[s].Count = 0;
		slots[s].Start = 0;
		slots[s].Pos = 0;
	}
	for(size_t j = 0; j < numJobs; j++)
	{
		size_t best = 0;
		for(size_t s = 1; s < numSlots; s++)
			if(slots[s].Load < slots[best].Load) best = s;
		jobSlot[j] = byte(best);
		slots[best].Load += jobs[j].Cost;
	}

	// Самую нагруженную очередь забирает главный поток.
	size_t mainSlot = 0;
	for(size_t s = 1; s < numSlots; s++)
		if(slots[s].Load > slots[mainSlot].Load) mainSlot = s;

	// Переупорядочиваем задачи по слотам, чтобы каждому слоту соответствовал
	// непрерывный участок массива ordered.
	for(size_t j = 0; j < numJobs; j++) slots[jobSlot[j]].Count++;
	uint16 acc = 0;
	for(size_t s = 0; s < numSlots; s++)
	{
		slots[s].Start = acc;
		slots[s].Pos = acc;
		acc += slots[s].Count;
	}

	SamplerJob ordered[MaxJobs];
	for(size_t j = 0; j < numJobs; j++)
		ordered[slots[jobSlot[j]].Pos++] = jobs[j];

	// Раздаём не-главные слоты рабочим потокам под мьютексом работы, затем будим их.
	size_t activeWorkers = 0;
	{
		Intra::Concurrency::Lock<Intra::Concurrency::Mutex> lock(mShared.WorkMutex);
		for(size_t i = 0; i < numWorkers; i++)
			mWorkers[i]->RangeCount = 0; // неактивные в этом кадре ничего не делают
		for(size_t s = 0; s < numSlots; s++)
		{
			if(s == mainSlot) continue;
			Worker& worker = *mWorkers[activeWorkers++];
			worker.Tasks = &tasks;
			worker.FrameLength = uint16(frameLength);
			worker.RangeCount = slots[s].Count;
			for(uint16 k = 0; k < slots[s].Count; k++)
			{
				const SamplerJob& job = ordered[slots[s].Start + k];
				worker.Ranges[k] = {job.Begin, job.End};
			}
		}
		mShared.DoneCount.Set(int32(activeWorkers));
		mShared.Generation.GetAdd(1);
		mShared.WorkCV.NotifyAll();
	}

	// Главный поток выполняет свою (самую нагруженную) очередь в выходной контекст.
	frame.ClearChannels();
	for(uint16 k = 0; k < slots[mainSlot].Count; k++)
	{
		const SamplerJob& job = ordered[slots[mainSlot].Start + k];
		for(uint16 t = job.Begin; t < job.End; t++)
		{
			SamplerTask* task = tasks[t];
			if(task) (*task)(frame);
		}
	}

	// Ждём завершения рабочих через condition variable (без busy-wait).
	{
		Intra::Concurrency::Lock<Intra::Concurrency::Mutex> lock(mShared.DoneMutex);
		mShared.DoneCV.Wait(lock, [this]() {return mShared.DoneCount.Get() == 0;});
	}

	// Суммируем приватные буферы рабочих в выходной контекст.
	for(size_t wi = 0; wi < activeWorkers; wi++)
	{
		SamplerTaskContext& context = mWorkers[wi]->Context;
		for(int c = 0; c < 2; c++)
		{
			if((context.UsedChannels & (1 << c)) == 0) continue;
			Add(frame.Channels[c].Take(frameLength), context.Channels[c].Take(frameLength));
			frame.UsedChannels |= uint16(1 << c);
		}
	}
}

INTRA_WARNING_POP

#endif // !__EMSCRIPTEN__
