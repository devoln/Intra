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
//! ������ ���� �������� ��������� ����������� ������ ���������� �� ��� ���,
//! ���� �� �������� �������� ��� �� ��������� ����� ������ �� ����������.
/**
���������� 4 ������ ���������, ������� ���������� ����� ��������� �� ���������:
1) CopyAdvanceToAdvanceUntil: ������������� src � dst � �� �������, �� ������� ������������ �����������.
2) CopyAdvanceToUntil: ���������� �.1 ������������� ������ src, � �� dst �� ������.
3) CopyToAdvanceUntil: ���������� �.1 ������������� ������ dst, � �� src �� ������.
4) CopyToUntil: �� �������� ������������ ���������.

��� ������� ���������� ���������� ������������� ���������.
����������� ��������������� ���������� ����� ���������� ������ �����,
����� ������ ��������� src ����������� ����� ������ ��������� dst.
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
