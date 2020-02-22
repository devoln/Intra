#pragma once

#include "Core/CArray.h"
#include "Core/Range/Concepts.h"
#include "Core/Range/Operations.h"
#include "Core/Range/Take.h"
#include "Core/Misc/RawMemory.h"

INTRA_BEGIN
INTRA_WARNING_DISABLE_SIGN_CONVERSION
INTRA_WARNING_DISABLE_LOSING_CONVERSION

//! @file
//! This file contains range copying algorithms.
/**
  All of copying functions copy ``n`` = Min(Length(src), Length(dst)) elements, so buffer overflow errors are not possible.

  There are 4 versions of the algorithm, differing by their effect on arguments:
  1) ReadWrite: advances ``src`` and ``dst`` by ``n`` elements, where ``n`` is a number of copied arguments.
  The first argument acts as an input stream, from which this function *reads* elements advancing its position.
  The second argument acts as an output stream, in which this function *writes* elements advancing its position.

2) ReadTo: advances ``src``` by ``n`` elements, where ``n`` is a number of copied arguments.
  The first argument acts as an input stream, from which this function *reads* elements advancing its position.
  The second argument acts as an output range, in which this function puts elements without modifying its position.

3) WriteTo: advances ``dst`` by ``n`` elements, where ``n`` is a number of copied arguments.
  The first argument acts as an input range, used as a source of the elements to output without modifying its position.
  The second argument acts as an output stream, in which this function *writes* elements advancing its position.

4) CopyTo: copies ``n`` elements from ``src`` to ``dst``, where ``n`` is a number of copied arguments.
  The first argument acts as an input range, used as a source of the elements to output without modifying its position.
  The second argument acts as an output range, in which this function puts elements without modifying its position.

All function return ``n`` - number of copied elements that is the minimum Length of ``src`` and ``dst``.
``dst`` and ``src`` may overlap only if ``src`` elements come after ``dst`` elements
**/


/** Same as ReadWrite but without optimization for trivial types.
  May be faster for short arrays of trivial types where their length is not known at compile time.
*/
template<typename R, typename OR> constexpr Requires<
	CInputRange<R> &&
	!CConst<R> &&
	COutputRangeOf<OR, TValueTypeOf<R>> &&
	CHasFull<OR> &&
	!CInfiniteRange<OR>,
index_t> ReadWriteByOne(R& src, OR& dst)
{
	index_t minLen = 0;
	while(!src.Empty() && !dst.Full())
	{
		dst.Put(src.First());
		src.PopFirst();
		minLen++;
	}
	return minLen;
}

template<typename R, typename OR> constexpr Requires<
	CInputRange<R> && !CConst<R> &&
	COutputRangeOf<OR, TValueTypeOf<R>> &&
	(!CHasFull<OR> || CInfiniteRange<OR>),
index_t> ReadWriteByOne(R& src, OR& dst)
{
	index_t minLen = 0;
	while(!src.Empty())
	{
		dst.Put(src.First());
		src.PopFirst();
		minLen++;
	}
	return minLen;
}


template<typename R, typename OR> constexpr Requires<
	CTrivCopyCompatibleArrayWith<R, OR>,
index_t> ReadWrite(R& src, OR& dst)
{
	const auto minLen = FMin(LengthOf(src), LengthOf(dst));
	Misc::CopyBits(DataOf(dst), DataOf(src), minLen);
	PopFirstExactly(dst, minLen);
	PopFirstExactly(src, minLen);
	return minLen;
}

template<typename R, typename OR> constexpr Requires<
	!CTrivCopyCompatibleArrayWith<R, OR> &&
	CInputRange<R> && !CConst<R> &&
	COutputRangeOf<OR, TValueTypeOf<R>> &&
	CHasFull<OR> && !CInfiniteRange<OR> &&
	!CHasReadWriteMethod<R&, OR&> &&
	!CHasPutAllAdvanceMethod<OR&, R&>,
index_t> ReadWrite(R& src, OR& dst)
{return ReadWriteByOne(src, dst);}

template<typename R, typename OR> constexpr Requires<
	!CTrivCopyCompatibleArrayWith<R, OR> &&
	CNonInfiniteInputRange<R> &&
	!CConst<R> &&
	COutputRangeOf<OR, TValueTypeOf<R>> &&
	(!CHasFull<OR> || CInfiniteRange<OR>) &&
	!CHasReadWriteMethod<R&, OR&> &&
	!CHasPutAllAdvanceMethod<OR&, R&>,
index_t> ReadWrite(R& src, OR& dst)
{
	index_t minLen = 0;
	while(!src.Empty())
	{
		dst.Put(src.First());
		src.PopFirst();
		minLen++;
	}
	return minLen;
}

template<typename R, typename OR> constexpr forceinline Requires<
	CInputRange<R> && !CConst<R> &&
	COutputRangeOf<OR, TValueTypeOf<R>> &&
	(!CInfiniteRange<R> || CHasFull<OR>) &&
	CHasReadWriteMethod<R&, OR&> &&
	!CTrivCopyCompatibleArrayWith<R, OR>,
index_t> ReadWrite(R& src, OR& dst)
{return src.ReadWrite(dst);}

template<typename R, typename OR> constexpr forceinline Requires<
	CInputRange<R> && !CConst<R> &&
	COutputRangeOf<OR, TValueTypeOf<R>> &&
	(!CInfiniteRange<R> || CHasFull<OR>) &&
	!CHasReadWriteMethod<R&, OR&> &&
	CHasPutAllAdvanceMethod<OR&, R&>,
index_t> ReadWrite(R& src, OR& dst)
{return dst.PutAllAdvance(src);}

template<typename R, typename OR> constexpr forceinline Requires<
	CInputRange<R> && !CConst<R> &&
	CAsOutputRangeOf<TRemoveConstRef<OR>, TValueTypeOf<R>> &&
	(!CInfiniteRange<R> || CHasEmpty<TRangeOfType<OR>>),
index_t> ReadTo(R& src, OR&& dst)
{
	auto dstRange = ForwardAsRange<OR>(dst);
	return ReadWrite(src, dstRange);
}


template<typename R, typename OR,
	typename AsR = TRangeOfType<R>
> constexpr forceinline Requires<
	CAccessibleRange<AsR> &&
	COutputRangeOf<OR, TValueTypeOf<AsR>>,
index_t> CopyToAdvanceByOne(R&& src, OR& dst)
{
	auto srcCopy = ForwardAsRange<R>(src);
	return ReadWriteByOne(srcCopy, dst);
}

template<typename R, typename OR,
	typename AsR = TRangeOfType<R>
> constexpr forceinline Requires<
	CAccessibleRange<AsR> &&
	CHasPut<OR, TValueTypeOf<AsR>>,
index_t> WriteTo(R&& src, OR& dst)
{
	auto range = ForwardAsRange<R>(src);
	return ReadWrite(range, dst);
}

template<typename R, typename OR, typename AsR = TRangeOfType<R>, typename AsOR = TRangeOfType<OR>> constexpr forceinline Requires<
	COutputRange<TRemoveConst<AsOR>> &&
	(CConsumableRange<AsR> || CAccessibleRange<AsR> && CHasFull<AsOR>),
index_t> CopyTo(R&& src, OR&& dst)
{
	auto dstCopy = ForwardAsRange<OR>(dst);
	return WriteTo(ForwardAsRange<R>(src), dstCopy);
}
INTRA_END
