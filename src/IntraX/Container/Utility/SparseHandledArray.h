#pragma once

#include "SparseArray.h"
#include "IntraX/Container/Sequential/Array.h"
#include "IndexAllocator.h"

INTRA_BEGIN
/// Разреженный массив, который вместо индексов возвращает идентификаторы, корректность которых может быть проверена
template<typename T, typename Index, typename Generation> class SparseHandledArray
{
public:
	typedef CheckedId<Index, Generation, T> Id;

	SparseHandledArray(decltype(null)=null) {}
	SparseHandledArray(Index size): data(size) {}

	/// Переместить элемент в массив.
	/// \param[in] val Перемещаемый элемент.
	/// \param[out] oIndex Указатель, по которому будет записан индекс созданного элемента в массиве.
	T& Add(T&& val, Id* oId=null)
	{
		Index index;
		T& result = data.Add(Move(val), &index);
		if(oId==null) return result;
		oId->value = index;
		oId->generation = generations[index];
		return result;
	}

	/// Добавить копию элемента в массив.
	/// \param[in] val Копируемый элемент.
	/// \param[out] oIndex Указатель, по которому будет записан индекс созданного элемента в массиве.
	T& Add(const T& val, Id* oId=null)
	{
		Index index;
		T& result = data.Add(val, &index);
		if(oId==null) return result;
		oId->value = index;
		oId->generation = generations[index];
		return result;
	}

	/// Сконструировать элемент в массиве.
	/// \param[in] args Параметры конструктора.
	/// \param[out] oIndex Указатель, по которому будет записан индекс созданного элемента в массиве.
	template<typename... Args> T& Emplace(Args&&... args, Id* oId=null)
	{
		Index index;
		T& result = data.Emplace(Forward<Args>(args)..., &index);
		if(oId==null) return result;
		oId->value = index;
		oId->generation = generations[index];
		return result;
	}

	/// Удалить из массива элемент с идентификатором id
	void Remove(Id id)
	{
		if(!IsValidId(id)) return;
		data.Remove(id.value);
		generations[id.value]++;
	}

	INTRA_FORCEINLINE bool IsValidId(Id id) const
	{
		return id != null && id.generation == generations[id.value];
	}


	/// Удалить все элементы массива и освободить память
	void Clear()
	{
		data.Clear();
		for(Id& gen: generations) gen++;
	}
	
	/// Доступ по индексу следует использовать только тогда, когда точно известно, что элемент с этим индексом не был удалён.
	INTRA_FORCEINLINE T& ByIndex(Index index) {return data[index];}
	INTRA_FORCEINLINE const T& ByIndex(Index index) const {return data[index];}

	INTRA_FORCEINLINE T& operator[](Id id) {return data[id.value];}
	INTRA_FORCEINLINE const T& operator[](Id id) const {return data[id.value];}

	/// Возвращает, заполнен ли массив.
	/// Это означает, что массив не содержит свободных элементов,
	/// и следующая вставка элемента приведёт к перераспределению памяти.
	INTRA_FORCEINLINE bool IsFull() const {return data.IsFull();}


	INTRA_FORCEINLINE bool Empty() const {return data.Empty();}

	INTRA_FORCEINLINE size_t Capacity() const {return data.Capacity();}

	INTRA_FORCEINLINE bool operator==(decltype(null)) const {return Empty();}
	INTRA_FORCEINLINE bool operator!=(decltype(null)) const {return !Empty();}

private:
	SparseArray<T, Index> data;
	Array<Generation> generations;
};
INTRA_END
