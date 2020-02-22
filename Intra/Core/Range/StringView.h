#pragma once

#include "Core/Type.h"
#include "Core/Tuple.h"
#include "Core/Range/Span.h"
#include "Core/Misc/RawMemory.h"
#include "Core/CArray.h"

INTRA_BEGIN

//! Not-owning reference to string.
template<typename Char> struct GenericStringView: Span<Char>
{
	static_assert(CChar<Char>, "Type must be character type.");

	using Span<Char>::Span;
	INTRA_NODISCARD constexpr forceinline bool Empty() const noexcept {return Span<Char>::Empty();}
	INTRA_NODISCARD constexpr forceinline index_t Length() const noexcept {return Span<Char>::Length();}
	INTRA_NODISCARD constexpr forceinline Char* Data() const noexcept {return Span<Char>::Data();}

	//constexpr forceinline GenericStringView(const Span<Char>& span) noexcept: Span<Char>(span) {}

	constexpr forceinline GenericStringView() = default;
	constexpr forceinline GenericStringView(const GenericStringView&) = default;
	constexpr forceinline GenericStringView(const Span<Char>& rhs): Span<Char>(rhs) {}
	template<size_t N> INTRA_NODISCARD static constexpr forceinline GenericStringView FromBuffer(Char(&buffer)[N]) noexcept {return GenericStringView(buffer, N);}

	//! Construct from null-terminated C-string.
	//! Null terminator itself will not be a part of constructed view.
	forceinline constexpr GenericStringView(Char* cstr):
		Span<Char>(cstr, cstr == null? 0: Misc::CStringLength(cstr)) {}

	static INTRA_NODISCARD constexpr forceinline GenericStringView FromPointerRange(Char* startPtr, Char* endPtr) {return Span<Char>::FromPointerRange(startPtr, endPtr);}

	///@{
	//! String comparison
	INTRA_NODISCARD constexpr bool operator==(const Char* rhs) const
	{
		return (Empty() &&
				(rhs == null || *rhs=='\0')) ||
			(rhs != null &&
				Misc::BitsEqual(Data(), rhs, Length()) &&
				rhs[Length()] == '\0');
	}

	INTRA_NODISCARD constexpr forceinline bool operator==(const GenericStringView& rhs) const noexcept
	{
		return Length() == rhs.Length() &&
			Misc::BitsEqual(Data(), rhs.Data(), Length());
	}

	INTRA_NODISCARD constexpr forceinline bool operator!=(const GenericStringView& rhs) const noexcept {return !operator==(rhs);}

	INTRA_NODISCARD constexpr forceinline bool operator==(null_t) const noexcept {return Empty();}
	INTRA_NODISCARD constexpr forceinline bool operator!=(null_t) const noexcept {return !Empty();}

	INTRA_NODISCARD constexpr forceinline bool operator<(const GenericStringView& rhs) const noexcept
	{return LexCompare(*this, rhs) < 0;}

	INTRA_NODISCARD constexpr forceinline bool operator<(const Char* rhs) const
	{
		return (rhs != null && *rhs != '\0') && (Length() == 0 ||
			LexCompare(*this, GenericStringView(rhs, Length())) < 0);
	}

	INTRA_NODISCARD constexpr forceinline bool operator>(const Char* rhs) const
	{
		return !Empty() && (rhs==null || *rhs=='\0' ||
			LexCompare(*this, GenericStringView(rhs, Length())) > 0);
	}
	///@}

	//! Get string slice [``startIndex``; ``endIndex``)
	INTRA_NODISCARD constexpr forceinline GenericStringView operator()(size_t startIndex, size_t endIndex) const
	{return Span<Char>::operator()(startIndex, endIndex);}

	INTRA_NODISCARD constexpr forceinline GenericStringView TakeNone() const noexcept {return {Data(), 0};}
	INTRA_NODISCARD constexpr forceinline GenericStringView Drop(size_t count=1) const noexcept {return Span<Char>::Drop(count);}
	INTRA_NODISCARD constexpr forceinline GenericStringView DropLast(size_t count=1) const noexcept {return Span<Char>::DropLast(count);}
	INTRA_NODISCARD constexpr forceinline GenericStringView Take(size_t count) const noexcept {return Span<Char>::Take(count);}
	INTRA_NODISCARD constexpr forceinline GenericStringView Tail(size_t count) const noexcept {return Span<Char>::Tail(count);}

	constexpr forceinline GenericStringView Find(Char c) const {return Span<Char>::Find(c);}

	constexpr forceinline GenericStringView Find(CSpan<Char> str) const {return Span<Char>::Find(str);}
};

typedef GenericStringView<const char> StringView;
typedef GenericStringView<char> MutStringView;

#if defined(__cpp_user_defined_literals) && __cpp_user_defined_literals >= 200809

INTRA_NODISCARD constexpr forceinline StringView operator"" _v(const char* str, size_t len) noexcept
{return {str, len};}

#endif

#if INTRA_CONSTEXPR_TEST
static_assert(CNonInfiniteForwardRange<StringView>, "IsNonInfiniteForwardRange error.");
static_assert(CAsNonInfiniteForwardRange<StringView>, "IsAsNonInfiniteForwardRange error!");
static_assert(CAsFiniteForwardRange<CSpan<Tuple<StringView, StringView>>>, "IsAsFiniteForwardRange error!");
static_assert(StringView::FromBuffer("IsNonInfiniteForwardRange") != StringView::FromBuffer("IsAsNonInfiniteForwardRange"), "TEST FAILED");

constexpr auto ptr1 = "IsAsFiniteForwardRange";
constexpr auto ptr2 = ptr1;
static_assert(ptr1 == ptr2, "FAIL");

static_assert(StringView::FromBuffer("IsAsFiniteForwardRange") == StringView::FromBuffer("IsAsFiniteForwardRange"), "TEST FAILED");
static_assert(StringView::FromBuffer("IsAsFiniteForwardRange")(0, 10) == "IsAsFinite", "TEST FAILED");
#endif

INTRA_END
