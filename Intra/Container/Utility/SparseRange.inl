#pragma once

#include "SparseRange.h"

#include "Memory/Memory.h"

#include "Utils/FixedArray.h"

#include "Cpp/Warnings.h"
#include "Cpp/Intrinsics.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Range {

template<typename T, typename Index> T& SparseRange<T, Index>::append_first_free(Index* oIndex)
{
	if(Empty()) init_free_list();
	if(oIndex!=null) *oIndex = mFirstFree;
	T& result = mData[size_t(mFirstFree)];
	mFirstFree = reinterpret_cast<Index&>(result);
	return result;
}

template<typename T, typename Index> void SparseRange<T, Index>::init_free_list()
{
	INTRA_ASSERT(mData.Length() >= 1);
	for(size_t i=0; i<mData.Length()-1; i++)
		reinterpret_cast<Index&>(mData[i]) = Index(i+1);
	reinterpret_cast<Index&>(mData.Last()) = end_index();
	mFirstFree = 0;
}

template<typename T, typename Index> T& SparseRange<T, Index>::Add(T&& val, Index* oIndex)
{
	INTRA_DEBUG_ASSERT(!IsFull());
	T& result = append_first_free(oIndex);
	new(&result) T(Cpp::Move(val));
	return result;
}

template<typename T, typename Index> T& SparseRange<T, Index>::Add(const T& val, Index* oIndex)
{
	INTRA_DEBUG_ASSERT(!IsFull());
	T& result = append_first_free(oIndex);
	new(&result) T(val);
	return result;
}

template<typename T, typename Index> template<typename... Args> T& SparseRange<T, Index>::Emplace(Args&&... args, Index* oIndex)
{
	INTRA_DEBUG_ASSERT(!IsFull());
	T& result = append_first_free(oIndex);
	new(&result) T(Cpp::Forward<Args>(args)...);
	return result;
}


template<typename T, typename Index> void SparseRange<T, Index>::Remove(Index index)
{
	mData[size_t(index)].~T();
	Index& nextFree = reinterpret_cast<Index&>(mData[size_t(index)]);
	nextFree = mFirstFree;
	mFirstFree = index;
}

template<typename T, typename Index> FixedArray<flag32> SparseRange<T, Index>::DeadBitfield() const
{
	if(Empty()) return null;
	enum {ValueBits = sizeof(flag32)*8};
	FixedArray<flag32> result(mData.Length()/ValueBits); //Заполнит все биты нулями
	Index ff = mFirstFree;
	while(ff != end_index())
	{
		INTRA_DEBUG_ASSERT(size_t(ff) < mData.Length());
		result[size_t(ff/ValueBits)] |= (1u << (ff % ValueBits));
		ff = reinterpret_cast<Index&>(mData[size_t(ff)]);
	}
	return result;
}

template<typename T, typename Index> template<typename U> Meta::EnableIf<
	!Meta::IsTriviallyDestructible<U>::_
> SparseRange<T, Index>::Clear()
{
	if(Empty()) return;
	if(IsFull())
	{
		Memory::Destruct(mData);
		return;
	}

	//Уже удалённые объекты нельзя удалять, поэтому удалим только те, которые нужно.
	//Для этого составим битовое поле, обозначающее, какие объекты живы, а какие уже нет
	enum {ValueBits = sizeof(flag32)*8};
	const auto deadBitfield = DeadBitfield();
	for(size_t i=0; i<mData.Length(); i++)
	{
		if(deadBitfield[i/ValueBits] & (1 << (i % ValueBits))) continue; //Этот объект уже удалён
		mData[i].~T();
	}
}




template<typename T, typename Index> void SparseRange<T, Index>::MoveTo(SparseRange& dst)
{
	if(Empty()) return;
	INTRA_DEBUG_ASSERT(dst.mData.Length() >= mData.Length());
	enum {ValueBits = sizeof(flag32)*8};
	const auto deadBitfield = DeadBitfield();
	Index* prevEmpty = &dst.mFirstFree;
	for(size_t i=0; i<mData.Length(); i++)
	{
		if(deadBitfield[i/ValueBits] & (1u << (i % ValueBits)))
		{
			//Используем свободные элементы для удлинения списка индексов свободных элементов
			*prevEmpty = Index(i);
			prevEmpty = reinterpret_cast<Index*>(&dst.mData[i]);
			continue;
		}
		//Переносим элемент из одного разреженного массива в другой
		new(&dst.mData[i]) T(Cpp::Move(mData[i]));
		mData[i].~T();
	}

	//Текущий список снова становится неинициализирован, и будет инициализирован при первой вставке элемента
	mFirstFree = empty_index();

	//Инициализируем список до конца
	for(size_t i=mData.Length(); i<dst.mData.Length()-1; i++)
	{
		*prevEmpty = Index(i);
		prevEmpty = reinterpret_cast<Index*>(&dst.mData[i]);
	}
	*prevEmpty = end_index();
}







template<typename Index> byte* SparseTypelessRange<Index>::append_first_free(Index* oIndex)
{
	if(Empty()) init_free_list();
	if(oIndex!=null) *oIndex = mFirstFree;
	byte* result = operator[](mFirstFree);
	mFirstFree = *reinterpret_cast<Index*>(result);
	return result;
}

template<typename Index> void SparseTypelessRange<Index>::init_free_list()
{
	for(size_t i=0; i<Capacity()-1; i++)
		reinterpret_cast<Index&>(operator[](i)) = i+1;
	reinterpret_cast<Index&>(operator[](Capacity()-1)) = end_index();
	mFirstFree = 0;
}

template<typename Index> byte* SparseTypelessRange<Index>::Emplace(Index* oIndex)
{
	INTRA_DEBUG_ASSERT(!IsFull());
	return append_first_free(oIndex);
}


template<typename Index> void SparseTypelessRange<Index>::Remove(Index index)
{
	Index& nextFree = reinterpret_cast<Index&>(mData[index]);
	nextFree = mFirstFree;
	mFirstFree = index;
}

template<typename Index> FixedArray<flag32> SparseTypelessRange<Index>::DeadBitfield() const
{
	enum {ValueBits = sizeof(flag32)*8};
	FixedArray<flag32> result(mData.Length()/ValueBits); //Заполнит все биты нулями
	size_t ff = mFirstFree;
	while(ff != end_index())
	{
		INTRA_DEBUG_ASSERT(ff < Capacity());
		result[ff/ValueBits] |= (1 << (ff % ValueBits));
		ff = reinterpret_cast<Index&>(mData[ff]);
	}
	return result;
}


template<typename Index> void SparseTypelessRange<Index>::MoveTo(SparseTypelessRange& dst)
{
	INTRA_DEBUG_ASSERT(dst.mData.Length() >= mData.Length());
	C::memcpy(dst.mData.Begin, mData.Begin, mData.Length());
	dst.mNodeSize = mNodeSize;

	//Инициализируем список до конца
	Index* prevEmpty = &dst.mFirstFree;
	for(size_t i=mData.Length(); i<dst.mData.Length()-1; i += mNodeSize)
	{
		*prevEmpty = Index(i);
		prevEmpty = reinterpret_cast<Index*>(&dst.mData[i]);
	}
	*prevEmpty = mFirstFree;
}



}}

INTRA_WARNING_POP
