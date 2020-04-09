#pragma once

#include "Intra/Range/Concepts.h"
#include "Intra/Range/Operations.h"
#include "Intra/Range/Take.h"

//TODO: delete this (superceded by CopyTo(dst).From(TakeUntil(src)))

//! @file
//! ������ ���� �������� ��������� ����������� ������ ���������� �� ��� ���,
//! ���� �� �������� �������� ��� �� ��������� ����� ������ �� ����������.
/**
���������� 4 ������ ���������, ������� ���������� ����� ��������� �� ���������:
1) ReadWriteUntil: ������������� src � dst � �� �������, �� ������� ������������ �����������.
2) ReadToUntil: ���������� �.1 ������������� ������ src, � �� dst �� ������.
3) WriteToUntil: ���������� �.1 ������������� ������ dst, � �� src �� ������.
4) CopyToUntil: �� �������� ������������ ���������.

��� ������� ���������� ���������� ������������� ���������.
����������� ��������������� ���������� ����� ���������� ������ �����,
����� ������ ��������� src ����������� ����� ������ ��������� dst.

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
