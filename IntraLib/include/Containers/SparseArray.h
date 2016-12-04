#pragma once

#include "Range/SparseRange.h"
#include "Memory/Allocator.h"

namespace Intra {

//! Разреженный массив
template<typename T, typename Index> class SparseArray
{
public:
	SparseArray(null_t=null) {}
	SparseArray(Index size): data(Memory::AllocateRangeUninitialized(size)) {}

	//! Переместить элемент в массив.
	//! \param[in] val Перемещаемый элемент.
	//! \param[out] oIndex Указатель, по которому будет записан индекс созданного элемента в массиве.
	T& Add(T&& val, Index* oIndex=null)
	{
		check_space();
		return data.Add(core::move(val), oIndex);
	}

	//! Добавить копию элемента в массив.
	//! \param[in] val Копируемый элемент.
	//! \param[out] oIndex Указатель, по которому будет записан индекс созданного элемента в массиве.
	T& Add(const T& val, Index* oIndex=null)
	{
		check_space();
		return data.Add(val, oIndex);
	}

	//! Сконструировать элемент в массиве.
	//! \param[in] args Параметры конструктора.
	//! \param[out] oIndex Указатель, по которому будет записан индекс созданного элемента в массиве.
	template<typename... Args> T& Emplace(Args&&... args, Index* oIndex=null)
	{
		check_space();
		data.Emplace(core::forward<Args>(args)..., oIndex);
	}

	//! Удалить из массива элемент с индексом index
	void Remove(Index index) {data.Remove(index);}


	//! Удалить все элементы массива и освободить память
	void Clear()
	{
		auto buffer = data.GetInternalDataBuffer();
		data.MakeNull();
		Memory::FreeRangeUninitialized(buffer);
	}
	
	//! Доступ по индексу следует использовать только тогда, когда точно известно, что элемент с этим индексом не был удалён.
	T& operator[](Index index) {return data[index];}
	const T& operator[](Index index) const {return data[index];}

	//! Возвращает, заполнен ли массив.
	//! Это означает, что массив не содержит свободных элементов,
	//! и следующая вставка элемента приведёт к перераспределению памяти.
	bool IsFull() const {return data.IsFull();}


	bool Empty() const {return data.Empty();}

	size_t Capacity() const {return data.Capacity();}

	bool operator==(null_t) const {return Empty();}
	bool operator!=(null_t) const {return !Empty();}

private:
	Range::SparseRange<T, Index> data;
	void check_space()
	{
		if(!IsFull()) return;
		size_t count = Capacity()+Capacity()/2;
		auto range = Memory::AllocateRangeUninitialized<T>(Memory::GlobalHeap, count, INTRA_SOURCE_INFO);
		Range::SparseRange<T, Index> newData(range);
		data.MoveTo(newData);
		Memory::FreeRangeUninitialized(Memory::GlobalHeap, data.GetInternalDataBuffer());
		data = core::move(newData);
	}
};

}
