#pragma once

#include "Platform/CppWarnings.h"
#include "Platform/CppFeatures.h"
#include "Meta/Type.h"
#include "Containers/ForwardDeclarations.h"
#include "Algo/String/Parse.h"
#include "Range/ArrayRange.h"
#include "Range/Iteration/Split.h"
#include "Algo/Op.h"
#include "Algo/Comparison.h"
#include "Algo/Mutation.h"
#include "Algo/Search.h"
#include "Algo/String/CStr.h"
#include "Algo/Mutation/Replace.h"
#include "Memory/Allocator.h"
#include "Containers/AsciiSet.h"

namespace Intra { namespace Range {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename Char> struct GenericStringView
{
	constexpr forceinline GenericStringView(null_t=null): cstart(null), cend(null) {}
	template<size_t N> constexpr forceinline GenericStringView(const Char(&str)[N]): cstart(str), cend(str+(N-1)) {}
	constexpr forceinline GenericStringView(const Char* str, size_t len): cstart(str), cend(str+len) {}
	constexpr forceinline GenericStringView(const Char* startPtr, const Char* endPtr): cstart(startPtr), cend(endPtr) {}

	constexpr forceinline GenericStringView(const GenericStringView& rhs): cstart(rhs.cstart), cend(rhs.cend) {}

	forceinline GenericStringView(const Char* cstr):
		cstart(cstr), cend(cstr==null? null: cstr+Algo::CStringLength(cstr)) {}

	constexpr forceinline const Char* Data() const {return cstart;}
	constexpr forceinline const Char* End() const {return cend;}
	constexpr forceinline size_t Length() const {return size_t(cend-cstart);}
	constexpr forceinline Char operator[](size_t n) const {return cstart[n];}

	constexpr forceinline bool Empty() const {return cstart==cend;}
	forceinline Char First() const {INTRA_ASSERT(!Empty()); return *cstart;}
	forceinline void PopFirst() {INTRA_ASSERT(!Empty()); cstart++;}
	forceinline Char Last() const {INTRA_ASSERT(!Empty()); return *(cend-1);}
	forceinline void PopLast() {INTRA_ASSERT(!Empty()); cend--;}

	forceinline void PopFirstExactly(size_t elementsToPop)
	{
		INTRA_ASSERT(elementsToPop <= Length());
		cstart += elementsToPop;
	}

	forceinline void PopLastExactly(size_t elementsToPop)
	{
		INTRA_ASSERT(elementsToPop <= Length());
		cend -= elementsToPop;
	}

	forceinline size_t PopFirstN(size_t count)
	{
		if(count>Length()) count = Length();
		cstart += count;
		return count;
	}

	forceinline size_t PopLastN(size_t count)
	{
		if(count>Length()) count = Length();
		cend -= count;
		return count;
	}

	//Сравнение строк
	bool operator==(const Char* rhs) const
	{
		return (Empty() && (rhs==null || *rhs=='\0')) ||
			(rhs!=null && C::memcmp(cstart, rhs, Length()*sizeof(Char))==0 && rhs[Length()]=='\0');
	}

	bool operator==(const GenericStringView& rhs) const
	{
		return Length()==rhs.Length() &&
			(cstart==rhs.cstart ||
				Algo::Equals(AsRange(), ArrayRange<const Char>(rhs.cstart, Length())));
	}

	bool operator!=(const GenericStringView& rhs) const {return !operator==(rhs);}

	forceinline bool operator==(null_t) const {return Empty();}
	forceinline bool operator!=(null_t) const {return !Empty();}

	forceinline GenericStringView& operator=(null_t) {cstart=cend=null; return *this;}
	forceinline GenericStringView& operator=(const GenericStringView& rhs)
	{cstart = rhs.cstart; cend = rhs.cend; return *this;}

	bool operator<(const GenericStringView& rhs) const
	{
		return Algo::LexCompare(AsRange(), rhs.AsRange())<0;
	}

	bool operator<(const Char* rhs) const
	{
		return (rhs!=null && *rhs!='\0') && (Length()==0 ||
			Algo::LexCompare(AsRange(), ArrayRange<const Char>(rhs, Length()))<0);
	}

	bool operator>(const Char* rhs) const
	{
		return cstart!=cend && (rhs==null || *rhs=='\0' ||
			Algo::LexCompare(AsRange(), ArrayRange<const Char>(rhs, Length()))>0);
	}



	/*Array<wchar> ToUTF16(bool addNullTerminator) const
	{
		return UTF8(cstart, cend).ToUTF16(addNullTerminator);
	}*/
	/*Array<dchar> ToUTF32(bool addNullTerminator) const;
	{
		return UTF8(cstart, cend).ToUTF32(addNullTerminator);
	}*/

	//Получить подстроку, начиная с кодовой единицы с индексом start и заканчивая end
	constexpr forceinline GenericStringView operator()(size_t startIndex, size_t endIndex) const
	{
		return GenericStringView(cstart+startIndex, cstart+endIndex);
	}

	forceinline GenericStringView operator()(RelativeIndex firstIndex, RelativeIndex endIndex) const
	{
		return operator()(firstIndex.GetRealIndex(Length()), endIndex.GetRealIndex(Length()));
	}

	forceinline GenericStringView operator()(size_t firstIndex, RelativeIndex endIndex) const
	{
		return operator()(firstIndex, endIndex.GetRealIndex(Length()));
	}

	forceinline GenericStringView operator()(RelativeIndex firstIndex, size_t endIndex) const
	{
		return operator()(firstIndex.GetRealIndex(Length()), endIndex);
	}

	forceinline constexpr GenericStringView TakeNone() const {return {cstart, cstart};}

	forceinline constexpr ArrayRange<const Char> AsRange() const {return {cstart, cend};}

	//! Возвращает диапазон, полученный из этого диапазона удалением всех первых и последних символов, равных x.
	template<typename X> forceinline Meta::EnableIf<
		Meta::IsConvertible<X, Char>::_,
	GenericStringView> TrimLeft(X x) const {return Algo::TrimLeft(*this, x);}

	//! Возвращает диапазон, полученный из этого диапазона удалением
	//! всех первых и последних символов, для которых выполнен предикат pred.
	template<typename P> forceinline Meta::EnableIf<
		Meta::IsCallable<P, Char>::_,
	GenericStringView> TrimLeft(P pred) const {return Algo::TrimLeft(*this, pred);}

	//! Возвращает диапазон, полученный из этого диапазона удалением всех первых и последних символов, равных x.
	template<typename X> forceinline Meta::EnableIf<
		Meta::IsConvertible<X, Char>::_,
	GenericStringView> TrimRight(X x) const {return Algo::TrimRight(*this, x);}

	//! Возвращает диапазон, полученный из этого диапазона удалением
	//! всех первых и последних символов, для которых выполнен предикат pred.
	template<typename P> forceinline Meta::EnableIf<
		Meta::IsCallable<P, Char>::_,
	GenericStringView> TrimRight(P pred) const {return Algo::TrimRight(*this, pred);}

	//! Возвращает диапазон, полученный из этого диапазона удалением всех первых и последних символов, равных x.
	template<typename X> forceinline Meta::EnableIf<
		Meta::IsConvertible<X, Char>::_,
	GenericStringView> Trim(X x) const {return Algo::Trim(*this, x);}

	//! Возвращает диапазон, полученный из этого диапазона удалением
	//! всех первых и последних символов, для которых выполнен предикат pred.
	template<typename P> forceinline Meta::EnableIf<
		Meta::IsCallable<P, Char>::_,
	GenericStringView> Trim(P pred) const {return Algo::Trim(*this, pred);}

	template<typename RW> Meta::EnableIf<
		(IsForwardRange<RW>::_ && !IsInfiniteRange<RW>::_ && ValueTypeIsConvertible<RW, Char>::_) ||
		Meta::TypeEquals<RW, Char>::_,
	bool> StartsWith(const RW& what) const
	{return Algo::StartsWith(*this, what);}

	template<size_t N> bool StartsWith(const Char(&what)[N]) const
	{return Algo::StartsWith(*this, what);}

	template<typename RW> Meta::EnableIf<
		(IsFiniteForwardRange<RW>::_ && ValueTypeIsConvertible<RW, Char>::_) ||
		Meta::TypeEquals<RW, Char>::_,
	bool> EndsWith(const RW& what) const
	{return Algo::EndsWith(*this, what);}

	template<size_t N> bool EndsWith(const Char(&what)[N]) const
	{return Algo::EndsWith(*this, what);}

	template<typename RW> Meta::EnableIf<
		(IsFiniteForwardRange<RW>::_ &&
		(ValueTypeIsConvertible<RW, Char>::_ ||
		ValueTypeIsConvertible<AsRangeResult<RW>, Char>::_)) ||
		Meta::TypeEquals<RW, Char>::_,
	bool> Contains(const RW& what) const
	{return Algo::Contains(*this, Range::AsRange(what));}

	template<size_t N> bool Contains(const Char(&what)[N]) const
	{return Algo::Contains(*this, what);}


	forceinline GenericStringView Drop(size_t count=1) const
	{return GenericStringView(cstart+count>cend? cend: cstart+count, cend);}

	forceinline GenericStringView DropBack(size_t count=1) const
	{return GenericStringView(cstart, cend-count<cstart? cstart: cend-count);}

	forceinline GenericStringView Take(size_t count) const
	{return GenericStringView(cstart, cstart+count>cend? cend: cstart+count);}

	forceinline GenericStringView Tail(size_t count) const
	{return GenericStringView(cend-count<cstart? cstart: cend-count, cend);}


	//! Получить целое число, записанное в строке.
	//! В строке не должно быть пробелов и посторонних символов.
	//! \param[out] success Адрес булевой переменной, в которую нужно записать, удалось преобразование или нет. Может быть null.
	//! \returns Прочитанное из строки число или 0 в случае неудачи.
	long64 ToInt(bool* oSuccess=null) const
	{
		if(!Empty() && uint(cstart[0]-'0')<=9)
		{
			GenericStringView nextPart = *this;
			const long64 result = Algo::ParseAdvance<long64>(nextPart);
			if(nextPart.Empty()) return result;
		}

		if(oSuccess!=null) *oSuccess=false;
		return 0;
	}


	//! Получить вещественное число, записанное в строке.
	//! В строке не должно быть пробелов и посторонних символов.
	//! \return Прочитанное из строки число или NaN в случае неудачи.
	real ToFloat() const
	{
		if(Empty()) return Math::NaN;
		bool minus=false;
		if(*cstart=='-') minus=true;
		if(uint(cstart[size_t(minus)]-'0')>9 && cstart[size_t(minus)]!='.')
			return Math::NaN;
		GenericStringView nextPart = *this;
		real result = Algo::ParseAdvance<float>(nextPart);
		if(nextPart.Empty()) return result;
		return Math::NaN;
	}

	//! Разбить строку на подстроки, используя указанные разделители
	//! \param spaceDelimiters Разделяющие символы, которые не будут входить в подстроки
	//! \param punctDelimiters Разделяющие символы, которые сами будут образовывать подстроки
	//! \returns Массив полученных подстрок
	RSplit<GenericStringView, AsciiSet, AsciiSet> Split(
		const AsciiSet& spaceDelimiters, const AsciiSet& punctDelimiters=null) const
	{return Range::Split(*this, spaceDelimiters, punctDelimiters);}

	//! Заменить все вхождения subStr на newSubStr
	GenericString<Char> ReplaceAll(GenericStringView subStr, StringView newSubStr) const
	{
		GenericString<Char> result;
		CountRange<Char> counter;
		Algo::MultiReplaceToAdvance(*this, counter, AsRange({Meta::TupleL(subStr, newSubStr)}));
		result.SetLengthUninitialized(counter.Counter);
		Algo::MultiReplaceToAdvance(*this, result.AsRange(), AsRange({Meta::TupleL(subStr, newSubStr)}));
		return result;
	}

	GenericString<Char> ReplaceAll(Char c, Char newc) const
	{
		if(c==newc) return *this;
		GenericString<Char> result;
		result.SetLengthUninitialized(Length());
		for(size_t i=0; i<Length(); i++)
			result[i] = cstart[i]==c? newc: cstart[i];
		return result;
	}

	GenericString<Char> MultiReplace(ArrayRange<const GenericStringView> subStrs, ArrayRange<const GenericStringView> newSubStrs) const;


	//! Удалить из строки все символы из removeCharset
	GenericString<Char> RemoveChars(const AsciiSet& removedCharset) const
	{
		GenericString<Char> result;
		for(size_t i=0; i<Length(); i++)
		{
			if((cstart[i] & 0x80)==0 && removedCharset[cstart[i]]) continue;
			result += cstart[i];
		}
		return result;
	}

	//! Перевести все символы латиницы в нижний регистр
	GenericString<Char> ToLowerAscii() const
	{
		GenericString<Char> result;
		result.SetLengthUninitialized(Length());
		Algo::Transform(result.AsRange(), Op::ToLowerAscii<Char>);
		return result;
	}

	//! Перевести все символы латиницы в верхний регистр
	GenericString<Char> ToUpperAscii() const
	{
		GenericString<Char> result;
		result.SetLengthUninitialized(Length());
		Algo::Transform(result.AsRange(), Op::ToUpperAscii<Char>);
		return result;
	}

	GenericString<Char> Reverse() const
	{
		GenericString<Char> result;
		result.SetLengthUninitialized(Length());
		Algo::CopyTo(Retro(*this), result.AsRange());
		return result;
	}

	//! Повторить строку n раз
	GenericString<Char> Repeat(size_t n) const
	{
		if(Empty()) return null;
		GenericString<Char> result;
		result.SetLengthUninitialized(Length()*n);
		for(size_t i=0; i<n; i++)
			C::memcpy(result.Data()+Length()*i, Data(), Length()*sizeof(Char));
		return result;
	}

	const Char* begin() const {return cstart;}
	const Char* end() const {return cend;}

protected:
	const Char* cstart;
	const Char* cend;
};

template<typename Char> GenericString<Char> GenericStringView<Char>::MultiReplace(
	ArrayRange<const GenericStringView<Char>> subStrs, ArrayRange<const GenericStringView<Char>> newSubStrs) const
{
	GenericString<Char> result;
	CountRange<Char> counter;
	Algo::MultiReplaceToAdvance(*this, counter, Range::Zip(subStrs, newSubStrs));
	result.SetLengthUninitialized(counter.Counter);
	Algo::MultiReplaceTo(*this, result.AsRange(), Range::Zip(subStrs, newSubStrs));
	return result;
}

#ifdef INTRA_USER_DEFINED_LITERAL_SUPPORT
forceinline constexpr GenericStringView<char> operator"" _v(const char* str, size_t len)
{return GenericStringView<char>(str, len);}

forceinline constexpr GenericStringView<wchar> operator"" _wv(const wchar* str, size_t len)
{return GenericStringView<wchar>(str, len);}
#endif

template<typename T, size_t N> forceinline Meta::EnableIf<
	Meta::IsCharType<T>::_,
GenericStringView<T>> AsRange(const T(&stringLiteral)[N]) {return GenericStringView<T>(stringLiteral);}

template<typename T, size_t N> forceinline Meta::EnableIf<
	Meta::IsCharType<T>::_,
GenericStringView<T>> AsConstRange(const T(&arr)[N]) {return GenericStringView<T>(arr);}

template<typename T, size_t N> forceinline Meta::EnableIf<
	!Meta::IsCharType<T>::_,
ArrayRange<const T>> AsConstRange(const T(&arr)[N]) {return ArrayRange<const T>(arr);}

INTRA_WARNING_POP

}

using Range::GenericStringView;
using Range::StringView;

}

