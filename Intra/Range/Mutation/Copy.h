#pragma once

#include "Cpp/Warnings.h"
#include "Cpp/Intrinsics.h"

#include "Funal/Op.h"

#include "Concepts/Array.h"
#include "Concepts/Range.h"
#include "Concepts/RangeOf.h"

#include "Range/Operations.h"
#include "Range/Decorators/Take.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_SIGN_CONVERSION
INTRA_WARNING_DISABLE_LOSING_CONVERSION

namespace Intra { namespace Range {

//! @file
//! Данный файл содержит алгоритмы копирования содержимого диапазонов.
/**
Существует три варианта копирования:
1) Копирование до тех пор, пока источник или приёмник не закончатся.
 Таким образом, для правильно спроектированных диапазонов ошибка переполнения невозможна.
2) Аналогично п.1, но при достижении n скопированных элементов копирование останавливается.
3) Аналогично п.1, но элементы источника, не удовлетворяющие предикату pred, пропускаются.

Для каждого из этих вариантов существует по 4 версии алгоритма, которые отличаются своим действием на аргументы:
1) ReadWrite: устанавливает src и dst в ту позицию, на которой остановилось копирование.
Интерпретация названия:
Первый аргумент можно рассматривать как поток ввода, из которого идёт чтение (Read) элементов.
Второй аргумент можно рассматривать как поток вывода, в который записываются (Write) прочитанные элементы.

2) ReadTo: аналогично п.1 устанавливает только src, а на dst не влияет.
Интерпретация названия:
Первый аргумент можно рассматривать как поток ввода, из которого идёт чтение (Read) элементов.
Второй аргумент потоком не является, поэтому его позиция не изменяется.

3) WriteTo: аналогично п.1 устанавливает только dst, а на src не влияет.
Интерпретация названия:
Второй аргумент можно представить как поток вывода, в который записываются элементы из первого диапазона.
Первый аргумент не является потоком - его позиция не изменяется.

4) CopyTo: не изменяет передаваемые аргументы.
Интерпретация названия:
Аргументы не являются потоками, их позиция не изменяется.

Все функции возвращают количество скопированных элементов.
Копирование перекрывающихся диапазонов будет корректным только тогда,
когда начало диапазона src расположено после начала диапазона dst,
либо src расположено до начала dst, но оба аргумента являются массивами одинакового тривиально копируемого типа.
**/


//! Неоптимизированная версия алгоритма копирования. Может быть эффективнее оптимизированной версии
//! для копирования очень малого количества элементов массива или символов строк в обход memcpy\memmove.
template<typename R, typename OR> Meta::EnableIf<
	Concepts::IsInputRange<R>::_ &&
	!Meta::IsConst<R>::_ &&
	Concepts::IsOutputRangeOf<OR, Concepts::ValueTypeOf<R>>::_ &&
	Concepts::HasFull<OR>::_ &&
	!Concepts::IsInfiniteRange<OR>::_,
size_t> ReadWriteByOne(R& src, OR& dst)
{
	size_t minLen = 0;
	while(!src.Empty() && !dst.Full())
	{
		dst.Put(src.First());
		src.PopFirst();
		minLen++;
	}
	return minLen;
}

template<typename R, typename OR> Meta::EnableIf<
	Concepts::IsInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	Concepts::IsOutputRangeOf<OR, Concepts::ValueTypeOf<R>>::_ &&
	(!Concepts::HasFull<OR>::_ ||
		Concepts::IsInfiniteRange<OR>::_),
size_t> ReadWriteByOne(R& src, OR& dst)
{
	size_t minLen = 0;
	while(!src.Empty())
	{
		dst.Put(src.First());
		src.PopFirst();
		minLen++;
	}
	return minLen;
}


template<typename R, typename OR> Meta::EnableIf<
	Concepts::IsTrivCopyCompatibleArrayWith<R, OR>::_,
size_t> ReadWrite(R& src, OR& dst)
{
	const size_t minLen = Funal::Min(src.Length(), dst.Length());
	C::memmove(dst.Data(), src.Data(), minLen*sizeof(src.First()));
	PopFirstExactly(src, minLen);
	PopFirstExactly(dst, minLen);
	return minLen;
}

template<typename R, typename OR> Meta::EnableIf<
	!Concepts::IsTrivCopyCompatibleArrayWith<R, OR>::_ &&
	Concepts::IsInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	Concepts::IsOutputRangeOf<OR, Concepts::ValueTypeOf<R>>::_ &&
	Concepts::HasFull<OR>::_ && !Concepts::IsInfiniteRange<OR>::_ &&
	!Concepts::HasReadWriteMethod<R&, OR&>::_ &&
	!Concepts::HasPutAllAdvanceMethod<OR&, R&>::_,
size_t> ReadWrite(R& src, OR& dst)
{return ReadWriteByOne(src, dst);}

template<typename R, typename OR> Meta::EnableIf<
	!Concepts::IsTrivCopyCompatibleArrayWith<R, OR>::_ &&
	Concepts::IsNonInfiniteInputRange<R>::_ &&
	!Meta::IsConst<R>::_ &&
	Concepts::IsOutputRangeOf<OR, Concepts::ValueTypeOf<R>>::_ &&
	(!Concepts::HasFull<OR>::_ ||
		Concepts::IsInfiniteRange<OR>::_) &&
	!Concepts::HasReadWriteMethod<R&, OR&>::_ &&
	!Concepts::HasPutAllAdvanceMethod<OR&, R&>::_,
size_t> ReadWrite(R& src, OR& dst)
{
	size_t minLen = 0;
	while(!src.Empty())
	{
		dst.Put(src.First());
		src.PopFirst();
		minLen++;
	}
	return minLen;
}

template<typename R, typename OR> forceinline Meta::EnableIf<
	Concepts::IsInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	Concepts::IsOutputRangeOf<OR, Concepts::ValueTypeOf<R>>::_ &&
	(!Concepts::IsInfiniteRange<R>::_ ||
		Concepts::HasFull<OR>::_) &&
	Concepts::HasReadWriteMethod<R&, OR&>::_ &&
	!Concepts::IsTrivCopyCompatibleArrayWith<R, OR>::_,
size_t> ReadWrite(R& src, OR& dst)
{return src.ReadWrite(dst);}

template<typename R, typename OR> forceinline Meta::EnableIf<
	Concepts::IsInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	Concepts::IsOutputRangeOf<OR, Concepts::ValueTypeOf<R>>::_ &&
	(!Concepts::IsInfiniteRange<R>::_ ||
		Concepts::HasFull<OR>::_) &&
	!Concepts::HasReadWriteMethod<R&, OR&>::_ &&
	Concepts::HasPutAllAdvanceMethod<OR&, R&>::_,
size_t> ReadWrite(R& src, OR& dst)
{return dst.PutAllAdvance(src);}

template<typename R, typename OR> Meta::EnableIf<
	Concepts::IsInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	Concepts::IsOutputRangeOf<OR, Concepts::ValueTypeOf<R>>::_ &&
	Concepts::HasFull<OR>::_ &&
	!Concepts::IsTrivCopyCompatibleArrayWith<R, OR>::_,
size_t> ReadWrite(R& src, size_t n, OR& dst)
{
	size_t left = n;
	while(!src.Empty() && !dst.Full() && left --> 0)
	{
		dst.Put(src.First());
		src.PopFirst();
	}
	return n - left;
}

template<typename R, typename OR> Meta::EnableIf<
	Concepts::IsInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	Concepts::IsOutputRangeOf<OR, Concepts::ValueTypeOf<R>>::_ &&
	!Concepts::HasFull<OR>::_ &&
	!Concepts::IsTrivCopyCompatibleArrayWith<R, OR>::_,
size_t> ReadWrite(R& src, size_t n, OR& dst)
{
	size_t left = n;
	while(!src.Empty() && left --> 0)
	{
		dst.Put(src.First());
		src.PopFirst();
	}
	return n - left;
}

template<typename R, typename OR> Meta::EnableIf<
	Concepts::IsTrivCopyCompatibleArrayWith<R, OR>::_,
size_t> ReadWrite(R& src, size_t n, OR& dst)
{
	size_t minLen = Funal::Min(src.Length(), dst.Length());
	if(minLen>n) minLen = n;
	C::memmove(dst.Data(), src.Data(), minLen*sizeof(src.First()));
	PopFirstExactly(src, minLen);
	PopFirstExactly(dst, minLen);
	return minLen;
}

template<typename R, typename OR, typename P> Meta::EnableIf<
	Concepts::IsInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	Concepts::IsOutputRangeOf<OR, Concepts::ValueTypeOf<R>>::_ &&
	Concepts::HasFull<OR>::_ &&
	Meta::IsCallable<P, Concepts::ValueTypeOf<R>>::_,
size_t> ReadWrite(R& src, OR& dst, P pred)
{
	size_t count = 0;
	while(!src.Empty() && !dst.Full())
	{
		auto value = src.First();
		if(pred(value))
		{
			dst.Put(value);
			count++;
		}
		src.PopFirst();
	}
	return count;
}

template<typename R, typename OR, typename P> Meta::EnableIf<
	Concepts::IsNonInfiniteInputRange<R>::_ &&
	!Meta::IsConst<R>::_ &&
	Concepts::IsOutputRangeOf<OR, Concepts::ValueTypeOf<R>>::_ &&
	!Concepts::HasFull<OR>::_ &&
	Meta::IsCallable<P, Concepts::ValueTypeOf<R>>::_,
size_t> ReadWrite(R& src, OR& dst, P pred)
{
	size_t count = 0;
	while(!src.Empty())
	{
		auto value = src.First();
		if(pred(value))
		{
			dst.Put(value);
			count++;
		}
		src.PopFirst();
	}
	return count;
}



template<typename R, typename OR> forceinline Meta::EnableIf<
	Concepts::IsInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	Concepts::IsAsOutputRangeOf<Meta::RemoveConstRef<OR>, Concepts::ValueTypeOf<R>>::_ &&
	(!Concepts::IsInfiniteRange<R>::_ || Concepts::HasEmpty<Concepts::RangeOfType<OR>>::_),
size_t> ReadTo(R& src, OR&& dst)
{
	auto dstRange = Range::Forward<OR>(dst);
	return ReadWrite(src, dstRange);
}

template<typename R, typename OR> forceinline Meta::EnableIf<
	Concepts::IsInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	Concepts::IsAsOutputRangeOf<Meta::RemoveConst<OR>, Concepts::ValueTypeOf<R>>::_,
size_t> ReadTo(R& src, size_t n, OR&& dst)
{
	auto dstRange = Range::Forward<OR>(dst);
	return ReadWrite(src, n, dstRange);
}

template<typename R, typename OR, typename P> forceinline Meta::EnableIf<
	Concepts::IsInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	Concepts::IsAsOutputRangeOf<OR, Concepts::ValueTypeOf<R>>::_ &&
	Meta::IsCallable<P, Concepts::ValueTypeOf<R>>::_,
size_t> ReadTo(R& src, OR&& dst, P pred)
{
	auto dstCopy = Range::Forward<OR>(dst);
	return ReadWrite(src, dstCopy, pred);
}



template<typename R, typename OR,
	typename AsR = Concepts::RangeOfType<R>
> Meta::EnableIf<
	Concepts::IsAccessibleRange<AsR>::_ &&
	Concepts::IsOutputRangeOf<OR, Concepts::ValueTypeOf<AsR>>::_,
size_t> CopyToAdvanceByOne(R&& src, OR& dst)
{
	auto srcCopy = Range::Forward<R>(src);
	return ReadWriteByOne(srcCopy, dst);
}

template<typename R, typename OR,
	typename AsR = Concepts::RangeOfType<R>
> forceinline Meta::EnableIf<
	Concepts::IsAccessibleRange<AsR>::_ &&
	Concepts::HasPut<OR, Concepts::ValueTypeOf<AsR>>::_,
size_t> WriteTo(R&& src, OR& dst)
{
	auto range = Range::Forward<R>(src);
	return ReadWrite(range, dst);
}

template<typename R, typename OR,
	typename AsR = Concepts::RangeOfType<R>
> forceinline Meta::EnableIf<
	Concepts::IsAccessibleRange<AsR>::_ &&
	Concepts::HasPut<OR, Concepts::ValueTypeOf<AsR>>::_,
size_t> WriteTo(R&& src, size_t n, OR& dst)
{
	auto range = Range::Forward<R>(src);
	return ReadWrite(range, n, dst);
}

template<typename R, typename OR, typename P,
	typename AsR = Concepts::RangeOfType<R>
> forceinline Meta::EnableIf<
	Concepts::IsConsumableRange<AsR>::_ &&
	Concepts::IsOutputRange<OR>::_,
size_t> WriteTo(R&& src, OR& dst, P pred)
{
	auto range = Range::Forward<R>(src);
	return ReadWrite(range, dst, pred);
}


template<typename R, typename OR> forceinline Meta::EnableIf<
	Concepts::IsAsConsumableRange<R>::_ &&
	Concepts::IsAsOutputRange<Meta::RemoveConst<OR>>::_,
size_t> CopyTo(R&& src, OR&& dst)
{
	auto dstCopy = Range::Forward<OR>(dst);
	return WriteTo(Range::Forward<R>(src), dstCopy);
}

template<typename R, typename OR> forceinline Meta::EnableIf<
	Concepts::IsAsAccessibleRange<R>::_ &&
	Concepts::IsAsOutputRange<Meta::RemoveConst<OR>>::_,
size_t> CopyTo(R&& src, size_t n, OR&& dst)
{
	auto dstCopy = Range::Forward<OR>(dst);
	return WriteTo(Range::Forward<R>(src), n, dstCopy);
}

template<typename R, typename OR, typename P,
	typename AsR = Concepts::RangeOfType<R>
> forceinline Meta::EnableIf<
	Concepts::IsConsumableRange<AsR>::_ &&
	Concepts::IsAsOutputRange<Meta::RemoveConstRef<OR>>::_ &&
	Meta::IsCallable<P, Concepts::ValueTypeOf<AsR>>::_,
size_t> CopyTo(R&& range, OR&& dst, P pred)
{
	auto dstCopy = Range::Forward<OR>(dst);
	return WriteTo(Range::Forward<R>(range), dstCopy, pred);
}

}}

INTRA_WARNING_POP
