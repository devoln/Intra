#pragma once

#include "Containers/ForwardDeclarations.h"
#include "StringView.h"
#include "Memory/Memory.h"
#include "Memory/AllocatorInterface.h"
#include "Text/UtfConversion.h"
#include "Data/Reflection.h"

namespace Intra {

template<typename Char, typename AllocatorType> class GenericString:
	public Memory::AllocatorRef<AllocatorType, Meta::ComparableMixin<GenericString<Char, AllocatorType>>>
{
	typedef Memory::AllocatorRef<AllocatorType, Meta::ComparableMixin<GenericString<Char, AllocatorType>>> AllocatorRef;
	class Formatter;
public:
	typedef AllocatorType Allocator;

	GenericString(const Char* str, Allocator& allocator):
		GenericString(str, (str==null)? 0: CStringLength(str), allocator) {}

	GenericString(const Char* str): GenericString(str, (str==null)? 0: CStringLength(str)) {}

	template<size_t N> GenericString(const Char(&str)[N], Allocator& allocator): GenericString(str, N-1, allocator) {}

	template<size_t N> GenericString(const Char(&str)[N]): GenericString(str, N-1) {}

	GenericString(null_t, Allocator& allocator): AllocatorRef(allocator), len(0), data(null) {}

	GenericString(null_t=null): AllocatorRef(null), len(0), data(null) {}

	explicit GenericString(const Char* str, size_t len, Allocator& allocator): AllocatorRef(allocator), data(null)
	{
		SetLengthUninitialized(len);
		core::memcpy(data, str, len*sizeof(Char));
	}

	explicit GenericString(const Char* str, size_t len): len(0), data(null)
	{
		SetLengthUninitialized(len);
		core::memcpy(data, str, len*sizeof(Char));
	}

	explicit forceinline GenericString(const Char* begin, const Char* end):
		GenericString(begin, end-begin) {}

	explicit forceinline GenericString(const Char* begin, const Char* end, Allocator& allocator):
		GenericString(begin, end-begin, allocator) {}
	
	explicit forceinline GenericString(size_t len, Char filler):
		len(0), data(null) {SetLength(len, filler);}

	forceinline GenericString(StringView rhs):
		GenericString(rhs.Data(), rhs.Length()) {}

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
	forceinline GenericStringView<Char> operator()(size_t start, size_t end) const {return View(start, end);}

	//! Получить подстроку, начиная с кодовой единицы с индексом start и заканчивая end.
	forceinline GenericStringView<Char> operator()(Range::RelativeIndex start, Range::RelativeIndex end) const {return View(start, end);}

	//! Получить подстроку, начиная с кодовой единицы с индексом start и до конца.
	forceinline GenericStringView<Char> operator()(Range::RelativeIndex start, Range::RelativeIndexEnd) const {return View(start, $);}

	//! Получить подстроку, начиная с кодовой единицы с индексом start и до конца.
	forceinline GenericStringView<Char> operator()(size_t start, Range::RelativeIndexEnd) const {return View(start, $);}


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
		core::memcpy(Data(), rhs.Data(), rhs.Length()*sizeof(Char));
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
	friend GenericString operator+(GenericString&& lhs, GenericStringView<Char> rhs)
	{
		const size_t oldLength = lhs.Length();
		lhs.SetLengthUninitialized(oldLength+rhs.Length());
		core::memcpy(lhs.Data()+oldLength, rhs.Data(), rhs.Length()*sizeof(Char));
		return core::move(lhs);
	}

	friend forceinline GenericString operator+(const GenericString& lhs, GenericStringView<Char> rhs) {return lhs.View()+rhs;}

	forceinline GenericString& operator+=(GenericStringView<Char> rhs)
	{
		size_t oldLen = Length();
		SetLengthUninitialized(oldLen+rhs.Length());
		core::memcpy(Data()+oldLen, rhs.Data(), rhs.Length()*sizeof(Char));
		return *this;
	}

	forceinline GenericString& operator+=(Formatter& rhs) {return operator+=(*rhs);}

	template<size_t N> forceinline GenericString& operator+=(const Char(&rhs)[N]) {return operator+=(GenericStringView<Char>(rhs));}
	//String& operator+=(const char* rhs) {return operator+=(View(), StringView(rhs));}
	//!@}

	//!@{
	//! Добавление символа к строке
	friend forceinline GenericString operator+(GenericString&& lhs, Char rhs) {return core::move(lhs+=rhs);}

	forceinline GenericString& operator+=(Char rhs)
	{
		SetLengthUninitialized(Length()+1);
		Last() = rhs;
		return *this;
	}

	//friend GenericString operator+(GenericString&& lhs, dchar rhs) {return core::move(lhs+=rhs);}

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
	friend forceinline bool operator==(const GenericStringView<Char>& lhs, const GenericString& rhs) {return lhs==rhs.View();}

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
		core::memcpy(newData, data, len*sizeof(Char));
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
		if(newLen>oldLength) core::memset(data+oldLength, filler, (newLen-oldLength)*sizeof(Char));
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
		core::memcpy(result.Data(), chars.Begin, chars.Length()*sizeof(Char));
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
	template<typename R> static GenericString Join(const R& strs,
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
			INTRA_ASSERT(format==null || rest.Empty());
			GenericString result;
			result.data = data;
			result.len = size_t(buffer_rest.Begin-data);
			result.AllocatorRef::operator=(*this);
			data = null;
			buffer_rest = null;
			rest = null;
			format = null;
			isEnded = true;
			return result;
		}
		forceinline GenericString operator*() {return operator GenericString();}

	private:
		void WriteNextPart()
		{
			GenericStringView<Char> str = rest.ReadUntilAdvance(StringView("<^>"));
			RequireSpace(str.Length());
			str.CopyToAdvance(buffer_rest);
			rest.PopFirstN(3);
		}

		void RequireSpace(size_t newChars)
		{
			if(buffer_rest.Length()>=newChars) return; //Места достаточно, увеличивать буфер не надо
			size_t currentLength = size_t(buffer_rest.Begin-data);
			size_t oldSize = size_t(buffer_rest.End-data);
			size_t newSize = (oldSize + Math::Max(oldSize, newChars)*sizeof(Char));
			Char* newData = AllocatorRef::Allocate(newSize, {__FILE__, (uint)__LINE__});
			core::memcpy(newData, data, currentLength*sizeof(Char));
			buffer_rest = {newData+currentLength, newData+newSize};
			AllocatorRef::Free(data, oldSize);
			data = newData;
		}

		const Char* format;
		GenericStringView<Char> rest;
		Char* data;
		ArrayRange<Char> buffer_rest;
		bool isEnded;

		Formatter(const Formatter&) = delete;
		Formatter& operator=(const Formatter&) = delete;

		template<typename CharRange> struct StructVisitor
		{
			Formatter& Me;
			bool Began;
			ArrayRange<const StringView> FieldNames;
			const CharRange& Separator;
			const CharRange& Assignment;

			template<typename T> StructVisitor& operator()(const T& value)
			{
				if(Began) Me(Separator);
				Began = true;
				if(!FieldNames.Empty())
				{
					Me(FieldNames.First());
					Me(Assignment);
					FieldNames.PopFirst();
				}
				Me(value);
				return *this;
			}
		};

	public:
		Formatter(GenericStringView<Char> format):
			format(format.Data()), rest(format), data(null), isEnded(false) {init();}

		Formatter(GenericStringView<Char> format, Allocator& allocator): AllocatorRef(allocator),
			format(format.Data()), rest(format), data(null), isEnded(false) {init();}

		Formatter(Formatter&& rhs): AllocatorRef(rhs), format(rhs.format),
			rest(rhs.rest), data(rhs.data), buffer_rest(rhs.buffer_rest), isEnded(rhs.isEnded)
		{
			rhs.rest = null;
			rhs.format = null;
			rhs.buffer_rest = null;
			rhs.isEnded = true;
			rhs.data = null;
		}

		~Formatter() {INTRA_ASSERT(buffer_rest.Empty());}

		template<typename T, typename... Args> Meta::EnableIf<
			Meta::IsArithmeticType<T>::_ ||
			Meta::IsPointerType<T>::_ ||
			Range::IsFiniteForwardRangeOf<T, Char>::_ ||
			Meta::IsTuple<T>::_,
		Formatter&> operator()(const T& value, Args&&... args)
		{
			INTRA_ASSERT(!isEnded);
			RequireSpace(GenericStringView<Char>::MaxLengthOf(value, core::forward<Args>(args)...));
			buffer_rest.AppendAdvance(value, core::forward<Args>(args)...);
			if(format!=null) WriteNextPart();
			return *this;
		}

		template<typename T, typename CharRange=StringView> Meta::EnableIf<
			!(Meta::IsArithmeticType<T>::_ ||
				Range::IsFiniteInputRangeOf<T, Char>::_ ||
				Meta::IsTuple<T>::_) &&
			Data::HasReflection<T>::_ &&
			Range::IsCharRange<CharRange>::_,
		Formatter&> operator()(const T& structure, bool printNames=true,
			const CharRange& separator=StringView(", "),
			const CharRange& lBracket=StringView("{"),
			const CharRange& rBracket=StringView("}"),
			const CharRange& assignment=" = ")
		{
			INTRA_ASSERT(!isEnded);
			StructVisitor<CharRange> visitor{*this, false,
				printNames? T::ReflectionFieldNames(): null, separator, assignment};
			operator()(lBracket);
			structure.VisitEachField(visitor);
			operator()(rBracket);
			return *this;
		}

		template<typename T, typename CharRange=StringView> Meta::EnableIf<
			Meta::IsTuple<T>::_,
		Formatter&> operator()(const T& tuple,
			const CharRange& separator=StringView(", "),
			const CharRange& lBracket=StringView("{"),
			const CharRange& rBracket=StringView("}"),
			const CharRange& assignment=StringView(" = "))
		{
			INTRA_ASSERT(!isEnded);
			StructVisitor<CharRange> visitor{*this, false, null, separator, assignment};
			operator()(lBracket);
			tuple.ForEachField(visitor);
			operator()(rBracket);
			return *this;
		}

		template<size_t N, typename Char2> forceinline Meta::EnableIf<
			Meta::IsCharType<Char2>::_,
		Formatter&> operator()(const Char2(&str)[N])
		{
			return operator()(GenericStringView<Char2>(str));
		}

		template<typename T> forceinline
			Formatter& operator()(std::initializer_list<T> list,
				StringView separator=", ",
				StringView lBracket="[",
				StringView rBracket="]")
		{
			return operator()(Range::AsRange(list), separator, lBracket, rBracket);
		}

		template<typename R> forceinline Meta::EnableIf<
			(Range::IsFiniteForwardRange<R>::_ || Range::HasAsRange<R>::_) &&
			!Range::IsCharRange<R>::_,
		Formatter&> operator()(const R& range,
			StringView separator=", ",
			StringView lBracket="[",
			StringView rBracket="]")
		{
			INTRA_ASSERT(!isEnded);
			size_t maxLen = GenericStringView<Char>::MaxLengthOf(Range::AsRange(range), separator, lBracket, rBracket);
			RequireSpace(maxLen);
			buffer_rest.AppendAdvance(Range::AsRange(range), separator, lBracket, rBracket);
			if(format!=null) WriteNextPart();
			return *this;
		}

		template<typename R> forceinline Meta::EnableIf<
			(Range::IsFiniteForwardRange<R>::_ || Range::HasAsRange<R>::_) &&
			Range::IsCharRange<R>::_,
		Formatter&> operator()(const R& range)
		{
			INTRA_ASSERT(!isEnded);
			size_t maxLen = GenericStringView<Char>::MaxLengthOf(Range::AsRange(range));
			RequireSpace(maxLen);
			buffer_rest.AppendAdvance(Range::AsRange(range));
			if(format!=null) WriteNextPart();
			return *this;
		}

		template<typename R> forceinline Meta::EnableIf<
			!Range::IsForwardRange<R>::_ &&
			Range::IsFiniteInputNonCharRange<R>::_,
		Formatter&> operator()(R& range,
			StringView separator=", ",
			StringView lBracket="[",
			StringView rBracket="]")
		{
			INTRA_ASSERT(!isEnded);
			operator()(lBracket);
			while(!range.Empty())
			{
				RequireSpace(GenericStringView<Char>::MaxLengthOf(range.First())+separator.Length());
				buffer_rest.AppendAdvance(range.First());
				range.PopFirst();
				if(!range.Empty()) buffer_rest.AppendAdvance(separator);
			}
			operator()(rBracket);
			if(format!=null) WriteNextPart();
			return *this;
		}

		template<typename T, size_t N, typename CharRange=StringView> forceinline Meta::EnableIf<
			!Meta::IsCharType<T>::_,
		Formatter&> operator()(const T(&arr)[N],
			CharRange separator=StringView(", "),
			CharRange lBracket=StringView("["),
			CharRange rBracket=StringView("]"))
		{
			return operator()(ArrayRange<const T>(arr), separator, lBracket, rBracket);
		}

		template<typename Char2, typename Allocator2> forceinline Formatter& operator()(const GenericString<Char2, Allocator2>& rhs)
		{
			return operator()(rhs.View());
		}

	private:
		void init()
		{
			size_t initialSize = rest.End() - format+8u;
			data = AllocatorRef::Allocate(initialSize, INTRA_SOURCE_INFO);
			buffer_rest = {data, initialSize};
			if(format!=null) WriteNextPart();
		}
	};
};

namespace Meta
{
	template<typename Char, typename Allocator> struct IsTriviallyRelocatable<GenericString<Char, Allocator>>: TypeFromValue<bool, true> {};

	namespace detail
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

template<typename Char> GenericString<Char> operator+(GenericStringView<Char> lhs, GenericStringView<Char> rhs)
{
	GenericString<Char> result;
	result.SetLengthUninitialized(lhs.Length()+rhs.Length());
	core::memcpy(result.Data(), lhs.Data(), lhs.Length()*sizeof(Char));
	core::memcpy(result.Data()+lhs.Length(), rhs.Data(), rhs.Length()*sizeof(Char));
	return result;
}

template<typename Char> GenericString<Char> operator+(Char lhs, GenericStringView<Char> rhs)
{
	GenericString<Char> result;
	result.SetLengthUninitialized(1+rhs.Length());
	result[0]=lhs;
	core::memcpy(result.Data()+1, rhs.Data(), rhs.Length()*sizeof(Char));
	return result;
}

template<typename Char> forceinline GenericString<Char> operator+(Char lhs, const GenericString<Char>& rhs) {return lhs+rhs.View();}

inline String operator+(StringView lhs, dchar rhs)
{
	String result;
	result.SetLengthUninitialized(lhs.Length()+6);
	core::memcpy(result.Data(), lhs.Data(), lhs.Length());
	auto byteCount = UTF32::CharToUTF8Sequence(rhs, result.Data()+lhs.Length());
	result.SetLengthUninitialized(lhs.Length()+byteCount);
	return result;
}

template<typename Char, size_t N> forceinline GenericString<Char> operator+(const Char(&lhs)[N], GenericStringView<Char> rhs)
{
	return operator+(GenericStringView<Char>(lhs), rhs);
}

template<typename Char, size_t N> forceinline GenericString<Char> operator+(GenericStringView<Char> lhs, const Char(&rhs)[N])
{
	return operator+(lhs, GenericStringView<Char>(rhs));
}

template<typename Char> GenericString<Char> operator+(GenericStringView<Char> lhs, Char rhs)
{
	GenericString<Char> result;
	result.SetLengthUninitialized(lhs.Length()+1);
	core::memcpy(result.Data(), lhs.Data(), lhs.Length()*sizeof(Char));
	result[lhs.Length()]=rhs;
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
	return String::Format()(value, core::forward<Args>(args)...);
}

forceinline const String& ToString(const String& value) {return value;}
forceinline StringView ToString(const StringView& value) {return value;}
forceinline StringView ToString(const char* value) {return StringView(value);}
template<size_t N> forceinline StringView ToString(const char(&value)[N]) {return StringView(value);}

template<typename Char, typename Allocator> template<typename R> GenericString<Char, Allocator>
	GenericString<Char, Allocator>::Join(const R& strs,
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

}
