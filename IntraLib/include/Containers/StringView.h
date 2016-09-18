#pragma once

#include "Containers/ForwardDeclarations.h"
#include "Algorithms/Algorithms.h"
#include "Algorithms/Range.h"
#include "Algorithms/RangeConstruct.h"
#include "Algorithms/RangeIteration.h"
#include "Meta/Type.h"
#include "Meta/Mixins.h"
#include "Memory/Allocator.h"
#include "Containers/AsciiSet.h"

namespace Intra {

forceinline size_t CStringLength(const char* str) {return core::strlen(str);}
forceinline size_t CStringLength(const wchar_t* str) {return core::wcslen(str);}

template<typename U=wchar> forceinline Meta::EnableIf<
	sizeof(U)==sizeof(wchar_t),
size_t> CStringLength(const wchar* str) {return core::wcslen(reinterpret_cast<const wchar_t*>(str));}

template<typename U=wchar> forceinline Meta::EnableIf<
	sizeof(U)!=sizeof(wchar_t),
size_t> CStringLength(const wchar* str)
{
	const wchar* ptr = str-1;
	while(*++ptr!=0) {}
	return size_t(ptr-str);
}

template<typename U=dchar> forceinline Meta::EnableIf<
	sizeof(U)==sizeof(wchar_t),
size_t> CStringLength(const dchar* str) {return core::wcslen(reinterpret_cast<const wchar_t*>(str));}

template<typename U=dchar> forceinline Meta::EnableIf<
	sizeof(U)!=sizeof(wchar_t),
size_t> CStringLength(const dchar* str)
{
	const dchar* ptr = str-1;
	while(*++ptr!=0) {}
	return ptr-str;
}

template<typename Char> struct GenericStringView:
	Meta::ComparableMixin<GenericStringView<Char>, Range::RangeMixin<GenericStringView<Char>, Char, Range::TypeEnum::Array, true>>
{
	typedef Range::RangeMixin<GenericStringView<Char>, Char, Range::TypeEnum::Array, true> BaseRange;
	using BaseRange::MultiReplaceTo;
	using BaseRange::MultiReplaceToAdvance;

	typedef Char value_type;
	typedef Char return_value_type;

	constexpr forceinline GenericStringView(null_t=null): cstart(null), cend(null) {}
	template<size_t N> constexpr forceinline GenericStringView(const Char(&str)[N]): cstart(str), cend(str+(N-1)) {}
	constexpr forceinline GenericStringView(const Char* str, size_t len): cstart(str), cend(str+len) {}
	constexpr forceinline GenericStringView(const Char* startPtr, const Char* endPtr): cstart(startPtr), cend(endPtr) {}

	constexpr forceinline GenericStringView(const GenericStringView& rhs): cstart(rhs.cstart), cend(rhs.cend) {}

	forceinline GenericStringView(const Char* cstr):
		cstart(cstr), cend(cstr==null? null: cstr+CStringLength(cstr)) {}

	constexpr forceinline const Char* Data() const {return cstart;}
	constexpr forceinline const Char* End() const {return cend;}
	constexpr forceinline size_t Length() const {return size_t(cend-cstart);}
	constexpr forceinline Char operator[](size_t n) const {return cstart[n];}

	constexpr forceinline bool Empty() const {return cstart==cend;}
	forceinline Char First() const {INTRA_ASSERT(!Empty()); return *cstart;}
	forceinline void PopFirst() {INTRA_ASSERT(!Empty()); cstart++;}
	forceinline Char Last() const {INTRA_ASSERT(!Empty()); return *(cend-1);}
	forceinline void PopLast() {INTRA_ASSERT(!Empty()); cend--;}

	//Сравнение строк
	bool operator==(const Char* rhs) const
	{
		return (Empty() && (rhs==null || *rhs=='\0')) ||
			(rhs!=null && core::memcmp(cstart, rhs, Length()*sizeof(Char))==0 && rhs[Length()]=='\0');
	}

	bool operator==(const GenericStringView& rhs) const
	{
		return Length()==rhs.Length() &&
			(cstart==rhs.cstart ||
				Range::Equals(
					AsRange(), ArrayRange<const Char>(rhs.cstart, Length())
				));
	}

	bool operator!=(const GenericStringView& rhs) const {return !operator==(rhs);}

	forceinline bool operator==(null_t) const {return Empty();}
	forceinline bool operator!=(null_t) const {return !Empty();}

	forceinline GenericStringView& operator=(null_t) {cstart=cend=null; return *this;}
	forceinline GenericStringView& operator=(const GenericStringView& rhs) {cstart = rhs.cstart; cend = rhs.cend; return *this;}

	bool operator<(const GenericStringView& rhs) const
	{
		return Range::LexCompare(AsRange(), rhs.AsRange())<0;
	}

	bool operator<(const Char* rhs) const
	{
		return (rhs!=null && *rhs!='\0') && (Length()==0 ||
			Range::LexCompare(AsRange(), ArrayRange<const Char>(rhs, Length()))<0);
	}

	bool operator>(const Char* rhs) const
	{
		return cstart!=cend && (rhs==null || *rhs=='\0' ||
			Range::LexCompare(AsRange(), ArrayRange<const Char>(rhs, Length()))>0);
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
	constexpr forceinline GenericStringView opSlice(size_t startIndex, size_t endIndex) const
	{
		return GenericStringView(cstart+startIndex, cstart+endIndex);
	}

	/*constexpr forceinline GenericStringView operator()(size_t startIndex, size_t endIndex) const
	{
		return opSlice(startIndex, endIndex);
	}*/

	forceinline constexpr ArrayRange<const Char> AsRange() const {return {cstart, cend};}

	//! Получить целое число, записанное в строке.
	//! В строке не должно быть пробелов и посторонних символов.
	//! \param[out] success Адрес булевой переменной, в которую нужно записать, удалось преобразование или нет. Может быть null.
	//! \returns Прочитанное из строки число или 0 в случае неудачи.
	long64 ToInt(bool* success=null) const
	{
		if(!Empty() && uint(cstart[0]-'0')<=9)
		{
			GenericStringView nextPart = *this;
			const long64 result = nextPart.template ParseAdvance<long64>();
			if(nextPart.Empty()) return result;
		}

		if(success!=null) *success=false;
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
		real result = nextPart.template ParseAdvance<float>();
		if(nextPart.Empty()) return result;
		return Math::NaN;
	}

	//! Разбить строку на подстроки, используя указанные разделители
	//! \param spaceDelimiters Разделяющие символы, которые не будут входить в подстроки
	//! \param punctDelimiters Разделяющие символы, которые сами будут образовывать подстроки
	//! \returns Массив полученных подстрок
	Array<GenericStringView> Split(const AsciiSet& spaceDelimiters, const AsciiSet& punctDelimiters=null) const
	{
		if(spaceDelimiters==null && punctDelimiters==null)
		{
			Array<GenericStringView> result;
			result.AddLast(*this);
			return result;
		}

		Array<GenericStringView> result;
		size_t pos = 0;
		const size_t len = Length();
		while(pos<len)
		{
			size_t newpos = ~size_t(0);
			if(pos<len)
			{
				for(const Char* p = cstart+pos; p<cend; p++)
				{
					if((uint(*p) & 0xFFFFFF80u) || (!spaceDelimiters[*p] && !punctDelimiters[*p])) continue;
					newpos = size_t(p-cstart);
					break;
				}
			}
			if(newpos == ~size_t(0)) newpos = len;
			if(newpos>pos) result.AddLast(opSlice(pos, newpos));
			if(newpos<len && punctDelimiters(cstart[newpos]))
			{
				result.AddLast(opSlice(newpos, newpos+1));
			}
			pos = newpos+1;
		}
		return result;
	}

	size_t StringEscapeLength(ArrayRange<const StringView> escapeSequences, ArrayRange<const char> chars)
	{
		StringView src = *this;
		size_t len = 0;
		AsciiSet charset = AsciiSet(chars);
		while(len += src.CountUntilAdvanceAny(chars), !src.Empty())
		{
			size_t index = 0;
			chars.Find(src.First(), &index);
			len += 1+escapeSequences[index].Length();
		}
		return len;
	}

	GenericStringView StringEscapeToAdvance(ArrayRange<Char>& dstBuffer, Char escapeChar,
		ArrayRange<const GenericStringView> escapeSequences, ArrayRange<const Char> chars)
	{
		GenericStringView src = *this;
		char* dstBegin = dstBuffer.Begin;
		AsciiSet charset = AsciiSet(chars);
		while(src.ReadUntilAdvance([charset](Char c){return charset.Contains(c);}).CopyToAdvance(dstBuffer), !src.Empty())
		{
			dstBuffer.Put(escapeChar);
			size_t index = 0;
			chars.Find(src.First(), &index);
			escapeSequences[index].CopyToAdvance(dstBuffer);
		}
		return {dstBegin, dstBuffer.Begin};
	}


	GenericStringView ParseIdentifierAdvance(const AsciiSet& notIdentifierFirstChars, const AsciiSet& notIdentifierChars)
	{
		this->TrimLeftAdvance(Range::IsHorSpace<char>);
		if(Empty() || notIdentifierFirstChars.Contains(First())) return null;
		GenericStringView result = *this;
		PopFirst();
		while(!Empty() && !notIdentifierChars.Contains(First())) PopFirst();
		return result.Take(size_t(Data()-result.Data()));
	}

	//! Заменить все вхождения subStr на newSubStr
	GenericString<Char> ReplaceAll(GenericStringView subStr, StringView newSubStr) const
	{
		GenericString<Char> result;
		Range::CountRange<Char> counter;
		MultiReplaceToAdvance(counter, AsRange({Meta::TupleL(subStr, newSubStr)}));
		result.SetLengthUninitialized(counter.Counter);
		MultiReplaceToAdvance(result.AsRange(), AsRange({Meta::TupleL(subStr, newSubStr)}));
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
		Transform(result.AsRange(), Range::ToLowerAscii<Char>);
		return result;
	}

	//! Перевести все символы латиницы в верхний регистр
	GenericString<Char> ToUpperAscii() const
	{
		GenericString<Char> result;
		result.SetLengthUninitialized(Length());
		result.AsRange().Transform(Range::ToUpperAscii<Char>);
		return result;
	}

	GenericString<Char> Reverse() const
	{
		GenericString<Char> result;
		result.SetLengthUninitialized(Length());
		this->Retro().CopyTo(result.AsRange());
		return result;
	}

	//! Повторить строку n раз
	GenericString<Char> Repeat(size_t n) const
	{
		if(Empty()) return null;
		GenericString<Char> result;
		result.SetLengthUninitialized(Length()*n);
		for(size_t i=0; i<n; i++)
			core::memcpy(result.Data()+Length()*i, Data(), Length()*sizeof(Char));
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
	Range::CountRange<Char> counter;
	MultiReplaceToAdvance(counter, Range::Zip(subStrs, newSubStrs));
	result.SetLengthUninitialized(counter.Counter);
	MultiReplaceTo(result.AsRange(), Range::Zip(subStrs, newSubStrs));
	return result;
}

#ifdef INTRA_USER_DEFINED_LITERAL_SUPPORT
forceinline constexpr GenericStringView<char> operator"" _v(const char* str, size_t len)
{
	return GenericStringView<char>(str, len);
}

forceinline constexpr GenericStringView<wchar> operator"" _wv(const wchar* str, size_t len)
{
	return GenericStringView<wchar>(str, len);
}
#endif

namespace Range {

template<typename T, size_t N> forceinline Meta::EnableIf<
	Meta::IsCharType<T>::_,
GenericStringView<T>> AsRange(const T(&stringLiteral)[N]) {return GenericStringView<T>(stringLiteral);}

template<typename T, size_t N> forceinline Meta::EnableIf<
	Meta::IsCharType<T>::_,
GenericStringView<T>> AsConstRange(const T(&arr)[N]) {return GenericStringView<T>(arr);}

template<typename T, size_t N> forceinline Meta::EnableIf<
	!Meta::IsCharType<T>::_,
ArrayRange<const T>> AsConstRange(const T(&arr)[N]) {return ArrayRange<const T>(arr);}

}


}

