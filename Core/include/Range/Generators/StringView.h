#pragma once

#include "Platform/CppWarnings.h"
#include "Platform/CppFeatures.h"
#include "Meta/Type.h"
#include "Range/ForwardDecls.h"
#include "Range/Generators/ArrayRange.h"
#include "Algo/Comparison/Equals.h"
#include "Algo/Comparison/LexCompare.h"
#include "Algo/String/CStr.h"

namespace Intra { namespace Range {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename Char> struct GenericStringView
{
	constexpr forceinline GenericStringView(null_t=null): mStart(null), mEnd(null) {}
	constexpr forceinline GenericStringView(Char* str, size_t len): mStart(str), mEnd(str+len) {}
	constexpr forceinline GenericStringView(Char* startPtr, Char* endPtr): mStart(startPtr), mEnd(endPtr) {}

	//! Конструктор из строкового литерала.
	//! Внимание! Конструктор предполагает, что массив целиком содержит C-строку вместе с нулевым символом в позиции N-1.
	//! Если это не так, убедитесь, что используются другие перегрузки конструктора.
	template<size_t N> constexpr forceinline GenericStringView(Char(&stringLiteral)[N]):
		mStart(stringLiteral), mEnd(stringLiteral+(N-1u)) {}

	constexpr forceinline GenericStringView(const GenericStringView& rhs) = default;

	//! Конструирование из любого подходящего диапазона или контейнера
	template<typename R, typename=Meta::EnableIf<
		IsAsArrayRange<R>::_ && !Meta::IsArrayType<Meta::RemoveReference<R>>::_ &&
		Meta::TypeEqualsIgnoreCV<ValueTypeOfAs<R>, Char>::_ &&
		(IsAsAssignableRange<R>::_ || Meta::IsConst<Char>::_) &&
		!Meta::TypeEqualsIgnoreCVRef<R, GenericStringView>::_
	>> forceinline GenericStringView(R&& range)
	{
		auto r = Range::Forward<R>(range);
		mStart = r.Data();
		mEnd = mStart+r.Length();
	}

	//! Конструирование из ограниченной нулевым символом C-строки.
	//! Сам нулевой символ при этом не входит в полученный диапазон.
	//! Хотелось бы сделать его explicit, но не все компиляторы будут неявно конструировать StringView из строковых литералов.
	forceinline /*explicit*/ GenericStringView(Char* cstr):
		mStart(cstr), mEnd(cstr==null? null: cstr+Algo::CStringLength(cstr)) {}

	constexpr forceinline Char* Data() const {return mStart;}
	constexpr forceinline Char* End() const {return mEnd;}
	
	forceinline size_t Length() const
	{
		INTRA_DEBUG_ASSERT(mEnd >= mStart);
		return size_t(mEnd-mStart);
	}

	forceinline Char operator[](size_t n) const
	{
		INTRA_DEBUG_ASSERT(n < Length());
		return mStart[n];
	}

	forceinline bool Empty() const
	{
		INTRA_DEBUG_ASSERT(mEnd >= mStart);
		return mStart>=mEnd;
	}

	forceinline Char& First() const
	{
		INTRA_DEBUG_ASSERT(!Empty());
		return *mStart;
	}

	forceinline void PopFirst()
	{
		INTRA_DEBUG_ASSERT(!Empty());
		mStart++;
	}

	forceinline Char& Last() const
	{
		INTRA_DEBUG_ASSERT(!Empty());
		return *(mEnd-1);
	}

	forceinline void PopLast()
	{
		INTRA_DEBUG_ASSERT(!Empty());
		mEnd--;
	}

	forceinline void PopFirstExactly(size_t elementsToPop)
	{
		INTRA_DEBUG_ASSERT(elementsToPop <= Length());
		mStart += elementsToPop;
	}

	forceinline void PopLastExactly(size_t elementsToPop)
	{
		INTRA_DEBUG_ASSERT(elementsToPop <= Length());
		mEnd -= elementsToPop;
	}

	forceinline size_t PopFirstN(size_t count)
	{
		if(count>Length()) count = Length();
		mStart += count;
		return count;
	}

	forceinline size_t PopLastN(size_t count)
	{
		if(count>Length()) count = Length();
		mEnd -= count;
		return count;
	}

	forceinline void Put(Char c) {INTRA_DEBUG_ASSERT(!Empty()); *mStart++ = c;}

	//Сравнение строк
	bool operator==(const Char* rhs) const
	{
		return (Empty() && (rhs==null || *rhs=='\0')) ||
			(rhs!=null && Algo::Equals(*this, GenericStringView<const Char>(rhs, Length())) && rhs[Length()]=='\0');
	}

	bool operator==(const GenericStringView& rhs) const
	{
		return Length()==rhs.Length() &&
			(mStart==rhs.mStart || Algo::Equals(*this, rhs));
	}

	bool operator!=(const GenericStringView& rhs) const {return !operator==(rhs);}

	forceinline bool operator==(null_t) const {return Empty();}
	forceinline bool operator!=(null_t) const {return !Empty();}

	forceinline GenericStringView& operator=(null_t) {mStart=mEnd=null; return *this;}

	forceinline GenericStringView& operator=(const GenericStringView& rhs) = default;

	bool operator<(const GenericStringView& rhs) const
	{return Algo::LexCompare(*this, rhs)<0;}

	bool operator<(const Char* rhs) const
	{
		return (rhs!=null && *rhs!='\0') && (Length()==0 ||
			Algo::LexCompare(*this, GenericStringView(rhs, Length()))<0);
	}

	bool operator>(const Char* rhs) const
	{
		return mStart!=mEnd && (rhs==null || *rhs=='\0' ||
			Algo::LexCompare(*this, GenericStringView(rhs, Length()))>0);
	}


	//Получить подстроку, начиная с кодовой единицы с индексом start и заканчивая end
	constexpr forceinline GenericStringView operator()(size_t startIndex, size_t endIndex) const
	{return GenericStringView(mStart+startIndex, mStart+endIndex);}

	forceinline constexpr GenericStringView TakeNone() const {return {mStart, mStart};}

	forceinline constexpr ArrayRange<const Char> AsRange() const {return {mStart, mEnd};}


	forceinline GenericStringView Drop(size_t count=1) const
	{return GenericStringView(mStart+count>mEnd? mEnd: mStart+count, mEnd);}

	forceinline GenericStringView DropBack(size_t count=1) const
	{return GenericStringView(mStart, mEnd-count<mStart? mStart: mEnd-count);}

	forceinline GenericStringView Take(size_t count) const
	{return GenericStringView(mStart, mStart+count>mEnd? mEnd: mStart+count);}

	forceinline GenericStringView Tail(size_t count) const
	{return GenericStringView(mEnd-count<mStart? mStart: mEnd-count, mEnd);}

	Char* begin() const {return mStart;}
	Char* end() const {return mEnd;}

private:
	Char* mStart;
	Char* mEnd;
};

#ifdef INTRA_USER_DEFINED_LITERAL_SUPPORT
forceinline constexpr StringView operator"" _v(const char* str, size_t len)
{return StringView(str, len);}

forceinline constexpr WStringView operator"" _wv(const wchar* str, size_t len)
{return WStringView(str, len);}

forceinline constexpr DStringView operator"" _dv(const dchar* str, size_t len)
{return DStringView(str, len);}
#endif

template<typename T, size_t N> forceinline Meta::EnableIf<
	Meta::IsCharType<T>::_,
GenericStringView<const T>> AsRange(const T(&stringLiteral)[N])
{return GenericStringView<const T>(stringLiteral, N-1);}

template<typename T, size_t N> forceinline Meta::EnableIf<
	Meta::IsCharType<T>::_,
GenericStringView<T>> AsRange(T(&stringLiteral)[N])
{return GenericStringView<T>(stringLiteral, N);}

INTRA_WARNING_POP

}

}

