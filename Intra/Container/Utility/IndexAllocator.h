#pragma once

#include "Container/Sequential/Array.h"
#include "Range/Search/Single.h"
#include "Range/Sort/Insertion.h"

namespace Intra {

template<typename T> class IndexAllocator
{
public:
	forceinline IndexAllocator(null_t=null): mCount(0), mMaxCount(0) {}
	forceinline explicit IndexAllocator(T maxCount): mCount(0), mMaxCount(maxCount) {}

	//! Выделить любой свободный идентификатор
	T Allocate()
	{
		if(mFreeList!=null)
		{
			T result = mFreeList.Last();
			mFreeList.RemoveLast();
			return result;
		}
		INTRA_DEBUG_ASSERT(mCount<mMaxCount);
		return mCount++;
	}

	//! Выделить наименьшие возможные n идентификаторов, таких, что каждый следующий больше предыдущего ровно на 1
	T AllocateFirst(T n)
	{
		if(mFreeList.Count()>=n)
		{
			Range::ShellSort<T>(mFreeList);
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

	//! Выделить наименьший возможный неиспользуемый идентификатор
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
		INTRA_DEBUG_ASSERT(mCount<mMaxCount);
		return mCount++;
	}

	//! Освободить идентификатор
	void Deallocate(T id)
	{
		INTRA_DEBUG_ASSERT(!Range::Contains(mFreeList, id) && id<mCount);
		if(id+1==mCount)
		{
			mCount--;
			for(;;)
			{
				size_t loc = Range::CountUntil(mFreeList, T(mCount-1));
				if(loc==mFreeList.Count()) return;
				mCount--;
				mFreeList.RemoveUnordered(loc);
			}
		}
		mFreeList.AddLast(id);
	}

	forceinline T GetUsedCount() const {return mCount-mFreeList.Count();}
	forceinline T GetCount() const {return mCount;}

	void SetMaxCount(T maxCount)
	{
		INTRA_DEBUG_ASSERT(mCount<=maxCount);
		mMaxCount = maxCount;
	}

	forceinline bool IsFull() const {return mCount == mMaxCount && mFreeList == null;}
	forceinline bool IsId(T id) const {return id < mCount && !Range::Contains(mFreeList, id);}

private:
	Array<T> mFreeList;
	T mCount;
	T mMaxCount;
};

template<typename T, typename G, typename TYPE=void> struct CheckedId
{
	forceinline operator T() const {return Value;}

	forceinline CheckedId(null_t=null):
		Value(Meta::NumericLimits<T>::Max()),
		Generation(Meta::NumericLimits<G>::Max()) {}

	forceinline bool operator==(null_t) const
	{
		return Value==Meta::NumericLimits<T>::Max() &&
			Generation==Meta::NumericLimits<G>::Max();
	}

	forceinline bool operator!=(null_t) const {return !operator==(null);}

	forceinline bool operator==(const CheckedId& rhs) const
	{return Value==rhs.Value && Generation==rhs.Generation;}
	
	forceinline bool operator!=(const CheckedId& rhs) const {return !operator==(rhs);}

	template<typename TYPE2> forceinline const CheckedId<T, G, TYPE2>& Cast() const
	{return *reinterpret_cast<const CheckedId<T, G, TYPE2>*>(this);}

	size_t ToHash(CheckedId id) {return ToHash(id.Value)^ToHash(id.Generation);}

	T Value;
	G Generation;
};

template<typename T, typename G, typename TYPE=void> class CheckedIdAllocator
{
public:
	typedef CheckedId<T, G, TYPE> Id;

	CheckedIdAllocator(null_t=null): mIdAlloc(null) {}
	explicit CheckedIdAllocator(T maxCount): mIdAlloc(maxCount) {}

	//! Выделить любой свободный идентификатор
	Id Allocate()
	{
		Id result;
		result.Value = mIdAlloc.Allocate();
		if(result>=mGenerations.Count()) mGenerations.SetCount(mIdAlloc.GetCount());
		result.Generation = mGenerations[result];
		return result;
	}

	//! Выделить наименьший возможный неиспользуемый идентификатор
	Id AllocateFirst()
	{
		Id result;
		result.Value = mIdAlloc.AllocateFirst();
		if(result>=mGenerations.Count()) mGenerations.SetCount(mIdAlloc.GetCount());
		result.Generation = mGenerations[result];
		return result;
	}


	//! Освободить идентификатор
	void Deallocate(Id id)
	{
		INTRA_DEBUG_ASSERT(id.Generation==mGenerations[id]);
		if(id.Generation!=mGenerations[id]) return;
		mIdAlloc.Deallocate(id.Value);
		++mGenerations[id];
	}

	//! Определить количество используемых идентификаторов
	forceinline T GetUsedCount() const {return mIdAlloc.GetUsedCount();}

	//! Определить наибольший выделенный идентификатор
	forceinline T GetCount() const {return mIdAlloc.GetCount();}

	//! Определить поколение идентификатора
	forceinline G GetGeneration(T index) const {return mGenerations[index];}

	forceinline void SetMaxCount(T maxCount) {mIdAlloc.SetMaxCount(maxCount);}

	//! Вовзращает, является ли контейнер идентификаторов полным.
	//! Это означает, что попытка выделения любого идентификатора приведёт к ошибке.
	forceinline bool IsFull() const {return mIdAlloc.IsFull();}

	//! Определяет, является ли этот идентификатор актуальным.
	forceinline bool IsId(Id id) const {return mIdAlloc.IsId(id.Value) && mGenerations[id.Value]==id.Generation;}
	
	//! Проверяет, является ли указанный слот не пустым.
	bool SlotHasId(T index) const {return mIdAlloc.IsId(index);}

private:
	IndexAllocator<T> mIdAlloc;
	Array<G> mGenerations;
};

}

