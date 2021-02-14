#pragma once

#include "SparseRange.h"
#include "IntraX/Memory/Allocator.hh"

namespace Intra { INTRA_BEGIN
template<typename T, typename Index = size_t> class SparseArray
{
public:
	INTRA_FORCEINLINE SparseArray(decltype(nullptr)=nullptr) {}
	explicit SparseArray(Index size): mData(AllocateRangeUninitialized(size)) {}

	SparseArray(InitializerList<T> values):
		SparseArray(Span<const T>(values)) {}
	
	SparseArray(Span<const T> values): mData(values.Length())
	{
		for(auto& v: values) Add(v);
	}

	template<size_t N> SparseArray(const T(&values)[N]): SparseArray(SpanOf(values)) {}

	/// Переместить элемент в массив.
	/// @param[in] val Перемещаемый элемент.
	/// @param[out] oIndex Указатель, по которому будет записан индекс созданного элемента в массиве.
	INTRA_FORCEINLINE T& Add(T&& val, Index* oIndex=nullptr)
	{
		check_space();
		return mData.Add(Move(val), oIndex);
	}

	/// Добавить копию элемента в массив.
	/// @param[in] val Копируемый элемент.
	/// @param[out] oIndex Указатель, по которому будет записан индекс созданного элемента в массиве.
	INTRA_FORCEINLINE T& Add(const T& val, Index* oIndex = nullptr)
	{
		check_space();
		return mData.Add(val, oIndex);
	}

	/// Сконструировать элемент в массиве.
	/// @param[in] args Параметры конструктора.
	/// @param[out] oIndex Указатель, по которому будет записан индекс созданного элемента в массиве.
	template<typename... Args> INTRA_FORCEINLINE T& Emplace(Args&&... args, Index* oIndex = nullptr)
	{
		check_space();
		mData.Emplace(Forward<Args>(args)..., oIndex);
	}

	/// Удалить из массива элемент с индексом index
	INTRA_FORCEINLINE void Remove(Index index) {mData.Remove(index);}


	/// Удалить все элементы массива и освободить память
	void Clear()
	{
		auto buffer = mData.GetInternalDataBuffer();
		mData.MakeNull();
		FreeRangeUninitialized(buffer);
	}

	/// Доступ по индексу следует использовать только тогда, когда точно известно, что элемент с этим индексом не был удалён.
	INTRA_FORCEINLINE T& operator[](Index index) {return mData[index];}
	INTRA_FORCEINLINE const T& operator[](Index index) const {return mData[index];}

	/// Возвращает, заполнен ли массив.
	/// Это означает, что массив не содержит свободных элементов,
	/// и следующая вставка элемента приведёт к перераспределению памяти.
	INTRA_FORCEINLINE bool IsFull() const {return mData.IsFull();}


	INTRA_FORCEINLINE bool Empty() const {return mData.Empty();}

	INTRA_FORCEINLINE size_t Capacity() const {return size_t(mData.Capacity());}

	INTRA_FORCEINLINE bool operator==(decltype(nullptr)) const {return Empty();}
	INTRA_FORCEINLINE bool operator!=(decltype(nullptr)) const {return !Empty();}

private:
	SparseRange<T, Index> mData;

	void check_space()
	{
		if(!IsFull()) return;
		size_t count = Capacity() + Capacity()/2;
		if(count == 0) count = 4;
		auto range = AllocateRangeUninitialized<T>(GlobalHeap, count, INTRA_SOURCE_INFO);
		SparseRange<T, Index> newData(range);
		mData.MoveTo(newData);
		FreeRangeUninitialized(GlobalHeap, mData.GetInternalDataBuffer());
		mData = Move(newData);
	}

	SparseArray(const SparseArray&) = delete;
	SparseArray& operator=(const SparseArray&) = delete;
};
} INTRA_END
