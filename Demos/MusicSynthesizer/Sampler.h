#pragma once

#include "Container/Utility/SparseArray.h"
#include "Container/Sequential/Array.h"
#include "SamplerTask.h"

//! Семплер-наследник должен быть тривиально перемещаемым,
//! то есть он и его члены не должны иметь указателей и ссылок на свои поля
//! или иметь какую-то другую логику перемещения
class Sampler
{
	alignas(8) byte mInfo[15];
	flag8 mFlags = 0;
public:
	virtual ~Sampler() {}
	virtual bool Generate(SamplerTaskDispatcher& dispatcher) {return false;}
	virtual void MultiplyPitch(float freqMultiplier) {}
	virtual void MultiplyVolume(float volumeMultiplier) {}
	virtual void SetPan(float newPan) {}
	virtual void SetReverbCoeff(float newCoeff) {}
	virtual void NoteRelease() {}
	
	forceinline void MarkForDeletion() {mFlags |= 1;}
	forceinline bool IsMarkedForDeletion() const {return (mFlags & 1) != 0;}
	template<typename T> forceinline Meta::EnableIf<
		sizeof(T) <= 15,
	T&> GetInfo() {return *reinterpret_cast<T*>(mInfo);}
};

class SamplerContainer
{
public:
	enum: size_t
	{
		MaxSamplerSizeLimit = 124
	};

	enum: ushort {NullIndex = 0xFFFF, DeadIndex = 0xFFFE, InvalidIndexMask = 0xFFFE};
private:
	struct Holder
	{
		alignas(8) byte Raw[MaxSamplerSizeLimit];
		ushort PrevAliveIndex = NullIndex;
		ushort NextAliveIndex = NullIndex;

		//TODO: разобратьс¤, насколько верен и безопасен этот метод
		//после удалени¤ объекта из SparseArray
		//по идее, безопасен, а дл¤ отладки будет полезен даже если иногда будет неверно возвращать true
		forceinline bool IsAlive() const {return NextAliveIndex != DeadIndex;}

		forceinline Sampler& Get() {return *reinterpret_cast<Sampler*>(Raw);}
	};
	SparseArray<Holder, ushort> mPool;
	ushort mFirstAliveIndex = NullIndex, mLastAliveIndex = NullIndex;

public:
	void* AddRaw(ushort* oIndex = null)
	{
		ushort index;
		Holder& holder = mPool.Emplace(&index);
		holder.PrevAliveIndex = mLastAliveIndex;
		if(oIndex) *oIndex = index;
		if(mLastAliveIndex != NullIndex)
			mPool[mLastAliveIndex].NextAliveIndex = index;
		mLastAliveIndex = index;
		if(mFirstAliveIndex == NullIndex) mFirstAliveIndex = mLastAliveIndex;
		return holder.Raw;
	}

	template<typename T, typename... Args> Meta::EnableIf<
		Meta::IsInherited<T, Sampler>::_ &&
		sizeof(T) <= MaxSamplerSizeLimit,
	T&> Add(Args&&... args, ushort* oIndex = null)
	{
		return new(AddRaw(oIndex)) T(Cpp::Forward<Args>(args)...);
	}

	Sampler& DeleteRaw(ushort index)
	{
		auto& holder = mPool[index];
		INTRA_DEBUG_ASSERT(holder.IsAlive());
		if(mFirstAliveIndex == index) mFirstAliveIndex = holder.NextAliveIndex;
		if(mLastAliveIndex == index) mLastAliveIndex = holder.PrevAliveIndex;
		if(holder.PrevAliveIndex != NullIndex)
		{
			auto& prevHolder = mPool[holder.PrevAliveIndex];
			INTRA_DEBUG_ASSERT(prevHolder.NextAliveIndex == index);
			prevHolder.NextAliveIndex = holder.NextAliveIndex;
		}
		if(holder.NextAliveIndex != NullIndex)
		{
			auto& nextHolder = mPool[holder.NextAliveIndex];
			INTRA_DEBUG_ASSERT(nextHolder.PrevAliveIndex == index);
			nextHolder.PrevAliveIndex = holder.PrevAliveIndex;
		}
		holder.NextAliveIndex = DeadIndex;
		return holder.Get();
	}

	void Delete(ushort index)
	{
		DeleteRaw(index).~Sampler();
	}

	Sampler& Get(ushort index)
	{
		auto& holder = mPool[index];
		INTRA_DEBUG_ASSERT(holder.IsAlive());
		return holder.Get();
	}

	forceinline bool Empty() const {return mFirstAliveIndex == NullIndex;}

	forceinline ushort NextAliveIndex(ushort index) const {return mPool[index].NextAliveIndex;}
	forceinline ushort PrevAliveIndex(ushort index) const {return mPool[index].PrevAliveIndex;}
	forceinline ushort FirstAliveIndex(ushort index) const {return mFirstAliveIndex;}

	struct Range
	{
		SamplerContainer* MyContainer;
		ushort Index;
		
		forceinline Sampler& First() const
		{
			INTRA_DEBUG_ASSERT(!Empty());
			return MyContainer->Get(Index);
		}

		forceinline void PopFirst()
		{
			INTRA_DEBUG_ASSERT(!Empty());
			Index = MyContainer->NextAliveIndex(Index);
		}

		forceinline bool Empty() const
		{
			return MyContainer == null ||
				(Index & InvalidIndexMask) != 0;
		}

		forceinline Sampler& Next()
		{
			Sampler& result = First();
			PopFirst();
			return result;
		}
	};

	forceinline Range AsRange() {return {this, mFirstAliveIndex};}
};
