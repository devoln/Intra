#pragma once

#include "Intra/Range/Span.h"
#include "IntraX/Utils/FixedArray.h"

namespace Intra { INTRA_BEGIN
/// TODO: this class should be renamed
///  It uses external memory from Span range but it manages the lifetime of its elements: it calls their constructors
///  and destructors Also it lacks range semantics.

template<typename T, typename Index> struct SparseRange
{
	static_assert(sizeof(T) >= sizeof(Index), "Type T must not be shorter than Index!");
	static_assert(CBasicUnsignedIntegral<Index>, "Type Index must be unsigned!");

	SparseRange(Span<T> sparseBuffer = nullptr): mData(sparseBuffer), mFirstFree(empty_index()) {}

	SparseRange(SparseRange&& rhs): mData(rhs.mData), mFirstFree(rhs.mFirstFree)
	{
		rhs.mData = nullptr;
		rhs.mFirstFree = empty_index();
	}

	SparseRange& operator=(SparseRange&& rhs)
	{
		Clear();
		mData = rhs.mData;
		mFirstFree = rhs.mFirstFree;
		rhs.mData = nullptr;
		rhs.mFirstFree = empty_index();
		return *this;
	}
	~SparseRange() { Clear(); }

	/// Move \p val into the array returning new element's index via \p oIndex pointer.
	/*!
	  @returns The reference to the added element.
	*/
	T& Add(T&& val, Optional<Index&> oIndex = nullptr);

	/// Add \p val returning a new element's index via \p oIndex pointer.
	/*!
	  @returns The reference to the added element.
	*/
	T& Add(const T& val, Optional<Index&> oIndex = nullptr);

	/// Construct an element inplace passing \p args to its constructor and returning new element's index via \p oIndex
	/// pointer.
	/*!
	  @returns The reference to the added element.
	*/
	template<typename... Args> T& Emplace(Args&&... args, Optional<Index&> oIndex = nullptr);

	/// Remove the element at \p index.
	void Remove(Index index);

	/// Remove all the elements.
	void Clear();

	void MakeNull()
	{
		Clear();
		mData = nullptr;
	}

	/// Warning: unsafe, use only when you know that corresponding element hasn't been deleted.
	T& operator[](Index index) { return mData[index_t(index)]; }
	const T& operator[](Index index) const { return mData[index_t(index)]; }

	/// @return true if there are no free elements, so that next insertion will lead to a reallocation.
	bool IsFull() const { return mFirstFree == end_index() || mData.Empty(); }

	/// @return true if this container contains no elements.
	bool Empty() const { return mFirstFree == empty_index(); }

	Index Capacity() const { return Index(mData.Length()); }

	// TODO: implement bitset to replace FixedArray<unsigned>

	/// @return a bitfield. Each bit means if the element with corresponding index has been deleted.
	FixedArray<unsigned> DeadBitfield() const;

	/// Move all elements to another larger or equal size container.
	void MoveTo(SparseRange& rhs);

	/// @return internal data buffer.
	/// It is only to safe the returned buffer when this->Empty() returns true.
	/// In this case adding a new element into SparseArray overwrites all the data in this buffer.
	Span<T> GetInternalDataBuffer() { return mData; }

	bool operator==(decltype(nullptr)) const { return Empty(); }
	bool operator!=(decltype(nullptr)) const { return !Empty(); }

private:
	Span<T> mData;
	Index mFirstFree;

	T& append_first_free(Optional<Index&> oIndex);
	void init_free_list();

	static Index end_index() { return MaxValueOf(Index()); }
	static Index empty_index() { return Index(end_index() - 1); }

	SparseRange& operator=(const SparseRange&) = delete;
	SparseRange(const SparseRange&) = delete;
};

template<typename Index> struct SparseTypelessRange
{
	SparseTypelessRange(decltype(nullptr) = nullptr): mFirstFree(empty_index()), mNodeSize(0) {}

	SparseTypelessRange(Span<byte> sparseBuffer, size_t nodeSize):
		mData(sparseBuffer), mFirstFree(empty_index()), mNodeSize(nodeSize)
	{
	}

	SparseTypelessRange(SparseTypelessRange&& rhs):
		mData(rhs.mData), mFirstFree(rhs.mFirstFree), mNodeSize(rhs.mNodeSize)
	{
		rhs.mData = nullptr;
		rhs.mFirstFree = empty_index();
		rhs.mNodeSize = 0;
	}

	SparseTypelessRange& operator=(SparseTypelessRange&& rhs)
	{
		mData = rhs.mData;
		mFirstFree = rhs.mFirstFree;
		mNodeSize = rhs.mNodeSize;
		rhs.mData = nullptr;
		rhs.mFirstFree = empty_index();
		rhs.mNodeSize = 0;
		return *this;
	}

	/// Construct a new element in the container.
	/// @param[out] oIndex Where to write the index of the constructed element.
	byte* Emplace(Optional<Index&> oIndex = nullptr);

	/// Delete the element at index.
	void Remove(Index index);

	/// Delete all container elements.
	void Clear() { mFirstFree = empty_index(); }

	void MakeNull()
	{
		Clear();
		mData = nullptr;
	}

	/// Warning: unsafe, use only when you know that corresponding element hasn't been deleted.
	byte* operator[](Index index) { return mData[index * mNodeSize]; }
	const byte* operator[](Index index) const { return mData[index * mNodeSize]; }

	/// @return true if there are no free elements, so that next insertion will lead to a reallocation.
	bool IsFull() const { return mFirstFree = end_index(); }

	/// @return true if this container contains no elements.
	bool Empty() const { return mFirstFree = empty_index(); }

	Index Capacity() const { return Index(mData.Length() / mNodeSize); }

	/// @return a bitfield. Each bit means if the element with corresponding index has been deleted.
	FixedArray<unsigned> DeadBitfield() const;

	/// Move all elements to another larger or equal size container.
	void MoveTo(SparseTypelessRange& rhs);

	/// @return internal data buffer.
	/// It is only to safe the returned buffer when this->Empty() returns true.
	/// In this case adding a new element into SparseArray overwrites all the data in this buffer.
	Span<byte> GetInternalDataBuffer() { return mData; }

	bool operator==(decltype(nullptr)) const { return Empty(); }
	bool operator!=(decltype(nullptr)) const { return !Empty(); }

private:
	Span<byte> mData;
	Index mFirstFree;
	size_t mNodeSize;

	byte* append_first_free(Optional<Index&> oIndex);
	void init_free_list();

	static Index end_index() { return MaxValueOf(Index()); }
	static Index empty_index() { return end_index() - 1; }

	SparseTypelessRange& operator=(const SparseTypelessRange&) = delete;
	SparseTypelessRange(const SparseTypelessRange&) = delete;
};

} INTRA_END

#include "SparseRange.inl"
