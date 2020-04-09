#pragma once

#include "Intra/Assert.h"
#include "Intra/Range/Concepts.h"
#include "Intra/Range/Operations.h"

INTRA_BEGIN
INTRA_IGNORE_WARNING_COPY_MOVE_CONSTRUCT_IMPLICITLY_DELETED

INTRA_DEFINE_CONCEPT_REQUIRES(CHasTakeMethod, Val<T>().Take(index_t()));

template<typename R> struct RTake: CopyableIf<!CReference<R>>
{
	static constexpr bool IsAnyInstanceFinite = true;

	constexpr RTake() = default;

	template<typename R2> constexpr RTake(R2&& range, ClampedSize count):
		mOriginalRange(Forward<R2>(range)) {set_len(count);}

	/*RTake(RTake&&) = default;
	RTake(const RTake&) = default;
	RTake& operator=(RTake&&) = default;
	RTake& operator=(const RTake&) = default;*/

	[[nodiscard]] constexpr bool Empty() const
	{
		return mLen == 0 ||
			!CHasLength<TRemoveConstRef<R>> &&
			!CInfiniteRange<TRemoveConstRef<R>> &&
			mOriginalRange.Empty();
	}


	[[nodiscard]] constexpr TReturnValueTypeOf<R> First() const
	{
		INTRA_PRECONDITION(!Empty());
		return mOriginalRange.First();
	}

	constexpr void PopFirst()
	{
		INTRA_PRECONDITION(!Empty());
		mOriginalRange.PopFirst();
		mLen--;
	}
	
	template<typename U = R> [[nodiscard]] constexpr Requires<
		CHasIndex<U>,
	TReturnValueTypeOf<U>> Last() const
	{
		INTRA_PRECONDITION(!Empty());
		return mOriginalRange[mLen-1];
	}

	template<typename U = R> constexpr Requires<
		CHasIndex<U>
	> PopLast() {mLen--;}

	template<typename U = R> [[nodiscard]] constexpr Requires<
		CHasIndex<U>,
	TReturnValueTypeOf<U>> operator[](Index index) const
	{
		INTRA_PRECONDITION(size_t(index) < mLen);
		return mOriginalRange[index];
	}

	template<typename U = R> [[nodiscard]] constexpr Requires<
		CHasLength<U> ||
		CInfiniteRange<U>,
	index_t> Length() const noexcept {return index_t(mLen);}

	[[nodiscard]] constexpr index_t LengthLimit() const noexcept {return index_t(mLen);}
	
	[[nodiscard]] constexpr RTake Take(Size count) const
	{
		return Take(mOriginalRange, FMin(size_t(count), mLen));
	}

private:
	R mOriginalRange;
	size_t mLen = 0;

	template<typename U = R> constexpr Requires<
		CHasLength<U>
	> set_len(ClampedSize maxLen)
	{
		mLen = size_t(maxLen);
		mLen = FMin(mLen, size_t(mOriginalRange.Length()));
	}

	template<typename U = R> constexpr Requires<
		!CHasLength<U>
	> set_len(ClampedSize maxLen) {mLen = size_t(maxLen);}
};


template<typename R> [[nodiscard]] constexpr Requires<
	CAsInputRange<R> &&
	!CHasTakeMethod<R>,
RTake<TRangeOf<R>>> Take(R&& range, ClampedSize count)
{return {ForwardAsRange<R>(range), count};}

template<typename R, typename = Requires<
	CHasTakeMethod<R>
>> [[nodiscard]] constexpr auto Take(R&& range, ClampedSize count) {return range.Take(count);}


namespace z_D {
INTRA_DEFINE_CONCEPT_REQUIRES(CHasTake, Take(Val<T>(), ClampedSize()));

template<typename R, bool = CHasTake<R>> struct TTakeResult_
{typedef decltype(Take(Val<R>(), ClampedSize())) _;};

template<typename R> struct TTakeResult_<R, false>
{typedef void _;};
}

template<typename R> using TTakeResult = typename z_D::TTakeResult_<R>::_;

INTRA_END
