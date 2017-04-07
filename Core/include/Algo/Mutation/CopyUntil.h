#pragma once

#include "Range/Concepts.h"
#include "Range/AsRange.h"
#include "Range/Operations.h"
#include "Range/Decorators/Take.h"
#include "Platform/CppWarnings.h"
#include "Algo/Op.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_SIGN_CONVERSION
INTRA_WARNING_DISABLE_LOSING_CONVERSION

//! @file
//! Данный файл содержит алгоритмы копирования данных диапазонов до тех пор,
//! пока не выполнен предикат или не достигнут конец одного из диапазонов.
/**
Существует 4 версии алгоритма, которые отличаются своим действием на аргументы:
1) CopyAdvanceToAdvanceUntil: устанавливает src и dst в ту позицию, на которой остановилось копирование.
2) CopyAdvanceToUntil: аналогично п.1 устанавливает только src, а на dst не влияет.
3) CopyToAdvanceUntil: аналогично п.1 устанавливает только dst, а на src не влияет.
4) CopyToUntil: не изменяет передаваемые аргументы.

Все функции возвращают количество скопированных элементов.
Копирование перекрывающихся диапазонов будет корректным только тогда,
когда начало диапазона src расположено после начала диапазона dst.
**/

namespace Intra { namespace Algo {

using namespace Range::Concepts;

template<typename R, typename OR, typename P> Meta::EnableIf<
	IsInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	IsOutputRangeOf<OR, ValueTypeOf<R>>::_ &&
	HasEmpty<OR>::_ && !IsInfiniteRange<OR>::_ &&
	Meta::IsCallable<P, ValueTypeOf<R>>::_,
size_t> CopyAdvanceToAdvanceUntil(R& src, OR& dst, P&& pred)
{
	size_t elementsCopied = 0;
	while(!src.Empty() && !dst.Empty() && !pred(src.First()))
	{
		dst.Put(src.First());
		src.PopFirst();
		elementsCopied++;
	}
	return elementsCopied;
}

template<typename R, typename OR, typename P> Meta::EnableIf<
	IsInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	IsOutputRangeOf<OR, ValueTypeOf<R>>::_ &&
	(!HasEmpty<OR>::_ || IsInfiniteRange<OR>::_) &&
	Meta::IsCallable<P, ValueTypeOf<R>>::_,
size_t> CopyAdvanceToAdvanceUntil(R& src, OR& dst, P&& pred)
{
	size_t elementsCopied = 0;
	while(!src.Empty() && !pred(src.First()))
	{
		dst.Put(src.First());
		src.PopFirst();
		elementsCopied++;
	}
	return elementsCopied;
}

template<typename R, typename OR, typename X> Meta::EnableIf<
	IsInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	IsOutputRangeOf<OR, ValueTypeOf<R>>::_ &&
	HasEmpty<OR>::_ && !IsInfiniteRange<OR>::_ &&
	!Meta::IsCallable<X, ValueTypeOf<R>>::_,
size_t> CopyAdvanceToAdvanceUntil(R& src, OR& dst, const X& stopValue)
{
	size_t elementsCopied = 0;
	while(!src.Empty() && !dst.Empty() && src.First() != stopValue)
	{
		dst.Put(src.First());
		src.PopFirst();
		elementsCopied++;
	}
	return elementsCopied;
}

template<typename R, typename OR, typename X> Meta::EnableIf<
	IsInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	IsOutputRangeOf<OR, ValueTypeOf<R>>::_ &&
	(!HasEmpty<OR>::_ || IsInfiniteRange<OR>::_) &&
	!Meta::IsCallable<X, ValueTypeOf<R>>::_,
size_t> CopyAdvanceToAdvanceUntil(R& src, OR& dst, const X& stopValue)
{
	size_t elementsCopied = 0;
	while(!src.Empty() && src.First() != stopValue)
	{
		dst.Put(src.First());
		src.PopFirst();
		elementsCopied++;
	}
	return elementsCopied;
}

template<typename R, typename OR, typename X> forceinline Meta::EnableIf<
	IsInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	IsAsOutputRangeOf<Meta::RemoveConstRef<OR>, ValueTypeOf<R>>::_,
size_t> CopyAdvanceToUntil(R& src, OR&& dst, const X& x)
{
	auto dstRange = Range::Forward<OR>(dst);
	return Algo::CopyAdvanceToAdvanceUntil(src, dstRange, x);
}

template<typename R, typename OR, typename X> forceinline Meta::EnableIf<
	IsInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	IsAsOutputRangeOf<Meta::RemoveConstRef<OR>, ValueTypeOf<R>>::_,
size_t> CopyToAdvanceUntil(R&& src, OR& dst, const X& x)
{
	auto srcRange = Range::Forward<R>(src);
	return Algo::CopyAdvanceToAdvanceUntil(srcRange, dst, x);
}

template<typename R, typename OR, typename X> forceinline Meta::EnableIf<
	IsInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	IsAsOutputRangeOf<Meta::RemoveConstRef<OR>, ValueTypeOf<R>>::_,
size_t> CopyToUntil(R&& src, OR& dst, const X& x)
{
	auto srcRange = Range::Forward<R>(src);
	return Algo::CopyAdvanceToUntil(srcRange, dst, x);
}

}}

INTRA_WARNING_POP
