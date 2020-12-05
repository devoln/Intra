#pragma once

#include "IntraX/Container/Sequential/Array.h"
#include "Intra/Range/Search/Single.h"
#include "Intra/Range/Sort/Insertion.h"

INTRA_BEGIN
template<typename T> class IndexAllocator
{
public:
	INTRA_FORCEINLINE IndexAllocator(decltype(null)=null): mCount(0), mMaxCount(0) {}
	INTRA_FORCEINLINE explicit IndexAllocator(T maxCount): mCount(0), mMaxCount(maxCount) {}

	/// Выделить любой свободный идентификатор
	T Allocate()
	{
		if(mFreeList != null)
		{
			T result = mFreeList.Last();
			mFreeList.RemoveLast();
			return result;
		}
		INTRA_DEBUG_ASSERT(mCount < mMaxCount);
		return mCount++;
	}

	/// Выделить наименьшие возможные n идентификаторов, таких, что каждый следующий больше предыдущего ровно на 1
	T AllocateFirst(T n)
	{
		if(mFreeList.Count()>=n)
		{
			ShellSort<T>(mFreeList);
			for(T i=0; i<mFreeList.Count(); i++)
			{
				T first = mFreeList[i];
				T last = first;
				for(T j=i+1; j<mFreeList.Count(); j++)
				{
					if(mFreeList[j]!=last+1 || last-first==n) break;
					last++;
				}
				INTRA_DEBUG_ASSERT(T(last-first) <= n);
				if(last-first==n)
				{
					mFreeList.Remove(first, last);
					return first;
				}
			}
		}
		INTRA_DEBUG_ASSERT(mCount+n <= mMaxCount);
		mCount += n;
		return mCount-n;
	}

	/// Выделить наименьший возможный неиспользуемый идентификатор
	T AllocateFirst()
	{
		if(mFreeList!=null)
		{
			T minimal = mCount, index = mCount;
			for(T i=0; i<mFreeList.Count(); i++)
			{
				if(mFreeList[i]>=minimal) continue;
				minimal = mFreeList[i];
				index = i;
			}
			mFreeList.RemoveUnordered(index);
			return minimal;
		}
		INTRA_POSTCONDITION(mCount < mMaxCount);
		return mCount++;
	}

	/// Освободить идентификатор
	void Deallocate(T id)
	{
		INTRA_PRECONDITION(!Contains(mFreeList, id));
		INTRA_PRECONDITION(id < mCount);
		if(id+1==mCount)
		{
			mCount--;
			for(;;)
			{
				size_t loc = CountUntil(mFreeList, T(mCount-1));
				if(loc==mFreeList.Count()) return;
				mCount--;
				mFreeList.RemoveUnordered(loc);
			}
		}
		mFreeList.AddLast(id);
	}

	INTRA_FORCEINLINE T GetUsedCount() const {return mCount-mFreeList.Count();}
	INTRA_FORCEINLINE T GetCount() const {return mCount;}

	void SetMaxCount(T maxCount)
	{
		INTRA_PRECONDITION(mCount <= maxCount);
		mMaxCount = maxCount;
	}

	INTRA_FORCEINLINE bool IsFull() const {return mCount == mMaxCount && mFreeList.Empty();}
	INTRA_FORCEINLINE bool IsId(T id) const {return id < mCount && !Contains(mFreeList, id);}

private:
	Array<T> mFreeList;
	T mCount;
	T mMaxCount;
};

template<typename T, typename G, typename TYPE=void> struct CheckedId
{
	INTRA_FORCEINLINE operator T() const {return Value;}

	INTRA_FORCEINLINE CheckedId(decltype(null)=null):
		Value(MaxValueOf(T())),
		Generation(MaxValueOf(G())) {}

	INTRA_FORCEINLINE bool operator==(decltype(null)) const
	{
		return Value == MaxValueOf(T()) &&
			Generation == MaxValueOf(G());
	}

	INTRA_FORCEINLINE bool operator!=(decltype(null)) const {return !operator==(null);}

	INTRA_FORCEINLINE bool operator==(const CheckedId& rhs) const
	{return Value == rhs.Value && Generation == rhs.Generation;}
	
	INTRA_FORCEINLINE bool operator!=(const CheckedId& rhs) const {return !operator==(rhs);}

	template<typename TYPE2> INTRA_FORCEINLINE const CheckedId<T, G, TYPE2>& Cast() const
	{return *reinterpret_cast<const CheckedId<T, G, TYPE2>*>(this);}

	size_t ToHash(CheckedId id) {return ToHash(id.Value)^ToHash(id.Generation);}

	T Value;
	G Generation;
};

template<typename T, typename G, typename TYPE=void> class CheckedIdAllocator
{
public:
	typedef CheckedId<T, G, TYPE> Id;

	CheckedIdAllocator(decltype(null)=null): mIdAlloc(null) {}
	explicit CheckedIdAllocator(T maxCount): mIdAlloc(maxCount) {}

	/// Выделить любой свободный идентификатор
	Id Allocate()
	{
		Id result;
		result.Value = mIdAlloc.Allocate();
		if(result>=mGenerations.Count()) mGenerations.SetCount(mIdAlloc.GetCount());
		result.Generation = mGenerations[result];
		return result;
	}

	/// Выделить наименьший возможный неиспользуемый идентификатор
	Id AllocateFirst()
	{
		Id result;
		result.Value = mIdAlloc.AllocateFirst();
		if(result>=mGenerations.Count()) mGenerations.SetCount(mIdAlloc.GetCount());
		result.Generation = mGenerations[result];
		return result;
	}


	/// Освободить идентификатор
	void Deallocate(Id id)
	{
		INTRA_PRECONDITION(id.Generation == mGenerations[id]);
		if(id.Generation!=mGenerations[id]) return;
		mIdAlloc.Deallocate(id.Value);
		++mGenerations[id];
	}

	/// Определить количество используемых идентификаторов
	INTRA_FORCEINLINE T GetUsedCount() const {return mIdAlloc.GetUsedCount();}

	/// Определить наибольший выделенный идентификатор
	INTRA_FORCEINLINE T GetCount() const {return mIdAlloc.GetCount();}

	/// Определить поколение идентификатора
	INTRA_FORCEINLINE G GetGeneration(T index) const {return mGenerations[index];}

	INTRA_FORCEINLINE void SetMaxCount(T maxCount) {mIdAlloc.SetMaxCount(maxCount);}

	/// Вовзращает, является ли контейнер идентификаторов полным.
	/// Это означает, что попытка выделения любого идентификатора приведёт к ошибке.
	INTRA_FORCEINLINE bool IsFull() const {return mIdAlloc.IsFull();}

	/// Определяет, является ли этот идентификатор актуальным.
	INTRA_FORCEINLINE bool IsId(Id id) const {return mIdAlloc.IsId(id.Value) && mGenerations[id.Value]==id.Generation;}
	
	/// Проверяет, является ли указанный слот не пустым.
	bool SlotHasId(T index) const {return mIdAlloc.IsId(index);}

private:
	IndexAllocator<T> mIdAlloc;
	Array<G> mGenerations;
};
INTRA_END
