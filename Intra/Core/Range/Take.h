#pragma once

#include "Core/Range/Concepts.h"
#include "Core/Assert.h"
#include "Core/Range/Operations.h"

INTRA_CORE_RANGE_BEGIN
INTRA_WARNING_DISABLE_COPY_MOVE_IMPLICITLY_DELETED

INTRA_DEFINE_CONCEPT_REQUIRES(CHasTakeMethod, Val<T>().Take(size_t()));

template<typename R> struct RTake: CopyableIf<!CReference<R>>
{
	enum: bool {RangeIsFinite = true};

	template<typename R2> INTRA_CONSTEXPR2 forceinline RTake(R2&& range, size_t count):
		mOriginalRange(Forward<R2>(range)) {set_len(count);}

	RTake(RTake&&) = default;
	RTake(const RTake&) = default;

	template<typename U = TRemoveConstRef<R>> INTRA_NODISCARD constexpr forceinline Requires<
		CHasLength<U> ||
		CInfiniteRange<U>,
	bool> Empty() const {return mLen == 0;}

	template<typename U = TRemoveConstRef<R>> INTRA_NODISCARD constexpr forceinline Requires<
		!CHasLength<U> &&
		!CInfiniteRange<U>,
	bool> Empty() const {return mLen == 0 || mOriginalRange.Empty();}


	INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline TReturnValueTypeOf<R> First() const
	{
		INTRA_DEBUG_ASSERT(!Empty());
		return mOriginalRange.First();
	}

	INTRA_CONSTEXPR2 forceinline void PopFirst()
	{mOriginalRange.PopFirst(); mLen--;}
	
	INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline auto Last() const
	{
		INTRA_DEBUG_ASSERT(!Empty());
		return mOriginalRange[mLen-1];
	}

	INTRA_CONSTEXPR2 forceinline void PopLast() {mLen--;}

	template<typename U = R> INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline Requires<
		CHasIndex<U>,
	TReturnValueTypeOf<U>> operator[](size_t index) const
	{
		INTRA_DEBUG_ASSERT(index < mLen);
		return mOriginalRange[index];
	}

	template<typename U = R> INTRA_NODISCARD constexpr forceinline Requires<
		CHasLength<U> ||
		CInfiniteRange<U>,
	index_t> Length() const noexcept {return mLen;}

	INTRA_NODISCARD constexpr forceinline size_t LengthLimit() const noexcept {return mLen;}

	template<typename U = R> INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline Requires<
		CSliceable<U>,
	TSliceTypeOf<U>> operator()(size_t startIndex, size_t endIndex) const
	{
		INTRA_DEBUG_ASSERT(startIndex <= endIndex);
		INTRA_DEBUG_ASSERT(endIndex <= mLen);
		return mOriginalRange(startIndex, endIndex);
	}

	
	INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline RTake Take(size_t count) const
	{
		if(count > mLen) count = mLen;
		return Take(mOriginalRange, count);
	}

private:
	R mOriginalRange;
	size_t mLen;

	template<typename U = R> INTRA_CONSTEXPR2 forceinline Requires<
		CHasLength<U>
	> set_len(size_t maxLen)
	{
		mLen = mOriginalRange.Length();
		if(mLen > maxLen) mLen = maxLen;
	}

	template<typename U = R> INTRA_CONSTEXPR2 forceinline Requires<
		!CHasLength<U>
	> set_len(size_t maxLen) {mLen = maxLen;}
};


template<typename R> INTRA_NODISCARD constexpr forceinline Requires<
	CInputRange<R> &&
	!(CSliceable<R> && CHasLength<R>) &&
	!CHasTakeMethod<R>,
RTake<TRemoveConstRef<R>>> Take(R&& range, size_t count)
{return {Forward<R>(range), count};}

template<typename R> INTRA_NODISCARD constexpr forceinline Requires<
	CInputRange<R> &&
	CSliceable<R> && CHasLength<R> &&
	!CHasTakeMethod<R>,
TSliceTypeOf<R>> Take(R&& range, size_t count)
{return Forward<R>(range)(0, range.Length() > count? count: range.Length());}

template<typename R, typename = Requires<
	CInputRange<R> && CHasTakeMethod<R>
>> INTRA_NODISCARD constexpr forceinline auto Take(R&& range, size_t count)
{return range.Take(count);}


namespace z__R {

INTRA_DEFINE_CONCEPT_REQUIRES(CHasTake, Take(Val<T>(), size_t()));

template<typename R, bool= CHasTake<R>> struct TTakeResult_
{typedef decltype(Take(Val<R>(), size_t())) _;};

template<typename R> struct TTakeResult_<R, false>
{typedef void _;};

}

template<typename R> using TTakeResult = typename z__R::TTakeResult_<R>::_;

template<typename R> INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline Requires<
	!CInputRange<R> && CAsInputRange<R>,
TTakeResult<TRangeOfType<R>>> Take(R&& range, size_t count)
{return Take(ForwardAsRange<R>(range), count);}
INTRA_CORE_RANGE_END
