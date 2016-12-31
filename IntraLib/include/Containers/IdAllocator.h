#pragma once

#include "Array.h"
#include "Algo/Sort.h"

namespace Intra {

template<typename T> class IdAllocator
{
public:
	IdAllocator(null_t=null): count(0), max_count(0) {}
	explicit IdAllocator(T maxCount): count(0), max_count(maxCount) {}

	//Выделить любой свободный идентификатор
	T Allocate()
	{
		if(free_list!=null)
		{
			T result = free_list.Last();
			free_list.RemoveLast();
			return result;
		}
		INTRA_ASSERT(count<max_count);
		return count++;
	}

	//Выделить наименьшие возможные n идентификаторов, таких, что каждый следующий больше предыдущего ровно на 1
	T AllocateFirst(T n)
	{
		if(free_list.Count()>=n)
		{
			Algo::ShellSort<T>(free_list);
			for(T i=0; i<free_list.Count(); i++)
			{
				T first = free_list[i];
				T last = first;
				for(T j=i+1; j<free_list.Count(); j++)
				{
					if(free_list[j]!=last+1 || last-first==n) break;
					last++;
				}
				INTRA_ASSERT(last-first<=n);
				if(last-first==n)
				{
					free_list.Remove(first, last);
					return first;
				}
			}
		}
		INTRA_ASSERT(count+n<=max_count);
		count+=n;
		return count-n;
	}

	//Выделить наименьший возможный неиспользуемый идентификатор
	T AllocateFirst()
	{
		if(free_list!=null)
		{
			T minimal=count, index=count;
			for(T i=0; i<free_list.Count(); i++)
			{
				if(free_list[i]>=minimal) continue;
				minimal=free_list[i];
				index=i;
			}
			free_list.RemoveUnordered(index);
			return minimal;
		}
		INTRA_ASSERT(count<max_count);
		return count++;
	}

	//Освободить идентификатор
	void Deallocate(T id)
	{
		INTRA_ASSERT(!free_list().Contains(id) && id<count);
		if(id+1==count)
		{
			count--;
			for(;;)
			{
				size_t loc = free_list().CountUntil(T(count-1));
				if(loc==free_list.Count()) return;
				count--;
				free_list.RemoveUnordered(loc);
			}
		}
		free_list.AddLast(id);
	}

	T GetUsedCount() const {return count-free_list.Count();}
	T GetCount() const {return count;}

	void SetMaxCount(T maxCount)
	{
		INTRA_ASSERT(count<=maxCount);
		max_count=maxCount;
	}

	bool IsFull() const {return count==max_count && free_list==null;}
	bool IsId(T id) const {return id<count && !free_list.Contains(id);}

private:
	Array<T> free_list;
	T count;
	T max_count;
};

template<typename T, typename G, typename TYPE=void> struct CheckedId
{
	operator T() const {return value;}
	CheckedId(null_t=null): value(Meta::NumericLimits<T>::Max()), generation(Meta::NumericLimits<G>::Max()) {}

	bool operator==(null_t) const {return value==Meta::NumericLimits<T>::Max() && generation==Meta::NumericLimits<G>::Max();}
	bool operator!=(null_t) const {return !operator==(null);}

	bool operator==(const CheckedId& rhs) const {return value==rhs.value && generation==rhs.generation;}
	bool operator!=(const CheckedId& rhs) const {return !operator==(rhs);}

	template<typename TYPE2> const CheckedId<T, G, TYPE2>& Cast() const
	{
		return *reinterpret_cast<const CheckedId<T, G, TYPE2>*>(this);
	}

	T value;
	G generation;
};

template<typename T, typename G, typename TYPE> inline size_t ToHash(CheckedId<T, G, TYPE> id)
{
	return ToHash<T>(id.value)^ToHash<G>(id.generation);
}

template<typename T, typename G, typename TYPE=void> class CheckedIdAllocator
{
	IdAllocator<T> idalloc;
public:
	typedef CheckedId<T,G,TYPE> Id;

	CheckedIdAllocator(null_t=null): idalloc(null) {}
	explicit CheckedIdAllocator(T maxCount): idalloc(maxCount) {}

	//! Выделить любой свободный идентификатор
	Id Allocate()
	{
		Id result;
		result.value = idalloc.Allocate();
		if(result>=generations.Count()) generations.SetCount(idalloc.GetCount());
		result.generation = generations[result];
		return result;
	}

	//! Выделить наименьший возможный неиспользуемый идентификатор
	Id AllocateFirst()
	{
		Id result;
		result.value = idalloc.AllocateFirst();
		if(result>=generations.Count()) generations.SetCount(idalloc.GetCount());
		result.generation = generations[result];
		return result;
	}


	//! Освободить идентификатор
	void Deallocate(Id id)
	{
		INTRA_ASSERT(id.generation==generations[id]);
		if(id.generation!=generations[id]) return;
		idalloc.Deallocate(id.value);
		++generations[id];
	}

	//! Определить количество используемых идентификаторов
	T GetUsedCount() const {return idalloc.GetUsedCount();}

	//! Определить наибольший выделенный идентификатор
	T GetCount() const {return idalloc.GetCount();}

	//! Определить поколение идентификатора
	G GetGeneration(T index) const {return generations[index];}

	void SetMaxCount(T maxCount)
	{
		idalloc.SetMaxCount(maxCount);
	}

	//! Вовзращает, является ли контейнер идентификаторов полным.
	//! Это означает, что попытка выделения любого идентификатора приведёт к ошибке.
	bool IsFull() const {return idalloc.IsFull();}

	//! Определяет, является ли этот идентификатор актуальным.
	bool IsId(Id id) const {return idalloc.IsId(id.value) && generations[id.value]==id.generation;}
	
	//! Проверяет, является ли указанный слот не пустым.
	bool SlotHasId(T index) const {return idalloc.IsId(index);}

private:
	Array<G> generations;
};

}

