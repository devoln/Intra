#pragma once

#include "SparseArray.h"
#include "Container/Sequential/Array.h"
#include "IndexAllocator.h"

namespace Intra {

//! Разреженный массив, который вместо индексов возвращает идентификаторы, корректность которых может быть проверена
template<typename T, typename Index, typename Generation> class SparseHandledArray
{
public:
	typedef CheckedId<Index, Generation, T> Id;

	SparseHandledArray(null_t=null) {}
	SparseHandledArray(Index size): data(size) {}

	//! Переместить элемент в массив.
	//! \param[in] val Перемещаемый элемент.
	//! \param[out] oIndex Указатель, по которому будет записан индекс созданного элемента в массиве.
	T& Add(T&& val, Id* oId=null)
	{
		Index index;
		T& result = data.Add(Cpp::Move(val), &index);
		if(oId==null) return result;
		oId->value = index;
		oId->generation = generations[index];
		return result;
	}

	//! Добавить копию элемента в массив.
	//! \param[in] val Копируемый элемент.
	//! \param[out] oIndex Указатель, по которому будет записан индекс созданного элемента в массиве.
	T& Add(const T& val, Id* oId=null)
	{
		Index index;
		T& result = data.Add(val, &index);
		if(oId==null) return result;
		oId->value = index;
		oId->generation = generations[index];
		return result;
	}

	//! Сконструировать элемент в массиве.
	//! \param[in] args Параметры конструктора.
	//! \param[out] oIndex Указатель, по которому будет записан индекс созданного элемента в массиве.
	template<typename... Args> T& Emplace(Args&&... args, Id* oId=null)
	{
		Index index;
		T& result = data.Emplace(Cpp::Forward<Args>(args)..., &index);
		if(oId==null) return result;
		oId->value = index;
		oId->generation = generations[index];
		return result;
	}

	//! Удалить из массива элемент с идентификатором id
	void Remove(Id id)
	{
		if(!IsValidId(id)) return;
		data.Remove(id.value);
		generations[id.value]++;
	}

	forceinline bool IsValidId(Id id) const
	{
		return id != null && id.generation == generations[id.value];
	}


	//! Удалить все элементы массива и освободить память
	void Clear()
	{
		data.Clear();
		for(Id& gen: generations) gen++;
	}
	
	//! Доступ по индексу следует использовать только тогда, когда точно известно, что элемент с этим индексом не был удалён.
	forceinline T& ByIndex(Index index) {return data[index];}
	forceinline const T& ByIndex(Index index) const {return data[index];}

	forceinline T& operator[](Id id) {return data[id.value];}
	forceinline const T& operator[](Id id) const {return data[id.value];}

	//! Возвращает, заполнен ли массив.
	//! Это означает, что массив не содержит свободных элементов,
	//! и следующая вставка элемента приведёт к перераспределению памяти.
	forceinline bool IsFull() const {return data.IsFull();}


	forceinline bool Empty() const {return data.Empty();}

	forceinline size_t Capacity() const {return data.Capacity();}

	forceinline bool operator==(null_t) const {return Empty();}
	forceinline bool operator!=(null_t) const {return !Empty();}

private:
	SparseArray<T, Index> data;
	Array<Generation> generations;
};

}
