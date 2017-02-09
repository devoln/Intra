#pragma once

#include "Platform/CppFeatures.h"
#include "Platform/CppWarnings.h"
#include "Platform/Debug.h"
#include "Meta/Mixins.h"
#include "Containers/ForwardDeclarations.h"
#include "Range/Generators/ArrayRange.h"
#include "Range/Generators/StringView.h"
#include "Algo/String/CStr.h"
#include "Memory/Memory.h"
#include "Memory/Allocator/AllocatorRef.h"
#include "Range/Special/Unicode.h"
#include "Data/Reflection.h"
#include "Algo/Mutation/Fill.h"
#include "Algo/Mutation/Copy.h"
#include "Algo/Mutation/ReplaceSubrange.h"
#include "Range/Decorators/TakeUntil.h"
#include "Range/Compositors/Zip.h"
#include "Algo/String/ToString.h"
#include "Algo/String/MaxLengthOfToString.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_SIGN_CONVERSION


namespace Intra {

template<typename Char, typename A> class GenericString:
	public Memory::AllocatorRef<A,
		Meta::ComparableMixin<GenericString<Char, A>>>
{
	typedef Memory::AllocatorRef<A,
		Meta::ComparableMixin<GenericString<Char, A>>> AllocatorRef;
	class Formatter;
public:
	typedef A Allocator;

	GenericString(const Char* str, Allocator& allocator):
		GenericString(str, (str==null)? 0: Algo::CStringLength(str), allocator) {}

	GenericString(const Char* str):
		GenericString(str, (str==null)? 0: Algo::CStringLength(str)) {}

	template<size_t N> GenericString(const Char(&str)[N], Allocator& allocator):
		GenericString(str, N-1, allocator) {}

	template<size_t N> GenericString(const Char(&str)[N]):
		GenericString(str, N-1) {}

	//Создать пустую строку с указанным аллокатором
	GenericString(null_t, Allocator& allocator):
		AllocatorRef(allocator), mLen(0), mData(null) {}

	//Создать пустую строку с тем же аллокатором, что и строка strWithAllocator
	GenericString(null_t, const GenericString& strWithAllocator):
		AllocatorRef(strWithAllocator), mLen(0), mData(null) {}

	GenericString(null_t=null):
		AllocatorRef(null), mLen(0), mData(null) {}

	explicit GenericString(const Char* str, size_t strLength, Allocator& allocator):
		AllocatorRef(allocator), mLen(0), mData(null)
	{
		SetLengthUninitialized(strLength);
		C::memcpy(mData, str, strLength*sizeof(Char));
	}

	explicit GenericString(const Char* str, size_t strLength):
		mLen(0), mData(null)
	{
		SetLengthUninitialized(strLength);
		C::memcpy(mData, str, strLength*sizeof(Char));
	}

	explicit forceinline GenericString(const Char* startPtr, const Char* endPtr):
		GenericString(startPtr, size_t(endPtr-startPtr)) {}

	explicit forceinline GenericString(const Char* startPtr, const Char* endPtr, Allocator& allocator):
		GenericString(startPtr, size_t(endPtr-startPtr), allocator) {}
	
	explicit forceinline GenericString(size_t initialLength, Char filler):
		mLen(0), mData(null) {SetLength(initialLength, filler);}

	template<typename R=StringView, typename = Meta::EnableIf<
		Range::IsAsConsumableRangeOf<R, Char>::_ &&
		Meta::IsCharType<Range::ValueTypeOfAs<R>>::_
	>> forceinline GenericString(R&& rhs): mLen(0), mData(null)
	{
		SetLengthUninitialized(Range::Count(rhs));
		Algo::CopyTo(Meta::Forward<R>(rhs), AsRange());
	}

	forceinline GenericString(const GenericString& rhs):
		GenericString(rhs.Data(), rhs.Length()) {}
	
	forceinline GenericString(GenericString&& rhs):
		AllocatorRef(Meta::Move(rhs)), mLen(rhs.mLen), mData(rhs.mData)
	{
		rhs.mData = null;
		rhs.mLen = 0;
	}
	
	~GenericString()
	{
		if(mData==null) return;
		AllocatorRef::Free(mData, AllocatorRef::GetAllocationSize(mData));
	}

	//! Получить диапазон из UTF-32 кодов символов
	//UTF8 ByChar() const {return UTF8(mData, mLen);}

	forceinline GenericStringView<const Char> operator()() const {return View();}

	//! Получить подстроку, начиная с кодовой единицы с индексом start и заканчивая end.
	forceinline GenericStringView<const Char> operator()(
		size_t startIndex, size_t endIndex) const
	{return View(startIndex, endIndex);}

	//! Получить подстроку, начиная с кодовой единицы с индексом start и заканчивая end.
	forceinline GenericStringView<const Char> operator()(
		Range::RelativeIndex startIndex, Range::RelativeIndex endIndex) const
	{return View(startIndex, endIndex);}

	//! Получить подстроку, начиная с кодовой единицы с индексом start и до конца.
	forceinline GenericStringView<const Char> operator()(
		Range::RelativeIndex startIndex, Range::RelativeIndexEnd) const
	{return View(startIndex, $);}

	//! Получить подстроку, начиная с кодовой единицы с индексом start и до конца.
	forceinline GenericStringView<const Char> operator()(
		size_t startIndex, Range::RelativeIndexEnd) const
	{return View(startIndex, $);}


	//! Получить указатель на C-строку для передачи в различные C API.
	/**
	Полученный указатель временный и может стать некорректным при любой модификации или удалении этого экземпляра класса.
	Эту функцию следует использовать осторожно с временными объектами:
	const char* cstr = String("a").CStr(); //Ошибка: cstr - висячий указатель.
	strcpy(dst, String("a").CStr()); //Ок: указатель до возврата strcpy корректен
	**/
	const Char* CStr()
	{
		if(Empty())
		{
			static const Char empty[] = {'\0'};
			return empty;
		}
		RequireSpace(1);
		mData[mLen] = '\0';
		return Data();
	}

	//!@{
	//! Возвращает диапазон символов строки.
	/**
	Возвращаемый диапазон корректен только до тех пор, пока длина строки не изменится или экземпляр класса не будет удалён.
	Эту функцию следует использовать осторожно с временными объектами:
	auto range = String("a").AsRange(); //Ошибка: range содержит висячие указатели.
	foo(String("a").AsRange()); //Ок: диапазон до возврата из foo корректен
	**/
	forceinline GenericStringView<Char> AsRange() {return {mData, mLen};}
	forceinline GenericStringView<const Char> AsRange() const {return {mData, mLen};}
	forceinline GenericStringView<const Char> AsConstRange() const {return {mData, mLen};}
	//!@}

	//!@{
	//! Присваивание строк
	GenericString& operator=(GenericString&& rhs)
	{
		if(mData!=null) AllocatorRef::Free(mData, AllocatorRef::GetAllocationSize(mData));
		mData = rhs.mData;
		mLen = rhs.mLen;
		rhs.mData = null;
		rhs.mLen = 0;
		AllocatorRef::operator=(rhs);
		return *this;
	}

	GenericString& operator=(GenericStringView<const Char> rhs)
	{
		SetLengthUninitialized(0);
		SetLengthUninitialized(rhs.Length());
		C::memcpy(Data(), rhs.Data(), rhs.Length()*sizeof(Char));
		return *this;
	}

	template<typename Allocator2> forceinline GenericString& operator=(const GenericString<Char, Allocator2>& rhs)
	{return operator=(rhs.AsRange());}

	forceinline GenericString& operator=(const GenericString& rhs)
	{return operator=(rhs.AsRange());}

	forceinline GenericString& operator=(const Char* rhs)
	{return operator=(GenericStringView<const Char>(rhs));}
	
	template<size_t N> forceinline GenericString& operator=(const Char(&rhs)[N])
	{return operator=(GenericStringView<const Char>(rhs));}

	GenericString& operator=(null_t)
	{
		if(mData!=null)
		{
			AllocatorRef::Free(mData, AllocatorRef::GetAllocationSize(mData));
			mData = null;
		}
		mLen = 0;
		return *this;
	}
	//!@}

	//!@{
	//! Конкатенация строк
	template<size_t N> friend forceinline GenericString operator+(const Char(&lhs)[N], const GenericString& rhs)
	{return GenericStringView<const Char>(lhs)+rhs;}

	template<size_t N> friend forceinline GenericString operator+(const GenericString& lhs, const Char(&rhs)[N])
	{return lhs+GenericStringView<const Char>(rhs);}

	forceinline GenericString& operator+=(GenericStringView<const Char> rhs)
	{
		size_t oldLen = Length();
		SetLengthUninitialized(oldLen+rhs.Length());
		C::memcpy(Data()+oldLen, rhs.Data(), rhs.Length()*sizeof(Char));
		return *this;
	}

	forceinline GenericString& operator+=(Formatter& rhs) {return operator+=(*rhs);}

	template<size_t N> forceinline GenericString& operator+=(const Char(&rhs)[N])
	{return operator+=(GenericStringView<const Char>(rhs));}
	//!@}

	//!@{
	//! Добавление символа к строке
	GenericString operator+(GenericStringView<const Char> rhs) const
	{
		GenericString<Char, Allocator> result;
		result.Reserve(Length()+rhs.Length());
		result = View();
		result += rhs;
		return result;
	}

	forceinline GenericString& operator+=(Char rhs)
	{
		SetLengthUninitialized(Length()+1);
		Last() = rhs;
		return *this;
	}

	//friend GenericString operator+(GenericString&& lhs, dchar rhs) {return Meta::Move(lhs+=rhs);}

	/*GenericString& operator+=(dchar rhs)
	{
		RequireSpace(6);
		const size_t byteCount = UTF32::CharToUTF8Sequence(rhs, mData+Length());
		SetLengthUninitialized(Length()+byteCount);
		return *this;
	}*/
	//!@}

	forceinline bool operator==(null_t) const {return Empty();}
	forceinline bool operator==(const GenericString& rhs) const {return View()==rhs.View();}

	template<size_t N> forceinline bool operator==(const Char(&rhs)[N]) const
	{return View()==GenericStringView<const Char>(rhs);}

	template<size_t N> friend forceinline bool operator==(const Char(&lhs)[N], const GenericString<Char, Allocator>& rhs)
	{return GenericStringView<const Char>(rhs)==lhs.View();}

	forceinline bool operator==(const GenericStringView<const Char>& rhs) const {return View()==rhs;}

	forceinline bool operator>(const GenericStringView<const Char>& rhs) const {return View()>rhs;}
	forceinline bool operator<(const Char* rhs) const {return View()<rhs;}
	forceinline bool operator<(const GenericStringView<const Char>& rhs) const {return View()<rhs;}


	//! Убедиться, что буфер строки имеет достаточно свободного места для хранения minCapacity символов.
	void Reserve(size_t minCapacity)
	{
		size_t currentCapacityInBytes = 0;
		if(mData!=null) currentCapacityInBytes = AllocatorRef::GetAllocationSize(mData);
		if(currentCapacityInBytes >= minCapacity*sizeof(Char)) return;
		Reallocate(minCapacity+mLen/2);
	}


	//! Изменить ёмкость строки, чтобы вместить newCapacity символов.
	//! Реальный размер буфера может быть выше newCapacity. Это зависит от используемого аллокатора.
	void Reallocate(size_t newCapacity)
	{
		if(newCapacity<mLen) newCapacity=mLen;
		size_t newCapacityInBytes = newCapacity*sizeof(Char);
		if(mData==null)
		{
			mData = AllocatorRef::Allocate(newCapacityInBytes, INTRA_SOURCE_INFO);
			return;
		}
		Char* newData = AllocatorRef::Allocate(newCapacityInBytes, INTRA_SOURCE_INFO);
		C::memcpy(newData, mData, mLen*sizeof(Char));
		AllocatorRef::Free(mData, AllocatorRef::GetAllocationSize(mData));
		mData = newData;
	}

	//! Если ёмкость буфера вмещает выше, чем длина строки более, чем на 20%, она уменьшается, чтобы совпадать с длиной.
	//! Реальный размер буфера может получиться больше, чем длина строки. Это зависит от используемого аллокатора.
	void TrimExcessCapacity()
	{if(Capacity()>mLen*5/4) Reallocate(mLen);}

	//! Убедиться, что буфер строки имеет достаточно свободного места для добавления minSpace символов в конец.
	void RequireSpace(size_t minSpace) {Reserve(Length()+minSpace);}

	//! Установить длину строки в newLen.
	//! Если newLen<Length(), то лишние символы отбрасываются.
	//! Если newLen>Length(), то новые символы заполняются с помощью filler.
	void SetLength(size_t newLen, Char filler='\0')
	{
		const size_t oldLength = Length();
		SetLengthUninitialized(newLen);
		if(newLen>oldLength) Algo::Fill(Range::Drop(*this, oldLength), filler);
	}

	//! Установить длину строки в newLen.
	//! Если newLen<Length(), то лишние символы отбрасываются.
	//! Если newLen>Length(), то новые символы остаются неинициализированными
	forceinline void SetLengthUninitialized(size_t newLen)
	{
		Reserve(newLen);
		mLen = newLen;
	}

	//! Количество символов, которое может уместить текущий буфер строки без перераспределения памяти
	forceinline size_t Capacity() const
	{
		if(mData==null) return 0;
		return AllocatorRef::GetAllocationSize(mData)/sizeof(Char);
	}

	forceinline Char* Data() {return mData;}
	forceinline const Char* Data() const {return mData;}
	forceinline Char* End() {return mData+mLen;}
	forceinline const Char* End() const {return mData+mLen;}

	forceinline Char& operator[](size_t index)
	{
		INTRA_ASSERT(index<Length());
		return mData[index];
	}

	forceinline const Char& operator[](size_t index) const
	{
		INTRA_ASSERT(index<Length());
		return mData[index];
	}

	forceinline bool Empty() const {return mLen==0;}
	forceinline Char& First() {INTRA_ASSERT(!Empty()); return *mData;}
	forceinline const Char& First() const {INTRA_ASSERT(!Empty()); return *mData;}
	forceinline Char& Last() {INTRA_ASSERT(!Empty()); return mData[mLen-1];}
	forceinline const Char& Last() const {INTRA_ASSERT(!Empty()); return mData[mLen-1];}
	forceinline void PopLast() {INTRA_ASSERT(!Empty()); mLen--;}
	forceinline size_t Length() const {return mLen;}


	forceinline Char* begin() {return mData;}
	forceinline Char* end() {return mData+mLen;}
	forceinline const Char* begin() const {return mData;}
	forceinline const Char* end() const {return mData+mLen;}

	//! Заменить все вхождения subStr на newSubStr
	static GenericString ReplaceAll(GenericStringView<const Char> str,
		GenericStringView<const Char> subStr, GenericStringView<const Char> newSubStr)
	{
		GenericString result;
		Range::CountRange<Char> counter;
		Algo::MultiReplaceToAdvance(str, counter, Range::AsRange({Meta::TupleL(subStr, newSubStr)}));
		result.SetLengthUninitialized(counter.Counter);
		Algo::MultiReplaceToAdvance(str, result.AsRange(), Range::AsRange({Meta::TupleL(subStr, newSubStr)}));
		return result;
	}

	static GenericString ReplaceAll(GenericStringView<const Char> str, Char c, Char newc)
	{
		if(c==newc) return str;
		GenericString result;
		result.SetLengthUninitialized(str.Length());
		for(size_t i = 0; i<str.Length(); i++)
			result[i] = str[i]==c? newc: str[i];
		return result;
	}

	static GenericString MultiReplace(GenericStringView<const Char> str,
		ArrayRange<const GenericStringView<const Char>> subStrs,
		ArrayRange<const GenericStringView<const Char>> newSubStrs)
	{
		GenericString result;
		Range::CountRange<Char> counter;
		Algo::MultiReplaceToAdvance(str, counter, Range::Zip(subStrs, newSubStrs));
		result.SetLengthUninitialized(counter.Counter);
		Algo::MultiReplaceTo(str, result, Range::Zip(subStrs, newSubStrs));
		return result;
	}

	//! Повторить строку n раз
	static GenericString Repeat(GenericStringView<const Char> str, size_t n)
	{
		if(str.Empty()) return null;
		GenericString result;
		result.SetLengthUninitialized(str.Length()*n);
		for(size_t i = 0; i<n; i++)
			C::memcpy(result.Data()+str.Length()*i, str.Data(), str.Length()*sizeof(Char));
		return result;
	}

#ifdef INTRA_STL_INTERFACE
	typedef char* iterator;
	typedef const char* const_iterator;
	typedef Char value_type;

	forceinline void push_back(Char c) {operator+=(c);}
	forceinline void reserve(size_t capacity) {Reserve(capacity);}
	forceinline void resize(size_t newLength) {SetLength(newLength);}
	forceinline size_t size() const {return Length();}
	forceinline size_t length() const {return Length();}
	forceinline const Char* c_str() {return CStr();} //Не const, в отличие от STL
	forceinline GenericString substr(size_t start, size_t count) const {return operator()(start, start+count);}
#endif


	//! Форматирование строки
	//! \param format Строка, содержащая метки <^>, в которые будут подставляться аргументы.
	//! \param () Используйте скобки для передачи параметров и указания их форматирования
	//! \returns Прокси-объект для форматирования, неявно преобразующийся к String.
	static forceinline Formatter Format(GenericStringView<const Char> format=null) {return Formatter(format);}
	static forceinline Formatter Format(GenericStringView<const Char> format, Allocator& allocator) {return Formatter(format, allocator);}

	static GenericString FromCharRange(ArrayRange<const Char> chars)
	{
		GenericString result;
		result.SetLengthUninitialized(chars.Length());
		C::memcpy(result.Data(), chars.Begin, chars.Length()*sizeof(Char));
		return result;
	}

	forceinline GenericStringView<const Char> View() const
	{return {mData, mLen};}
	
	forceinline GenericStringView<const Char> View(size_t index, Range::RelativeIndexEnd) const
	{
		INTRA_ASSERT(index <= Length());
		return {mData+index, mData+mLen};
	}
	
	forceinline GenericStringView<const Char> View(size_t startIndex, size_t endIndex) const
	{
		INTRA_ASSERT(startIndex <= endIndex);
		INTRA_ASSERT(endIndex <= Length());
		return {mData+startIndex, mData+endIndex};
	}

	GenericStringView<const Char> View(Range::RelativeIndex startIndex, Range::RelativeIndex endIndex) const
	{return View(startIndex.GetRealIndex(mLen), endIndex.GetRealIndex(mLen));}

private:
	size_t mLen;
	mutable Char* mData;


	class Formatter: AllocatorRef
	{
	public:
		operator GenericString()
		{
			INTRA_ASSERT(mFormatBegin==null || mFormatRest.Empty());
			GenericString result;
			result.mData = mData;
			result.mLen = size_t(mBufferRest.Begin-mData);
			result.AllocatorRef::operator=(*this);
			mData = null;
			mBufferRest = null;
			mFormatRest = null;
			mFormatBegin = null;
			mIsEnded = true;
			return result;
		}
		forceinline GenericString operator*() {return operator GenericString();}

	private:
		void WriteNextPart()
		{
			const Char formatSeq[3] = {'<', '^', '>'};
			GenericStringView<const Char> str = Range::TakeUntilAdvance(mFormatRest, GenericStringView<const Char>(formatSeq));
			RequireSpace(str.Length());
			Algo::CopyToAdvance(str, mBufferRest);
			mFormatRest.PopFirstN(3);
		}

		void RequireSpace(size_t newChars)
		{
			if(mBufferRest.Length()>=newChars) return; //Места достаточно, увеличивать буфер не надо
			size_t currentLength = size_t(mBufferRest.Begin-mData);
			size_t oldSize = size_t(mBufferRest.End-mData);
			size_t newSize = (oldSize + Math::Max(oldSize, newChars)*sizeof(Char));
			Char* newData = AllocatorRef::Allocate(newSize, INTRA_SOURCE_INFO);
			C::memcpy(newData, mData, currentLength*sizeof(Char));
			mBufferRest = {newData+currentLength, newData+newSize};
			AllocatorRef::Free(mData, oldSize);
			mData = newData;
		}

		const Char* mFormatBegin;
		GenericStringView<const Char> mFormatRest;
		Char* mData;
		ArrayRange<Char> mBufferRest;
		bool mIsEnded;

		Formatter(const Formatter&) = delete;
		Formatter& operator=(const Formatter&) = delete;

	public:
		Formatter(GenericStringView<const Char> formatStr):
			mFormatBegin(formatStr.Data()), mFormatRest(formatStr),
			mData(null), mBufferRest(null), mIsEnded(false) {init();}

		Formatter(GenericStringView<const Char> formatStr, Allocator& myAllocator):
			AllocatorRef(myAllocator),
			mFormatBegin(formatStr.Data()), mFormatRest(formatStr),
			mData(null), mBufferRest(null), mIsEnded(false) {init();}

		Formatter(Formatter&& rhs): AllocatorRef(rhs),
			mFormatBegin(rhs.mFormatBegin), mFormatRest(rhs.mFormatRest),
			mData(rhs.mData), mBufferRest(rhs.mBufferRest), mIsEnded(rhs.mIsEnded)
		{
			rhs.mFormatRest = null;
			rhs.mFormatBegin  = null;
			rhs.mData = null;
			rhs.mBufferRest = null;
			rhs.mIsEnded = true;
		}

		~Formatter() {INTRA_ASSERT(mBufferRest.Empty());}

		template<typename T, typename... Args> Formatter& operator()(const T& value, Args&&... args)
		{
			INTRA_ASSERT(!mIsEnded);
			const size_t maxLen = Algo::MaxLengthOfToString(value, Meta::Forward<Args>(args)...);
			RequireSpace(maxLen);
			Range::CountRange<char> realLenCounter;
			Algo::ToString(realLenCounter, value, Meta::Forward<Args>(args)...);
			INTRA_ASSERT(maxLen>=realLenCounter.Counter);
			Algo::ToString(mBufferRest, value, Meta::Forward<Args>(args)...);
			if(mFormatBegin!=null) WriteNextPart();
			return *this;
		}

		template<typename T, size_t N> forceinline
		Formatter& operator()(const T(&arr)[N]) {return operator()(Range::AsRange(arr));}

		forceinline Formatter& operator()(const Char* arr) {return operator()(GenericStringView<const Char>(arr));}

		/*template<typename T, typename Char2, size_t N1, size_t N2, size_t N3, typename... Args> Formatter& operator()(const T& value,
			const Char2(&separator)[N1], const Char2(&leftBracket)[N2], const Char2(&rightBracket)[N3], Args&&... args)
		{
			return operator()(value, GenericStringView<const Char2>(separator),
				GenericStringView<const Char2>(leftBracket), GenericStringView<const Char2>(rightBracket), Meta::Forward<Args>(args)...);
		}

		template<typename T, size_t N, typename... Args> Formatter& operator()(const T(&value)[N], Args&&... args)
		{return operator()(Range::AsRange(value), Meta::Forward<Args>(args)...);}*/

	private:
		void init()
		{
			size_t initialSize = size_t(mFormatRest.End()-mFormatBegin)+8u;
			mData = AllocatorRef::Allocate(initialSize, INTRA_SOURCE_INFO);
			mBufferRest = {mData, initialSize};
			if(mFormatBegin!=null) WriteNextPart();
		}
	};
};

namespace Meta {

template<typename Char, typename Allocator>
struct IsTriviallyRelocatable<GenericString<Char, Allocator>>:
	TypeFromValue<bool, true> {};


namespace D {
template<typename Char, typename Allocator> struct CommonTypeRef<GenericStringView<Char>, GenericString<Char, Allocator>> { typedef GenericString<Char, Allocator> _; };
template<typename Char, typename Allocator> struct CommonTypeRef<GenericStringView<Char>&, GenericString<Char, Allocator>> { typedef GenericString<Char, Allocator> _; };
template<typename Char, typename Allocator> struct CommonTypeRef<GenericStringView<Char>&&, GenericString<Char, Allocator>> { typedef GenericString<Char, Allocator> _; };
template<typename Char, typename Allocator> struct CommonTypeRef<const GenericStringView<Char>&, GenericString<Char, Allocator>> { typedef GenericString<Char, Allocator> _; };

template<typename Char, typename Allocator> struct CommonTypeRef<GenericStringView<Char>, GenericString<Char, Allocator>&> { typedef GenericString<Char, Allocator> _; };
template<typename Char, typename Allocator> struct CommonTypeRef<GenericStringView<Char>&, GenericString<Char, Allocator>&> { typedef GenericString<Char, Allocator> _; };
template<typename Char, typename Allocator> struct CommonTypeRef<GenericStringView<Char>&&, GenericString<Char, Allocator>&> { typedef GenericString<Char, Allocator> _; };
template<typename Char, typename Allocator> struct CommonTypeRef<const GenericStringView<Char>&, GenericString<Char, Allocator>&> { typedef GenericString<Char, Allocator> _; };

template<typename Char, typename Allocator> struct CommonTypeRef<GenericStringView<Char>, GenericString<Char, Allocator>&&> { typedef GenericString<Char, Allocator> _; };
template<typename Char, typename Allocator> struct CommonTypeRef<GenericStringView<Char>&, GenericString<Char, Allocator>&&> { typedef GenericString<Char, Allocator> _; };
template<typename Char, typename Allocator> struct CommonTypeRef<GenericStringView<Char>&&, GenericString<Char, Allocator>&&> { typedef GenericString<Char, Allocator> _; };
template<typename Char, typename Allocator> struct CommonTypeRef<const GenericStringView<Char>&, GenericString<Char, Allocator>&&> { typedef GenericString<Char, Allocator> _; };

template<typename Char, typename Allocator> struct CommonTypeRef<GenericStringView<Char>, const GenericString<Char, Allocator>&> { typedef GenericString<Char, Allocator> _; };
template<typename Char, typename Allocator> struct CommonTypeRef<GenericStringView<Char>&, const GenericString<Char, Allocator>&> { typedef GenericString<Char, Allocator> _; };
template<typename Char, typename Allocator> struct CommonTypeRef<GenericStringView<Char>&&, const GenericString<Char, Allocator>&> { typedef GenericString<Char, Allocator> _; };
template<typename Char, typename Allocator> struct CommonTypeRef<const GenericStringView<Char>&, const GenericString<Char, Allocator>&> { typedef GenericString<Char, Allocator> _; };



template<typename Char, typename Allocator> struct CommonTypeRef<GenericString<Char, Allocator>, GenericStringView<Char>> { typedef GenericString<Char, Allocator> _; };
template<typename Char, typename Allocator> struct CommonTypeRef<GenericString<Char, Allocator>, GenericStringView<Char>&> { typedef GenericString<Char, Allocator> _; };
template<typename Char, typename Allocator> struct CommonTypeRef<GenericString<Char, Allocator>, GenericStringView<Char>&&> { typedef GenericString<Char, Allocator> _; };
template<typename Char, typename Allocator> struct CommonTypeRef<GenericString<Char, Allocator>, const GenericStringView<Char>&> { typedef GenericString<Char, Allocator> _; };

template<typename Char, typename Allocator> struct CommonTypeRef<GenericString<Char, Allocator>&, GenericStringView<Char>> { typedef GenericString<Char, Allocator> _; };
template<typename Char, typename Allocator> struct CommonTypeRef<GenericString<Char, Allocator>&, GenericStringView<Char>&> { typedef GenericString<Char, Allocator> _; };
template<typename Char, typename Allocator> struct CommonTypeRef<GenericString<Char, Allocator>&, GenericStringView<Char>&&> { typedef GenericString<Char, Allocator> _; };
template<typename Char, typename Allocator> struct CommonTypeRef<GenericString<Char, Allocator>&, const GenericStringView<Char>&> { typedef GenericString<Char, Allocator> _; };

template<typename Char, typename Allocator> struct CommonTypeRef<GenericString<Char, Allocator>&&, GenericStringView<Char>> { typedef GenericString<Char, Allocator> _; };
template<typename Char, typename Allocator> struct CommonTypeRef<GenericString<Char, Allocator>&&, GenericStringView<Char>&> { typedef GenericString<Char, Allocator> _; };
template<typename Char, typename Allocator> struct CommonTypeRef<GenericString<Char, Allocator>&&, GenericStringView<Char>&&> { typedef GenericString<Char, Allocator> _; };
template<typename Char, typename Allocator> struct CommonTypeRef<GenericString<Char, Allocator>&&, const GenericStringView<Char>&> { typedef GenericString<Char, Allocator> _; };

template<typename Char, typename Allocator> struct CommonTypeRef<const GenericString<Char, Allocator>&, GenericStringView<Char>> { typedef GenericString<Char, Allocator> _; };
template<typename Char, typename Allocator> struct CommonTypeRef<const GenericString<Char, Allocator>&, GenericStringView<Char>&> { typedef GenericString<Char, Allocator> _; };
template<typename Char, typename Allocator> struct CommonTypeRef<const GenericString<Char, Allocator>&, GenericStringView<Char>&&> { typedef GenericString<Char, Allocator> _; };
template<typename Char, typename Allocator> struct CommonTypeRef<const GenericString<Char, Allocator>&, const GenericStringView<Char>&> { typedef GenericString<Char, Allocator> _; };



template<typename Char, typename Allocator> struct CommonTypeRef<GenericStringView<const Char>, GenericString<Char, Allocator>> { typedef GenericString<Char, Allocator> _; };
template<typename Char, typename Allocator> struct CommonTypeRef<GenericStringView<const Char>&, GenericString<Char, Allocator>> { typedef GenericString<Char, Allocator> _; };
template<typename Char, typename Allocator> struct CommonTypeRef<GenericStringView<const Char>&&, GenericString<Char, Allocator>> { typedef GenericString<Char, Allocator> _; };
template<typename Char, typename Allocator> struct CommonTypeRef<const GenericStringView<const Char>&, GenericString<Char, Allocator>> { typedef GenericString<Char, Allocator> _; };

template<typename Char, typename Allocator> struct CommonTypeRef<GenericStringView<const Char>, GenericString<Char, Allocator>&> { typedef GenericString<Char, Allocator> _; };
template<typename Char, typename Allocator> struct CommonTypeRef<GenericStringView<const Char>&, GenericString<Char, Allocator>&> { typedef GenericString<Char, Allocator> _; };
template<typename Char, typename Allocator> struct CommonTypeRef<GenericStringView<const Char>&&, GenericString<Char, Allocator>&> { typedef GenericString<Char, Allocator> _; };
template<typename Char, typename Allocator> struct CommonTypeRef<const GenericStringView<const Char>&, GenericString<Char, Allocator>&> { typedef GenericString<Char, Allocator> _; };

template<typename Char, typename Allocator> struct CommonTypeRef<GenericStringView<const Char>, GenericString<Char, Allocator>&&> { typedef GenericString<Char, Allocator> _; };
template<typename Char, typename Allocator> struct CommonTypeRef<GenericStringView<const Char>&, GenericString<Char, Allocator>&&> { typedef GenericString<Char, Allocator> _; };
template<typename Char, typename Allocator> struct CommonTypeRef<GenericStringView<const Char>&&, GenericString<Char, Allocator>&&> { typedef GenericString<Char, Allocator> _; };
template<typename Char, typename Allocator> struct CommonTypeRef<const GenericStringView<const Char>&, GenericString<Char, Allocator>&&> { typedef GenericString<Char, Allocator> _; };

template<typename Char, typename Allocator> struct CommonTypeRef<GenericStringView<const Char>, const GenericString<Char, Allocator>&> { typedef GenericString<Char, Allocator> _; };
template<typename Char, typename Allocator> struct CommonTypeRef<GenericStringView<const Char>&, const GenericString<Char, Allocator>&> { typedef GenericString<Char, Allocator> _; };
template<typename Char, typename Allocator> struct CommonTypeRef<GenericStringView<const Char>&&, const GenericString<Char, Allocator>&> { typedef GenericString<Char, Allocator> _; };
template<typename Char, typename Allocator> struct CommonTypeRef<const GenericStringView<const Char>&, const GenericString<Char, Allocator>&> { typedef GenericString<Char, Allocator> _; };



template<typename Char, typename Allocator> struct CommonTypeRef<GenericString<Char, Allocator>, GenericStringView<const Char>> { typedef GenericString<Char, Allocator> _; };
template<typename Char, typename Allocator> struct CommonTypeRef<GenericString<Char, Allocator>, GenericStringView<const Char>&> { typedef GenericString<Char, Allocator> _; };
template<typename Char, typename Allocator> struct CommonTypeRef<GenericString<Char, Allocator>, GenericStringView<const Char>&&> { typedef GenericString<Char, Allocator> _; };
template<typename Char, typename Allocator> struct CommonTypeRef<GenericString<Char, Allocator>, const GenericStringView<const Char>&> { typedef GenericString<Char, Allocator> _; };

template<typename Char, typename Allocator> struct CommonTypeRef<GenericString<Char, Allocator>&, GenericStringView<const Char>> { typedef GenericString<Char, Allocator> _; };
template<typename Char, typename Allocator> struct CommonTypeRef<GenericString<Char, Allocator>&, GenericStringView<const Char>&> { typedef GenericString<Char, Allocator> _; };
template<typename Char, typename Allocator> struct CommonTypeRef<GenericString<Char, Allocator>&, GenericStringView<const Char>&&> { typedef GenericString<Char, Allocator> _; };
template<typename Char, typename Allocator> struct CommonTypeRef<GenericString<Char, Allocator>&, const GenericStringView<const Char>&> { typedef GenericString<Char, Allocator> _; };

template<typename Char, typename Allocator> struct CommonTypeRef<GenericString<Char, Allocator>&&, GenericStringView<const Char>> { typedef GenericString<Char, Allocator> _; };
template<typename Char, typename Allocator> struct CommonTypeRef<GenericString<Char, Allocator>&&, GenericStringView<const Char>&> { typedef GenericString<Char, Allocator> _; };
template<typename Char, typename Allocator> struct CommonTypeRef<GenericString<Char, Allocator>&&, GenericStringView<const Char>&&> { typedef GenericString<Char, Allocator> _; };
template<typename Char, typename Allocator> struct CommonTypeRef<GenericString<Char, Allocator>&&, const GenericStringView<const Char>&> { typedef GenericString<Char, Allocator> _; };

template<typename Char, typename Allocator> struct CommonTypeRef<const GenericString<Char, Allocator>&, GenericStringView<const Char>> { typedef GenericString<Char, Allocator> _; };
template<typename Char, typename Allocator> struct CommonTypeRef<const GenericString<Char, Allocator>&, GenericStringView<const Char>&> { typedef GenericString<Char, Allocator> _; };
template<typename Char, typename Allocator> struct CommonTypeRef<const GenericString<Char, Allocator>&, GenericStringView<const Char>&&> { typedef GenericString<Char, Allocator> _; };
template<typename Char, typename Allocator> struct CommonTypeRef<const GenericString<Char, Allocator>&, const GenericStringView<const Char>&> { typedef GenericString<Char, Allocator> _; };

}


static_assert(TypeEquals<CommonTypeRef<String, StringView>, String>::_, "ERROR!");

}

template<typename Char, typename Allocator> forceinline
bool operator==(const GenericStringView<Char>& lhs, const GenericString<Char, Allocator>& rhs)
{return lhs==rhs.View();}

template<typename Char> GenericString<Char> operator+(GenericStringView<const Char> lhs, GenericStringView<const Char> rhs)
{
	GenericString<Char> result;
	result.Reserve(lhs.Length()+rhs.Length());
	result += lhs;
	result += rhs;
	return result;
}

template<typename Char> GenericString<Char> operator+(GenericStringView<const Char> lhs, GenericStringView<Char> rhs)
{return lhs+GenericStringView<const Char>(rhs);}

template<typename Char> GenericString<Char> operator+(GenericStringView<Char> lhs, GenericStringView<const Char> rhs)
{return GenericStringView<const Char>(lhs)+rhs;}

template<typename Char> GenericString<Char> operator+(GenericStringView<Char> lhs, GenericStringView<Char> rhs)
{return GenericStringView<const Char>(lhs)+GenericStringView<const Char>(rhs);}

template<typename Char, class Allocator> GenericString<Char, Allocator> operator+(
	const GenericString<Char, Allocator>& lhs, GenericStringView<const Char> rhs)
{
	GenericString<Char, Allocator> result;
	result.Reserve(lhs.Length()+rhs.Length());
	result += lhs;
	result += rhs;
	return result;
}

template<typename Char, class Allocator> GenericString<Char, Allocator> operator+(
	GenericStringView<const Char> lhs, const GenericString<Char, Allocator>& rhs)
{
	GenericString<Char, Allocator> result;
	result.Reserve(lhs.Length()+rhs.Length());
	result += lhs;
	result += rhs;
	return result;
}

template<typename Char> GenericString<Char> operator+(Char lhs, GenericStringView<const Char> rhs)
{
	GenericString<Char> result;
	result.Reserve(1+rhs.Length());
	result += lhs;
	result += rhs;
	return result;
}

template<typename Char, typename Allocator> forceinline
GenericString<Char, Allocator> operator+(Char lhs, const GenericString<Char, Allocator>& rhs)
{
	GenericString<Char, Allocator> result(null, rhs);
	result.Reserve(1+rhs.Length());
	result += lhs;
	result += rhs;
	return result;
}

template<typename Char, typename Allocator> forceinline
GenericString<Char, Allocator> operator+(GenericString<Char, Allocator>&& lhs, GenericStringView<const Char> rhs)
{return Meta::Move(lhs+=rhs);}


template<typename Char, typename Allocator> forceinline
GenericString<Char, Allocator> operator+(GenericString<Char, Allocator>&& lhs, Char rhs)
{return Meta::Move(lhs+=rhs);}

inline String operator+(StringView lhs, dchar rhs)
{
	String result;
	result.Reserve(lhs.Length()+5);
	result += lhs;
	auto byteCount = UTF32::CharToUTF8Sequence(rhs, result.Data()+lhs.Length());
	result.SetLengthUninitialized(lhs.Length()+byteCount);
	return result;
}

template<typename Char, size_t N> forceinline
GenericString<Char> operator+(const Char(&lhs)[N], GenericStringView<const Char> rhs)
{return GenericStringView<const Char>(lhs)+rhs;}

template<typename Char, size_t N> forceinline
GenericString<Char> operator+(GenericStringView<const Char> lhs, const Char(&rhs)[N])
{return lhs+GenericStringView<const Char>(rhs);}

template<typename Char> GenericString<Char> operator+(GenericStringView<const Char> lhs, Char rhs)
{
	GenericString<Char> result;
	result.Reserve(lhs.Length()+1);
	result += lhs;
	result += rhs;
	return result;
}

#ifdef INTRA_USER_DEFINED_LITERALS_SUPPORT
forceinline String operator ""_s(const char* str, size_t len)
{return String(str, len);}

forceinline WString operator ""_w(const wchar* str, size_t len)
{return WString(str, len);}

forceinline DString operator ""_d(const dchar* str, size_t len)
{return DString(str, len);}
#endif


template<typename T, typename... Args> forceinline String ToString(const T& value, Args&&... args)
{return String::Format()(value, Meta::Forward<Args>(args)...);}

forceinline const String& ToString(const String& value) {return value;}
forceinline StringView ToString(const StringView& value) {return value;}
forceinline StringView ToString(const char* value) {return StringView(value);}
template<size_t N> forceinline StringView ToString(const char(&value)[N]) {return StringView(value);}


template<typename T, typename... Args> forceinline WString ToWString(const T& value, Args&&... args)
{return WString::Format()(value, Meta::Forward<Args>(args)...);}

forceinline const WString& ToWString(const WString& value) {return value;}
forceinline WStringView ToWString(const WStringView& value) {return value;}
forceinline WStringView ToWString(const wchar* value) {return WStringView(value);}
template<size_t N> forceinline WStringView ToWString(const wchar(&value)[N]) {return WStringView(value);}


template<typename T, typename... Args> forceinline DString ToDString(const T& value, Args&&... args)
{return DString::Format()(value, Meta::Forward<Args>(args)...);}

forceinline const DString& ToDString(const DString& value) {return value;}
forceinline DStringView ToDString(const DStringView& value) {return value;}
forceinline DStringView ToDString(const dchar* value) {return DStringView(value);}
template<size_t N> forceinline DStringView ToDString(const dchar(&value)[N]) {return DStringView(value);}

}

INTRA_WARNING_POP
