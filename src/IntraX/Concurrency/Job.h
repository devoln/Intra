#pragma once

#include <Intra/Platform/Atomic.h>
#include <Intra/Range/Span.h>

// TODO: experimental, incomplete and untested
// Based on an article from Molecular Matters blog

namespace Intra { INTRA_BEGIN
struct Job
{
	using Function = void(*)(Job*, const void*);

	Job* Parent;
	Function Function; // TODO: replace with a functor with small buffer optimization
	Atomic<int> UnfinishedJobCount;
	uint8 Data[sizeof(size_t) == 8? 52: 44];

	Job(const Job& rhs):
		UnfinishedJobCount(rhs.UnfinishedJobCount.Get())
		Function(rhs.mFunction), Parent(rhs.mParent),
	{SpanOf(rhs.Data)|CopyTo(SpanOf(Data));}


	template<typename T, typename S> static Job* parallel_for(Span<T> data, Function function, const S& splitter)
	{
		const ParallelForData<T, S> jobData(data, function, splitter);
		return CreateJob(&parallel_for_job<ParallelForData<T, S>>, jobData);
	}

	static Job* Allocate();
	static Job* Get();
	bool IsEmpty();
	void Finish();
	void Execute();

	void Run();
	void Wait() const;

	static Job* CreateJob(Job::Function function);
	static Job* CreateJobAsChild(Job* parent, Job::Function function);

private:
	template<typename T, CCallableWithSignature<bool(size_t)> S> struct ParallelForData
	{
		using DataType = T;
		using SplitterType = S;
		using Function = void(*)(Span<T>);

		Span<T> Data;
		Function Function;
		SplitterType NeedSplit;
	};

	template<typename JobData> static void parallel_for_job(Job* job, const void* jobData)
	{
		const JobData* data = static_cast<const JobData*>(jobData);
		if(data->NeedSplit(data->count))
		{
			// split in two
			const unsigned leftCount = data->Data.Length() >> 1;
			const JobData leftData(data->Data|TakeExactly(leftCount), data->Function, data->NeedSplit);
			const auto left = Job::CreateJobAsChild(job, &Job::parallel_for_job<JobData>, leftData);
			left->Run();

			const unsigned rightCount = data->Data.Length() - leftCount;
			const JobData rightData(data->Data|Drop(leftCount), rightCount, data->Function, data->NeedSplit);
			const auto right = Job::CreateJobAsChild(job, &Job::parallel_for_job<JobData>, rightData);
			right->Run();
		}
		else data->Function(data->Data);
	}

	Job& operator=(const Job&) = delete;
};

void WorkerMain();
} INTRA_END
