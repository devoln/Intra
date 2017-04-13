#pragma once

#include "Platform/CppWarnings.h"
#include "Platform/CppFeatures.h"
#include "Meta/Type.h"
#include "Range/ForwardDecls.h"
#include "Range/Generators/Span.h"
#include "Algo/Comparison/Equals.h"
#include "Algo/Comparison/LexCompare.h"
#include "Algo/String/CStr.h"

namespace Intra { namespace Range {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename Char> struct GenericStringView: Span<Char>
{
	static_assert(Meta::IsCharType<Char>::_, "Type must be character type.");

	using Span<Char>::Span;
	constexpr forceinline bool Empty() const noexcept {return Span<Char>::Empty();}
	forceinline size_t Length() const noexcept {return Span<Char>::Length();}
	constexpr forceinline Char* Data() const noexcept {return Span<Char>::Data();}

	constexpr forceinline GenericStringView(null_t=null) noexcept {}
	constexpr forceinline GenericStringView(const GenericStringView& rhs) noexcept: Span<Char>(static_cast<const Span<Char>&>(rhs)) {}

	template<size_t N> static forceinline constexpr
	GenericStringView FromBuffer(Char(&buffer)[N]) noexcept {return {buffer, buffer+N};}

	//! Конструирование из ограниченной нулевым символом C-строки.
	//! Сам нулевой символ при этом не входит в полученный диапазон.
	//! Хотелось бы сделать его explicit, но не все компиляторы будут неявно конструировать StringView из строковых литералов.
	forceinline /*explicit*/ GenericStringView(Char* cstr):
		Span<Char>(cstr, cstr==null? 0: Algo::CStringLength(cstr)) {}

	//Сравнение строк
	bool operator==(const Char* rhs) const
	{
		return (Empty() && (rhs==null || *rhs=='\0')) ||
			(rhs!=null && Algo::Equals(*this, GenericStringView<const Char>(rhs, Length())) && rhs[Length()]=='\0');
	}

	bool operator==(const GenericStringView& rhs) const noexcept
	{
		return Length()==rhs.Length() &&
			(Data()==rhs.Data() || Algo::Equals(*this, rhs));
	}

	forceinline bool operator!=(const GenericStringView& rhs) const noexcept {return !operator==(rhs);}

	constexpr forceinline bool operator==(null_t) const noexcept {return Empty();}
	constexpr forceinline bool operator!=(null_t) const noexcept {return !Empty();}

	bool operator<(const GenericStringView& rhs) const noexcept
	{return Algo::LexCompare(*this, rhs)<0;}

	bool operator<(const Char* rhs) const
	{
		return (rhs!=null && *rhs!='\0') && (Length()==0 ||
			Algo::LexCompare(*this, GenericStringView(rhs, Length()))<0);
	}

	bool operator>(const Char* rhs) const
	{
		return !Empty() && (rhs==null || *rhs=='\0' ||
			Algo::LexCompare(*this, GenericStringView(rhs, Length()))>0);
	}


	//Получить подстроку, начиная с кодовой единицы с индексом start и заканчивая end
	forceinline GenericStringView operator()(size_t startIndex, size_t endIndex) const
	{return Span<Char>::operator()(startIndex, endIndex);}

	constexpr forceinline GenericStringView TakeNone() const noexcept {return {Data(), 0};}

	constexpr forceinline GenericStringView Drop(size_t count=1) const noexcept
	{return Span<Char>::Drop(count);}

	constexpr forceinline GenericStringView DropBack(size_t count=1) const noexcept
	{return Span<Char>::DropBack(count);}

	constexpr forceinline GenericStringView Take(size_t count) const noexcept
	{return Span<Char>::Take(count);}

	constexpr forceinline GenericStringView Tail(size_t count) const noexcept
	{return Span<Char>::Tail(count);}
};

#ifdef INTRA_USER_DEFINED_LITERAL_SUPPORT
forceinline constexpr StringView operator"" _v(const char* str, size_t len) noexcept
{return {str, len};}

constexpr forceinline WStringView operator"" _wv(const wchar* str, size_t len) noexcept
{return {str, len};}

constexpr forceinline DStringView operator"" _dv(const dchar* str, size_t len) noexcept
{return {str, len};}
#endif

}

}


INTRA_WARNING_POP
