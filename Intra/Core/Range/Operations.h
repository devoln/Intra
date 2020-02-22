#pragma once

#include "Core/Range/Concepts.h"
#include "Core/Assert.h"
#include "Core/Optional.h"

INTRA_BEGIN
template<typename R> INTRA_NODISCARD constexpr forceinline Requires<
	CHasLength<R>,
size_t> Count(const R& range) {return range.Length();}

template<typename R> constexpr Requires<
	CFiniteForwardRange<R> &&
	!CConst<R>,
size_t> CountAdvance(R& range)
{
	size_t result = 0;
	while(!range.Empty()) range.PopFirst(), result++;
	return result;
}

template<typename R> INTRA_NODISCARD constexpr forceinline Requires<
	!CHasLength<R> &&
	CAsConsumableRange<R>,
size_t> Count(R&& range)
{
	auto rangeCopy = ForwardAsRange<R>(range);
	return CountAdvance(rangeCopy);
}


template<typename R> INTRA_NODISCARD constexpr forceinline Requires<
	CHasLength<TRangeOfType<R>>,
index_t> LengthOr(R&& range, index_t defaultValue = 0) {(void)defaultValue; return ForwardAsRange<R>(range).Length();}


template<typename R> INTRA_NODISCARD constexpr forceinline Requires<
	!CHasLength<TRangeOfType<R>>,
index_t> LengthOr(R&& range, index_t defaultValue = 0) {(void)range; return defaultValue;}


template<typename R> INTRA_NODISCARD constexpr forceinline Requires<
	CHasEmpty<TRangeOfType<R>>,
bool> EmptyOr(R&& range, bool defaultValue = false) {(void)defaultValue; return ForwardAsRange<R>(range).Empty();}


template<typename R> INTRA_NODISCARD constexpr forceinline Requires<
	!CHasEmpty<TRangeOfType<R>>,
bool> EmptyOr(R&& range, bool defaultValue = false) {(void)range; return defaultValue;}

template<typename R> INTRA_NODISCARD constexpr forceinline Requires<
	CHasFull<TRangeOfType<R>>,
bool> FullOr(R&& range, bool defaultValue = false) {(void)defaultValue; return ForwardAsRange<R>(range).Full();}


template<typename R> constexpr forceinline Requires<
	!CHasFull<TRangeOfType<R>>,
bool> FullOr(R&& range, bool defaultValue = false) {(void)range; return defaultValue;}



template<typename R> constexpr forceinline Requires<
	CSliceable<R> && CHasLength<R> &&
	!CConst<R> &&
	!CHasPopFirstN<R>
> PopFirstExactly(R& range, size_t elementsToPop)
{range = range(elementsToPop, range.Length());}

template<typename R> constexpr forceinline Requires<
	CInputRange<R> &&
	!(CSliceable<R> && CHasLength<R>) &&
	!CConst<R> &&
	!CHasPopFirstN<R>
> PopFirstExactly(R& range, index_t elementsToPop)
{while(elementsToPop --> 0) range.PopFirst();}

template<typename R> constexpr forceinline Requires<
	CHasPopFirstN<R>
> PopFirstExactly(R& range, index_t elementsToPop)
{
	const index_t poppedElements = range.PopFirstN(elementsToPop);
	INTRA_DEBUG_ASSERT(poppedElements == elementsToPop);
	(void)poppedElements;
}


template<typename R> constexpr Requires<
	!CConst<R> &&
	!CHasPopFirstN<R> &&
	CFiniteInputRange<R> &&
	CSliceable<R>,
index_t> PopFirstN(R& range, index_t n)
{
	const index_t l = Count(range);
	const index_t elementsToPop = n<l? n: l;
	MoveAssign(range, range(elementsToPop, l));
	return elementsToPop;
}

template<typename R> constexpr Requires<
	CInputRange<R> &&
	!CConst<R> &&
	!(CSliceable<R> &&
		CFiniteRange<R>) &&
	!CHasPopFirstN<R>,
index_t> PopFirstN(R& range, index_t n)
{
	size_t elementsToPop = 0;
	while(!range.Empty() && n != 0)
		range.PopFirst(), n--, elementsToPop++;
	return elementsToPop;
}

template<typename R> constexpr forceinline Requires<
	CHasPopFirstN<R>,
index_t> PopFirstN(R& range, index_t n)
{return range.PopFirstN(n);}


template<typename R> constexpr forceinline Requires<
	CInputRange<R> &&
	!CConst<R> &&
	CSliceable<R> &&
	!CHasPopLastN<R>
> PopLastExactly(R& range, index_t elementsToPop)
{range = range(0, range.Length()-elementsToPop);}

template<typename R> constexpr forceinline Requires<
	CBidirectionalRange<R> &&
	!CConst<R> &&
	!CSliceable<R> &&
	!CHasPopLastN<R>
> PopLastExactly(R& range, index_t elementsToPop)
{while(elementsToPop --> 0) range.PopLast();}

template<typename R> constexpr forceinline Requires<
	CHasPopLastN<R>
> PopLastExactly(R& range, index_t elementsToPop)
{
	const index_t poppedElements = range.PopLastN(elementsToPop);
	INTRA_DEBUG_ASSERT(poppedElements == elementsToPop);
	(void)poppedElements;
}


template<typename R> constexpr Requires<
	CInputRange<R> &&
	!CConst<R> &&
	CSliceable<R> &&
	!CHasPopLastN<R>,
size_t> PopLastN(R& range, index_t n)
{
	const index_t l = Count(range);
	const index_t elementsToPop = n<l? n: l;
	range = range(0, l-elementsToPop);
	return elementsToPop;
}

template<typename R> constexpr Requires<
	CBidirectionalRange<R> &&
	!CConst<R> &&
	!CSliceable<R> &&
	!CHasPopLastN<R>,
index_t> PopLastN(R& range, index_t n)
{
	index_t elementsToPop = 0;
	while(!range.Empty() && n != 0)
		range.PopLast(), n--, elementsToPop++;
	return elementsToPop;
}

template<typename R> constexpr forceinline Requires<
	CHasPopLastN<R>,
index_t> PopLastN(R& range, index_t n)
{return range.PopLastN(n);}

//! Take last ``n`` elements of ``range``.
//! @return Reference to ``range``.
template<typename R> constexpr Requires<
	CForwardRange<R> &&
	!CHasLength<R>,
R&&> TailAdvance(R&& range, index_t n)
{
	R temp = range;
	PopFirstN(temp, n);
	while(!temp.Empty())
	{
		temp.PopFirst();
		range.PopFirst();
	}
	return Forward<R>(range);
}

template<typename R> constexpr forceinline Requires<
	CInputRange<R> &&
	CHasLength<R>,
R&&> TailAdvance(R&& range, index_t n)
{
	if(range.Length() > n) PopFirstExactly(range, range.Length() - n);
	return Forward<R>(range);
}

//! @return range containing last ``n`` elements of ``range``.
template<typename R> INTRA_NODISCARD constexpr forceinline Requires<
	CAsNonInfiniteForwardRange<R> &&
	!CSliceable<R>,
TRemoveConstRef<TRangeOfType<R>>> Tail(R&& range, index_t n)
{
	auto rangeCopy = ForwardAsRange<R>(range);
	return TailAdvance(rangeCopy, n);
}

template<typename R> INTRA_NODISCARD constexpr forceinline Requires<
	CInputRange<R> &&
	CSliceable<R>,
R> Tail(const R& range, index_t n)
{
	const index_t len = range.Length();
	return range(len > n? len - n: 0, len);
}



template<typename R> INTRA_NODISCARD constexpr forceinline Requires<
	CAsAccessibleRange<R>,
TRangeOfTypeNoCRef<R>> Drop(R&& range)
{
	if(range.Empty()) return ForwardAsRange<R>(range);
	auto result = ForwardAsRange<R>(range);
	result.PopFirst();
	return result;
}

template<typename R> INTRA_NODISCARD constexpr forceinline Requires<
	CAsAccessibleRange<R>,
TRangeOfTypeNoCRef<R>> Drop(R&& range, index_t n)
{
	auto result = ForwardAsRange<R>(range);
	PopFirstN(result, n);
	return result;
}

template<typename R> INTRA_NODISCARD constexpr forceinline Requires<
	CAsAccessibleRange<R>,
TRangeOfTypeNoCRef<R>> DropExactly(R&& range)
{
	auto result = ForwardAsRange<R>(range);
	result.PopFirst();
	return result;
}

template<typename R> INTRA_NODISCARD constexpr forceinline Requires<
	CAsAccessibleRange<R>,
TRangeOfTypeNoCRef<R>> DropExactly(R&& range, index_t n)
{
	auto result = ForwardAsRange<R>(range);
	PopFirstExactly(result, n);
	return result;
}

template<typename R,
	typename AsR = TRangeOfType<R>
> INTRA_NODISCARD constexpr forceinline Requires<
	CInputRange<AsR> &&
	CHasIndex<AsR>,
TReturnValueTypeOf<AsR>> AtIndex(R&& range, index_t index)
{return ForwardAsRange<R>(range)[index];}

template<typename R,
	typename AsR = TRangeOfType<R>
> INTRA_NODISCARD constexpr forceinline Requires<
	CAccessibleRange<AsR> &&
	!CHasIndex<AsR>,
TReturnValueTypeOf<AsR>> AtIndex(R&& range, index_t index)
{return DropExactly(ForwardAsRange<R>(range), index).First();}


template<typename R,
	typename AsR = TRangeOfTypeNoCRef<R>
> INTRA_NODISCARD constexpr forceinline Requires<
	CBidirectionalRange<AsR>,
AsR> DropLast(R&& range)
{
	if(range.Empty()) return ForwardAsRange<R>(range);
	auto result = ForwardAsRange<R>(range);
	result.PopLast();
	return result;
}

template<typename R,
	typename AsR = TRangeOfTypeNoCRef<R>
> INTRA_NODISCARD constexpr forceinline Requires<
	CBidirectionalRange<AsR>,
AsR> DropLast(R&& range, size_t n)
{
	auto result = ForwardAsRange<R>(range);
	PopLastN(result, n);
	return result;
}

template<typename R,
	typename AsR = TRangeOfTypeNoCRef<R>
> INTRA_NODISCARD constexpr forceinline Requires<
	CBidirectionalRange<AsR>,
AsR> DropLastExactly(R&& range)
{
	auto result = ForwardAsRange<R>(range);
	result.PopLast();
	return result;
}

template<typename R,
	typename AsR = TRangeOfTypeNoCRef<R>
> INTRA_NODISCARD constexpr forceinline Requires<
	CBidirectionalRange<AsR>,
AsR> DropLastExactly(R&& range, size_t n)
{
	auto result = ForwardAsRange<R>(range);
	PopLastExactly(result, n);
	return result;
}

INTRA_DEFINE_CONCEPT_REQUIRES(CHasNext, static_cast<TReturnValueTypeOf<T>>(Val<T>().Next()));
template<typename R> INTRA_NODISCARD constexpr forceinline Requires<
	CHasNext<R>,
TReturnValueTypeOf<R>> Next(R&& range) {return range.Next();}

template<typename R> INTRA_NODISCARD constexpr forceinline Requires<
	!CHasNext<R> && CInputRange<R>,
TReturnValueTypeOf<R>> Next(R&& range)
{
	auto&& res = range.First();
	range.PopFirst();
	return res;
}

template<typename R> INTRA_NODISCARD constexpr forceinline Requires<
	CHasNext<R> || CInputRange<R>,
Optional<TReturnValueTypeOfAs<R>>> TryNext(R&& range)
{
	if(range.Empty()) return null;
	return range.Next();
}

template<typename R, typename T> constexpr forceinline Requires<
	CHasPut<R, T>
> Put(R&& range, T&& val) {return range.Put(val);}

template<typename R, typename T> INTRA_NODISCARD constexpr forceinline Requires<
	!CHasPut<R, T> && CAssignableRange<R>
> Put(R&& range, T&& val)
{
	range.First() = Forward<T>(val);
	range.PopFirst();
}
INTRA_END
