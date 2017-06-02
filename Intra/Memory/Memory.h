#pragma once

#include "Cpp/Fundamental.h"
#include "Cpp/Features.h"
#include "Cpp/Warnings.h"
#include "Cpp/Intrinsics.h"
#include "Cpp/PlacementNew.h"

#include "Meta/Type.h"
#include "Concepts/Range.h"

#include "Utils/Span.h"
#include "Utils/Debug.h"


namespace Intra { namespace Memory {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

//Вызов конструкторов
template<typename T> Meta::EnableIf<
	!Meta::IsTriviallyConstructible<T>::_
> Initialize(Span<T> dst)
{
	while(!dst.Empty())
	{
		new(&dst.First()) T;
		dst.PopFirst();
	}
}

//Для POD типов просто зануление памяти
template<typename T> forceinline Meta::EnableIf<
	Meta::IsTriviallyConstructible<T>::_
> Initialize(Span<T> dst)
{
	if(dst.Empty()) return;
	C::memset(dst.Begin, 0, dst.Length()*sizeof(T));
}


//Вызов конструктора
template<typename T> forceinline Meta::EnableIf<
	!Meta::IsTriviallyConstructible<T>::_
> InitializeObj(T& dst)
{new(&dst) T;}

//Для POD типов просто зануление памяти
template<typename T> forceinline Meta::EnableIf<
	Meta::IsTriviallyConstructible<T>::_
> InitializeObj(T& dst)
{C::memset(&dst, 0, sizeof(T));}

//Вызов деструкторов
template<typename T> Meta::EnableIf<
	!Meta::IsTriviallyDestructible<T>::_
> Destruct(Span<T> dst)
{
	while(!dst.Empty())
	{
		dst.First().~T();
		dst.PopFirst();
	}
}

template<typename T> forceinline Meta::EnableIf<
	Meta::IsTriviallyDestructible<T>::_
> Destruct(Span<T> dst)
{
#ifdef INTRA_DEBUG
	if(dst.Empty()) return;
	C::memset(dst.Data(), 0xDE, dst.Length()*sizeof(dst.First()));
#endif
	(void)dst;
}

//Вызов деструктора
template<typename T> forceinline Meta::EnableIf<
	!Meta::IsTriviallyDestructible<T>::_
> DestructObj(T& dst)
{dst.~T();}

template<typename T> Meta::EnableIf<
	Meta::IsTriviallyDestructible<T>::_
> DestructObj(T& dst)
{
#ifdef INTRA_DEBUG
	C::memset(&dst, 0xDE, sizeof(T));
#endif
	(void)dst;
}

//Побитовое копирование
template<typename T> void CopyBits(Span<T> dst, CSpan<T> src)
{
	INTRA_DEBUG_ASSERT(dst.Length()>=src.Length());
	C::memmove(dst.Begin, src.Begin, src.Length()*sizeof(T));
	//C::memcpy(dst.Begin, src.Begin, src.Length()*sizeof(T));
}

template<typename T> void CopyBits(T* dst, const T* src, size_t count)
{
	CopyBits(Span<T>(dst, count), CSpan<T>(src, count));
}

template<typename T> void CopyBitsBackwards(Span<T> dst, CSpan<T> src)
{
	INTRA_DEBUG_ASSERT(dst.Length()>=src.Length());
	C::memmove(dst.Begin, src.Begin, src.Length()*sizeof(T));
}

template<typename T> void CopyBitsBackwards(T* dst, const T* src, size_t count)
{
	CopyBitsBackwards(Span<T>(dst, count), CSpan<T>(src, count));
}

template<typename T> void CopyObjectBits(T& dst, const T& src)
{
	CopyBits<T>({&dst, 1}, {&src, 1});
}

//Копирование оператором присваивания
template<typename OR, typename R> Meta::EnableIf<
	Concepts::IsAssignableRange<OR>::_ &&
	Concepts::IsForwardRange<R>::_ &&
	!Meta::IsTriviallyCopyAssignable<Concepts::ValueTypeOf<OR>>::_
> CopyAssign(const OR& dst, const R& src)
{
	auto dstCopy = dst;
	auto srcCopy = src;
	INTRA_DEBUG_ASSERT(Range::Count(dst) <= Range::Count(src));
	while(!dstCopy.Empty())
	{
		dstCopy.First() = srcCopy.First();
		dstCopy.PopFirst();
		srcCopy.PopFirst();
	}
}

template<typename OR, typename R> Meta::EnableIf<
	Concepts::IsArrayClass<OR>::_ &&
	Concepts::IsArrayClass<R>::_ &&
	Meta::IsTriviallyCopyAssignable<Concepts::ElementTypeOfArray<OR>>::_ &&
	Concepts::IsFiniteInputRangeOfExactly<R, Concepts::ElementTypeOfArray<OR>>::_
> CopyAssign(const OR& dst, const R& src)
{
	INTRA_DEBUG_ASSERT(dst.Length()==src.Length());
	CopyBits(dst.Data(), src.Data(), dst.Length());
}

//Копирование оператором присваивания
template<typename T> Meta::EnableIf<
	!Meta::IsTriviallyCopyAssignable<T>::_
> CopyAssignBackwards(Span<T> dst, CSpan<T> src)
{
	INTRA_DEBUG_ASSERT(dst.Length() <= src.Length());
	while(!dst.Empty())
	{
		dst.Last() = src.Last();
		dst.PopLast();
		src.PopLast();
	}
}

template<typename T> Meta::EnableIf<
	Meta::IsTriviallyCopyAssignable<T>::_
> CopyAssignBackwards(Span<T> dst, CSpan<T> src)
{
	CopyBitsBackwards(dst, src);
}

//Удаление деструктором и копирование конструктором копирования
template<typename T> Meta::EnableIf<
	!Meta::IsTriviallyCopyable<T>::_ ||
	!Meta::IsTriviallyDestructible<T>::_
> CopyRecreate(Span<T> dst, CSpan<T> src)
{
	INTRA_DEBUG_ASSERT(dst.Length()==src.Length());
	while(!dst.Empty())
	{
		dst.First().~T();
		new(&dst.First()) T(src.First());
		src.PopFirst();
	}
}

template<typename T> Meta::EnableIf<
	Meta::IsTriviallyCopyable<T>::_ &&
	Meta::IsTriviallyDestructible<T>::_
> CopyRecreate(Span<T> dst, CSpan<T> src)
{
	CopyBits(dst, src);
}

//Удаление деструктором и копирование конструктором копирования
template<typename T> Meta::EnableIf<
	!Meta::IsTriviallyCopyable<T>::_ ||
	!Meta::IsTriviallyDestructible<T>::_
> CopyRecreateBackwards(Span<T> dst, CSpan<T> src)
{
	INTRA_DEBUG_ASSERT(dst.Length()==src.Length());
	while(!dst.Empty())
	{
		dst.Last().~T();
		new(&dst.Last()) T(src.Last());
		src.PopLast();
	}
}

template<typename T> Meta::EnableIf<
	Meta::IsTriviallyCopyable<T>::_ &&
	Meta::IsTriviallyDestructible<T>::_
> CopyRecreateBackwards(Span<T> dst, CSpan<T> src)
{
	CopyBitsBackwards(dst, src);
}

template<typename T, typename U> Meta::EnableIf<
	!Meta::IsTriviallyCopyable<T>::_
> CopyInit(Span<T> dst, CSpan<U> src)
{
	INTRA_DEBUG_ASSERT(dst.Length()==src.Length());
	while(!src.Empty())
	{
		new(&dst.First()) T(src.First());
		dst.PopFirst();
		src.PopFirst();
	}
}

template<typename T> Meta::EnableIf<
	Meta::IsTriviallyCopyable<T>::_
> CopyInit(Span<T> dst, CSpan<T> src)
{
	CopyBits(dst, src);
}

template<typename T, typename U> Meta::EnableIf<
	!Meta::IsTriviallyCopyable<T>::_
> CopyInitBackwards(Span<T> dst, CSpan<U> src)
{
	INTRA_DEBUG_ASSERT(dst.Length()==src.Length());
	while(!src.Empty())
	{
		new(&dst.Last()) T(src.Last());
		dst.PopLast();
		src.PopLast();
	}
}

template<typename T> Meta::EnableIf<
	Meta::IsTriviallyCopyable<T>::_
> CopyInitBackwards(Span<T> dst, CSpan<T> src)
{
	CopyBitsBackwards(dst, src);
}

//Инициализация конструктором перемещения
template<typename T> Meta::EnableIf<
	!Meta::IsTriviallyMovable<T>::_
> MoveInit(Span<T> dst, Span<T> src)
{
	INTRA_DEBUG_ASSERT(dst.Length()==src.Length());
	while(!dst.Empty())
	{
		new(&dst.First()) T(Cpp::Move(src.First()));
		dst.PopFirst();
		src.PopFirst();
	}
}

//Инициализация конструктором перемещения
template<typename T> Meta::EnableIf<
	Meta::IsTriviallyMovable<T>::_
> MoveInit(Span<T> dst, Span<T> src)
{
	CopyBits(dst, src.AsConstRange());
}

//Инициализация конструктором перемещения
template<typename T> Meta::EnableIf<
	!Meta::IsTriviallyCopyable<T>::_
> MoveInitBackwards(Span<T> dst, Span<T> src)
{
	INTRA_DEBUG_ASSERT(dst.Length()==src.Length());
	while(!dst.Empty())
	{
		new(&dst.Last()) T(Cpp::Move(src.Last()));
		dst.PopLast();
		src.PopLast();
	}
}

//Инициализация конструктором перемещения
template<typename T> Meta::EnableIf<
	Meta::IsTriviallyCopyable<T>::_
> MoveInitBackwards(Span<T> dst, Span<T> src)
{CopyBitsBackwards(dst, src.AsConstRange());}

//Инициализация конструктором перемещения и вызов деструкторов перемещённых элементов
template<typename T> Meta::EnableIf<
	!Meta::IsTriviallyRelocatable<T>::_
> MoveInitDelete(Span<T> dst, Span<T> src)
{
	INTRA_DEBUG_ASSERT(dst.Length()==src.Length());
	while(!dst.Empty())
	{
		new(&dst.First()) T(Cpp::Move(src.First()));
		src.First().~T();
		dst.PopFirst();
		src.PopFirst();
	}
}

//Инициализация конструктором перемещения и вызов деструкторов перемещённых элементов
template<typename T> Meta::EnableIf<
	Meta::IsTriviallyRelocatable<T>::_
> MoveInitDelete(Span<T> dst, Span<T> src)
{
	CopyBits(dst, src.AsConstRange());
	//if(!dst.Overlaps(src)) Destruct(src); //В дебаге перезаписывает память, в релизе ничего не делает. Если диапазоны перекрываются, будут проблемы!
}

//Инициализация конструктором перемещения и вызов деструкторов перемещённых элементов
template<typename T> Meta::EnableIf<
	!Meta::IsTriviallyRelocatable<T>::_
> MoveInitDeleteBackwards(Span<T> dst, Span<T> src)
{
	INTRA_DEBUG_ASSERT(dst.Length()==src.Length());
	while(!dst.Empty())
	{
		new(&dst.Last()) T(Cpp::Move(src.Last()));
		src.Last().~T();
		dst.PopLast();
		src.PopLast();
	}
}

//Инициализация конструктором перемещения и вызов деструкторов перемещённых элементов
template<typename T> Meta::EnableIf<
	Meta::IsTriviallyRelocatable<T>::_
> MoveInitDeleteBackwards(Span<T> dst, Span<T> src)
{
	CopyBitsBackwards(dst, src.AsConstRange());
	//if(!dst.Overlaps(src)) Destruct(src); //В дебаге перезаписывает память, в релизе ничего не делает. Если диапазоны перекрываются, будут проблемы!
}

//Перемещение оператором присваивания и вызов деструкторов перемещённых элементов
template<typename T> Meta::EnableIf<
	!Meta::IsTriviallyMovable<T>::_
> MoveAssignDelete(Span<T> dst, Span<T> src)
{
	INTRA_DEBUG_ASSERT(dst.Length()==src.Length());
	while(!dst.Empty())
	{
		dst.First() = Cpp::Move(src.First());
		src.First().~T();
		dst.PopFirst();
		src.PopFirst();
	}
}

template<typename T> Meta::EnableIf<
	Meta::IsTriviallyMovable<T>::_
> MoveAssignDelete(Span<T> dst, Span<T> src)
{
	CopyBits(dst, src);
	if(!dst.Overlaps(src)) Destruct(src);
}

//Перемещение оператором присваивания и вызов деструкторов перемещённых элементов
template<typename T> Meta::EnableIf<
	!Meta::IsTriviallyMovable<T>::_
> MoveAssignDeleteBackwards(Span<T> dst, Span<T> src)
{
	INTRA_DEBUG_ASSERT(dst.Length()==src.Length());
	while(!dst.Empty())
	{
		dst.Last() = Cpp::Move(src.Last());
		src.Last().~T();
		dst.PopLast();
		src.PopLast();
	}
}

template<typename T> Meta::EnableIf<
	Meta::IsTriviallyMovable<T>::_
> MoveAssignDeleteBackwards(Span<T> dst, Span<T> src)
{
	CopyBitsBackwards(dst, src);
	//if(!dst.Overlaps(src)) Destruct(src);
}




template<typename T, typename Allocator> Span<T> AllocateRangeUninitialized(
	Allocator& allocator, size_t& count, const Utils::SourceInfo& sourceInfo)
{
	(void)allocator; //Чтобы устранить ложное предупреждение MSVC
	size_t size = count*sizeof(T);
	T* const result = allocator.Allocate(size, sourceInfo);
	count = size/sizeof(T);
	return Span<T>(result, count);
}

template<typename T, typename Allocator> Span<T> AllocateRange(
	Allocator& allocator, size_t& count, const Utils::SourceInfo& sourceInfo)
{
	auto result = AllocateRangeUninitialized(allocator, count, sourceInfo);
	Memory::Initialize(result);
	return result;
}

template<typename T, typename Allocator> void FreeRangeUninitialized(Allocator& allocator, Span<T> range)
{
	(void)allocator;
	if(range==null) return;
	allocator.Free(range.Begin, range.Length()*sizeof(T));
}

template<typename T, typename Allocator> void FreeRange(Allocator& allocator, Span<T> range)
{
	Memory::Destruct(range);
	FreeRangeUninitialized(allocator, range);
}

INTRA_WARNING_POP

}}
