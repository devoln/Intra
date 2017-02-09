#pragma once

#include "Range/Concepts.h"
#include "Range/AsRange.h"
#include "Range/Operations.h"
#include "Range/Decorators/Take.h"
#include "Platform/Debug.h"
#include "Platform/CppWarnings.h"
#include "Platform/Intrinsics.h"
#include "Algo/Op.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_SIGN_CONVERSION


namespace Intra {namespace Algo {

INTRA_DEFINE_EXPRESSION_CHECKER2(HasCopyAdvanceToAdvanceMethod, Meta::Val<T1>().CopyAdvanceToAdvance(Meta::Val<T2&>()),,);
INTRA_DEFINE_EXPRESSION_CHECKER2(HasCopyAdvanceToAdvanceMethodN, Meta::Val<T1>().CopyAdvanceToAdvance(Meta::Val<T2&>(), size_t()),,);

//! Данный файл содержит алгоритмы копирования данных диапазонов
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

template<typename R, typename OR> Meta::EnableIf<
	Range::IsTrivCopyCompatibleArrayWith<R, OR>::_,
size_t> CopyAdvanceToAdvance(R& src, OR& dst)
{
	size_t minLen = Op::Min(src.Length(), dst.Length());
	C::memmove(dst.Data(), src.Data(), minLen*sizeof(src.First()));
	Range::PopFirstExactly(src, minLen);
	Range::PopFirstExactly(dst, minLen);
	return minLen;
}

template<typename R, typename OR> Meta::EnableIf<
	!Range::IsTrivCopyCompatibleArrayWith<R, OR>::_ &&
	Range::IsInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	Range::IsOutputRangeOf<OR, Range::ValueTypeOf<R>>::_ &&
	Range::HasEmpty<OR>::_ && !Range::IsInfiniteRange<OR>::_ &&
	!HasCopyAdvanceToAdvanceMethod<R&, OR&>::_,
size_t> CopyAdvanceToAdvance(R& src, OR& dst)
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
	!Range::IsTrivCopyCompatibleArrayWith<R, OR>::_ &&
	Range::IsNonInfiniteInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	Range::IsOutputRangeOf<OR, Range::ValueTypeOf<R>>::_ &&
	(!Range::HasEmpty<OR>::_ || Range::IsInfiniteRange<OR>::_) &&
	!HasCopyAdvanceToAdvanceMethod<R&, OR&>::_,
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
	Range::IsInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	Range::IsOutputRangeOf<OR, Range::ValueTypeOf<R>>::_ &&
	(!Range::IsInfiniteRange<R>::_ || Range::HasEmpty<OR>::_) &&
	HasCopyAdvanceToAdvanceMethod<R&, OR&>::_,
size_t> CopyAdvanceToAdvance(R& src, OR& dst)
{return src.CopyAdvanceToAdvance(dst);}

template<typename R, typename OR> Meta::EnableIf<
	Range::IsInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	Range::IsOutputRangeOf<OR, Range::ValueTypeOf<R>>::_ &&
	HasCopyAdvanceToAdvanceMethodN<R&, OR&>::_,
size_t> CopyAdvanceToAdvance(R& src, size_t n, OR& dst)
{return src.CopyAdvanceToAdvanceN(dst, n);}

template<typename R, typename OR> Meta::EnableIf<
	Range::IsInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	Range::IsOutputRangeOf<OR, Range::ValueTypeOf<R>>::_ && Range::HasEmpty<OR>::_ &&
	!Range::IsTrivCopyCompatibleArrayWith<R, OR>::_ &&
	!HasCopyAdvanceToAdvanceMethodN<R&, OR&>::_,
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
	Range::IsInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	Range::IsOutputRangeOf<OR, Range::ValueTypeOf<R>>::_ && !Range::HasEmpty<OR>::_ &&
	!Range::IsTrivCopyCompatibleArrayWith<R, OR>::_ &&
	!HasCopyAdvanceToAdvanceMethodN<R&, OR&>::_,
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
	Range::IsTrivCopyCompatibleArrayWith<R, OR>::_,
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
	Range::IsInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	Range::IsOutputRangeOf<OR, Range::ValueTypeOf<R>>::_ && Range::HasEmpty<OR>::_ &&
	Meta::IsCallable<P, Range::ValueTypeOf<R>>::_,
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
	Range::IsNonInfiniteInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	Range::IsOutputRangeOf<OR, Range::ValueTypeOf<R>>::_ && !Range::HasEmpty<OR>::_ &&
	Meta::IsCallable<P, Range::ValueTypeOf<R>>::_,
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
	Range::IsInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	Range::IsAsOutputRangeOf<Meta::RemoveConstRef<OR>, Range::ValueTypeOf<R>>::_ &&
	(!Range::IsInfiniteRange<R>::_ || Range::HasEmpty<Range::AsRangeResult<OR>>::_),
size_t> CopyAdvanceTo(R& src, OR&& dst)
{
	auto dstRange = Range::Forward<OR>(dst);
	return Algo::CopyAdvanceToAdvance(src, dstRange);
}

template<typename R, typename OR> forceinline Meta::EnableIf<
	Range::IsInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	Range::IsAsOutputRangeOf<Meta::RemoveConst<OR>, Range::ValueTypeOf<R>>::_,
size_t> CopyAdvanceTo(R& src, size_t n, OR&& dst)
{
	auto dstRange = Range::Forward<OR>(dst);
	return Algo::CopyAdvanceToAdvance(src, n, dstRange);
}

template<typename R, typename OR, typename P> forceinline Meta::EnableIf<
	Range::IsInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	Range::IsAsOutputRangeOf<OR, Range::ValueTypeOf<R>>::_ &&
	Meta::IsCallable<P, Range::ValueTypeOf<R>>::_,
size_t> CopyAdvanceTo(R& src, OR&& dst, P pred)
{
	auto dstCopy = Range::Forward<OR>(dst);
	return Algo::CopyAdvanceToAdvance(src, dstCopy, pred);
}


template<typename R, typename OR> forceinline Meta::EnableIf<
	Range::IsAsAccessibleRange<R>::_ &&
	Range::HasPut<OR, Range::ValueTypeOfAs<R>>::_,
size_t> CopyToAdvance(R&& src, OR& dst)
{
	auto range = Range::Forward<R>(src);
	return Algo::CopyAdvanceToAdvance(range, dst);
}

template<typename R, typename OR> forceinline Meta::EnableIf<
	Range::IsAsAccessibleRange<R>::_ &&
	Range::HasPut<OR, Range::ValueTypeOfAs<R>>::_,
size_t> CopyToAdvance(R&& src, size_t n, OR& dst)
{
	auto range = Range::Forward<R>(src);
	return Algo::CopyAdvanceToAdvance(range, n, dst);
}

template<typename R, typename OR, typename P> forceinline Meta::EnableIf<
	Range::IsAsConsumableRange<R>::_ &&
	Range::IsOutputRange<OR>::_,
size_t> CopyToAdvance(R&& src, OR& dst, P pred)
{
	auto range = Range::Forward<R>(src);
	return Algo::CopyAdvanceToAdvance(range, dst, pred);
}


template<typename R, typename OR> forceinline Meta::EnableIf<
	Range::IsAsConsumableRange<R>::_ &&
	Range::IsAsOutputRange<Meta::RemoveConst<OR>>::_,
size_t> CopyTo(R&& src, OR&& dst)
{
	auto dstCopy = Range::Forward<OR>(dst);
	return Algo::CopyToAdvance(Range::Forward<R>(src), dstCopy);
}

template<typename R, typename OR> forceinline Meta::EnableIf<
	Range::IsAsAccessibleRange<R>::_ &&
	Range::IsAsOutputRange<Meta::RemoveConst<OR>>::_,
size_t> CopyTo(R&& src, size_t n, OR&& dst)
{
	auto dstCopy = Range::Forward<OR>(dst);
	return Algo::CopyToAdvance(Range::Forward<R>(src), n, dstCopy);
}

template<typename R, typename OR, typename P> forceinline Meta::EnableIf<
	Range::IsAsConsumableRange<R>::_ &&
	Range::IsAsOutputRange<Meta::RemoveConstRef<OR>>::_ &&
	Meta::IsCallable<P, Range::ValueTypeOfAs<R>>::_,
size_t> CopyTo(R&& range, OR&& dst, P pred)
{
	auto dstCopy = Range::Forward<OR>(dst);
	return Algo::CopyToAdvance(Range::Forward<R>(range), dstCopy, pred);
}

INTRA_WARNING_POP

}}
