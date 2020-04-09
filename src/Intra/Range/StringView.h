#pragma once

#include "Intra/Concepts.h"
#include "Intra/Misc/RawMemory.h"
#include "Intra/Range/Span.h"
#include "Intra/TypeSafe.h"
#include "Intra/Range/Special/Unicode.h"
#include "Intra/Range/Operations.h"

INTRA_BEGIN
//! Not-owning reference to string.
template<typename Char> class GenericStringView
{
	static_assert(CChar<Char>);
	CSpan<Char> mData;
public:
    [[nodiscard]] constexpr auto RawUnicodeUnits() const {return mData;}

	constexpr GenericStringView() = default;

    template<class R, class = Requires<CArrayClass<R> && CSame<Char, TArrayElement<R>>>>
	explicit constexpr GenericStringView(R&& rhs): mData(rhs) {}

	explicit constexpr GenericStringView(decltype(ConstructFromPtr), const Char* begin, Size length):
	    mData(ConstructFromPtr, begin, length) {}

    explicit constexpr GenericStringView(decltype(ConstructFromPtr), const Char* begin, const char* end):
        mData(ConstructFromPtr, begin, end) {}

	//! Construct from null-terminated C-string.
	//! Null terminator itself will not be a part of constructed view.
    explicit constexpr GenericStringView(decltype(ConstructFromPtr), const Char* nullTermStr):
        mData(ConstructFromPtr, nullTermStr, nullTermStr? Misc::CStringLength(nullTermStr): 0) {}

	///@{
	//! String comparison
	[[nodiscard]] constexpr bool operator==(const GenericStringView& rhs) const noexcept
	{
		return Length() == rhs.Length() &&
			__builtin_memcmp(Data(), rhs.Data(), Length()*sizeof(Char)) == 0;
	}
	[[nodiscard]] constexpr bool operator!=(const GenericStringView& rhs) const noexcept {return !operator==(rhs);}
	[[nodiscard]] constexpr bool operator<(const GenericStringView& rhs) const noexcept {return *this|LexCompare(rhs) < 0;}
    [[nodiscard]] constexpr bool operator<=(const GenericStringView& rhs) const noexcept {return *this|LexCompare(rhs) <= 0;}
    [[nodiscard]] constexpr bool operator>(const GenericStringView& rhs) const noexcept {return *this|LexCompare(rhs) > 0;}
    [[nodiscard]] constexpr bool operator>=(const GenericStringView& rhs) const noexcept {return *this|LexCompare(rhs) >= 0;}
	///@}

	[[nodiscard]] constexpr char32_t First() const
	{
	    if constexpr(CSame<Char, char32_t>) return mData.First();
	    else return Unicode::DecodeCodepoint(StringData());
	}

	constexpr void PopFirst()
	{
        if constexpr(CSame<Char, char32_t>) return mData.PopFirst();
	    else mData|PopFirstExactly(Unicode::ValidContinuationBytes(mdata.First()));
	}

    constexpr index_t PopFirstCount(ClampedSize maxCharsToPop)
    {
        if constexpr(CSame<Char, char32_t>) return mData.PopFirstCount(maxCharsToPop);
        else return *this|PopFirstCount(maxCharsToPop);
    }

    [[nodiscard]] constexpr GenericStringView DropCodeUnits(ClampedSize count) const
    {
        return GenericStringView(mData|DropLast(count));
    }

	[[nodiscard]] constexpr GenericStringView DropLastCodeUnits(ClampedSize count) const
	{
	    return GenericStringView(mData|DropLast(count));
	}

	[[nodiscard]] constexpr GenericStringView TakeCodeUnits(ClampedSize count) const
	{
	    return GenericStringView(mData|Take(count));
	}

	[[nodiscard]] constexpr GenericStringView TailCodeUnits(ClampedSize count) const
	{
	    return GenericStringView(mData|Tail(count));
	}
};
using StringView = GenericStringView<char>;

[[nodiscard]] constexpr StringView operator"" _v(const char* str, size_t len) noexcept
{
    return StringView(ConstructFromPtr, str, len);
}

[[nodiscard]] constexpr auto operator"" _v(const wchar_t* str, size_t len) noexcept
{
    return GenericStringView(ConstructFromPtr, str, len);
}

[[nodiscard]] constexpr auto operator"" _v(const char16_t* str, size_t len) noexcept
{
    return GenericStringView(ConstructFromPtr, str, len);
}

[[nodiscard]] constexpr auto operator"" _v(const char32_t* str, size_t len) noexcept
{
    return GenericStringView(ConstructFromPtr, str, len);
}

#ifdef __cpp_char8_t
[[nodiscard]] constexpr auto operator"" _v(const char8_t* str, size_t len) noexcept
{
    return GenericStringView(ConstructFromPtr, str, len);
}
#endif

#if INTRA_CONSTEXPR_TEST
static_assert(CForwardRange<StringView>);
static_assert(CFiniteInputRange<StringView>);
static_assert("test string"_v != "test string 2"_v);
static_assert("test string"_v == "test string"_v);
static_assert("test string"_v|Take(4) == "test"_v);
static_assert("тестовая строка"_v|Take(4) == "тест"_v);
static_assert("тестовая строка"_v < "тестовая строка."_v);
static_assert("тестовая строкА"_v < "тестовая строка"_v);
#endif

INTRA_END
