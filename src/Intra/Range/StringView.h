#pragma once

#include <Intra/Concepts.h>
#include <Intra/Binary.h>
#include <Intra/Range.h>
#include <Intra/Range/Special/Unicode.h>

namespace Intra { INTRA_BEGIN
/// Not-owning reference to a string.
template<CUnqualedChar CodeUnit, bool NullTerminated = false> class GenericStringView
{
	Span<const CodeUnit> mRawUnicodeUnits;
public:
	using TagAnyInstanceFinite = TTag<>;

	[[nodiscard]] INTRA_FORCEINLINE constexpr auto RawUnicodeUnits() const noexcept {return mRawUnicodeUnits;}
	[[nodiscard]] INTRA_FORCEINLINE constexpr auto CStr() const noexcept requires NullTerminated {return mRawUnicodeUnits.Begin;}

	constexpr GenericStringView() = default;
	constexpr GenericStringView(const GenericStringView&) = default;

	template<CConvertibleToSpan L> requires CSame<CodeUnit, TArrayListValue<L>>
	explicit constexpr GenericStringView(L&& src): GenericStringView(Unsafe, src)
	{
		INTRA_PRECONDITION(Unicode::IsValidUnicode(src));
		if constexpr(NullTerminated)
		{
			INTRA_PRECONDITION(src|Empty || src|Last == '\0');
			if(!mRawUnicodeUnits.Empty()) mRawUnicodeUnits.PopLast();
		}
	}

	INTRA_FORCEINLINE GenericStringView(const GenericStringView<CodeUnit, false>& rhs) requires NullTerminated:
		mRawUnicodeUnits(rhs.mRawUnicodeUnits) {}

	template<CConvertibleToSpan L> requires CSame<CodeUnit, TArrayListValue<L>>
	explicit INTRA_FORCEINLINE constexpr GenericStringView(TUnsafe, L&& src) noexcept: mRawUnicodeUnits(src) {}

	explicit INTRA_FORCEINLINE constexpr GenericStringView(TUnsafe, const CodeUnit* begin, Size length):
		mRawUnicodeUnits(Unsafe, begin, length) {}

	explicit INTRA_FORCEINLINE constexpr GenericStringView(TUnsafe, const CodeUnit* begin, const CodeUnit* end):
		mRawUnicodeUnits(Unsafe, begin, end) {}

	/// Construct from nullptr-terminated C-string.
	/// Null terminator itself will not be a part of the constructed view.
	explicit INTRA_FORCEINLINE constexpr GenericStringView(TUnsafe, const CodeUnit* nullTermStr):
		GenericStringView(Unsafe, nullTermStr, nullTermStr? CStringLength(nullTermStr): 0) {}

	[[nodiscard]] INTRA_FORCEINLINE constexpr bool operator==(const GenericStringView& rhs) const noexcept
	{
		return mRawUnicodeUnits|MatchesWith(rhs.mRawUnicodeUnits);
	}
	[[nodiscard]] INTRA_FORCEINLINE constexpr bool operator<(const GenericStringView& rhs) const noexcept {return (*this|LexCompareTo(rhs)) < 0;}

	[[nodiscard]] INTRA_FORCEINLINE constexpr char32_t First() const
	{
		if constexpr(CSame<CodeUnit, char32_t>) return mRawUnicodeUnits.First();
		else return Unicode::DecodeCodepoint(RawUnicodeUnits());
	}

	constexpr void PopFirst()
	{
		if constexpr(CSameSize<CodeUnit, char32_t>) return mRawUnicodeUnits.PopFirst();
		else if constexpr(CSameSize<CodeUnit, char>)
			mRawUnicodeUnits|PopFirstExactly(1 + Unicode::Utf8ContinuationBytes(mRawUnicodeUnits.First()));
		else if constexpr(CSameSize<CodeUnit, char16_t>)
			mRawUnicodeUnits|PopFirstExactly(1 + Unicode::IsUtf16LeadingSurrogate(mRawUnicodeUnits.First()));
	}

	constexpr void PopLast() requires(!NullTerminated)
	{
		if constexpr(sizeof(CodeUnit) == sizeof(char)) mRawUnicodeUnits|PopLastWhile(Unicode::IsUtf8ContinuationByte);
		else if constexpr(sizeof(CodeUnit) == sizeof(char16_t)) mRawUnicodeUnits|PopLastWhile(Unicode::IsUtf16TrailingSurrogate);
		mRawUnicodeUnits.PopLast();
	}

	[[nodiscard]] INTRA_FORCEINLINE constexpr bool Empty() const noexcept {return mRawUnicodeUnits.Empty();}

	INTRA_FORCEINLINE constexpr index_t PopFirstCount(ClampedSize maxCharsToPop) requires(sizeof(CodeUnit) == sizeof(char32_t))
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

	constexpr index_t PopLastCodeUnits(ClampedSize maxCodeUnitsToPop) requires(!NullTerminated)
	{
		if constexpr(sizeof(CodeUnit) == sizeof(char))
			INTRA_PRECONDITION(maxCodeUnitsToPop >= RawUnicodeUnits().Length() ||
				!Unicode::IsUtf8ContinuationByte(RawUnicodeUnits()[RawUnicodeUnits().Length() - (maxCodeUnitsToPop+1)]));
		if constexpr(sizeof(CodeUnit) == sizeof(char16_t))
			INTRA_PRECONDITION(maxCodeUnitsToPop >= RawUnicodeUnits().Length() ||
				!Unicode::IsUtf16TrailingSurrogate(RawUnicodeUnits()[maxCodeUnitsToPop]));
		return mRawUnicodeUnits.PopFirstCount(maxCodeUnitsToPop);
	}

	[[nodiscard]] INTRA_FORCEINLINE constexpr GenericStringView DropCodeUnits(ClampedSize count) const
	{
		auto result = *this;
		result.PopFirstCodeUnits(count);
		return result;
	}

	[[nodiscard]] INTRA_FORCEINLINE constexpr GenericStringView DropLastCodeUnits(ClampedSize count) const
	{
		return GenericStringView<CodeUnit, false>(Unsafe, mRawUnicodeUnits|DropLast(count));
	}

	[[nodiscard]] constexpr GenericStringView TakeCodeUnits(ClampedSize count) const
	{
		auto res = mRawUnicodeUnits|Take(count);
		if constexpr(CSameSize<CodeUnit, char>) res|PopLastWhile(Unicode::IsUtf8ContinuationByte);
		else if constexpr(CSameSize<CodeUnit, char16_t>) res.Begin += Unicode::IsUtf16LeadingSurrogate;
		return GenericStringView(Unsafe, res);
	}

	[[nodiscard]] constexpr GenericStringView TailCodeUnits(ClampedSize count) const
	{
		auto res = mRawUnicodeUnits|Tail(count);
		if constexpr(CSameSize<CodeUnit, char>) res|PopFirstWhile(Unicode::IsUtf8ContinuationByte);
		else if constexpr(CSameSize<CodeUnit, char16_t>) res.Begin += Unicode::IsUtf16TrailingSurrogate(res.First());
		return GenericStringView(Unsafe, res);
	}
};
using StringView = GenericStringView<char, false>;
using ZStringView = GenericStringView<char, true>;

inline namespace Literals {
[[nodiscard]] constexpr ZStringView operator""_v(const char* str, size_t len) noexcept {return ZStringView(Unsafe, str, len);}
[[nodiscard]] constexpr auto operator""_v(const wchar_t* str, size_t len) noexcept {return GenericStringView<wchar_t, true>(Unsafe, str, len);}
[[nodiscard]] constexpr auto operator""_v(const char16_t* str, size_t len) noexcept {return GenericStringView<char16_t, true>(Unsafe, str, len);}
[[nodiscard]] constexpr auto operator""_v(const char32_t* str, size_t len) noexcept {return GenericStringView<char32_t, true>(Unsafe, str, len);}
#ifdef __cpp_char8_t
[[nodiscard]] constexpr auto operator""_v(const char8_t* str, size_t len) noexcept {return GenericStringView<char8_t, true>(Unsafe, str, len);}
#endif
}

#if INTRA_CONSTEXPR_TEST
static_assert(CForwardRange<StringView>);
static_assert(CFiniteRange<StringView>);
static_assert(CForwardList<const StringView>);
static_assert(CRange<const Intra::GenericStringView<char8_t>&>);
static_assert(CForwardList<const Intra::GenericStringView<char8_t>&>);
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

} INTRA_END
