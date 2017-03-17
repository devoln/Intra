#pragma once

#include "Range/Concepts.h"
#include "Range/AsRange.h"
#include "Range/Operations.h"
#include "Range/Decorators/Take.h"
#include "Platform/CppWarnings.h"
#include "Platform/Intrinsics.h"
#include "Algo/Op.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_SIGN_CONVERSION
INTRA_WARNING_DISABLE_LOSING_CONVERSION

namespace Intra {namespace Algo {

using namespace Range::Concepts;

INTRA_DEFINE_EXPRESSION_CHECKER2(HasCopyAdvanceToAdvanceMethod, Meta::Val<T1>().CopyAdvanceToAdvance(Meta::Val<T2&>()),,);
INTRA_DEFINE_EXPRESSION_CHECKER2(HasCopyAdvanceFromAdvanceMethod, Meta::Val<T1>().CopyAdvanceFromAdvance(Meta::Val<T2&>()),,);

//! @file
//! Данный файл содержит алгоритмы копирования данных диапазонов.
/**
Существует три варианта копирования:
1) Копирование до тех пор, пока источник или приёмник не закончатся.
 Таким образом, для правильно спроектированных диапазонов ошибка переполнения невозможна.
2) Аналогично п.1, но при достижении n скопированных элементов копирование останавливается.
3) Аналогично п.1, но элементы источника, не удовлетворяющие предикату pred, пропускаются.

Для каждого из этих вариантов существует по 4 версии алгоритма, которые отличаются своим действием на аргументы:
1) CopyAdvanceToAdvance: устанавливает src и dst в ту позицию, на которой остановилось копирование.
2) CopyAdvanceTo: аналогично п.1 устанавливает только src, а на dst не влияет.
3) CopyToAdvance: аналогично п.1 устанавливает только dst, а на src не влияет.
4) CopyTo: не изменяет передаваемые аргументы.

Все функции возвращают количество скопированных элементов.
Копирование перекрывающихся диапазонов будет корректным только тогда,
когда начало диапазона src расположено после начала диапазона dst.
**/


//! Неоптимизированная версия алгоритма копирования. Может быть эффективнее оптимизированной версии
//! для копирования очень малого количества элементов массива или символов строк в обход memcpy\memmove.
template<typename R, typename OR> Meta::EnableIf<
	IsInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	IsOutputRangeOf<OR, ValueTypeOf<R>>::_ &&
	HasEmpty<OR>::_ && !IsInfiniteRange<OR>::_,
size_t> CopyAdvanceToAdvanceByOne(R& src, OR& dst)
{
	size_t minLen = 0;
	while(!src.Empty() && !dst.Empty())
	{
		dst.Put(src.First());
		src.PopFirst();
		minLen++;
	}
	return minLen;
}

template<typename R, typename OR> Meta::EnableIf<
	IsInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	IsOutputRangeOf<OR, ValueTypeOf<R>>::_ &&
	(!HasEmpty<OR>::_ || IsInfiniteRange<OR>::_),
size_t> CopyAdvanceToAdvanceByOne(R& src, OR& dst)
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
	IsTrivCopyCompatibleArrayWith<R, OR>::_,
size_t> CopyAdvanceToAdvance(R& src, OR& dst)
{
	size_t minLen = Op::Min(src.Length(), dst.Length());
	C::memmove(dst.Data(), src.Data(), minLen*sizeof(src.First()));
	Range::PopFirstExactly(src, minLen);
	Range::PopFirstExactly(dst, minLen);
	return minLen;
}

template<typename R, typename OR> Meta::EnableIf<
	!IsTrivCopyCompatibleArrayWith<R, OR>::_ &&
	IsInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	IsOutputRangeOf<OR, ValueTypeOf<R>>::_ &&
	HasEmpty<OR>::_ && !IsInfiniteRange<OR>::_ &&
	!HasCopyAdvanceToAdvanceMethod<R&, OR&>::_ &&
	!HasCopyAdvanceFromAdvanceMethod<OR&, R&>::_,
size_t> CopyAdvanceToAdvance(R& src, OR& dst)
{return CopyAdvanceToAdvanceByOne(src, dst);}

template<typename R, typename OR> Meta::EnableIf<
	!IsTrivCopyCompatibleArrayWith<R, OR>::_ &&
	IsNonInfiniteInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	IsOutputRangeOf<OR, ValueTypeOf<R>>::_ &&
	(!HasEmpty<OR>::_ || IsInfiniteRange<OR>::_) &&
	!HasCopyAdvanceToAdvanceMethod<R&, OR&>::_ &&
	!HasCopyAdvanceFromAdvanceMethod<OR&, R&>::_,
size_t> CopyAdvanceToAdvance(R& src, OR& dst)
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
	IsInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	IsOutputRangeOf<OR, ValueTypeOf<R>>::_ &&
	(!IsInfiniteRange<R>::_ || HasEmpty<OR>::_) &&
	HasCopyAdvanceToAdvanceMethod<R&, OR&>::_,
size_t> CopyAdvanceToAdvance(R& src, OR& dst)
{return src.CopyAdvanceToAdvance(dst);}

template<typename R, typename OR> forceinline Meta::EnableIf<
	IsInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	IsOutputRangeOf<OR, ValueTypeOf<R>>::_ &&
	(!IsInfiniteRange<R>::_ || HasEmpty<OR>::_) &&
	!HasCopyAdvanceToAdvanceMethod<R&, OR&>::_ &&
	HasCopyAdvanceFromAdvanceMethod<OR&, R&>::_,
size_t> CopyAdvanceToAdvance(R& src, OR& dst)
{return dst.CopyAdvanceFromAdvance(src);}

template<typename R, typename OR> Meta::EnableIf<
	IsInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	IsOutputRangeOf<OR, ValueTypeOf<R>>::_ && HasEmpty<OR>::_ &&
	!IsTrivCopyCompatibleArrayWith<R, OR>::_,
size_t> CopyAdvanceToAdvance(R& src, size_t n, OR& dst)
{
	size_t left = n;
	while(!src.Empty() && !dst.Empty() && left --> 0)
	{
		dst.Put(src.First());
		src.PopFirst();
	}
	return n-left;
}

template<typename R, typename OR> Meta::EnableIf<
	IsInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	IsOutputRangeOf<OR, ValueTypeOf<R>>::_ && !HasEmpty<OR>::_ &&
	!IsTrivCopyCompatibleArrayWith<R, OR>::_,
size_t> CopyAdvanceToAdvance(R& src, size_t n, OR& dst)
{
	size_t left = n;
	while(!src.Empty() && left --> 0)
	{
		dst.Put(src.First());
		src.PopFirst();
	}
	return n-left;
}

template<typename R, typename OR> Meta::EnableIf<
	IsTrivCopyCompatibleArrayWith<R, OR>::_,
size_t> CopyAdvanceToAdvance(R& src, size_t n, OR& dst)
{
	size_t minLen = Op::Min(src.Length(), dst.Length());
	if(minLen>n) minLen = n;
	C::memmove(dst.Data(), src.Data(), minLen*sizeof(src.First()));
	Range::PopFirstExactly(src, minLen);
	Range::PopFirstExactly(dst, minLen);
	return minLen;
}

template<typename R, typename OR, typename P> Meta::EnableIf<
	IsInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	IsOutputRangeOf<OR, ValueTypeOf<R>>::_ && HasEmpty<OR>::_ &&
	Meta::IsCallable<P, ValueTypeOf<R>>::_,
size_t> CopyAdvanceToAdvance(R& src, OR& dst, P pred)
{
	size_t count = 0;
	while(!src.Empty() && !dst.Empty())
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
	IsNonInfiniteInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	IsOutputRangeOf<OR, ValueTypeOf<R>>::_ && !HasEmpty<OR>::_ &&
	Meta::IsCallable<P, ValueTypeOf<R>>::_,
size_t> CopyAdvanceToAdvance(R& src, OR& dst, P pred)
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
	IsInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	IsAsOutputRangeOf<Meta::RemoveConstRef<OR>, ValueTypeOf<R>>::_ &&
	(!IsInfiniteRange<R>::_ || HasEmpty<AsRangeResult<OR>>::_),
size_t> CopyAdvanceTo(R& src, OR&& dst)
{
	auto dstRange = Range::Forward<OR>(dst);
	return Algo::CopyAdvanceToAdvance(src, dstRange);
}

template<typename R, typename OR> forceinline Meta::EnableIf<
	IsInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	IsAsOutputRangeOf<Meta::RemoveConst<OR>, ValueTypeOf<R>>::_,
size_t> CopyAdvanceTo(R& src, size_t n, OR&& dst)
{
	auto dstRange = Range::Forward<OR>(dst);
	return Algo::CopyAdvanceToAdvance(src, n, dstRange);
}

template<typename R, typename OR, typename P> forceinline Meta::EnableIf<
	IsInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	IsAsOutputRangeOf<OR, ValueTypeOf<R>>::_ &&
	Meta::IsCallable<P, ValueTypeOf<R>>::_,
size_t> CopyAdvanceTo(R& src, OR&& dst, P pred)
{
	auto dstCopy = Range::Forward<OR>(dst);
	return Algo::CopyAdvanceToAdvance(src, dstCopy, pred);
}



template<typename R, typename OR> Meta::EnableIf<
	IsAsAccessibleRange<R>::_ &&
	IsOutputRangeOf<OR, ValueTypeOfAs<R>>::_,
size_t> CopyToAdvanceByOne(R&& src, OR& dst)
{
	auto srcCopy = Range::Forward<R>(src);
	return Algo::CopyAdvanceToAdvanceByOne(srcCopy, dst);
}

template<typename R, typename OR> forceinline Meta::EnableIf<
	IsAsAccessibleRange<R>::_ &&
	HasPut<OR, ValueTypeOfAs<R>>::_,
size_t> CopyToAdvance(R&& src, OR& dst)
{
	auto range = Range::Forward<R>(src);
	return Algo::CopyAdvanceToAdvance(range, dst);
}

template<typename R, typename OR> forceinline Meta::EnableIf<
	IsAsAccessibleRange<R>::_ &&
	HasPut<OR, ValueTypeOfAs<R>>::_,
size_t> CopyToAdvance(R&& src, size_t n, OR& dst)
{
	auto range = Range::Forward<R>(src);
	return Algo::CopyAdvanceToAdvance(range, n, dst);
}

template<typename R, typename OR, typename P> forceinline Meta::EnableIf<
	IsAsConsumableRange<R>::_ &&
	IsOutputRange<OR>::_,
size_t> CopyToAdvance(R&& src, OR& dst, P pred)
{
	auto range = Range::Forward<R>(src);
	return Algo::CopyAdvanceToAdvance(range, dst, pred);
}


template<typename R, typename OR> forceinline Meta::EnableIf<
	IsAsConsumableRange<R>::_ &&
	IsAsOutputRange<Meta::RemoveConst<OR>>::_,
size_t> CopyTo(R&& src, OR&& dst)
{
	auto dstCopy = Range::Forward<OR>(dst);
	return Algo::CopyToAdvance(Range::Forward<R>(src), dstCopy);
}

template<typename R, typename OR> forceinline Meta::EnableIf<
	IsAsAccessibleRange<R>::_ &&
	IsAsOutputRange<Meta::RemoveConst<OR>>::_,
size_t> CopyTo(R&& src, size_t n, OR&& dst)
{
	auto dstCopy = Range::Forward<OR>(dst);
	return Algo::CopyToAdvance(Range::Forward<R>(src), n, dstCopy);
}

template<typename R, typename OR, typename P> forceinline Meta::EnableIf<
	IsAsConsumableRange<R>::_ &&
	IsAsOutputRange<Meta::RemoveConstRef<OR>>::_ &&
	Meta::IsCallable<P, ValueTypeOfAs<R>>::_,
size_t> CopyTo(R&& range, OR&& dst, P pred)
{
	auto dstCopy = Range::Forward<OR>(dst);
	return Algo::CopyToAdvance(Range::Forward<R>(range), dstCopy, pred);
}

}}

INTRA_WARNING_POP
