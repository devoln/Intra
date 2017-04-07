#pragma once

#include "Container/ForwardDecls.h"
#include "Range/Generators/ArrayRange.h"
#include "Platform/CppWarnings.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Range {

template<typename T, typename Index> struct SparseRange
{
	static_assert(sizeof(T)>=sizeof(Index), "Type T must not be shorter than index!");

	SparseRange(ArrayRange<T> sparseBuffer=null):
		data(sparseBuffer), first_free(empty_index()) {}

	SparseRange(SparseRange&& rhs):
		data(rhs.data), first_free(rhs.first_free)
	{rhs.data = null; rhs.first_free = empty_index();}

	SparseRange& operator=(SparseRange&& rhs)
	{
		Clear();
		data = rhs.data; first_free = rhs.first_free;
		rhs.data = null; rhs.first_free = empty_index();
		return *this;
	}
	~SparseRange() {Clear();}

	//! Переместить элемент в массив.
	//! \param[in] val Перещаемый элемент.
	//! \param[out] oIndex Указатель, по которому будет записан индекс созданного элемента в массиве.
	T& Add(T&& val, Index* oIndex=null);

	//! Добавить копию элемента в массив.
	//! \param[in] val Копируемый элемент.
	//! \param[out] oIndex Указатель, по которому будет записан индекс созданного элемента в массиве.
	T& Add(const T& val, Index* oIndex=null);

	//! Сконструировать элемент в массиве.
	//! \param[in] args Параметры конструктора.
	//! \param[out] oIndex Указатель, по которому будет записан индекс созданного элемента в массиве.
	template<typename... Args> T& Emplace(Args&&... args, Index* oIndex=null);

	//! Удалить из массива элемент с индексом index
	void Remove(Index index);


	//! Удалить все элементы массива
	template<typename U=T> Meta::EnableIf<
		!Meta::IsTriviallyDestructible<U>::_
	> Clear();

	template<typename U=T> Meta::EnableIf<
		Meta::IsTriviallyDestructible<U>::_
	> Clear() {first_free = empty_index();}


	void MakeNull() {Clear(); data=null;}

	//! Доступ по индексу следует использовать только тогда, когда точно известно, что элемент с этим индексом не был удалён.
	T& operator[](Index index) {return data[size_t(index)];}
	const T& operator[](Index index) const {return data[size_t(index)];}

	//! Возвращает, заполнен ли массив.
	//! Это означает, что массив не содержит свободных элементов,
	//! и следующая вставка элемента приведёт к перераспределению памяти.
	bool IsFull() const {return first_free == end_index() || data.Empty();}

	//! Возвращает, пуст ли этот разреженный массив.
	bool Empty() const {return first_free == empty_index();}

	Index Capacity() const {return Index(data.Length());}


	//! Возвращает битовое поле, каждый бит которого обозначает, существует ли элемент по соответствующему индексу или нет.
	Array<flag32> DeadBitfield() const;

	//! Переместить этот разреженный массив в другой разреженный массив не меньшего размера.
	void MoveTo(SparseRange& rhs);

	//! Возвращает диапазон, содержащий элементы разреженного массива.
	//! Этот буфер безопасно использовать только в том случае, когда разреженный массив пуст, то есть выполняется Empty().
	//! В этом случае добавление нового элемента в разреженный массив перезапишет все данные в этом буфере.
	ArrayRange<T> GetInternalDataBuffer() {return data;}

	bool operator==(null_t) const {return Empty();}
	bool operator!=(null_t) const {return !Empty();}

private:
	ArrayRange<T> data;
	Index first_free;

	T& append_first_free(Index* oIndex);
	void init_free_list();

	static Index end_index() {return Meta::NumericLimits<Index>::Max();}
	static Index empty_index() {return Index(end_index()-1);}

	SparseRange& operator=(const SparseRange&) = delete;
	SparseRange(const SparseRange&) = delete;
};




template<typename Index> struct SparseTypelessRange
{
	SparseTypelessRange(null_t=null): first_free(empty_index()), node_size(0) {}

	SparseTypelessRange(ArrayRange<byte> sparseBuffer, size_t nodeSize):
		data(sparseBuffer), first_free(empty_index()), node_size(nodeSize) {}

	SparseTypelessRange(SparseTypelessRange&& rhs):
		data(rhs.data), first_free(rhs.first_free), node_size(rhs.node_size)
	{rhs.data = null; rhs.first_free = empty_index(); rhs.node_size=0;}

	SparseTypelessRange& operator=(SparseTypelessRange&& rhs)
	{
		data = rhs.data; first_free = rhs.first_free; node_size = rhs.node_size;
		rhs.data = null; rhs.first_free = empty_index(); rhs.node_size = 0;
		return *this;
	}

	//! Сконструировать элемент в массиве.
	//! \param[in] args Параметры конструктора.
	//! \param[out] oIndex Указатель, по которому будет записан индекс созданного элемента в массиве.
	byte* Emplace(Index* oIndex=null);

	//! Удалить из массива элемент с индексом index
	void Remove(Index index);


	//! Удалить все элементы массива
	void Clear() {first_free = empty_index();}


	void MakeNull() {Clear(); data=null;}

	//! Доступ по индексу следует использовать только тогда, когда точно известно, что элемент с этим индексом не был удалён.
	byte* operator[](Index index) {return data.Begin+index*node_size;}
	const byte* operator[](Index index) const {return data.Begin+index*node_size;}

	//! Возвращает, заполнен ли массив.
	//! Это означает, что массив не содержит свободных элементов,
	//! и следующая вставка элемента приведёт к перераспределению памяти.
	bool IsFull() const {return first_free = end_index();}


	//! Возвращает, пуст ли этот разреженный массив.
	bool Empty() const {return first_free = empty_index();}

	Index Capacity() const {return Index(data.Length()/node_size);}


	//! Возвращает битовое поле, каждый бит которого обозначает, существует ли элемент по соответствующему индексу или нет.
	Array<flag32> DeadBitfield() const;

	//! Переместить этот разреженный массив в другой разреженный массив не меньшего размера.
	void MoveTo(SparseTypelessRange& rhs);

	//! Возвращает диапазон, содержащий элементы разреженного массива.
	//! Этот буфер безопасно использовать только в том случае, когда разреженный массив пуст, то есть выполняется Empty().
	//! В этом случае добавление нового элемента в разреженный массив перезапишет все данные в этом буфере.
	ArrayRange<byte> GetInternalDataBuffer() {return data;}

	bool operator==(null_t) const {return Empty();}
	bool operator!=(null_t) const {return !Empty();}

private:
	ArrayRange<byte> data;
	Index first_free;
	size_t node_size;

	byte* append_first_free(Index* oIndex);
	void init_free_list();

	static Index end_index() {return Meta::NumericLimits<Index>::Max();}
	static Index empty_index() {return end_index()-1;}

	SparseTypelessRange& operator=(const SparseTypelessRange&) = delete;
	SparseTypelessRange(const SparseTypelessRange&) = delete;
};




}}

INTRA_WARNING_POP

#include "SparseRange.inl"
