#pragma once

#include "Atomic.h"
#include "Core/Range/Span.h"

// TODO: experimental, incomplete and untested
// Based on an article from Molecular Matters blog

INTRA_BEGIN
struct Job
{
	typedef void(*Function)(Job*, const void*);

	Function mFunction;
	Job* mParent;
	AtomicInt mUnfinishedJobCount;
#ifndef INTRA_PLATFORM_IS_64
	enum {PADDING_SIZE = 52};
#else
	enum {PADDING_SIZE = 44};
#endif
	byte data[PADDING_SIZE];

	Job(const Job& rhs):
		mFunction(rhs.mFunction), mParent(rhs.mParent),
		mUnfinishedJobCount(rhs.mUnfinishedJobCount.Get())
	{SpanOf(rhs.data).CopyTo(SpanOf(data));}


	template<typename T, typename S> static Job* parallel_for(T* data, uint count, Function function, const S& splitter)
	{
		typedef parallel_for_data<T, S> JobData;
		const JobData jobData(data, count, function, splitter);

		Job* const job = CreateJob(&parallel_for_job<JobData>, jobData);

		return job;
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
	template<typename T, typename S> struct parallel_for_data
	{
		typedef T DataType;
		typedef S SplitterType;
		typedef void(*Function)(DataType*, uint);

		parallel_for_data(DataType* data_, uint count_, Function function_, const SplitterType& splitter_):
			data(data_), count(count_), function(function_), splitter(splitter_) {}

		T* data;
		uint count;
		Function function;
		SplitterType splitter;
	};

	template<typename JobData> static void parallel_for_job(Job* job, const void* jobData)
	{
		const JobData* data = static_cast<const JobData*>(jobData);
		const typename JobData::SplitterType& splitter = data->splitter;

		if(splitter.Split<JobData::DataType>(data->count))
		{
			// split in two
			const uint leftCount = data->count/2u;
			const JobData leftData(data->data, leftCount, data->function, splitter);
			Job* left = Job::CreateJobAsChild(job, &Job::parallel_for_job<JobData>, leftData);
			left->Run();

			const uint rightCount = data->count - leftCount;
			const JobData rightData(data->data + leftCount, rightCount, data->function, splitter);
			Job* right = Job::CreateJobAsChild(job, &Job::parallel_for_job<JobData>, rightData);
			right->Run();
		}
		else
		{
			// execute the function on the range of data
			data->function(data->data, data->count);
		}
	}

	Job& operator=(const Job&) = delete;
};

void WorkerMain();

class CountSplitter
{
public:
	explicit CountSplitter(uint count): mCount(count) {}

	template<typename T> bool Split(uint count) const
	{return count > mCount;}

private:
	uint mCount;
};

class DataSizeSplitter
{
public:
	explicit DataSizeSplitter(uint size): mSize(size) {}

	template<typename T> bool Split(unsigned int count) const
	{return count*sizeof(T) > mSize;}

private:
	uint mSize;
};
INTRA_END
