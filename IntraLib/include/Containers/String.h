#pragma once

#include "Meta/Mixins.h"
#include "Containers/ForwardDeclarations.h"
#include "Range/StringView.h"
#include "Algo/String/CStr.h"
#include "Memory/Memory.h"
#include "Memory/AllocatorInterface.h"
#include "Range/Unicode.h"
#include "Data/Reflection.h"
#include "Algo/Mutation/Fill.h"
#include "Algo/Mutation/Copy.h"
#include "Range/Construction/TakeUntil.h"

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4365)
#endif

namespace Intra {

template<typename Char, typename AllocatorType> class GenericString:
	public Memory::AllocatorRef<AllocatorType,
		Meta::ComparableMixin<GenericString<Char, AllocatorType>>>
{
	typedef Memory::AllocatorRef<AllocatorType,
		Meta::ComparableMixin<GenericString<Char, AllocatorType>>> AllocatorRef;
	class Formatter;
public:
	typedef AllocatorType Allocator;

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
		AllocatorRef(allocator), len(0), data(null) {}

	//Создать пустую строку с тем же аллокатором, что и строка strWithAllocator
	GenericString(null_t, const GenericString& strWithAllocator):
		AllocatorRef(strWithAllocator), len(0), data(null) {}

	GenericString(null_t=null):
		AllocatorRef(null), len(0), data(null) {}

	explicit GenericString(const Char* str, size_t strLength, Allocator& allocator):
		AllocatorRef(allocator), len(0), data(null)
	{
		SetLengthUninitialized(strLength);
		C::memcpy(data, str, strLength*sizeof(Char));
	}

	explicit GenericString(const Char* str, size_t strLength):
		len(0), data(null)
	{
		SetLengthUninitialized(strLength);
		C::memcpy(data, str, strLength*sizeof(Char));
	}

	explicit forceinline GenericString(const Char* startPtr, const Char* endPtr):
		GenericString(startPtr, size_t(endPtr-startPtr)) {}

	explicit forceinline GenericString(const Char* startPtr, const Char* endPtr, Allocator& allocator):
		GenericString(startPtr, size_t(endPtr-startPtr), allocator) {}
	
	explicit forceinline GenericString(size_t initialLength, Char filler):
		len(0), data(null) {SetLength(initialLength, filler);}

	template<typename R=StringView, typename = Meta::EnableIf<
		!Range::IsInfiniteRange<R>::_ &&
		(Range::IsInputRange<R>::_ && !Meta::IsConst<R>::_ ||
			Range::IsForwardRange<R>::_) &&
		Meta::TypeEquals<Range::ValueTypeOf<R>, Char>::_
	>> forceinline
	GenericString(R&& rhs): len(0), data(null)
	{
		SetLengthUninitialized(Range::Count(rhs));
		Algo::CopyTo(Meta::Forward<R>(rhs), AsRange());
	}

	forceinline GenericString(const GenericString& rhs):
		GenericString(rhs.Data(), rhs.Length()) {}
	
	forceinline GenericString(GenericString&& rhs):
		AllocatorRef(rhs), len(rhs.len), data(rhs.data)
	{
		rhs.data = null;
		rhs.len = 0;
	}
	
	~GenericString()
	{
		if(data==null) return;
		AllocatorRef::Free(data, AllocatorRef::GetAllocationSize(data));
	}

	//! Получить диапазон из UTF-32 кодов символов
	UTF8 ByChar() const {return UTF8(data, len);}

	forceinline GenericStringView<Char> operator()() const {return View();}

	//! Получить подстроку, начиная с кодовой единицы с индексом start и заканчивая end.
	forceinline GenericStringView<Char> operator()(size_t startIndex, size_t endIndex) const {return View(startIndex, endIndex);}

	//! Получить подстроку, начиная с кодовой единицы с индексом start и заканчивая end.
	forceinline GenericStringView<Char> operator()(Range::RelativeIndex startIndex, Range::RelativeIndex endIndex) const {return View(startIndex, endIndex);}

	//! Получить подстроку, начиная с кодовой единицы с индексом start и до конца.
	forceinline GenericStringView<Char> operator()(Range::RelativeIndex startIndex, Range::RelativeIndexEnd) const {return View(startIndex, $);}

	//! Получить подстроку, начиная с кодовой единицы с индексом start и до конца.
	forceinline GenericStringView<Char> operator()(size_t startIndex, Range::RelativeIndexEnd) const {return View(startIndex, $);}


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
		data[len] = '\0';
		return Data();
	}

	//!@{
	//! Возвращает диапазон символов строки.
	/**
	Возвращаемый диапазон корректен только до тех пор, пока длина строки не изменится или экземпляр класса не будет удалён.
	Эту функцию следует использовать осторожно с временными объектами:
	ArrayRange<char> range = String("a").AsRange(); //Ошибка: range содержит висячие указатели.
	foo(String("a").AsRange()); //Ок: диапазон до возврата strcpy корректен
	**/
	forceinline ArrayRange<Char> AsRange() {return {data, len};}
	forceinline ArrayRange<const Char> AsRange() const {return {data, len};}
	forceinline ArrayRange<const Char> AsConstRange() const {return {data, len};}
	//!@}

	//!@{
	//! Присваивание строк
	GenericString& operator=(String&& rhs)
	{
		if(data!=null) AllocatorRef::Free(data, AllocatorRef::GetAllocationSize(data));
		data = rhs.data;
		len = rhs.len;
		rhs.data = null;
		rhs.len = 0;
		AllocatorRef::operator=(rhs);
		return *this;
	}

	GenericString& operator=(GenericStringView<Char> rhs)
	{
		SetLengthUninitialized(0);
		SetLengthUninitialized(rhs.Length());
		C::memcpy(Data(), rhs.Data(), rhs.Length()*sizeof(Char));
		return *this;
	}

	template<typename Allocator2> forceinline GenericString& operator=(const GenericString<Char, Allocator2>& rhs)
	{
		return operator=(rhs.View());
	}

	forceinline GenericString& operator=(const GenericString& rhs)
	{
		return operator=(rhs.View());
	}

	forceinline GenericString& operator=(const Char* rhs) {return operator=(GenericStringView<Char>(rhs));}
	template<size_t N> forceinline GenericString& operator=(const Char(&rhs)[N]) {return operator=(GenericStringView<Char>(rhs));}

	GenericString& operator=(null_t)
	{
		if(data!=null)
		{
			AllocatorRef::Free(data, AllocatorRef::GetAllocationSize(data));
			data = null;
		}
		len = 0;
		return *this;
	}
	//!@}

	//!@{
	//! Конкатенация строк
	template<size_t N> friend forceinline GenericString operator+(const Char(&lhs)[N], const GenericString& rhs)
	{return GenericStringView<Char>(lhs)+rhs;}

	template<size_t N> friend forceinline GenericString operator+(const GenericString& lhs, const Char(&rhs)[N])
	{return lhs+GenericStringView<Char>(rhs);}

	forceinline GenericString& operator+=(GenericStringView<Char> rhs)
	{
		size_t oldLen = Length();
		SetLengthUninitialized(oldLen+rhs.Length());
		C::memcpy(Data()+oldLen, rhs.Data(), rhs.Length()*sizeof(Char));
		return *this;
	}

	forceinline GenericString& operator+=(Formatter& rhs) {return operator+=(*rhs);}

	template<size_t N> forceinline GenericString& operator+=(const Char(&rhs)[N])
	{return operator+=(GenericStringView<Char>(rhs));}
	//String& operator+=(const char* rhs) {return operator+=(View(), StringView(rhs));}
	//!@}

	//!@{
	//! Добавление символа к строке
	GenericString operator+(GenericStringView<Char> rhs) const
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
		const size_t byteCount = UTF32::CharToUTF8Sequence(rhs, data+Length());
		SetLengthUninitialized(Length()+byteCount);
		return *this;
	}*/
	//!@}

	forceinline bool operator==(null_t) const {return Empty();}
	forceinline bool operator==(const GenericString& rhs) const {return View()==rhs.View();}
	template<size_t N> forceinline bool operator==(const Char(&rhs)[N]) const {return View()==GenericStringView<Char>(rhs);}
	template<size_t N> friend forceinline bool operator==(const Char(&lhs)[N], const GenericString<Char, Allocator>& rhs) {return GenericStringView<Char>(rhs)==lhs.View();}
	forceinline bool operator==(const GenericStringView<Char>& rhs) const {return View()==rhs;}

	forceinline bool operator>(const GenericStringView<Char>& rhs) const {return View()>rhs;}
	forceinline bool operator<(const Char* rhs) const {return View()<rhs;}
	forceinline bool operator<(const GenericStringView<Char>& rhs) const {return View()<rhs;}


	//! Убедиться, что буфер строки имеет достаточно свободного места для хранения minCapacity символов.
	void Reserve(size_t minCapacity)
	{
		size_t currentCapacityInBytes = 0;
		if(data!=null) currentCapacityInBytes = AllocatorRef::GetAllocationSize(data);
		if(currentCapacityInBytes >= minCapacity*sizeof(Char)) return;
		Reallocate(minCapacity+len/2);
	}


	//! Изменить ёмкость строки, чтобы вместить newCapacity символов.
	//! Реальный размер буфера может быть выше newCapacity. Это зависит от используемого аллокатора.
	void Reallocate(size_t newCapacity)
	{
		if(newCapacity<len) newCapacity=len;
		size_t newCapacityInBytes = newCapacity*sizeof(Char);
		if(data==null)
		{
			data = AllocatorRef::Allocate(newCapacityInBytes, INTRA_SOURCE_INFO);
			return;
		}
		Char* newData = AllocatorRef::Allocate(newCapacityInBytes, INTRA_SOURCE_INFO);
		C::memcpy(newData, data, len*sizeof(Char));
		AllocatorRef::Free(data, AllocatorRef::GetAllocationSize(data));
		data = newData;
	}

	//! Если ёмкость буфера вмещает выше, чем длина строки более, чем на 20%, она уменьшается, чтобы совпадать с длиной.
	//! Реальный размер буфера может получиться больше, чем длина строки. Это зависит от используемого аллокатора.
	void TrimExcessCapacity()
	{
		if(Capacity()>len*5/4) Reallocate(len);
	}

	//! Убедиться, что буфер строки имеет достаточно свободного места для добавления minSpace символов в конец.
	void RequireSpace(size_t minSpace) {Reserve(Length()+minSpace);}

	//! Установить длину строки в newLen.
	//! Если newLen<Length(), то лишние символы отбрасываются.
	//! Если newLen>Length(), то новые символы заполняются с помощью filler.
	void SetLength(size_t newLen, Char filler='\0')
	{
		const size_t oldLength = Length();
		SetLengthUninitialized(newLen);
		if(newLen>oldLength) Algo::Fill(AsRange().Drop(oldLength), filler);
	}

	//! Установить длину строки в newLen.
	//! Если newLen<Length(), то лишние символы отбрасываются.
	//! Если newLen>Length(), то новые символы остаются неинициализированными
	forceinline void SetLengthUninitialized(size_t newLen)
	{
		Reserve(newLen);
		len = newLen;
	}

	//! Количество символов, которое может уместить текущий буфер строки без перераспределения памяти
	forceinline size_t Capacity() const {return data==null? 0: AllocatorRef::GetAllocationSize(data)/sizeof(Char);}

	forceinline Char* Data() {return data;}
	forceinline const Char* Data() const {return data;}
	forceinline Char* End() {return data+len;}
	forceinline const Char* End() const {return data+len;}

	forceinline Char& operator[](size_t index) {INTRA_ASSERT(index<Length()); return data[index];}
	forceinline const Char& operator[](size_t index) const {INTRA_ASSERT(index<Length()); return data[index];}

	forceinline bool Empty() const {return len==0;}
	forceinline Char& First() {INTRA_ASSERT(!Empty()); return *data;}
	forceinline const Char& First() const {INTRA_ASSERT(!Empty()); return *data;}
	forceinline Char& Last() {INTRA_ASSERT(!Empty()); return data[len-1];}
	forceinline const Char& Last() const {INTRA_ASSERT(!Empty()); return data[len-1];}
	forceinline void PopLast() {INTRA_ASSERT(!Empty()); len--;}
	forceinline size_t Length() const {return len;}



	forceinline Char* begin() {return data;}
	forceinline Char* end() {return data+len;}
	forceinline const Char* begin() const {return data;}
	forceinline const Char* end() const {return data+len;}

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
	static forceinline Formatter Format(GenericStringView<Char> format=null) {return Formatter(format);}
	static forceinline Formatter Format(GenericStringView<Char> format, Allocator& allocator) {return Formatter(format, allocator);}

	static GenericString FromCharRange(ArrayRange<const Char> chars)
	{
		GenericString result;
		result.SetLengthUninitialized(chars.Length());
		memcpy(result.Data(), chars.Begin, chars.Length()*sizeof(Char));
		return result;
	}

	forceinline GenericStringView<Char> View() const {return {data, len};}
	forceinline GenericStringView<Char> View(size_t index, Range::RelativeIndexEnd) const {return {data+index, data+len};}
	forceinline GenericStringView<Char> View(size_t startIndex, size_t endIndex) const {return {data+startIndex, data+endIndex};}

	GenericStringView<Char> View(Range::RelativeIndex startIndex, Range::RelativeIndex endIndex) const
	{
		return View(startIndex.GetRealIndex(len), endIndex.GetRealIndex(len));
	}


	//!@{
	//! Вернуть строку, полученную объединением строк с разделителем
	//! \param strings Объединяемые строки
	//! \param delimiter Разделитель между объединяемыми строками
	//! \param prefix Приставка, прибавляемая к началу каждой строки
	//! \param postfix Постфикс, прибавляемый к концу каждой строки
	template<typename R> static Meta::EnableIf<
		Range::IsFiniteForwardRange<R>::_,
	GenericString> Join(const R& strs,
		GenericStringView<Char> delim=" ", GenericStringView<Char> prefix=null, GenericStringView<Char> postfix=null);
	//!@}

	forceinline operator GenericStringView<Char>() const {return View();}

private:
	size_t len;
	mutable Char* data;


	class Formatter: AllocatorRef
	{
	public:
		operator GenericString()
		{
			INTRA_ASSERT(mFormatBegin==null || mFormatRest.Empty());
			GenericString result;
			result.data = mData;
			result.len = size_t(mBufferRest.Begin-mData);
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
			GenericStringView<Char> str = Range::TakeUntilAdvance(mFormatRest, GenericStringView<Char>(formatSeq));
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
		GenericStringView<Char> mFormatRest;
		Char* mData;
		ArrayRange<Char> mBufferRest;
		bool mIsEnded;

		Formatter(const Formatter&) = delete;
		Formatter& operator=(const Formatter&) = delete;

	public:
		Formatter(GenericStringView<Char> formatStr):
			mFormatBegin(formatStr.Data()), mFormatRest(formatStr),
			mData(null), mBufferRest(null), mIsEnded(false) {init();}

		Formatter(GenericStringView<Char> formatStr, Allocator& myAllocator):
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
			const size_t maxLen = Algo::MaxLengthOf(value, args...);
			RequireSpace(maxLen);
			Range::CountRange<char> realLenCounter;
			Algo::ToString(realLenCounter, value, args...);
			INTRA_ASSERT(maxLen>=realLenCounter.Counter);
			Algo::ToString(mBufferRest, value, Meta::Forward<Args>(args)...);
			if(mFormatBegin!=null) WriteNextPart();
			return *this;
		}

		template<typename T, typename Char2, size_t N1, size_t N2, size_t N3, typename... Args> Formatter& operator()(const T& value,
			const Char2(&separator)[N1], const Char2(&leftBracket)[N2], const Char2(&rightBracket)[N3], Args&&... args)
		{
			return operator()(value, GenericStringView<Char2>(separator),
				GenericStringView<Char2>(leftBracket), GenericStringView<Char2>(rightBracket), Meta::Forward<Args>(args)...);
		}

		template<typename T, size_t N, typename... Args> Formatter& operator()(const T(&value)[N], Args&&... args)
		{return operator()(Range::AsRange(value), Meta::Forward<Args>(args)...);}

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

namespace Meta
{
	template<typename Char, typename Allocator> struct IsTriviallyRelocatable<GenericString<Char, Allocator>>: TypeFromValue<bool, true> {};

	namespace D
	{
		template<typename Char, typename Allocator> struct CommonTypeRef<GenericStringView<Char>, GenericString<Char, Allocator>> {typedef GenericString<Char, Allocator> _;};
		template<typename Char, typename Allocator> struct CommonTypeRef<GenericStringView<Char>&, GenericString<Char, Allocator>> {typedef GenericString<Char, Allocator> _;};
		template<typename Char, typename Allocator> struct CommonTypeRef<GenericStringView<Char>&&, GenericString<Char, Allocator>> {typedef GenericString<Char, Allocator> _;};
		template<typename Char, typename Allocator> struct CommonTypeRef<const GenericStringView<Char>&, GenericString<Char, Allocator>> {typedef GenericString<Char, Allocator> _;};

		template<typename Char, typename Allocator> struct CommonTypeRef<GenericStringView<Char>, GenericString<Char, Allocator>&> {typedef GenericString<Char, Allocator> _;};
		template<typename Char, typename Allocator> struct CommonTypeRef<GenericStringView<Char>&, GenericString<Char, Allocator>&> {typedef GenericString<Char, Allocator> _;};
		template<typename Char, typename Allocator> struct CommonTypeRef<GenericStringView<Char>&&, GenericString<Char, Allocator>&> {typedef GenericString<Char, Allocator> _;};
		template<typename Char, typename Allocator> struct CommonTypeRef<const GenericStringView<Char>&, GenericString<Char, Allocator>&> {typedef GenericString<Char, Allocator> _;};

		template<typename Char, typename Allocator> struct CommonTypeRef<GenericStringView<Char>, GenericString<Char, Allocator>&&> {typedef GenericString<Char, Allocator> _;};
		template<typename Char, typename Allocator> struct CommonTypeRef<GenericStringView<Char>&, GenericString<Char, Allocator>&&> {typedef GenericString<Char, Allocator> _;};
		template<typename Char, typename Allocator> struct CommonTypeRef<GenericStringView<Char>&&, GenericString<Char, Allocator>&&> {typedef GenericString<Char, Allocator> _;};
		template<typename Char, typename Allocator> struct CommonTypeRef<const GenericStringView<Char>&, GenericString<Char, Allocator>&&> {typedef GenericString<Char, Allocator> _;};

		template<typename Char, typename Allocator> struct CommonTypeRef<GenericStringView<Char>, const GenericString<Char, Allocator>&> {typedef GenericString<Char, Allocator> _;};
		template<typename Char, typename Allocator> struct CommonTypeRef<GenericStringView<Char>&, const GenericString<Char, Allocator>&> {typedef GenericString<Char, Allocator> _;};
		template<typename Char, typename Allocator> struct CommonTypeRef<GenericStringView<Char>&&, const GenericString<Char, Allocator>&> {typedef GenericString<Char, Allocator> _;};
		template<typename Char, typename Allocator> struct CommonTypeRef<const GenericStringView<Char>&, const GenericString<Char, Allocator>&> {typedef GenericString<Char, Allocator> _;};



		template<typename Char, typename Allocator> struct CommonTypeRef<GenericString<Char, Allocator>, GenericStringView<Char>> {typedef GenericString<Char, Allocator> _;};
		template<typename Char, typename Allocator> struct CommonTypeRef<GenericString<Char, Allocator>, GenericStringView<Char>&> {typedef GenericString<Char, Allocator> _;};
		template<typename Char, typename Allocator> struct CommonTypeRef<GenericString<Char, Allocator>, GenericStringView<Char>&&> {typedef GenericString<Char, Allocator> _;};
		template<typename Char, typename Allocator> struct CommonTypeRef<GenericString<Char, Allocator>, const GenericStringView<Char>&> {typedef GenericString<Char, Allocator> _;};

		template<typename Char, typename Allocator> struct CommonTypeRef<GenericString<Char, Allocator>&, GenericStringView<Char>> {typedef GenericString<Char, Allocator> _;};
		template<typename Char, typename Allocator> struct CommonTypeRef<GenericString<Char, Allocator>&, GenericStringView<Char>&> {typedef GenericString<Char, Allocator> _;};
		template<typename Char, typename Allocator> struct CommonTypeRef<GenericString<Char, Allocator>&, GenericStringView<Char>&&> {typedef GenericString<Char, Allocator> _;};
		template<typename Char, typename Allocator> struct CommonTypeRef<GenericString<Char, Allocator>&, const GenericStringView<Char>&> {typedef GenericString<Char, Allocator> _;};

		template<typename Char, typename Allocator> struct CommonTypeRef<GenericString<Char, Allocator>&&, GenericStringView<Char>> {typedef GenericString<Char, Allocator> _;};
		template<typename Char, typename Allocator> struct CommonTypeRef<GenericString<Char, Allocator>&&, GenericStringView<Char>&> {typedef GenericString<Char, Allocator> _;};
		template<typename Char, typename Allocator> struct CommonTypeRef<GenericString<Char, Allocator>&&, GenericStringView<Char>&&> {typedef GenericString<Char, Allocator> _;};
		template<typename Char, typename Allocator> struct CommonTypeRef<GenericString<Char, Allocator>&&, const GenericStringView<Char>&> {typedef GenericString<Char, Allocator> _;};

		template<typename Char, typename Allocator> struct CommonTypeRef<const GenericString<Char, Allocator>&, GenericStringView<Char>> {typedef GenericString<Char, Allocator> _;};
		template<typename Char, typename Allocator> struct CommonTypeRef<const GenericString<Char, Allocator>&, GenericStringView<Char>&> {typedef GenericString<Char, Allocator> _;};
		template<typename Char, typename Allocator> struct CommonTypeRef<const GenericString<Char, Allocator>&, GenericStringView<Char>&&> {typedef GenericString<Char, Allocator> _;};
		template<typename Char, typename Allocator> struct CommonTypeRef<const GenericString<Char, Allocator>&, const GenericStringView<Char>&> {typedef GenericString<Char, Allocator> _;};
	}
}

template<typename Char, typename Allocator> forceinline
bool operator==(const GenericStringView<Char>& lhs, const GenericString<Char, Allocator>& rhs)
{return lhs==rhs.View();}

template<typename Char> GenericString<Char> operator+(GenericStringView<Char> lhs, GenericStringView<Char> rhs)
{
	GenericString<Char> result;
	result.Reserve(lhs.Length()+rhs.Length());
	result += lhs;
	result += rhs;
	return result;
}

template<typename Char, class Allocator> GenericString<Char, Allocator> operator+(
	const GenericString<Char, Allocator>& lhs, GenericStringView<Char> rhs)
{
	GenericString<Char, Allocator> result;
	result.Reserve(lhs.Length()+rhs.Length());
	result += lhs;
	result += rhs;
	return result;
}

template<typename Char, class Allocator> GenericString<Char, Allocator> operator+(
	GenericStringView<Char> lhs, const GenericString<Char, Allocator>& rhs)
{
	GenericString<Char, Allocator> result;
	result.Reserve(lhs.Length()+rhs.Length());
	result += lhs;
	result += rhs;
	return result;
}

template<typename Char> GenericString<Char> operator+(Char lhs, GenericStringView<Char> rhs)
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
GenericString<Char, Allocator> operator+(GenericString<Char, Allocator>&& lhs, GenericStringView<Char> rhs)
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
GenericString<Char> operator+(const Char(&lhs)[N], GenericStringView<Char> rhs)
{return operator+(GenericStringView<Char>(lhs), rhs);}

template<typename Char, size_t N> forceinline
GenericString<Char> operator+(GenericStringView<Char> lhs, const Char(&rhs)[N])
{return operator+(lhs, GenericStringView<Char>(rhs));}

template<typename Char> GenericString<Char> operator+(GenericStringView<Char> lhs, Char rhs)
{
	GenericString<Char> result;
	result.Reserve(lhs.Length()+1);
	result += lhs;
	result += rhs;
	return result;
}

#ifdef USER_DEFINED_LITERALS_SUPPORT
forceinline String operator ""_s(const char* str, size_t len)
{
	return String(str, len);
}
#endif


template<typename T, typename... Args> forceinline String ToString(const T& value, Args&&... args)
{
	return String::Format()(value, Meta::Forward<Args>(args)...);
}

forceinline const String& ToString(const String& value) {return value;}
forceinline StringView ToString(const StringView& value) {return value;}
forceinline StringView ToString(const char* value) {return StringView(value);}
template<size_t N> forceinline StringView ToString(const char(&value)[N]) {return StringView(value);}

template<typename Char, typename Allocator> template<typename R> Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_,
GenericString<Char, Allocator>> GenericString<Char, Allocator>::Join(const R& strs,
	GenericStringView<Char> delim, GenericStringView<Char> prefix, GenericStringView<Char> postfix)
{
	if(strs.Empty()) return null;
	R strRange = strs;
	String result = prefix+ToString(strRange.First())+postfix;
	result.Reserve((result.Length()+delim.Length())*10);
	strRange.PopFirst();
	String delim2 = delim+prefix;

	while(!strRange.Empty())
	{
		result += delim2;
		result += ToString(strRange.First());
		result += postfix;
		strRange.PopFirst();
	}
	result.TrimExcessCapacity();

	return result;
}

static_assert(Meta::TypeEquals<Range::AsRangeResult<GenericString<char>>, ArrayRange<char>>::_, "");

}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#ifdef _MSC_VER
#pragma warning(pop)
#endif
