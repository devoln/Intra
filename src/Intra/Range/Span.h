#pragma once

#include "Intra/Assert.h"
#include "Intra/Concepts.h"
#include "Intra/Functional.h"
#include "Intra/TypeSafe.h"

INTRA_BEGIN
/// Non-owning reference to an array.
template<typename T> struct Span
{
	constexpr Span() = default;

	template<CArrayList R> constexpr Span(R&& arr) noexcept: Span(Unsafe, DataOf(arr), LengthOf(arr)) {}

	constexpr Span(TUnsafe, T* begin, T* end) noexcept:
		Begin(begin), End(end) {INTRA_PRECONDITION(end >= begin);}

	constexpr Span(TUnsafe, T* begin, Size length) noexcept:
		Begin(begin), End(Begin + size_t(length)) {}

	[[nodiscard]] constexpr index_t Length() const noexcept {return End - Begin;}
	[[nodiscard]] constexpr bool Empty() const noexcept {return Begin >= End;}
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
		const auto poppedElements = Min(index_t(count), Length());
		Begin += poppedElements;
		return poppedElements;
	}

	constexpr index_t PopLastCount(ClampedSize count) noexcept
	{
		const auto poppedElements = Min(index_t(count), Length());
		End -= poppedElements;
		return poppedElements;
	}

	[[nodiscard]] constexpr Span Take(ClampedSize count) const noexcept
	{
	    return Span(Unsafe, Begin, Min(index_t(count), Length()));
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
template<class R> Span(R&&) -> Span<TArrayElementKeepConst<R>>;

template<CArrayList R> [[nodiscard]] constexpr auto CSpanOf(R&& r) noexcept {return CSpan<TListValue<R>>(r);}

inline namespace Literals {
[[nodiscard]] constexpr auto operator""_span(const char* str, size_t len) noexcept
{
	return Span(Unsafe, str, len);
}

[[nodiscard]] constexpr auto operator""_span(const wchar_t* str, size_t len) noexcept
{
	return Span(Unsafe, str, len);
}

[[nodiscard]] constexpr auto operator""_span(const char16_t* str, size_t len) noexcept
{
	return Span(Unsafe, str, len);
}

[[nodiscard]] constexpr auto operator""_span(const char32_t* str, size_t len) noexcept
{
	return Span(Unsafe, str, len);
}

#ifdef __cpp_char8_t
[[nodiscard]] constexpr auto operator""_span(const char8_t* str, size_t len) noexcept
{
	return Span(Unsafe, str, len);
}
#endif
}

/** This class is supposed to be used as an output range or a stream.

  Unlike Span, it remembers its original position,
  which allows to request the range of all written elements.
  It also can be considered as an input range of not yet written elements.
*/
template<typename T> class SpanOutput: private Span<T>
{
	T* mBegin = null;
public:
	SpanOutput() = default;

	template<CArrayList R> requires CSame<TArrayElement<R>, T>
	constexpr SpanOutput(R&& dst): Span<T>(dst), mBegin(Span<T>::Begin) {}

	/// Reset this range to its original state to overwrite all elements written earlier.
	constexpr void Reset() noexcept {Span<T>::Begin = mBegin;}

	/// Get a range of all written data.
	[[nodiscard]] constexpr Span<T> WrittenRange() const noexcept
	{
		return Span<T>(Unsafe, mBegin, Span<T>::Begin);
	}

	/// @return Number of elements written after last Reset() or construction.
	[[nodiscard]] constexpr index_t Position() const noexcept {return Span<T>::Begin - mBegin;}

	[[nodiscard]] constexpr bool IsInitialized() const noexcept {return mBegin != null;}

	[[nodiscard]] constexpr bool Full() const {return Span<T>::Empty();}

	using Span<T>::Data;
	using Span<T>::Length;
	using Span<T>::PopFirst;
	using Span<T>::PopFirstCount;

	template<CMoveAssignable U> void Put(U&& value)
	{
		INTRA_PRECONDITION(!Full());
		*Span<T>::Begin++ = INTRA_FWD(value);
	}
};
INTRA_END
