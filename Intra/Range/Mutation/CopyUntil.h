#pragma once

#include "Concepts/Range.h"
#include "Concepts/RangeOf.h"
#include "Range/Operations.h"
#include "Range/Decorators/Take.h"
#include "Cpp/Warnings.h"
#include "Utils/Op.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_SIGN_CONVERSION
INTRA_WARNING_DISABLE_LOSING_CONVERSION

//! @file
//! ������ ���� �������� ��������� ����������� ������ ���������� �� ��� ���,
//! ���� �� �������� �������� ��� �� ��������� ����� ������ �� ����������.
/**
���������� 4 ������ ���������, ������� ���������� ����� ��������� �� ���������:
1) ReadToAdvanceUntil: ������������� src � dst � �� �������, �� ������� ������������ �����������.
2) ReadToUntil: ���������� �.1 ������������� ������ src, � �� dst �� ������.
3) CopyToAdvanceUntil: ���������� �.1 ������������� ������ dst, � �� src �� ������.
4) CopyToUntil: �� �������� ������������ ���������.

��� ������� ���������� ���������� ������������� ���������.
����������� ��������������� ���������� ����� ���������� ������ �����,
����� ������ ��������� src ����������� ����� ������ ��������� dst.
**/

namespace Intra { namespace Range {

template<typename R, typename OR, typename P> Meta::EnableIf<
	Concepts::IsInputRange<R>::_ &&
	!Meta::IsConst<R>::_ &&
	Concepts::IsOutputRangeOf<OR, Concepts::ValueTypeOf<R>>::_ &&
	Concepts::HasEmpty<OR>::_ && !Concepts::IsInfiniteRange<OR>::_ &&
	Meta::IsCallable<P, Concepts::ValueTypeOf<R>>::_,
size_t> ReadToAdvanceUntil(R& src, OR& dst, P&& pred)
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
	Concepts::IsInputRange<R>::_ &&
	!Meta::IsConst<R>::_ &&
	Concepts::IsOutputRangeOf<OR, Concepts::ValueTypeOf<R>>::_ &&
	(!Concepts::HasEmpty<OR>::_ ||
		Concepts::IsInfiniteRange<OR>::_) &&
	Meta::IsCallable<P, Concepts::ValueTypeOf<R>>::_,
size_t> ReadToAdvanceUntil(R& src, OR& dst, P&& pred)
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
	Concepts::IsInputRange<R>::_ &&
	!Meta::IsConst<R>::_ &&
	Concepts::IsOutputRangeOf<OR, Concepts::ValueTypeOf<R>>::_ &&
	Concepts::HasEmpty<OR>::_ &&
	!Concepts::IsInfiniteRange<OR>::_ &&
	!Meta::IsCallable<X, Concepts::ValueTypeOf<R>>::_,
size_t> ReadToAdvanceUntil(R& src, OR& dst, const X& stopValue)
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
	Concepts::IsInputRange<R>::_ &&
	!Meta::IsConst<R>::_ &&
	Concepts::IsOutputRangeOf<OR, Concepts::ValueTypeOf<R>>::_ &&
	(!Concepts::HasEmpty<OR>::_ ||
		Concepts::IsInfiniteRange<OR>::_) &&
	!Meta::IsCallable<X, Concepts::ValueTypeOf<R>>::_,
size_t> ReadToAdvanceUntil(R& src, OR& dst, const X& stopValue)
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
	Concepts::IsInputRange<R>::_ &&
	!Meta::IsConst<R>::_ &&
	Concepts::IsAsOutputRangeOf<Meta::RemoveConstRef<OR>, Concepts::ValueTypeOf<R>>::_,
size_t> ReadToUntil(R& src, OR&& dst, const X& x)
{
	auto dstRange = Range::Forward<OR>(dst);
	return ReadToAdvanceUntil(src, dstRange, x);
}

template<typename R, typename OR, typename X> forceinline Meta::EnableIf<
	Concepts::IsInputRange<R>::_ &&
	!Meta::IsConst<R>::_ &&
	Concepts::IsAsOutputRangeOf<Meta::RemoveConstRef<OR>, Concepts::ValueTypeOf<R>>::_,
size_t> CopyToAdvanceUntil(R&& src, OR& dst, const X& x)
{
	auto srcRange = Range::Forward<R>(src);
	return ReadToAdvanceUntil(srcRange, dst, x);
}

template<typename R, typename OR, typename X> forceinline Meta::EnableIf<
	Concepts::IsInputRange<R>::_ &&
	!Meta::IsConst<R>::_ &&
	Concepts::IsAsOutputRangeOf<Meta::RemoveConstRef<OR>, Concepts::ValueTypeOf<R>>::_,
size_t> CopyToUntil(R&& src, OR& dst, const X& x)
{
	auto srcRange = Range::Forward<R>(src);
	return ReadToUntil(srcRange, dst, x);
}

}}

INTRA_WARNING_POP
