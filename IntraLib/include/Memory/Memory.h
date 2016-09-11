#pragma once

#include "Core/Core.h"
#include "Meta/Type.h"

#include "Algorithms/Range.h"
#include "Memory/Allocator.h"


namespace Intra { namespace Memory {

//Вызов конструкторов
template<typename T> Meta::EnableIfNotTrivConstructible<T> Initialize(ArrayRange<T> dst)
{
	while(!dst.Empty())
	{
		new(&dst.First()) T;
		dst.PopFirst();
	}
}

//Для POD типов просто зануление памяти
template<typename T> inline Meta::EnableIfTrivConstructible<T> Initialize(ArrayRange<T> dst)
{
	core::memset(dst.Begin, 0, dst.Length()*sizeof(T));
}


//Вызов конструктора
template<typename T> forceinline Meta::EnableIfNotTrivConstructible<T> InitializeObj(T& dst)
{
	new(&dst) T;
}

//Для POD типов просто зануление памяти
template<typename T> forceinline Meta::EnableIfTrivConstructible<T> InitializeObj(T& dst)
{
	core::memset(&dst, 0, sizeof(T));
}

//Вызов деструкторов
template<typename T> Meta::EnableIfNotTrivDestructible<T> Destruct(ArrayRange<T> dst)
{
	while(!dst.Empty())
	{
		dst.First().~T();
		dst.PopFirst();
	}
}

template<typename T> Meta::EnableIfTrivDestructible<T, void> Destruct(ArrayRange<T> dst)
{
#ifdef INTRA_DEBUG
	if(dst.Begin < dst.End)\
		core::memset(dst.Begin, 0xDEDEDEDE, dst.Length()*sizeof(T));
#endif
	(void)dst;
}

//Вызов деструктора
template<typename T> forceinline Meta::EnableIfNotTrivDestructible<T, void> DestructObj(T& dst)
{
	dst.~T();
}

template<typename T> Meta::EnableIfTrivDestructible<T, void> DestructObj(T& dst)
{
#ifdef INTRA_DEBUG
	core::memset(&dst, 0xDEDEDEDE, sizeof(T));
#endif
	(void)dst;
}

//Побитовое копирование
template<typename T> void CopyBits(ArrayRange<T> dst, ArrayRange<const T> src)
{
	INTRA_ASSERT(dst.Length()>=src.Length());
	//core::memmove(dst.Begin, src.Begin, src.Count()*sizeof(T));
	core::memcpy(dst.Begin, src.Begin, src.Count()*sizeof(T));
}

template<typename T> void CopyBits(T* dst, const T* src, size_t count)
{
	CopyBits(ArrayRange<T>(dst, count), ArrayRange<const T>(src, count));
}

template<typename T> void CopyBitsBackwards(ArrayRange<T> dst, ArrayRange<const T> src)
{
	INTRA_ASSERT(dst.Length()>=src.Length());
	core::memmove(dst.Begin, src.Begin, src.Count()*sizeof(T));
}

template<typename T> void CopyBitsBackwards(T* dst, const T* src, size_t count)
{
	CopyBitsBackwards(ArrayRange<T>(dst, count), ArrayRange<const T>(src, count));
}

template<typename T> void CopyObjectBits(T& dst, const T& src)
{
	CopyBits<T>({&dst, 1}, {&src, 1});
}

//Копирование оператором присваивания
template<typename DstRange, typename SrcRange> Meta::EnableIf<
	Range::IsRangeElementAssignable<DstRange>::_ &&
	Range::IsInputRange<SrcRange>::_ &&
	!Meta::IsTriviallyCopyAssignable<typename DstRange::value_type>::_
> CopyAssign(const DstRange& dst, const SrcRange& src)
{
	DstRange dstCopy = dst;
	SrcRange srcCopy = src;
	INTRA_ASSERT(dst.Count()<=src.Count());
	while(!dstCopy.Empty())
	{
		dstCopy.First() = srcCopy.First();
		dstCopy.PopFirst();
		srcCopy.PopFirst();
	}
}

template<typename DstRange, typename SrcRange> Meta::EnableIf<
	Range::IsArrayRange<DstRange>::_ &&
	Range::IsArrayRange<SrcRange>::_ &&
	Meta::IsTriviallyCopyAssignable<typename DstRange::value_type>::_ &&
	Range::IsFiniteInputRangeOfExactly<SrcRange, typename DstRange::value_type>::_
> CopyAssign(const DstRange& dst, const SrcRange& src)
{
	INTRA_ASSERT(dst.Count()==src.Count());
	CopyBits(dst.Data(), src.Data(), dst.Count());
}

//Копирование оператором присваивания
template<typename T> Meta::EnableIfNotTrivCopyAssignable<T> CopyAssignBackwards(ArrayRange<T> dst, ArrayRange<const T> src)
{
	INTRA_ASSERT(dst.Count()<=src.Count());
	while(!dst.Empty())
	{
		dst.Last() = src.Last();
		dst.PopLast();
		src.PopLast();
	}
}

template<typename T> Meta::EnableIfTrivCopyAssignable<T> CopyAssignBackwards(ArrayRange<T> dst, ArrayRange<const T> src)
{
	CopyBitsBackwards(dst, src);
}

//Удаление деструктором и копирование конструктором копирования
template<typename T> Meta::EnableIf<
	!Meta::IsTriviallyCopyable<T>::_ || !Meta::IsTriviallyDestructible<T>::_
> CopyRecreate(ArrayRange<T> dst, ArrayRange<const T> src)
{
	INTRA_ASSERT(dst.Count()==src.Count());
	while(!dst.Empty())
	{
		dst.First().~T();
		new(&dst.First()) T(src.First());
		src.PopFirst();
	}
}

template<typename T> Meta::EnableIf<
	Meta::IsTriviallyCopyable<T>::_ && Meta::IsTriviallyDestructible<T>::_
> CopyRecreate(ArrayRange<T> dst, ArrayRange<const T> src)
{
	CopyBits(dst, src);
}

//Удаление деструктором и копирование конструктором копирования
template<typename T> Meta::EnableIf<
	!Meta::IsTriviallyCopyable<T>::_ || !Meta::IsTriviallyDestructible<T>::_
> CopyRecreateBackwards(ArrayRange<T> dst, ArrayRange<const T> src)
{
	INTRA_ASSERT(dst.Count()==src.Count());
	while(!dst.Empty())
	{
		dst.Last().~T();
		new(&dst.Last()) T(src.Last());
		src.PopLast();
	}
}

template<typename T> Meta::EnableIf<
	Meta::IsTriviallyCopyable<T>::_ && Meta::IsTriviallyDestructible<T>::_
> CopyRecreateBackwards(ArrayRange<T> dst, ArrayRange<const T> src)
{
	CopyBitsBackwards(dst, src);
}

template<typename T, typename U> Meta::EnableIfNotTrivCopyable<T> CopyInit(ArrayRange<T> dst, ArrayRange<const U> src)
{
	INTRA_ASSERT(dst.Count()==src.Count());
	while(!src.Empty())
	{
		new(&dst.First()) T((T)src.First());
		dst.PopFirst();
		src.PopFirst();
	}
}

template<typename T> Meta::EnableIfTrivCopyable<T> CopyInit(ArrayRange<T> dst, ArrayRange<const T> src)
{
	CopyBits(dst, src);
}

template<typename T, typename U> Meta::EnableIfNotTrivCopyable<T> CopyInitBackwards(ArrayRange<T> dst, ArrayRange<const U> src)
{
	INTRA_ASSERT(dst.Count()==src.Count());
	while(!src.Empty())
	{
		new(&dst.Last()) T((T)src.Last());
		dst.PopLast();
		src.PopLast();
	}
}

template<typename T> Meta::EnableIfTrivCopyable<T> CopyInitBackwards(ArrayRange<T> dst, ArrayRange<const T> src)
{
	CopyBitsBackwards(dst, src);
}

//Инициализация конструктором перемещения
template<typename T> Meta::EnableIfNotTrivMovable<T> MoveInit(ArrayRange<T> dst, ArrayRange<T> src)
{
	INTRA_ASSERT(dst.Count()==src.Count());
	while(!dst.Empty())
	{
		new(&dst.First()) T(core::move(src.First()));
		dst.PopFirst();
		src.PopFirst();
	}
}

//Инициализация конструктором перемещения
template<typename T> Meta::EnableIfTrivMovable<T> MoveInit(ArrayRange<T> dst, ArrayRange<T> src)
{
	CopyBits(dst, src.AsConstRange());
}

//Инициализация конструктором перемещения
template<typename T> Meta::EnableIfNotTrivMovable<T> MoveInitBackwards(ArrayRange<T> dst, ArrayRange<T> src)
{
	INTRA_ASSERT(dst.Count()==src.Count());
	while(!dst.Empty())
	{
		new(&dst.Last()) T(core::move(src.Last()));
		dst.PopLast();
		src.PopLast();
	}
}

//Инициализация конструктором перемещения
template<typename T> Meta::EnableIfTrivMovable<T> MoveInitBackwards(ArrayRange<T> dst, ArrayRange<T> src)
{
	CopyBitsBackwards(dst, src.AsConstRange());
}

//Инициализация конструктором перемещения и вызов деструкторов перемещённых элементов
template<typename T> Meta::EnableIfNotTrivRelocatable<T> MoveInitDelete(ArrayRange<T> dst, ArrayRange<T> src)
{
	INTRA_ASSERT(dst.Count()==src.Count());
	while(!dst.Empty())
	{
		new(&dst.First()) T(core::move(src.First()));
		src.First().~T();
		dst.PopFirst();
		src.PopFirst();
	}
}

//Инициализация конструктором перемещения и вызов деструкторов перемещённых элементов
template<typename T> Meta::EnableIfTrivRelocatable<T> MoveInitDelete(ArrayRange<T> dst, ArrayRange<T> src)
{
	CopyBits(dst, src.AsConstRange());
	//if(!dst.Overlaps(src)) Destruct(src); //В дебаге перезаписывает память, в релизе ничего не делает. Если диапазоны перекрываются, будут проблемы!
}

//Инициализация конструктором перемещения и вызов деструкторов перемещённых элементов
template<typename T> Meta::EnableIfNotTrivRelocatable<T> MoveInitDeleteBackwards(ArrayRange<T> dst, ArrayRange<T> src)
{
	INTRA_ASSERT(dst.Count()==src.Count());
	while(!dst.Empty())
	{
		new(&dst.Last()) T(core::move(src.Last()));
		src.Last().~T();
		dst.PopLast();
		src.PopLast();
	}
}

//Инициализация конструктором перемещения и вызов деструкторов перемещённых элементов
template<typename T> Meta::EnableIfTrivRelocatable<T> MoveInitDeleteBackwards(ArrayRange<T> dst, ArrayRange<T> src)
{
	CopyBitsBackwards(dst, src.AsConstRange());
	//if(!dst.Overlaps(src)) Destruct(src); //В дебаге перезаписывает память, в релизе ничего не делает. Если диапазоны перекрываются, будут проблемы!
}

//Перемещение оператором присваивания и вызов деструкторов перемещённых элементов
template<typename T> Meta::EnableIfNotTrivMovable<T> MoveAssignDelete(ArrayRange<T> dst, ArrayRange<T> src)
{
	INTRA_ASSERT(dst.Count()==src.Count());
	while(!dst.Empty())
	{
		dst.First() = core::move(src.First());
		src.First().~T();
		dst.PopFirst();
		src.PopFirst();
	}
}

template<typename T> Meta::EnableIfTrivMovable<T> MoveAssignDelete(ArrayRange<T> dst, ArrayRange<T> src)
{
	CopyBits(dst, src);
	if(!dst.Overlaps(src)) Destruct(src);
}

//Перемещение оператором присваивания и вызов деструкторов перемещённых элементов
template<typename T> Meta::EnableIfNotTrivMovable<T> MoveAssignDeleteBackwards(ArrayRange<T> dst, ArrayRange<T> src)
{
	INTRA_ASSERT(dst.Count()==src.Count());
	while(!dst.Empty())
	{
		dst.Last() = core::move(src.Last());
		src.Last().~T();
		dst.PopLast();
		src.PopLast();
	}
}

template<typename T> Meta::EnableIfTrivMovable<T> MoveAssignDeleteBackwards(ArrayRange<T> dst, ArrayRange<T> src)
{
	CopyBitsBackwards(dst, src);
	//if(!dst.Overlaps(src)) Destruct(src);
}




template<typename T, typename Allocator> ArrayRange<T> AllocateRangeUninitialized(
	Allocator& allocator, size_t& count, const SourceInfo& sourceInfo)
{
	(void)allocator; //Чтобы устранить ложное предупреждение MSVC
	size_t size = count*sizeof(T);
	auto result = (T*)allocator.Allocate(size, sourceInfo);
	count = size/sizeof(T);
	return ArrayRange<T>(result, count);
}

template<typename T, typename Allocator> ArrayRange<T> AllocateRange(
	Allocator& allocator, size_t& count, const SourceInfo& sourceInfo)
{
	auto result = AllocateRangeUninitialized(allocator, count, sourceInfo);
	Memory::Initialize(result);
	return result;
}

template<typename T, typename Allocator> void FreeRangeUninitialized(Allocator& allocator, ArrayRange<T> range)
{
	if(range==null) return;
	allocator.Free(range.Begin, range.Length());
}

template<typename T, typename Allocator> void FreeRange(Allocator& allocator, ArrayRange<T> range)
{
	Memory::Destruct(range);
	FreeRangeUninitialized(allocator, range);
}



}}


