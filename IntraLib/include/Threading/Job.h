#pragma once

#include "Core/Core.h"
#include "Atomic.h"

namespace Intra {

struct Job
{
	typedef void(*Function)(Job*, const void*);

	Function function;
	Job* parent;
	Atomic<int> unfinishedJobs;
#ifndef INTRA_PLATFORM_IS_64
	enum {PADDING_SIZE = 52};
#else
	enum {PADDING_SIZE = 44};
#endif
	char data[PADDING_SIZE];

	Job(const Job& rhs):
		function(rhs.function), parent(rhs.parent),
		unfinishedJobs(rhs.unfinishedJobs.Load())
	{
		core::memcpy(data, rhs.data, sizeof(data));
	}


	template<typename T, typename S> static Job* parallel_for(T* data, uint count, Function function, const S& splitter)
	{
		typedef parallel_for_data<T, S> JobData;
		const JobData jobData(data, count, function, splitter);

		Job* job = CreateJob(&parallel_for_job<JobData>, jobData);

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
	explicit CountSplitter(uint count_): count(count_) {}

	template <typename T> bool Split(uint count_) const
	{
		return count_ > count;
	}

private:
	uint count;
};

class DataSizeSplitter
{
public:
	explicit DataSizeSplitter(uint size_): size(size_) {}

	template <typename T> bool Split(unsigned int count) const
	{
		return count*sizeof(T) > size;
	}

private:
	uint size;
};

}
