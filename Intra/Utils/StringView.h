#pragma once

#include "Cpp/Warnings.h"
#include "Cpp/Features.h"
#include "Cpp/Intrinsics.h"
#include "Meta/Type.h"
#include "Span.h"
#include "ArrayAlgo.h"
#include "Concepts/Array.h"

namespace Intra {

//Определяется в Range, чтобы работал ADL с алгоритмами, определёнными в Range.

namespace Range {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

//! Невладеющая ссылка на строку.
template<typename Char> struct GenericStringView: Span<Char>
{
	static_assert(Meta::IsCharType<Char>::_, "Type must be character type.");

	using Span<Char>::Span;
	constexpr forceinline bool Empty() const noexcept {return Span<Char>::Empty();}
	forceinline size_t Length() const noexcept {return Span<Char>::Length();}
	constexpr forceinline Char* Data() const noexcept {return Span<Char>::Data();}

	constexpr forceinline GenericStringView(null_t=null) noexcept {}

	template<size_t N> static forceinline constexpr
	GenericStringView FromBuffer(Char(&buffer)[N]) noexcept {return {buffer, buffer+N};}

	//! Конструирование из ограниченной нулевым символом C-строки.
	//! Сам нулевой символ при этом не входит в полученный диапазон.
	//! Если сделать его explicit, то не все компиляторы будут неявно конструировать StringView из строковых литералов.
	forceinline /*explicit*/ GenericStringView(Char* cstr):
		Span<Char>(cstr, cstr == null? 0: Utils::CStringLength(cstr)) {}

	//forceinline GenericStringView(Char* startPtr, Char* endPtr):
		//Span<Char>(startPtr, endPtr) {}

	//Сравнение строк
	bool operator==(const Char* rhs) const
	{
		return (Empty() &&
				(rhs==null || *rhs=='\0')) ||
			(rhs != null &&
				C::memcmp(Data(), rhs, Length()*sizeof(Char)) == 0 &&
				rhs[Length()] == '\0');
	}

	bool operator==(const GenericStringView& rhs) const noexcept
	{
		return Length() == rhs.Length() &&
			(Data() == rhs.Data() ||
				C::memcmp(Data(), rhs.Data(), Length()*sizeof(Char)) == 0);
	}

	forceinline bool operator!=(const GenericStringView& rhs) const noexcept {return !operator==(rhs);}

	constexpr forceinline bool operator==(null_t) const noexcept {return Empty();}
	constexpr forceinline bool operator!=(null_t) const noexcept {return !Empty();}

	bool operator<(const GenericStringView& rhs) const noexcept
	{return Utils::LexCompare(*this, rhs) < 0;}

	bool operator<(const Char* rhs) const
	{
		return (rhs!=null && *rhs!='\0') && (Length()==0 ||
			Utils::LexCompare(*this, GenericStringView(rhs, Length()))<0);
	}

	bool operator>(const Char* rhs) const
	{
		return !Empty() && (rhs==null || *rhs=='\0' ||
			Utils::LexCompare(*this, GenericStringView(rhs, Length()))>0);
	}


	//Получить подстроку, начиная с кодовой единицы с индексом start и заканчивая end
	forceinline GenericStringView operator()(size_t startIndex, size_t endIndex) const
	{return Span<Char>::operator()(startIndex, endIndex);}

	constexpr forceinline GenericStringView TakeNone() const noexcept {return {Data(), 0};}
	constexpr forceinline GenericStringView Drop(size_t count=1) const noexcept {return Span<Char>::Drop(count);}
	constexpr forceinline GenericStringView DropLast(size_t count=1) const noexcept {return Span<Char>::DropLast(count);}
	constexpr forceinline GenericStringView Take(size_t count) const noexcept {return Span<Char>::Take(count);}
	constexpr forceinline GenericStringView Tail(size_t count) const noexcept {return Span<Char>::Tail(count);}

	forceinline GenericStringView Find(Char c) const {return Span<Char>::Find(c);}

	forceinline GenericStringView Find(CSpan<Char> str) const {return Span<Char>::Find(str);}
};

typedef GenericStringView<const char> StringView;
typedef GenericStringView<const wchar> WStringView;
typedef GenericStringView<const dchar> DStringView;
typedef GenericStringView<char> MutStringView;
typedef GenericStringView<wchar> MutWStringView;
typedef GenericStringView<dchar> MutDStringView;

#ifdef INTRA_USER_DEFINED_LITERAL_SUPPORT
constexpr forceinline StringView operator"" _v(const char* str, size_t len) noexcept
{return {str, len};}

constexpr forceinline WStringView operator"" _wv(const wchar* str, size_t len) noexcept
{return {str, len};}

constexpr forceinline DStringView operator"" _dv(const dchar* str, size_t len) noexcept
{return {str, len};}
#endif

}

namespace Utils {
using Range::GenericStringView;
using Range::StringView;
using Range::WStringView;
using Range::DStringView;
using Range::MutStringView;
using Range::MutWStringView;
using Range::MutDStringView;
}

using Range::GenericStringView;
using Range::StringView;
using Range::WStringView;
using Range::DStringView;
using Range::MutStringView;
using Range::MutWStringView;
using Range::MutDStringView;

static_assert(Concepts::IsNonInfiniteForwardRange<StringView>::_, "IsNonInfiniteForwardRange error.");
static_assert(Concepts::IsAsNonInfiniteForwardRange<StringView>::_, "IsAsNonInfiniteForwardRange error!");
static_assert(Concepts::IsAsFiniteForwardRange<CSpan<Pair<StringView, StringView>>>::_, "IsAsFiniteForwardRange error!");

}


INTRA_WARNING_POP
