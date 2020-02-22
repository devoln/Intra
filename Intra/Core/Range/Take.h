#pragma once

#include "Core/Range/Concepts.h"
#include "Core/Assert.h"
#include "Core/Range/Operations.h"

INTRA_BEGIN
INTRA_WARNING_DISABLE_COPY_MOVE_CONSTRUCT_IMPLICITLY_DELETED

INTRA_DEFINE_CONCEPT_REQUIRES(CHasTakeMethod, Val<T>().Take(size_t()));

template<typename R> struct RTake: CopyableIf<!CReference<R>>
{
	enum: bool {RangeIsFinite = true};

	constexpr RTake() = default;

	template<typename R2> constexpr forceinline RTake(R2&& range, index_t count):
		mOriginalRange(Forward<R2>(range)) {set_len(count);}

	/*RTake(RTake&&) = default;
	RTake(const RTake&) = default;
	RTake& operator=(RTake&&) = default;
	RTake& operator=(const RTake&) = default;*/

	template<typename U = TRemoveConstRef<R>> INTRA_NODISCARD constexpr forceinline Requires<
		CHasLength<U> ||
		CInfiniteRange<U>,
	bool> Empty() const {return mLen == 0;}

	template<typename U = TRemoveConstRef<R>> INTRA_NODISCARD constexpr forceinline Requires<
		!CHasLength<U> &&
		!CInfiniteRange<U>,
	bool> Empty() const {return mLen == 0 || mOriginalRange.Empty();}


	INTRA_NODISCARD constexpr forceinline TReturnValueTypeOf<R> First() const
	{
		INTRA_PRECONDITION(!Empty());
		return mOriginalRange.First();
	}

	constexpr forceinline void PopFirst()
	{
		mOriginalRange.PopFirst();
		mLen--;
	}
	
	template<typename U = R> INTRA_NODISCARD constexpr forceinline Requires<
		CHasIndex<U>,
	TReturnValueTypeOf<U>> Last() const
	{
		INTRA_PRECONDITION(!Empty());
		return mOriginalRange[mLen-1];
	}

	template<typename U = R> constexpr forceinline Requires<
		CHasIndex<U>
	> PopLast() {mLen--;}

	template<typename U = R> INTRA_NODISCARD constexpr forceinline Requires<
		CHasIndex<U>,
	TReturnValueTypeOf<U>> operator[](index_t index) const
	{
		INTRA_PRECONDITION(index < mLen);
		return mOriginalRange[index];
	}

	template<typename U = R> INTRA_NODISCARD constexpr forceinline Requires<
		CHasLength<U> ||
		CInfiniteRange<U>,
	index_t> Length() const noexcept {return mLen;}

	INTRA_NODISCARD constexpr forceinline size_t LengthLimit() const noexcept {return mLen;}

	template<typename U = R> INTRA_NODISCARD constexpr forceinline Requires<
		CSliceable<U>,
	TSliceTypeOf<U>> operator()(index_t startIndex, index_t endIndex) const
	{
		INTRA_PRECONDITION(startIndex <= endIndex);
		INTRA_PRECONDITION(endIndex <= mLen);
		return mOriginalRange(startIndex, endIndex);
	}

	
	INTRA_NODISCARD constexpr forceinline RTake Take(index_t count) const
	{
		if(count > mLen) count = mLen;
		return Take(mOriginalRange, count);
	}

private:
	R mOriginalRange;
	index_t mLen = 0;

	template<typename U = R> constexpr forceinline Requires<
		CHasLength<U>
	> set_len(index_t maxLen)
	{
		mLen = mOriginalRange.Length();
		if(mLen > maxLen) mLen = maxLen;
	}

	template<typename U = R> constexpr forceinline Requires<
		!CHasLength<U>
	> set_len(index_t maxLen) {mLen = maxLen;}
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

template<typename R, bool = CHasTake<R>> struct TTakeResult_
{typedef decltype(Take(Val<R>(), size_t())) _;};

template<typename R> struct TTakeResult_<R, false>
{typedef void _;};

}

template<typename R> using TTakeResult = typename z__R::TTakeResult_<R>::_;

template<typename R> INTRA_NODISCARD constexpr forceinline Requires<
	!CInputRange<R> && CAsInputRange<R>,
TTakeResult<TRangeOfType<R>>> Take(R&& range, size_t count)
{return Take(ForwardAsRange<R>(range), count);}
INTRA_END
