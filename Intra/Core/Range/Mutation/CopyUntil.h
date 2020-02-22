#pragma once

#include "Core/Range/Concepts.h"

#include "Core/Range/Operations.h"
#include "Core/Range/Take.h"

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
INTRA_WARNING_DISABLE_SIGN_CONVERSION
INTRA_WARNING_DISABLE_LOSING_CONVERSION
enum class StopReason {SourceEmpty, DestFull, Found};
struct FindResult
{
	index_t ElementsRead = 0;
	StopReason StopReason = StopReason::SourceEmpty;

};

template<typename R, typename OR, typename P> constexpr Requires<
	CInputRange<R> &&
	!CConst<R> &&
	COutputRangeOf<OR, TValueTypeOf<R>> &&
	CHasEmpty<OR> && !CInfiniteRange<OR> &&
	CCallable<P, TValueTypeOf<R>>,
FindResult> ReadWriteUntil(R& src, OR& dst, P&& pred)
{
	FindResult res;
	for(;;)
	{
		if(src.Empty()) break;
		if(dst.Empty())
		{
			res.StopReason = StopReason::DestFull;
			break;
		}
		auto&& srcFirst = src.First();
		if(pred(srcFirst))
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

template<typename R, typename OR, typename P> constexpr Requires<
	CInputRange<R> &&
	!CConst<R> &&
	COutputRangeOf<OR, TValueTypeOf<R>> &&
	(!CHasEmpty<OR> || CInfiniteRange<OR>) &&
	CCallable<P, TValueTypeOf<R>>,
FindResult> ReadWriteUntil(R& src, OR& dst, P&& pred)
{
	FindResult res;
	for(;;)
	{
		if(src.Empty()) break;
		auto&& srcFirst = src.First();
		if(pred(srcFirst))
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

template<typename R, typename OR, typename X> constexpr Requires<
	CInputRange<R> &&
	!CConst<R> &&
	COutputRangeOf<OR, TValueTypeOf<R>> &&
	CHasEmpty<OR> && !CInfiniteRange<OR> &&
	!CCallable<X, TValueTypeOf<R>>,
FindResult> ReadWriteUntil(R& src, OR& dst, const X& stopValue)
{
	FindResult res;
	for(;;)
	{
		if(src.Empty()) break;
		if(dst.Empty())
		{
			res.StopReason = StopReason::DestFull;
			break;
		}
		auto&& srcFirst = src.First();
		if(srcFirst == stopValue)
		{
			res.StopReason = StopReason::Found;
			break;
		}
		dst.Put(src.First());
		src.PopFirst();
		res.ElementsRead++;
	}
	return res;
}

template<typename R, typename OR, typename X> constexpr Requires<
	CInputRange<R> &&
	!CConst<R> &&
	COutputRangeOf<OR, TValueTypeOf<R>> &&
	(!CHasEmpty<OR> || CInfiniteRange<OR>) &&
	!CCallable<X, TValueTypeOf<R>>,
FindResult> ReadWriteUntil(R& src, OR& dst, const X& stopValue)
{
	FindResult res;
	for(;;)
	{
		if(src.Empty()) break;
		auto&& srcFirst = src.First();
		if(srcFirst == stopValue)
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

template<typename R, typename OR, typename X> constexpr forceinline Requires<
	CInputRange<R> &&
	!CConst<R> &&
	CAsOutputRangeOf<TRemoveConstRef<OR>, TValueTypeOf<R>>,
FindResult> ReadToUntil(R& src, OR&& dst, const X& x)
{
	auto dstRange = ForwardAsRange<OR>(dst);
	return ReadWriteUntil(src, dstRange, x);
}

template<typename R, typename OR, typename X> constexpr forceinline Requires<
	CInputRange<R> &&
	!CConst<R> &&
	CAsOutputRangeOf<TRemoveConstRef<OR>, TValueTypeOf<R>>,
FindResult> WriteToUntil(R&& src, OR& dst, const X& x)
{
	auto srcRange = ForwardAsRange<R>(src);
	return ReadWriteUntil(srcRange, dst, x);
}

template<typename R, typename OR, typename X> constexpr forceinline Requires<
	CInputRange<R> &&
	!CConst<R> &&
	CAsOutputRangeOf<TRemoveConstRef<OR>, TValueTypeOf<R>>,
FindResult> CopyToUntil(R&& src, OR& dst, const X& x)
{
	auto srcRange = ForwardAsRange<R>(src);
	return ReadToUntil(srcRange, dst, x);
}
INTRA_END
