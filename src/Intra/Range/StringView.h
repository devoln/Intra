#pragma once

#include "Intra/Concepts.h"
#include "Intra/Misc/RawMemory.h"
#include "Intra/Range/Span.h"
#include "Intra/TypeSafe.h"
#include "Intra/Range/Special/Unicode.h"
#include "Intra/Range/Operations.h"

INTRA_BEGIN
/// Not-owning reference to a string.
template<typename CodeUnit> class GenericStringView
{
	static_assert(CChar<CodeUnit>);
	CSpan<CodeUnit> mRawUnicodeUnits;
public:
	static constexpr bool IsAnyInstanceFinite = true;

    [[nodiscard]] constexpr auto RawUnicodeUnits() const noexcept {return mRawUnicodeUnits;}

	constexpr GenericStringView() = default;

    template<CArrayList R> requires CSame<CodeUnit, TArrayElement<R>>
	explicit constexpr GenericStringView(R&& src): GenericStringView(Unsafe, src)
	{
		INTRA_PRECONDITION(Unicode::IsValidUnicode(src));
	}

	template<CArrayList R> requires CSame<CodeUnit, TArrayElement<R>>
	explicit constexpr GenericStringView(TUnsafe, R&& rhs) noexcept: mRawUnicodeUnits(rhs) {}

	explicit constexpr GenericStringView(TUnsafe, const CodeUnit* begin, Size length):
	    mRawUnicodeUnits(Unsafe, begin, length) {}

    explicit constexpr GenericStringView(TUnsafe, const CodeUnit* begin, const CodeUnit* end):
        mRawUnicodeUnits(Unsafe, begin, end) {}

	/// Construct from null-terminated C-string.
	/// Null terminator itself will not be a part of the constructed view.
    explicit constexpr GenericStringView(TUnsafe, const CodeUnit* nullTermStr):
		GenericStringView(Unsafe, nullTermStr, nullTermStr? CStringLength(nullTermStr): 0) {}

	[[nodiscard]] constexpr bool operator==(const GenericStringView& rhs) const noexcept
	{
		return mRawUnicodeUnits|MatchesWith(rhs.mRawUnicodeUnits);
	}
	[[nodiscard]] constexpr bool operator!=(const GenericStringView& rhs) const noexcept {return !operator==(rhs);}
	[[nodiscard]] constexpr bool operator<(const GenericStringView& rhs) const noexcept {return (*this|LexCompareTo(rhs)) < 0;}
    [[nodiscard]] constexpr bool operator<=(const GenericStringView& rhs) const noexcept {return (*this|LexCompareTo(rhs)) <= 0;}
    [[nodiscard]] constexpr bool operator>(const GenericStringView& rhs) const noexcept {return (*this|LexCompareTo(rhs)) > 0;}
    [[nodiscard]] constexpr bool operator>=(const GenericStringView& rhs) const noexcept {return (*this|LexCompareTo(rhs)) >= 0;}

	[[nodiscard]] constexpr char32_t First() const
	{
	    if constexpr(CSame<CodeUnit, char32_t>) return mRawUnicodeUnits.First();
	    else return Unicode::DecodeCodepoint(RawUnicodeUnits());
	}

	constexpr void PopFirst()
	{
        if constexpr(sizeof(CodeUnit) == sizeof(char32_t))
			return mRawUnicodeUnits.PopFirst();
	    else if constexpr(sizeof(CodeUnit) == sizeof(char))
			mRawUnicodeUnits|PopFirstExactly(1 + Unicode::Utf8ContinuationBytes(mRawUnicodeUnits.First()));
		else if constexpr(sizeof(CodeUnit) == sizeof(char16_t))
			mRawUnicodeUnits|PopFirstExactly(1 + Unicode::IsUtf16LeadingSurrogate(mRawUnicodeUnits.First()));
	}

	constexpr void PopLast()
	{
		if constexpr(sizeof(CodeUnit) == sizeof(char))
			while(Unicode::IsUtf8ContinuationByte(mRawUnicodeUnits.Last())) mRawUnicodeUnits.PopLast();
		else if constexpr(sizeof(CodeUnit) == sizeof(char16_t))
			if(Unicode::IsUtf16TrailingSurrogate(mRawUnicodeUnits.Last())) mRawUnicodeUnits.PopLast();
		mRawUnicodeUnits.PopLast();
	}

	[[nodiscard]] constexpr bool Empty() const noexcept {return mRawUnicodeUnits.Empty();}

    constexpr index_t PopFirstCount(ClampedSize maxCharsToPop) requires(sizeof(CodeUnit) == sizeof(char32_t))
    {
        return mRawUnicodeUnits.PopFirstCount(maxCharsToPop);
    }

	constexpr index_t PopFirstCodeUnits(ClampedSize maxCodeUnitsToPop)
	{
		if constexpr(sizeof(CodeUnit) == sizeof(char))
			INTRA_PRECONDITION(maxCodeUnitsToPop >= RawUnicodeUnits().Length() ||
				!Unicode::IsUtf8ContinuationByte(RawUnicodeUnits()[maxCodeUnitsToPop]));
		if constexpr(sizeof(CodeUnit) == sizeof(char16_t))
			INTRA_PRECONDITION(maxCodeUnitsToPop >= RawUnicodeUnits().Length() ||
				!Unicode::IsUtf16TrailingSurrogate(RawUnicodeUnits()[maxCodeUnitsToPop]));
		return mRawUnicodeUnits.PopFirstCount(maxCodeUnitsToPop);
	}

	constexpr index_t PopLastCodeUnits(ClampedSize maxCodeUnitsToPop)
	{
		if constexpr(sizeof(CodeUnit) == sizeof(char))
			INTRA_PRECONDITION(maxCodeUnitsToPop >= RawUnicodeUnits().Length() ||
				!Unicode::IsUtf8ContinuationByte(RawUnicodeUnits()[RawUnicodeUnits().Length() - (maxCodeUnitsToPop+1)]));
		if constexpr(sizeof(CodeUnit) == sizeof(char16_t))
			INTRA_PRECONDITION(maxCodeUnitsToPop >= RawUnicodeUnits().Length() ||
				!Unicode::IsUtf16TrailingSurrogate(RawUnicodeUnits()[maxCodeUnitsToPop]));
		return mRawUnicodeUnits.PopFirstCount(maxCodeUnitsToPop);
	}

    [[nodiscard]] constexpr GenericStringView DropCodeUnits(ClampedSize count) const
    {
		auto result = *this;
		result.PopFirstCodeUnits(count);
        return result;
    }

	[[nodiscard]] constexpr GenericStringView DropLastCodeUnits(ClampedSize count) const
	{
	    return GenericStringView(Unsafe, mRawUnicodeUnits|DropLast(count));
	}

	[[nodiscard]] constexpr GenericStringView TakeCodeUnits(ClampedSize count) const
	{
	    return GenericStringView(Unsafe, mRawUnicodeUnits|Take(count));
	}

	[[nodiscard]] constexpr GenericStringView TailCodeUnits(ClampedSize count) const
	{
	    return GenericStringView(Unsafe, mRawUnicodeUnits|Tail(count));
	}
};
using StringView = GenericStringView<char>;

inline namespace Literals {
[[nodiscard]] constexpr StringView operator""_v(const char* str, size_t len) noexcept
{
	return StringView(Unsafe, str, len);
}

[[nodiscard]] constexpr auto operator""_v(const wchar_t* str, size_t len) noexcept
{
	return GenericStringView(Unsafe, str, len);
}

[[nodiscard]] constexpr auto operator""_v(const char16_t* str, size_t len) noexcept
{
	return GenericStringView(Unsafe, str, len);
}

[[nodiscard]] constexpr auto operator""_v(const char32_t* str, size_t len) noexcept
{
	return GenericStringView(Unsafe, str, len);
}

#ifdef __cpp_char8_t
[[nodiscard]] constexpr auto operator""_v(const char8_t* str, size_t len) noexcept
{
	return GenericStringView(Unsafe, str, len);
}
#endif
}

#if INTRA_CONSTEXPR_TEST
static_assert(CForwardRange<StringView>);
static_assert(CFiniteRange<StringView>);
static_assert(CForwardList<const StringView>);
static_assert("test string"_v != "test string 2"_v);
static_assert("test string"_v == "test string"_v);
static_assert("test string"_v|Take(4)|MatchesWith("test"_v));
static_assert(u8"тестовая строка"_v|Take(4)|MatchesWith(u8"тест"_v));
static_assert(u8"тестовая строка"_v.TakeCodeUnits(4).RawUnicodeUnits().Length() == 4);
static_assert(u8"те"_v.RawUnicodeUnits().Length() == 4);
static_assert(u8"тестовая строка"_v.TakeCodeUnits(4) == u8"те"_v);
static_assert(u8"тестовая строка"_v|Take(4)|MatchesWith(u8"тестовая строка"_v.TakeCodeUnits(8)));
static_assert(u8"тестовая строка"_v < u8"тестовая строка."_v);
static_assert(u8"тестовая строкА"_v < u8"тестовая строка"_v);
#endif

INTRA_END
