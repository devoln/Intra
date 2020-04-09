#pragma once

#include "Extra/Memory/Memory.h"
#include "Extra/Utils/FixedArray.h"
#include "SparseRange.h"

INTRA_BEGIN
template<typename T, typename Index> T& SparseRange<T, Index>::append_first_free(Optional<Index&> oIndex)
{
	if(Empty()) init_free_list();
	if(oIndex) oIndex.Unwrap() = mFirstFree;
	T& result = mData[size_t(mFirstFree)];
	mFirstFree = reinterpret_cast<Index&>(result);
	return result;
}

template<typename T, typename Index> void SparseRange<T, Index>::init_free_list()
{
	INTRA_PRECONDITION(mData.Length() >= 1);
	for(size_t i = 0; i < mData.Length() - 1; i++) reinterpret_cast<Index&>(mData[i]) = Index(i + 1);
	reinterpret_cast<Index&>(mData.Last()) = end_index();
	mFirstFree = 0;
}

template<typename T, typename Index> T& SparseRange<T, Index>::Add(T&& val, Optional<Index&> oIndex)
{
	INTRA_PRECONDITION(!IsFull());
	T& result = append_first_free(oIndex);
	new(Construct, &result) T(Move(val));
	return result;
}

template<typename T, typename Index> T& SparseRange<T, Index>::Add(const T& val, Optional<Index&> oIndex)
{
	INTRA_PRECONDITION(!IsFull());
	T& result = append_first_free(oIndex);
	new(Construct, &result) T(val);
	return result;
}

template<typename T, typename Index> template<typename... Args>
T& SparseRange<T, Index>::Emplace(Args&&... args, Optional<Index&> oIndex)
{
	INTRA_PRECONDITION(!IsFull());
	T& result = append_first_free(oIndex);
	new(Construct, &result) T(Forward<Args>(args)...);
	return result;
}

template<typename T, typename Index> void SparseRange<T, Index>::Remove(Index index)
{
	mData[size_t(index)].~T();
	Index& nextFree = reinterpret_cast<Index&>(mData[size_t(index)]);
	nextFree = mFirstFree;
	mFirstFree = index;
}

template<typename T, typename Index> FixedArray<unsigned> SparseRange<T, Index>::DeadBitfield() const
{
	if(Empty()) return null;
	enum { ValueBits = sizeof(unsigned) * 8 };
	FixedArray<unsigned> result(mData.Length() / ValueBits); //Заполнит все биты нулями
	Index ff = mFirstFree;
	while(ff != end_index())
	{
		INTRA_DEBUG_ASSERT(ff < mData.Length());
		result[index_t(size_t(ff) / ValueBits)] |= (1u << (size_t(ff) % ValueBits));
		ff = reinterpret_cast<Index&>(mData[index_t(ff)]);
	}
	return result;
}

template<typename T, typename Index> void SparseRange<T, Index>::Clear()
{
	if(CTriviallyDestructible<T>)
	{
		mFirstFree = empty_index();
		return;
	}
	if(Empty()) return;
	if(IsFull())
	{
		Destruct(mData);
		return;
	}

	// Already deleted objects cannot be deleted.
	// Therefore we need to generate a bitfield of dead objects and then delete only alive ones
	enum { ValueBits = sizeof(unsigned) * 8 };
	const auto deadBitfield = DeadBitfield();
	for(size_t i = 0; i < size_t(mData.Length()); i++)
	{
		if(deadBitfield[index_t(i / ValueBits)] & (1 << (i % ValueBits))) continue; // This object is already deleted
		mData[i].~T();
	}
}

template<typename T, typename Index> void SparseRange<T, Index>::MoveTo(SparseRange& dst)
{
	INTRA_PRECONDITION(dst.mData.Length() >= mData.Length());
	if(Empty()) return;
	enum { ValueBits = sizeof(unsigned) * 8 };
	const auto deadBitfield = DeadBitfield();
	Index* prevEmpty = &dst.mFirstFree;
	for(size_t i = 0; i < size_t(mData.Length()); i++)
	{
		if(deadBitfield[index_t(i / ValueBits)] & (1u << (i % ValueBits)))
		{
			//Используем свободные элементы для удлинения списка индексов свободных элементов
			*prevEmpty = Index(i);
			prevEmpty = reinterpret_cast<Index*>(&dst.mData[i]);
			continue;
		}
		// Move the element from one sparse array to the other
		new(Construct, &dst.mData[i]) T(Move(mData[i]));
		mData[i].~T();
	}

	// The current list now becomes uninitialized, it will be initialized after a first insertion
	mFirstFree = empty_index();

	// Init the list until the end
	for(auto i = mData.Length(); i < dst.mData.Length() - 1; i++)
	{
		*prevEmpty = Index(i);
		prevEmpty = reinterpret_cast<Index*>(&dst.mData[i]);
	}
	*prevEmpty = end_index();
}

template<typename Index> byte* SparseTypelessRange<Index>::append_first_free(Optional<Index&> oIndex)
{
	if(Empty()) init_free_list();
	if(oIndex != null) oIndex.Unwrap() = mFirstFree;
	byte* result = operator[](mFirstFree);
	mFirstFree = *reinterpret_cast<Index*>(result);
	return result;
}

template<typename Index> void SparseTypelessRange<Index>::init_free_list()
{
	for(size_t i = 0; i < Capacity() - 1; i++) reinterpret_cast<Index&>(operator[](i)) = i + 1;
	reinterpret_cast<Index&>(operator[](Capacity() - 1)) = end_index();
	mFirstFree = 0;
}

template<typename Index> byte* SparseTypelessRange<Index>::Emplace(Optional<Index&> oIndex)
{
	INTRA_PRECONDITION(!IsFull());
	return append_first_free(oIndex);
}

template<typename Index> void SparseTypelessRange<Index>::Remove(Index index)
{
	Index& nextFree = reinterpret_cast<Index&>(mData[index]);
	nextFree = mFirstFree;
	mFirstFree = index;
}

template<typename Index> FixedArray<unsigned> SparseTypelessRange<Index>::DeadBitfield() const
{
	enum { ValueBits = sizeof(unsigned) * 8 };
	FixedArray<unsigned> result(index_t(size_t(mData.Length()) / ValueBits)); // initialized with all bits reset
	size_t ff = mFirstFree;
	while(ff != end_index())
	{
		INTRA_DEBUG_ASSERT(ff < Capacity());
		result[ff / ValueBits] |= (1 << (ff % ValueBits));
		ff = reinterpret_cast<Index&>(mData[ff]);
	}
	return result;
}

template<typename Index> void SparseTypelessRange<Index>::MoveTo(SparseTypelessRange& dst)
{
	INTRA_PRECONDITION(dst.mData.Length() >= mData.Length());
	Misc::CImp::memcpy(dst.mData.Begin, mData.Begin, mData.Length());
	dst.mNodeSize = mNodeSize;

	// Init the list until the end
	Index* prevEmpty = &dst.mFirstFree;
	for(auto i = mData.Length(); i < dst.mData.Length() - 1; i += mNodeSize)
	{
		*prevEmpty = Index(i);
		prevEmpty = reinterpret_cast<Index*>(&dst.mData[i]);
	}
	*prevEmpty = mFirstFree;
}
INTRA_END
