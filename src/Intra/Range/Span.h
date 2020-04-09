#pragma once

#include "Intra/Assert.h"
#include "Intra/Concepts.h"
#include "Intra/Functional.h"
#include "Intra/TypeSafe.h"

INTRA_BEGIN
constexpr struct {} ConstructFromPtr;

//! Non-owning reference to an array.
template<typename T> struct Span
{
	constexpr Span() = default;

	template<class R, class = Requires<CArrayClass<R>>>
	constexpr Span(R&& arr) noexcept:
	    Begin(DataOf(arr)), End(Begin + LengthOf(arr)) {INTRA_PRECONDITION(LengthOf(arr) >= 0);}

	constexpr Span(decltype(ConstructFromPtr), T* begin, T* end) noexcept:
		Begin(begin), End(end) {INTRA_PRECONDITION(end >= begin);}

	constexpr Span(decltype(ConstructFromPtr), T* begin, Size length) noexcept:
		Begin(begin), End(Begin + size_t(length)) {}

	[[nodiscard]] constexpr index_t Length() const noexcept {return End - Begin;}
	[[nodiscard]] constexpr bool Empty() const noexcept {return Begin < End;}
	[[nodiscard]] constexpr T* Data() const noexcept {return Begin;}

	[[nodiscard]] constexpr T& First() const
	{
	    INTRA_PRECONDITION(!Empty());
	    return *Begin;
	}

	constexpr void PopFirst()
	{
	    INTRA_PRECONDITION(!Empty());
	    Begin++;
	}

	[[nodiscard]] constexpr T& Last() const
	{
	    INTRA_PRECONDITION(!Empty());
	    return End[-1];
	}

	constexpr void PopLast()
	{
	    INTRA_PRECONDITION(!Empty());
	    End--;
	}

	constexpr index_t PopFirstCount(ClampedSize count) noexcept
	{
		const auto poppedElements = FMin(index_t(count), Length());
		Begin += poppedElements;
		return poppedElements;
	}

	constexpr index_t PopLastCount(ClampedSize count) noexcept
	{
		const auto poppedElements = FMin(index_t(count), Length());
		End -= poppedElements;
		return poppedElements;
	}

	[[nodiscard]] constexpr Span Take(ClampedSize count) const noexcept
	{
	    return Span(ConstructFromPtr, Begin, FMin(index_t(count), Length()));
	}

	[[nodiscard]] constexpr T& operator[](Index index) const
	{
		INTRA_PRECONDITION(index < Length());
		return Begin[size_t(index)];
	}

	T* Begin = null;
	T* End = null;
};
template<typename T> using CSpan = Span<const T>;

/** This class is supposed to be used as an output range or a stream.

  Unlike Span, it remembers its original position,
  which allows to request the range of all written elements.
  It also can be considered as an input range of not yet written elements.
*/
template<typename T> class SpanOutput: public Span<T>
{
	T* mBegin = null;
	
	// Don't directly modify base Span!
	using Span<T>::Begin;
	using Span<T>::End;
	using Span<T>::Empty;
public:
	SpanOutput() = default;

	template<typename R, typename = Requires<CArrayClass<R> && CSame<TArrayElement<R>, T>>>
	constexpr SpanOutput(R&& dst): Span<T>(dst), mBegin(Begin) {}

	//! Reset this range to its original state to overwrite all elements written earlier.
	constexpr void Reset() noexcept {Begin = mBegin;}

	//! Get a range of all written data.
	[[nodiscard]] constexpr Span<T> WrittenRange() const noexcept
	{
		return Span<T>(ConstructFromPtr, mBegin, Span<T>::Begin);
	}

	//! @return Number of elements written after last Reset() or construction.
	[[nodiscard]] constexpr index_t Position() const noexcept {return Begin - mBegin;}

	[[nodiscard]] constexpr bool IsInitialized() const noexcept {return mBegin != null;}
};
INTRA_END
