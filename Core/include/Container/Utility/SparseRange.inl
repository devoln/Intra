#pragma once

#include "SparseRange.h"
#include "Memory/Memory.h"
#include "Container/Sequential/Array.h"
#include "Platform/CppWarnings.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Range {

template<typename T, typename Index> T& SparseRange<T, Index>::append_first_free(Index* oIndex)
{
	if(Empty()) init_free_list();
	if(oIndex!=null) *oIndex = first_free;
	T& result = data[size_t(first_free)];
	first_free = reinterpret_cast<Index&>(result);
	return result;
}

template<typename T, typename Index> void SparseRange<T, Index>::init_free_list()
{
	INTRA_ASSERT(data.Length() >= 1);
	for(size_t i=0; i<data.Length()-1; i++)
		reinterpret_cast<Index&>(data[i]) = Index(i+1);
	reinterpret_cast<Index&>(data.Last()) = end_index();
	first_free = 0;
}

template<typename T, typename Index> T& SparseRange<T, Index>::Add(T&& val, Index* oIndex)
{
	INTRA_DEBUG_ASSERT(!IsFull());
	T& result = append_first_free(oIndex);
	new(&result) T(Meta::Move(val));
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
	new(&result) T(Meta::Forward<Args>(args)...);
	return result;
}


template<typename T, typename Index> void SparseRange<T, Index>::Remove(Index index)
{
	data[size_t(index)].~T();
	Index& nextFree = reinterpret_cast<Index&>(data[size_t(index)]);
	nextFree = first_free;
	first_free = index;
}

template<typename T, typename Index> Array<flag32> SparseRange<T, Index>::DeadBitfield() const
{
	if(Empty()) return null;
	enum {ValueBits = sizeof(flag32)*8};
	Array<flag32> result;
	result.SetCount(data.Length()/ValueBits); //Заполнит все биты нулями
	Index ff = first_free;
	while(ff != end_index())
	{
		INTRA_DEBUG_ASSERT(size_t(ff) < data.Length());
		result[size_t(ff/ValueBits)] |= (1u << (ff % ValueBits));
		ff = reinterpret_cast<Index&>(data[size_t(ff)]);
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
		Memory::Destruct(data);
		return;
	}

	//Уже удалённые объекты нельзя удалять, поэтому удалим только те, которые нужно.
	//Для этого составим битовое поле, обозначающее, какие объекты живы, а какие уже нет
	enum {ValueBits = sizeof(flag32)*8};
	Array<flag32> deadBitfield = DeadBitfield();
	for(size_t i=0; i<data.Length(); i++)
	{
		if(deadBitfield[i/ValueBits] & (1 << (i % ValueBits))) continue; //Этот объект уже удалён
		data[i].~T();
	}
}




template<typename T, typename Index> void SparseRange<T, Index>::MoveTo(SparseRange& dst)
{
	if(Empty()) return;
	INTRA_DEBUG_ASSERT(dst.data.Length() >= data.Length());
	enum {ValueBits = sizeof(flag32)*8};
	Array<flag32> deadBitfield = DeadBitfield();
	Index* prevEmpty = &dst.first_free;
	for(size_t i=0; i<data.Length(); i++)
	{
		if(deadBitfield[i/ValueBits] & (1 << (i % ValueBits)))
		{
			//Используем свободные элементы для удлинения списка индексов свободных элементов
			*prevEmpty = Index(i);
			prevEmpty = reinterpret_cast<Index*>(&dst.data[i]);
			continue;
		}
		//Переносим элемент из одного разреженного массива в другой
		new(&dst.data[i]) T(Meta::Move(data[i]));
		data[i].~T();
	}

	//Текущий список снова становится неинициализирован, и будет инициализирован при первой вставке элемента
	first_free = empty_index();

	//Инициализируем список до конца
	for(size_t i=data.Length(); i<dst.data.Length()-1; i++)
	{
		*prevEmpty = Index(i);
		prevEmpty = reinterpret_cast<Index*>(&dst.data[i]);
	}
	*prevEmpty = end_index();
}







template<typename Index> byte* SparseTypelessRange<Index>::append_first_free(Index* oIndex)
{
	if(Empty()) init_free_list();
	if(oIndex!=null) *oIndex = first_free;
	byte* result = operator[](first_free);
	first_free = *reinterpret_cast<Index*>(result);
	return result;
}

template<typename Index> void SparseTypelessRange<Index>::init_free_list()
{
	for(size_t i=0; i<Capacity()-1; i++)
		reinterpret_cast<Index&>(operator[](i)) = i+1;
	reinterpret_cast<Index&>(operator[](Capacity()-1)) = end_index();
	first_free = 0;
}

template<typename Index> byte* SparseTypelessRange<Index>::Emplace(Index* oIndex)
{
	INTRA_DEBUG_ASSERT(!IsFull());
	return append_first_free(oIndex);
}


template<typename Index> void SparseTypelessRange<Index>::Remove(Index index)
{
	Index& nextFree = reinterpret_cast<Index&>(data[index]);
	nextFree = first_free;
	first_free = index;
}

template<typename Index> Array<flag32> SparseTypelessRange<Index>::DeadBitfield() const
{
	enum {ValueBits = sizeof(flag32)*8};
	Array<flag32> result;
	result.SetCount(data.Length()/ValueBits); //Заполнит все биты нулями
	size_t ff = first_free;
	while(ff != end_index())
	{
		INTRA_DEBUG_ASSERT(ff<Capacity());
		result[ff/ValueBits] |= (1 << (ff % ValueBits));
		ff = reinterpret_cast<Index&>(data[ff]);
	}
	return result;
}


template<typename Index> void SparseTypelessRange<Index>::MoveTo(SparseTypelessRange& dst)
{
	INTRA_DEBUG_ASSERT(dst.data.Length()>=data.Length());
	memcpy(dst.data.Begin, data.Begin, data.Length());
	dst.node_size = node_size;

	//Инициализируем список до конца
	Index* prevEmpty = &dst.first_free;
	for(size_t i=data.Length(); i<dst.data.Length()-1; i+=node_size)
	{
		*prevEmpty = Index(i);
		prevEmpty = reinterpret_cast<Index*>(&dst.data[i]);
	}
	*prevEmpty = first_free;
}



}}

INTRA_WARNING_POP
