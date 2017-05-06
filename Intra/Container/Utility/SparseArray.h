#pragma once

#include "SparseRange.h"
#include "Memory/Allocator.hh"
#include "Cpp/Warnings.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Container {

//! Разреженный массив
template<typename T, typename Index> class SparseArray
{
public:
	forceinline SparseArray(null_t=null) {}
	explicit SparseArray(Index size): mData(Memory::AllocateRangeUninitialized(size)) {}

	//! Переместить элемент в массив.
	//! \param[in] val Перемещаемый элемент.
	//! \param[out] oIndex Указатель, по которому будет записан индекс созданного элемента в массиве.
	forceinline T& Add(T&& val, Index* oIndex=null)
	{
		check_space();
		return mData.Add(Cpp::Move(val), oIndex);
	}

	//! Добавить копию элемента в массив.
	//! \param[in] val Копируемый элемент.
	//! \param[out] oIndex Указатель, по которому будет записан индекс созданного элемента в массиве.
	forceinline T& Add(const T& val, Index* oIndex = null)
	{
		check_space();
		return mData.Add(val, oIndex);
	}

	//! Сконструировать элемент в массиве.
	//! \param[in] args Параметры конструктора.
	//! \param[out] oIndex Указатель, по которому будет записан индекс созданного элемента в массиве.
	template<typename... Args> forceinline T& Emplace(Args&&... args, Index* oIndex = null)
	{
		check_space();
		mData.Emplace(Cpp::Forward<Args>(args)..., oIndex);
	}

	//! Удалить из массива элемент с индексом index
	forceinline void Remove(Index index) {mData.Remove(index);}


	//! Удалить все элементы массива и освободить память
	void Clear()
	{
		auto buffer = mData.GetInternalDataBuffer();
		mData.MakeNull();
		Memory::FreeRangeUninitialized(buffer);
	}

	//! Доступ по индексу следует использовать только тогда, когда точно известно, что элемент с этим индексом не был удалён.
	forceinline T& operator[](Index index) {return mData[index];}
	forceinline const T& operator[](Index index) const {return mData[index];}

	//! Возвращает, заполнен ли массив.
	//! Это означает, что массив не содержит свободных элементов,
	//! и следующая вставка элемента приведёт к перераспределению памяти.
	forceinline bool IsFull() const {return mData.IsFull();}


	forceinline bool Empty() const {return mData.Empty();}

	forceinline size_t Capacity() const {return size_t(mData.Capacity());}

	forceinline bool operator==(null_t) const {return Empty();}
	forceinline bool operator!=(null_t) const {return !Empty();}

private:
	Range::SparseRange<T, Index> mData;

	void check_space()
	{
		if(!IsFull()) return;
		size_t count = Capacity()+Capacity()/2;
		if(count == 0) count = 4;
		auto range = Memory::AllocateRangeUninitialized<T>(Memory::GlobalHeap, count, INTRA_SOURCE_INFO);
		Range::SparseRange<T, Index> newData(range);
		mData.MoveTo(newData);
		Memory::FreeRangeUninitialized(Memory::GlobalHeap, mData.GetInternalDataBuffer());
		mData = Cpp::Move(newData);
	}

	SparseArray(const SparseArray&) = delete;
	SparseArray& operator=(const SparseArray&) = delete;
};

}
using Container::SparseArray;

}

INTRA_WARNING_POP
