#pragma once

#include "Intra/Range/Concepts.h"
#include "Intra/Range/Operations.h"
#include "Intra/Range/Take.h"

//TODO: delete this (superceded by CopyTo(dst).From(TakeUntil(src)))

//! @file
//! Данный файл содержит алгоритмы копирования данных диапазонов до тех пор,
//! пока не выполнен предикат или не достигнут конец одного из диапазонов.
/**
Существует 4 версии алгоритма, которые отличаются своим действием на аргументы:
1) ReadWriteUntil: устанавливает src и dst в ту позицию, на которой остановилось копирование.
2) ReadToUntil: аналогично п.1 устанавливает только src, а на dst не влияет.
3) WriteToUntil: аналогично п.1 устанавливает только dst, а на src не влияет.
4) CopyToUntil: не изменяет передаваемые аргументы.

Все функции возвращают количество скопированных элементов.
Копирование перекрывающихся диапазонов будет корректным только тогда,
когда начало диапазона src расположено после начала диапазона dst.

@sa Copy
**/

INTRA_BEGIN
INTRA_IGNORE_WARNING_SIGN_CONVERSION
INTRA_IGNORE_WARNING_LOSING_CONVERSION
enum class StopReason {SourceEmpty, DestFull, Found};
struct FindResult
{
	index_t ElementsRead = 0;
	StopReason StopReason = StopReason::SourceEmpty;

};

template<class R, class OR, typename P> constexpr Requires<
	CInputRange<R> && !CConst<R> &&
	COutputRangeOf<OR, TValueTypeOf<R>>,
FindResult> ReadWriteUntil(R& src, OR& dst, P&& predOrStopValue)
{
	FindResult res;
	for(;;)
	{
		if(src.Empty()) break;
		if constexpr(CHasEmpty<OR> && !CInfiniteRange<OR>)
			if(dst.Empty())
			{
				res.StopReason = StopReason::DestFull;
				break;
			}
		auto&& srcFirst = src.First();
		bool found = false;
		if constexpr(CCallable<P, TValueTypeOf<R>>) found = predOrStopValue(srcFirst);
		else found = srcFirst == predOrStopValue;
		if(found)
		{
			res.StopReason = StopReason::Found;
			break;
		}
		dst.Put(srcFirst);
		src.PopFirst();
		res.ElementsRead++;
	}
	return res;
}

template<class R, class OR, typename X> constexpr Requires<
	CInputRange<R> && !CConst<R> &&
	CAsOutputRangeOf<TRemoveConstRef<OR>, TValueTypeOf<R>>,
FindResult> ReadToUntil(R& src, OR&& dst, const X& x)
{
	auto dstRange = ForwardAsRange<OR>(dst);
	return ReadWriteUntil(src, dstRange, x);
}

template<class R, class OR, typename X> constexpr Requires<
	CInputRange<R> && !CConst<R> &&
	CAsOutputRangeOf<TRemoveConstRef<OR>, TValueTypeOf<R>>,
FindResult> WriteToUntil(R&& src, OR& dst, const X& x)
{
	auto srcRange = ForwardAsRange<R>(src);
	return ReadWriteUntil(srcRange, dst, x);
}

template<class R, class OR, typename X> constexpr Requires<
	CInputRange<R> && !CConst<R> &&
	CAsOutputRangeOf<TRemoveConstRef<OR>, TValueTypeOf<R>>,
FindResult> CopyToUntil(R&& src, OR& dst, const X& x)
{
	auto srcRange = ForwardAsRange<R>(src);
	return ReadToUntil(srcRange, dst, x);
}
INTRA_END
