#pragma once

#include "Meta/Type.h"
#include "Utils/Debug.h"
#include "Utils/Unique.h"
#include "Utils/FixedArray.h"
#include "Range/Mutation/Fill.h"

struct SamplerTaskContext;

//! Задача по генерации семплов. Представляет собой функцию и данные для её вызова (POD объект ограниченного размера)
struct SamplerTask
{
	//! Контейнер для данных, которые необходимы для выполнения задачи
	struct Data
	{
		enum: size_t
		{
			MaxDataSizeInBytes = 55,
			AlignmentInBytes = 8
		};

		byte RawData[MaxDataSizeInBytes];

		template<class T> Meta::EnableIf<
			Meta::IsTriviallyMovable<Meta::RemoveConst<T>>::_ &&
			Meta::IsTriviallyDestructible<Meta::RemoveConst<T>>::_ &&
			sizeof(T) <= MaxDataSizeInBytes,
		T&> Get() {return *reinterpret_cast<T*>(RawData);}
	};

	//! Функция, которая выполняет задачу по генерации семплов
	//! и при достижении конца потока возвращает true - иначе false.
	//! Для обработчика задачи true означает, что нужно пропустить выполнение всех
	//! последующих задач в очереди с этим семплером и удалить сам семплер.
	using Function = bool(*)(Data& data, SamplerTaskContext& context);

	enum Flag: flag8
	{
		LeftChannel = 1, RightChannel = 2, ReverbChannel = 4,
		ChannelMask = LeftChannel|RightChannel|ReverbChannel
	};

	alignas(Data::AlignmentInBytes) Data Data;
	flag8 Flags;
	Function Func;
};

//! Очередь задач (single-threaded) ограниченного размера
class SamplerTaskQueue
{
//TODO: сделать универсальный класс очереди, не только для тасков
public:
	enum: size_t
	{
		MaxSizeLimit = 512
	};
	static_assert((MaxSizeLimit & (MaxSizeLimit - 1)) == 0, "MaxSizeLimit must be a power of 2!");

private:
	enum: size_t
	{
		IndexMask = MaxSizeLimit - 1
	};

	SamplerTask mTasks[MaxSizeLimit];
	uint mPopIndex = 0;
	uint mPushIndex = 0;

public:
	forceinline SamplerTask& PushNewTask()
	{
		INTRA_DEBUG_ASSERT(!Full());
		return mTasks[mPushIndex++ & IndexMask];
	}

	forceinline void Put(const SamplerTask& task)
	{
		PushNewTask() = task;
	}

	forceinline SamplerTask& First()
	{
		INTRA_DEBUG_ASSERT(!Empty());
		return mTasks[mPopIndex & IndexMask];
	}

	forceinline void PopFirst()
	{
		INTRA_DEBUG_ASSERT(!Empty());
		mPopIndex++;
	}

	forceinline SamplerTask& Next()
	{
		PopFirst();
		return First();
	}

	forceinline bool Empty() const {return mPopIndex == mPushIndex;}
	forceinline bool Full() const {return mPushIndex + 1 - mPopIndex == MaxSizeLimit;}

	forceinline void Clear()
	{
		mPushIndex = mPopIndex = 0;
	}
};

struct SamplerTaskContext
{
	SamplerTaskQueue Queue;
	Span<float> LeftSamples, RightSamples, ReverbSamples;
	flag8 UsedChannels;

	//Загрузка очереди. Считается как сумма стоимостей всех задач
	//~ taskFuncCostConst[i] + numSamples[i] * taskFuncCostCoeff[i]
	uint QueueLoad = 0;

	void PrepareForTask(flag8 taskFlags)
	{
		const auto newChannels = taskFlags & SamplerTask::ChannelMask & ~UsedChannels;
		if(newChannels == 0) return;
		if(newChannels & SamplerTask::LeftChannel) FillZeros(LeftSamples);
		if(newChannels & SamplerTask::RightChannel) FillZeros(RightSamples);
		if(newChannels & SamplerTask::ReverbChannel) FillZeros(ReverbSamples);
		UsedChannels |= newChannels;
	}

	void Run()
	{
		while(!Queue.Empty())
		{
			auto& task = Queue.Next();
			PrepareForTask(task.Flags);
			task.Func(task.Data, *this);
		}
	}
};

class SamplerTaskDispatcher
{
	FixedArray<SamplerTaskContext> mContexts;

	size_t minLoadedContextIndex(flag8 taskFlags) const
	{
		//TODO: возможно, стоит переделать этот линейный перебор на что-то более эффективное (например, бинарную кучу)
		size_t minIndex = 0;
		uint minLoad = uint_MAX;
		for(size_t i = 0; i < mContexts.Length(); i++)
		{
			const uint load = mContexts[i].QueueLoad;
			if(minLoad > load || minLoad == load && mContexts[i].UsedChannels == taskFlags)
			{
				minIndex = i;
				minLoad = load;
			}
		}
		return minIndex;
	}
public:
	SamplerTaskDispatcher(int numContexts):
		mContexts(numContexts)
	{
	}

	SamplerTaskContext& GetContext(flag8 taskFlags) const
	{
		size_t minIndex = 0;
#if INTRA_DISABLED
		minIndex = minLoadedContextIndex(taskFlags);
#endif
		return mContexts[minIndex];
	}
};
