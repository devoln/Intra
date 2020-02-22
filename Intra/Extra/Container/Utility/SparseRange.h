#pragma once

#include "Core/Range/Span.h"
#include "Utils/FixedArray.h"

INTRA_BEGIN
//! TODO: this class must be renamed
//!  It uses external memory from Span range but it owns its elements: it calls their constructors and destructors
//!  Also it lacks range semantics.

template<typename T, typename Index> struct SparseRange
{
	static_assert(sizeof(T) >= sizeof(Index), "Type T must not be shorter than index!");

	SparseRange(Span<T> sparseBuffer = null):
		mData(sparseBuffer), mFirstFree(empty_index()) {}

	SparseRange(SparseRange&& rhs):
		mData(rhs.mData), mFirstFree(rhs.mFirstFree)
	{
		rhs.mData = null;
		rhs.mFirstFree = empty_index();
	}

	SparseRange& operator=(SparseRange&& rhs)
	{
		Clear();
		mData = rhs.mData;
		mFirstFree = rhs.mFirstFree;
		rhs.mData = null;
		rhs.mFirstFree = empty_index();
		return *this;
	}
	~SparseRange() {Clear();}

	//! Move \p val into the array returning new element's index via \p oIndex pointer.
	/*!
	  @returns The reference to the added element.
	*/
	T& Add(T&& val, Index* oIndex = null);

	//! Add \p val returning a new element's index via \p oIndex pointer.
	/*!
	  @returns The reference to the added element.
	*/
	T& Add(const T& val, Index* oIndex=null);

	//! Construct an element inplace passing \p args to its constructor and returning new element's index via \p oIndex pointer.
	/*!
	  @returns The reference to the added element.
	*/
	template<typename... Args> T& Emplace(Args&&... args, Index* oIndex = null);

	//! Remove the element at \p index.
	void Remove(Index index);


	//! Remove all the elements.
	template<typename U=T> Requires<
		!CTriviallyDestructible<U>
	> Clear();

	//! Remove all the elements.
	template<typename U=T> Requires<
		CTriviallyDestructible<U>
	> Clear() {mFirstFree = empty_index();}


	void MakeNull() {Clear(); mData = null;}

	//! Доступ по индексу следует использовать только тогда, когда точно известно, что элемент с этим индексом не был удалён.
	T& operator[](Index index) {return mData[size_t(index)];}
	const T& operator[](Index index) const {return mData[size_t(index)];}

	//! Возвращает, заполнен ли массив.
	//! Это означает, что массив не содержит свободных элементов,
	//! и следующая вставка элемента приведёт к перераспределению памяти.
	bool IsFull() const {return mFirstFree == end_index() || mData.Empty();}

	//! Возвращает, пуст ли этот разреженный массив.
	forceinline bool Empty() const {return mFirstFree == empty_index();}

	Index Capacity() const {return Index(mData.Length());}


	// TODO: implement bitset to replace FixedArray<uint>

	//! Возвращает битовое поле, каждый бит которого обозначает, существует ли элемент по соответствующему индексу или нет.
	FixedArray<uint> DeadBitfield() const;

	//! Переместить этот разреженный массив в другой разреженный массив не меньшего размера.
	void MoveTo(SparseRange& rhs);

	//! Возвращает диапазон, содержащий элементы разреженного массива.
	//! Этот буфер безопасно использовать только в том случае, когда разреженный массив пуст, то есть выполняется Empty().
	//! В этом случае добавление нового элемента в разреженный массив перезапишет все данные в этом буфере.
	Span<T> GetInternalDataBuffer() {return mData;}

	forceinline bool operator==(null_t) const {return Empty();}
	forceinline bool operator!=(null_t) const {return !Empty();}

private:
	Span<T> mData;
	Index mFirstFree;

	T& append_first_free(Index* oIndex);
	void init_free_list();

	static forceinline Index end_index() {return LMaxOf(Index());}
	static forceinline Index empty_index() {return Index(end_index()-1);}

	SparseRange& operator=(const SparseRange&) = delete;
	SparseRange(const SparseRange&) = delete;
};




template<typename Index> struct SparseTypelessRange
{
	SparseTypelessRange(null_t=null):
		mFirstFree(empty_index()), mNodeSize(0) {}

	SparseTypelessRange(Span<byte> sparseBuffer, size_t nodeSize):
		mData(sparseBuffer), mFirstFree(empty_index()), mNodeSize(nodeSize) {}

	SparseTypelessRange(SparseTypelessRange&& rhs):
		mData(rhs.mData), mFirstFree(rhs.mFirstFree), mNodeSize(rhs.mNodeSize)
	{
		rhs.mData = null;
		rhs.mFirstFree = empty_index();
		rhs.mNodeSize = 0;
	}

	SparseTypelessRange& operator=(SparseTypelessRange&& rhs)
	{
		mData = rhs.mData;
		mFirstFree = rhs.mFirstFree;
		mNodeSize = rhs.mNodeSize;
		rhs.mData = null;
		rhs.mFirstFree = empty_index();
		rhs.mNodeSize = 0;
		return *this;
	}

	//! Сконструировать элемент в массиве.
	//! \param[in] args Параметры конструктора.
	//! \param[out] oIndex Указатель, по которому будет записан индекс созданного элемента в массиве.
	byte* Emplace(Index* oIndex=null);

	//! Удалить из массива элемент с индексом index
	void Remove(Index index);


	//! Удалить все элементы массива
	void Clear() {mFirstFree = empty_index();}


	void MakeNull() {Clear(); mData=null;}

	//! Доступ по индексу следует использовать только тогда, когда точно известно, что элемент с этим индексом не был удалён.
	byte* operator[](Index index) {return mData[index*mNodeSize];}
	const byte* operator[](Index index) const {return mData[index*mNodeSize];}

	//! Возвращает, заполнен ли массив.
	//! Это означает, что массив не содержит свободных элементов,
	//! и следующая вставка элемента приведёт к перераспределению памяти.
	bool IsFull() const {return mFirstFree = end_index();}


	//! Возвращает, пуст ли этот разреженный массив.
	bool Empty() const {return mFirstFree = empty_index();}

	Index Capacity() const {return Index(mData.Length()/mNodeSize);}


	//! Возвращает битовое поле, каждый бит которого обозначает, существует ли элемент по соответствующему индексу или нет.
	FixedArray<uint> DeadBitfield() const;

	//! Переместить этот разреженный массив в другой разреженный массив не меньшего размера.
	void MoveTo(SparseTypelessRange& rhs);

	//! Возвращает диапазон, содержащий элементы разреженного массива.
	//! Этот буфер безопасно использовать только в том случае, когда разреженный массив пуст, то есть выполняется Empty().
	//! В этом случае добавление нового элемента в разреженный массив перезапишет все данные в этом буфере.
	Span<byte> GetInternalDataBuffer() {return mData;}

	bool operator==(null_t) const {return Empty();}
	bool operator!=(null_t) const {return !Empty();}

private:
	Span<byte> mData;
	Index mFirstFree;
	size_t mNodeSize;

	byte* append_first_free(Index* oIndex);
	void init_free_list();

	static Index end_index() {return NumericLimits<Index>::Max();}
	static Index empty_index() {return end_index()-1;}

	SparseTypelessRange& operator=(const SparseTypelessRange&) = delete;
	SparseTypelessRange(const SparseTypelessRange&) = delete;
};

INTRA_END

#include "SparseRange.inl"
