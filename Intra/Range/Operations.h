#pragma once

#include "Meta/Type.h"
#include "Cpp/Features.h"
#include "Cpp/Warnings.h"
#include "Concepts/Range.h"
#include "Concepts/RangeOf.h"
#include "Utils/Debug.h"

namespace Intra { namespace Range {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename R> forceinline Meta::EnableIf<
	Concepts::HasLength<R>::_,
size_t> Count(const R& range) {return range.Length();}

template<typename R> Meta::EnableIf<
	Concepts::IsFiniteForwardRange<R>::_ &&
	!Meta::IsConst<R>::_,
size_t> CountAdvance(R& range)
{
	size_t result=0;
	while(!range.Empty()) range.PopFirst(), result++;
	return result;
} 

template<typename R> forceinline Meta::EnableIf<
	!Concepts::HasLength<R>::_ &&
	Concepts::IsAsConsumableRange<R>::_,
size_t> Count(R&& range)
{
	auto rangeCopy = Range::Forward<R>(range);
	return CountAdvance(rangeCopy);
}


template<typename R> forceinline Meta::EnableIf<
	Concepts::HasLength<Concepts::RangeOfType<R>>::_,
size_t> LengthOr0(R&& range) {return Range::Forward<R>(range).Length();}


template<typename R> forceinline Meta::EnableIf<
	!Concepts::HasLength<Concepts::RangeOfType<R>>::_,
size_t> LengthOr0(R&& range) {(void)range; return 0;}


template<typename R> forceinline Meta::EnableIf<
	Concepts::HasEmpty<Concepts::RangeOfType<R>>::_,
bool> EmptyOrFalse(R&& range) {return Range::Forward<R>(range).Empty();}


template<typename R> forceinline Meta::EnableIf<
	!Concepts::HasEmpty<Concepts::RangeOfType<R>>::_,
bool> EmptyOrFalse(R&& range) {(void)range; return false;}

template<typename R> forceinline Meta::EnableIf<
	Concepts::HasFull<Concepts::RangeOfType<R>>::_,
bool> FullOrFalse(R&& range) {return Range::Forward<R>(range).Full();}


template<typename R> forceinline Meta::EnableIf<
	!Concepts::HasFull<Concepts::RangeOfType<R>>::_,
bool> FullOrFalse(R&& range) {(void)range; return false;}



template<typename R> forceinline Meta::EnableIf<
	Concepts::HasSlicing<R>::_ &&
	Concepts::HasLength<R>::_ &&
	!Meta::IsConst<R>::_ &&
	!Concepts::HasPopFirstN<R>::_
> PopFirstExactly(R& range, size_t elementsToPop)
{range = range(elementsToPop, range.Length());}

template<typename R> forceinline Meta::EnableIf<
	Concepts::IsInputRange<R>::_ &&
	(!Concepts::HasSlicing<R>::_ ||
		!Concepts::HasLength<R>::_) &&
	!Meta::IsConst<R>::_ &&
	!Concepts::HasPopFirstN<R>::_
> PopFirstExactly(R& range, size_t elementsToPop)
{while(elementsToPop --> 0) range.PopFirst();}

template<typename R> forceinline Meta::EnableIf<
	Concepts::HasPopFirstN<R>::_
> PopFirstExactly(R& range, size_t elementsToPop)
{
	size_t poppedElements = range.PopFirstN(elementsToPop);
	INTRA_DEBUG_ASSERT(poppedElements == elementsToPop);
	(void)poppedElements;
}


template<typename R> Meta::EnableIf<
	!Meta::IsConst<R>::_ &&
	!Concepts::HasPopFirstN<R>::_ &&
	Concepts::IsFiniteInputRange<R>::_ &&
	Concepts::HasSlicing<R>::_,
size_t> PopFirstN(R& range, size_t n)
{
	const size_t l = Count(range);
	const size_t elementsToPop = n<l? n: l;
	range = range(elementsToPop, l);
	return elementsToPop;
}

template<typename R> Meta::EnableIf<
	Concepts::IsInputRange<R>::_ &&
	!Meta::IsConst<R>::_ &&
	!(Concepts::HasSlicing<R>::_ &&
		Concepts::IsFiniteRange<R>::_) &&
	!Concepts::HasPopFirstN<R>::_,
size_t> PopFirstN(R& range, size_t n)
{
	size_t elementsToPop = 0;
	while(!range.Empty() && n!=0)
		range.PopFirst(), n--, elementsToPop++;
	return elementsToPop;
}

template<typename R> forceinline Meta::EnableIf<
	Concepts::HasPopFirstN<R>::_,
size_t> PopFirstN(R& range, size_t n)
{return range.PopFirstN(n);}


template<typename R> forceinline Meta::EnableIf<
	Concepts::IsInputRange<R>::_ &&
	!Meta::IsConst<R>::_ &&
	Concepts::HasSlicing<R>::_ &&
	!Concepts::HasPopLastN<R>::_
> PopLastExactly(R& range, size_t elementsToPop)
{range = range(0, range.Length()-elementsToPop);}

template<typename R> forceinline Meta::EnableIf<
	Concepts::IsBidirectionalRange<R>::_ &&
	!Meta::IsConst<R>::_ &&
	!Concepts::HasSlicing<R>::_ &&
	!Concepts::HasPopLastN<R>::_
> PopLastExactly(R& range, size_t elementsToPop)
{while(elementsToPop --> 0) range.PopLast();}

template<typename R> forceinline Meta::EnableIf<
	Concepts::HasPopLastN<R>::_
> PopLastExactly(R& range, size_t elementsToPop)
{
	size_t poppedElements = range.PopLastN(elementsToPop);
	INTRA_DEBUG_ASSERT(poppedElements==elementsToPop);
	(void)poppedElements;
}


template<typename R> Meta::EnableIf<
	Concepts::IsInputRange<R>::_ &&
	!Meta::IsConst<R>::_ &&
	Concepts::HasSlicing<R>::_ &&
	!Concepts::HasPopLastN<R>::_,
size_t> PopLastN(R& range, size_t n)
{
	const size_t l = Count(range);
	const size_t elementsToPop = n<l? n: l;
	range = range(0, l-elementsToPop);
	return elementsToPop;
}

template<typename R> Meta::EnableIf<
	Concepts::IsBidirectionalRange<R>::_ &&
	!Meta::IsConst<R>::_ &&
	!Concepts::HasSlicing<R>::_ &&
	!Concepts::HasPopLastN<R>::_,
size_t> PopLastN(R& range, size_t n)
{
	size_t elementsToPop = 0;
	while(!range.Empty() && n!=0)
		range.PopLast(), n--, elementsToPop++;
	return elementsToPop;
}

template<typename R> forceinline Meta::EnableIf<
	Concepts::HasPopLastN<R>::_,
size_t> PopLastN(R& range, size_t n)
{return range.PopLastN(n);}

//! Удаляет из диапазона все элементы кроме последних n элементов.
//! \return Возвращает ссылку на себя.
template<typename R> Meta::EnableIf<
	Concepts::IsForwardRange<R>::_ &&
	!Concepts::HasLength<R>::_,
R&&> TailAdvance(R&& range, size_t n)
{
	R temp = range;
	PopFirstN(temp, n);
	while(!temp.Empty())
	{
		temp.PopFirst();
		range.PopFirst();
	}
	return Cpp::Forward<R>(range);
}

template<typename R> forceinline Meta::EnableIf<
	Concepts::IsInputRange<R>::_ &&
	Concepts::HasLength<R>::_,
R&&> TailAdvance(R&& range, size_t n)
{
	if(range.Length()>n) PopFirstExactly(range, range.Length()-n);
	return Cpp::Forward<R>(range);
}

//! Возвращает диапазон, содержащий последние n элементов данного диапазона.
template<typename R> forceinline Meta::EnableIf<
	Concepts::IsAsNonInfiniteForwardRange<R>::_ &&
	!Concepts::HasSlicing<R>::_,
Meta::RemoveConstRef<Concepts::RangeOfType<R>>> Tail(R&& range, size_t n)
{
	auto rangeCopy = Range::Forward<R>(range);
	return TailAdvance(rangeCopy, n);
}

template<typename R> forceinline Meta::EnableIf<
	Concepts::IsInputRange<R>::_ &&
	Concepts::HasSlicing<R>::_,
R> Tail(const R& range, size_t n)
{
	size_t len = range.Length();
	return range(len>n? len-n: 0, len);
}



template<typename R> forceinline Meta::EnableIf<
	Concepts::IsAsAccessibleRange<R>::_,
Concepts::RangeOfTypeNoCRef<R>> Drop(R&& range)
{
	if(range.Empty()) return Range::Forward<R>(range);
	auto result = Range::Forward<R>(range);
	result.PopFirst();
	return result;
}

template<typename R> forceinline Meta::EnableIf<
	Concepts::IsAsAccessibleRange<R>::_,
Concepts::RangeOfTypeNoCRef<R>> Drop(R&& range, size_t n)
{
	auto result = Range::Forward<R>(range);
	PopFirstN(result, n);
	return result;
}

template<typename R> forceinline Meta::EnableIf<
	Concepts::IsAsAccessibleRange<R>::_,
Concepts::RangeOfTypeNoCRef<R>> DropExactly(R&& range)
{
	auto result = Range::Forward<R>(range);
	result.PopFirst();
	return result;
}

template<typename R> forceinline Meta::EnableIf<
	Concepts::IsAsAccessibleRange<R>::_,
Concepts::RangeOfTypeNoCRef<R>> DropExactly(R&& range, size_t n)
{
	auto result = Range::Forward<R>(range);
	PopFirstExactly(result, n);
	return result;
}

template<typename R,
	typename AsR = Concepts::RangeOfType<R>
> forceinline Meta::EnableIf<
	Concepts::IsInputRange<AsR>::_ &&
	Concepts::HasIndex<AsR>::_,
Concepts::ReturnValueTypeOf<AsR>> AtIndex(R&& range, size_t index)
{return Range::Forward<R>(range)[index];}

template<typename R,
	typename AsR = Concepts::RangeOfType<R>
> forceinline Meta::EnableIf<
	Concepts::IsAccessibleRange<AsR>::_ &&
	!Concepts::HasIndex<AsR>::_,
Concepts::ReturnValueTypeOf<AsR>> AtIndex(R&& range, size_t index)
{return DropExactly(Range::Forward<R>(range), index).First();}


template<typename R,
	typename AsR = Concepts::RangeOfTypeNoCRef<R>
> forceinline Meta::EnableIf<
	Concepts::IsBidirectionalRange<AsR>::_,
AsR> DropLast(R&& range)
{
	if(range.Empty()) return Range::Forward<R>(range);
	auto result = Range::Forward<R>(range);
	result.PopLast();
	return result;
}

template<typename R,
	typename AsR = Concepts::RangeOfTypeNoCRef<R>
> forceinline Meta::EnableIf<
	Concepts::IsBidirectionalRange<AsR>::_,
AsR> DropLast(R&& range, size_t n)
{
	auto result = Range::Forward<R>(range);
	PopLastN(result, n);
	return result;
}

template<typename R,
	typename AsR = Concepts::RangeOfTypeNoCRef<R>
> forceinline Meta::EnableIf<
	Concepts::IsBidirectionalRange<AsR>::_,
AsR> DropLastExactly(R&& range)
{
	auto result = Range::Forward<R>(range);
	result.PopLast();
	return result;
}

template<typename R,
	typename AsR = Concepts::RangeOfTypeNoCRef<R>
> forceinline Meta::EnableIf<
	Concepts::IsBidirectionalRange<AsR>::_,
AsR> DropLastExactly(R&& range, size_t n)
{
	auto result = Range::Forward<R>(range);
	PopLastExactly(result, n);
	return result;
}

//! Оператор == для сравнения с null для диапазонов эквивалентен вызову Empty()
template<typename R> forceinline Meta::EnableIf<
	Concepts::IsInputRange<R>::_,
bool> operator==(const R& range, null_t) {return range.Empty();}

//! Оператор == для сравнения с null для диапазонов эквивалентен вызову Empty()
template<typename R> forceinline Meta::EnableIf<
	Concepts::IsInputRange<R>::_,
bool> operator==(null_t, const R& range) {return range.Empty();}

//! Оператор != для сравнения с null для диапазонов эквивалентен вызову !Empty()
template<typename R> forceinline Meta::EnableIf<
	Concepts::IsInputRange<R>::_,
bool> operator!=(const R& range, null_t) {return !range.Empty();}

//! Оператор != для сравнения с null для диапазонов эквивалентен вызову !Empty()
template<typename R> forceinline Meta::EnableIf<
	Concepts::IsInputRange<R>::_,
bool> operator!=(null_t, const R& range) {return !range.Empty();}

INTRA_WARNING_POP

}}
