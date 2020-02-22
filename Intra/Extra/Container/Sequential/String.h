#pragma once

#include "Core/Assert.h"
#include "Core/CContainer.h"
#include "Core/Range/Span.h"
#include "Core/Range/StringView.h"
#include "Core/Misc/RawMemory.h"
#include "Core/Range/Special/Unicode.h"
#include "Core/Range/Mutation/Fill.h"
#include "Core/Range/Mutation/Copy.h"
#include "Core/Range/Mutation/ReplaceSubrange.h"
#include "Core/Range/TakeUntil.h"
#include "Core/Range/Zip.h"
#include "Core/Range/Inserter.h"

#include "Memory/Memory.h"
#include "Memory/Allocator.hh"

#include "Container/ForwardDecls.h"
#include "Container/Operations.hh"

#include "StringFormatter.h"


INTRA_BEGIN
INTRA_WARNING_DISABLE_SIGN_CONVERSION
//! Class used to create, store and operate strings.
/*!
  It takes a template argument of code point type.
  If Char = char then the string will be treated as UTF-8.
  Be aware that elements of a GenericString are not characters because one non-ASCII character may be encoded with multiple code points.
  If Char = char16_t then the string will be treated as UTF-16.
  Be aware that elements of a GenericString are not characters because one non-ASCII character may be encoded with multiple code points.
*/
template<typename Char> class GenericString
{
public:
	constexpr forceinline GenericString(const Char* str):
		GenericString(str, (str == null)? 0: Misc::CStringLength(str)) {}

	constexpr forceinline GenericString(null_t=null) noexcept {setShortLength(0);}

	constexpr forceinline static GenericString CreateReserve(size_t reservedCapacity)
	{
		GenericString result;
		result.Reserve(reservedCapacity);
		return result;
	}

	constexpr explicit GenericString(const Char* str, size_t strLength)
	{
		SetLengthUninitialized(strLength);
		Misc::CopyBits(Data(), str, strLength);
	}

	constexpr explicit forceinline GenericString(const Char* startPtr, const Char* endPtr):
		GenericString(startPtr, size_t(endPtr - startPtr)) {}
	
	constexpr explicit forceinline GenericString(size_t initialLength, Char filler): GenericString(null)
	{SetLength(initialLength, filler);}

	template<typename R = StringView, typename AsR = TRangeOfType<R>,
	typename = Requires<
		CConsumableRangeOf<AsR, Char> &&
		CChar<TValueTypeOf<AsR>>
	>> constexpr forceinline GenericString(R&& rhs)
	{
		setShortLength(0); //avoid unnecessary copying of default zeros on allocation
		SetLengthUninitialized(Count(rhs));
		CopyTo(ForwardAsRange<R>(rhs), AsRange());
	}

	constexpr forceinline GenericString(const GenericString& rhs): u(rhs.u)
	{
		if(!IsHeapAllocated()) return;
		u.m.Data = allocate(u.m.Len);
		Misc::CopyBits(u.m.Data, rhs.u.m.Data, u.m.Len);
	}
	
	constexpr forceinline GenericString(GenericString&& rhs):
		u(rhs.u) {rhs.resetToEmptySsoWithoutFreeing();}
	

	forceinline ~GenericString()
	{
		//The destructor is the most serious obstacle to String being constexpr
		if(IsHeapAllocated()) freeLongData();
	}

	//! Get UTF-32 character range
	//UTF8 ByChar() const {return UTF8(Data(), Length());}

	//! @returns a slice of the string containing code points with indices [startIndex; endIndex).
	constexpr forceinline GenericStringView<const Char> operator()(
		size_t startIndex, size_t endIndex) const
	{return View(startIndex, endIndex);}

	/*!
	  @name String assignment
	*/
	///@{
	constexpr GenericString& operator=(GenericString&& rhs) noexcept
	{
		if(this == &rhs) return *this;
		if(IsHeapAllocated()) freeLongData();
		u = rhs.u;
		rhs.resetToEmptySsoWithoutFreeing();
		return *this;
	}

	constexpr GenericString& operator=(GenericStringView<const Char> rhs)
	{
		if(sameRange(rhs)) return *this;
		INTRA_DEBUG_ASSERT(!containsView(rhs));
		SetLengthUninitialized(0); //avoid unnecessary copying on reallocation
		SetLengthUninitialized(rhs.Length());
		Misc::CopyBits(Data(), rhs.Data(), rhs.Length());
		return *this;
	}

	constexpr forceinline GenericString& operator=(const GenericString& rhs)
	{return operator=(rhs.AsConstRange());}

	constexpr forceinline GenericString& operator=(const Char* rhs)
	{return operator=(GenericStringView<const Char>(rhs));}
	
	template<typename R> constexpr forceinline Requires<
		CArrayClass<R>,
	GenericString&> operator=(R&& rhs)
	{return operator=(GenericStringView<const Char>(rhs));}

	template<typename R> constexpr forceinline Requires<
		!CArrayClass<R> &&
		CAsConsumableRangeOf<R, Char>,
	GenericString&> operator=(R&& rhs)
	{
		SetLengthUninitialized(0); //avoid unnecessary copying on reallocation
		SetLengthUninitialized(Count(rhs));
		CopyTo(ForwardAsRange<R>(rhs), AsRange());
		return *this;
	}

	constexpr GenericString& operator=(null_t)
	{
		if(IsHeapAllocated()) freeLongData();
		resetToEmptySsoWithoutFreeing();
		return *this;
	}
	///@}

	constexpr forceinline GenericString& operator+=(StringFormatter<GenericString>& rhs) {return operator+=(*rhs);}
	constexpr forceinline GenericString& operator+=(Char rhs)
	{
		const size_t oldLen = Length();
		const bool wasAllocated = IsHeapAllocated();
		if(!wasAllocated && oldLen < SSO_BUFFER_CAPACITY_CHARS)
		{
			shortLenIncrement();
			u.buf[oldLen]=rhs;
		}
		else
		{
			if(!wasAllocated || getLongCapacity() == oldLen)
				Reallocate(oldLen + 1 + oldLen/2);
			u.m.Data[u.m.Len++] = rhs;
		}
		return *this;
	}

	constexpr forceinline bool operator==(null_t) const {return Empty();}

	constexpr forceinline bool operator==(const GenericStringView<const Char>& rhs) const {return View() == rhs;}

	constexpr forceinline bool operator>(const GenericStringView<const Char>& rhs) const {return View() > rhs;}
	constexpr forceinline bool operator<(const Char* rhs) const {return View() < rhs;}
	constexpr forceinline bool operator<(const GenericStringView<const Char>& rhs) const {return View() < rhs;}


	//! Make sure that Capacity() >= \p minCapacity. Otherwise reallocates to at least \p minCapacity.
	constexpr forceinline void Reserve(size_t minCapacity)
	{
		if(Capacity() < minCapacity)
			Reallocate(minCapacity + Length()/2);
	}


	//! Reallocate String to hold \p newCapacity characters.
	void Reallocate(size_t newCapacity)
	{
		const bool wasAllocated = IsHeapAllocated();
		if(!wasAllocated && newCapacity <= SSO_BUFFER_CAPACITY_CHARS) return;
		const size_t len = Length();
		if(newCapacity < len) newCapacity = len;
		Char* newData;
		if(newCapacity > SSO_BUFFER_CAPACITY_CHARS)
		{
			size_t newCapacityInBytes = newCapacity*sizeof(Char);
			newData = allocate(newCapacityInBytes);
			newCapacity = newCapacityInBytes/sizeof(Char);
		}
		else
		{
			newData = u.buf;
			setShortLength(len);
		}
		Char* const oldData = u.m.Data;
		const size_t oldLongCapacity = getLongCapacity();
		CopyTo(CSpan<Char>(Data(), len), Span<Char>(newData, len));
		if(wasAllocated) deallocate(oldData, oldLongCapacity*sizeof(Char));
		if(newData != u.buf)
		{
			u.m.Data = newData;
			setLongCapacity(newCapacity);
			u.m.Len = len;
		}
	}

	//! If there is more than 20% excess capacity of heap allocation, reallocate so that Length() == Capacity().
	constexpr forceinline void TrimExcessCapacity()
	{if(Capacity() > Length() * 5/4) Reallocate(Length());}

	//! Make sure there is enough space to append minSpace chars.
	constexpr forceinline void RequireSpace(size_t minSpace) {Reserve(Length() + minSpace);}

	//! Set string length to \p newLen.
	/*!
	  If newLen < Length(), discards last Length() - newLen code points.
	  otherwise, adds newLen - Length() copies of \p filler.
	*/
	constexpr forceinline void SetLength(size_t newLen, Char filler='\0')
	{
		const size_t oldLength = Length();
		SetLengthUninitialized(newLen);
		if(newLen <= oldLength) return;
		if(filler == '\0') FillZeros(View(oldLength, newLen));
		else Fill(View(oldLength, newLen), filler);
	}

	constexpr forceinline void SetCount(size_t newLen, Char filler='\0') {SetLength(newLen, filler);}

	/** Set string length to ``newLen``.
	  If ``newLen`` < Length(), discards last Length() - newLen code points.
	  otherwise, adds ``newLen`` - Length() uninitialized code points.
	*/
	constexpr forceinline void SetLengthUninitialized(size_t newLen)
	{
		Reserve(newLen);
		if(IsHeapAllocated()) u.m.Len = newLen;
		else setShortLength(newLen);
	}

	//! Same as SetLengthUnitialized, used by template detection code.
	//! @see SetLengthUninitialized
	constexpr forceinline void SetCountUninitialized(size_t newLen)
	{SetLengthUninitialized(newLen);}

	//! Number of code points this string can hold without memory reallocation.
	constexpr forceinline size_t Capacity() const
	{return IsHeapAllocated()? getLongCapacity(): SSO_BUFFER_CAPACITY_CHARS;}

	constexpr forceinline Char& operator[](size_t index)
	{
		INTRA_DEBUG_ASSERT(index < Length());
		return Data()[index];
	}

	constexpr forceinline const Char& operator[](size_t index) const
	{
		INTRA_DEBUG_ASSERT(index < Length());
		return Data()[index];
	}

	INTRA_NODISCARD constexpr forceinline char Get(size_t index, Char defaultChar='\0') const noexcept
	{return index < Length()? Data()[index]: defaultChar;}

	INTRA_NODISCARD constexpr forceinline bool Empty() const
	{return IsHeapAllocated()? u.m.Len == 0: emptyShort();}

	constexpr forceinline void PopLast()
	{
		INTRA_DEBUG_ASSERT(!Empty());
		if(IsHeapAllocated()) u.m.Len--;
		else shortLenDecrement();
	}

	INTRA_NODISCARD constexpr forceinline index_t Length() const
	{return IsHeapAllocated()? u.m.Len: getShortLength();}

	INTRA_NODISCARD constexpr forceinline Char& First()
	{
		INTRA_DEBUG_ASSERT(!Empty());
		return *Data();
	}

	INTRA_NODISCARD constexpr forceinline Char First() const
	{
		return INTRA_DEBUG_ASSERT(!Empty()),
			*Data();
	}

	INTRA_NODISCARD constexpr forceinline Char& Last()
	{
		INTRA_DEBUG_ASSERT(!Empty());
		return Data()[Length()-1];
	}
	
	INTRA_NODISCARD constexpr forceinline Char Last() const
	{
		return INTRA_DEBUG_ASSERT(!Empty()),
			Data()[Length()-1];
	}

	/**
	  @name Views to internal data
	  @return StringView containing all string characters.
	  @warning
	  The returned range becomes invalid after any modification to this string or its destruction.
	  Use this method carefully with temporary objects:
	  auto range = String("a").AsRange(); //Error: range is dangling.
	  foo(String("a").AsRange()); //OK: the returned StringView is valid before foo returns.
	**/
	///@{
	INTRA_NODISCARD constexpr forceinline GenericStringView<Char> AsRange() {return {Data(), Length()};}
	INTRA_NODISCARD constexpr forceinline GenericStringView<const Char> AsRange() const {return {Data(), Length()};}
	INTRA_NODISCARD constexpr forceinline GenericStringView<const Char> AsConstRange() const {return {Data(), Length()};}

	INTRA_NODISCARD constexpr forceinline GenericStringView<Char> TakeNone() noexcept {return {Data(), 0};}
	INTRA_NODISCARD constexpr forceinline GenericStringView<const Char> TakeNone() const noexcept {return {Data(), 0};}

	INTRA_NODISCARD constexpr forceinline GenericStringView<Char> Drop(index_t count=1) noexcept {return AsRange().Drop(count);}
	INTRA_NODISCARD constexpr forceinline GenericStringView<const Char> Drop(index_t count=1) const noexcept {return AsRange().Drop(count);}

	INTRA_NODISCARD constexpr forceinline GenericStringView<Char> DropLast(index_t count=1) noexcept {return AsRange().DropLast(count);}
	INTRA_NODISCARD constexpr forceinline GenericStringView<const Char> DropLast(index_t count=1) const noexcept {return AsRange().DropLast(count);}

	INTRA_NODISCARD constexpr forceinline GenericStringView<Char> Take(index_t count) noexcept {return AsRange().Take(count);}
	INTRA_NODISCARD constexpr forceinline GenericStringView<const Char> Take(index_t count) const noexcept {return AsRange().Take(count);}

	INTRA_NODISCARD constexpr forceinline GenericStringView<Char> Tail(index_t count) noexcept {return AsRange().Tail(count);}
	INTRA_NODISCARD constexpr forceinline GenericStringView<const Char> Tail(index_t count) const noexcept {return AsRange().Tail(count);}

	
	INTRA_NODISCARD constexpr forceinline Char* Data() {return IsHeapAllocated()? u.m.Data: u.buf;}
	INTRA_NODISCARD constexpr forceinline const Char* Data() const {return IsHeapAllocated()? u.m.Data: u.buf;}
	INTRA_NODISCARD constexpr forceinline Char* End() {return Data()+Length();}
	INTRA_NODISCARD constexpr forceinline const Char* End() const {return Data()+Length();}

	INTRA_NODISCARD const Char* CStr()
	{
		const size_t len = Length();
		if(!IsHeapAllocated())
		{
			u.buf[len] = '\0';
			return u.buf;
		}
		RequireSpace(1);
		const auto data = Data();
		data[len] = '\0';
		return data;
	}
	///@}

	//! Replace all occurences of \p subStr to \p newSubStr.
	static INTRA_NODISCARD GenericString ReplaceAll(GenericStringView<const Char> str,
		GenericStringView<const Char> subStr, GenericStringView<const Char> newSubStr)
	{
		GenericString result;
		CountRange<Char> counter;
		MultiReplaceToAdvance(str, counter, RangeOf({TupleL(subStr, newSubStr)}));
		result.SetLengthUninitialized(counter.Counter);
		MultiReplaceToAdvance(str, result.AsRange(), RangeOf({TupleL(subStr, newSubStr)}));
		return result;
	}

	static INTRA_NODISCARD GenericString ReplaceAll(GenericStringView<const Char> str, Char c, Char newc)
	{
		if(c==newc) return str;
		GenericString result;
		result.SetLengthUninitialized(str.Length());
		for(size_t i = 0; i<str.Length(); i++)
			result[i] = str[i] == c? newc: str[i];
		return result;
	}

	static GenericString MultiReplace(GenericStringView<const Char> str,
		CSpan<GenericStringView<const Char>> subStrs,
		CSpan<GenericStringView<const Char>> newSubStrs)
	{
		GenericString result;
		CountRange<Char> counter;
		MultiReplaceToAdvance(str, counter, Zip(subStrs, newSubStrs));
		result.SetLengthUninitialized(counter.Counter);
		MultiReplaceTo(str, result, Zip(subStrs, newSubStrs));
		return result;
	}

	//! Repeat ``str`` ``n`` times.
	static GenericString Repeat(GenericStringView<const Char> str, size_t n)
	{
		if(str.Empty()) return null;
		GenericString result;
		result.SetLengthUninitialized(str.Length()*n);
		FillPattern(result, str);
		return result;
	}

	constexpr forceinline void AddLast(Char c) {operator+=(c);}


	INTRA_NODISCARD constexpr forceinline Char* begin() {return Data();}
	INTRA_NODISCARD constexpr forceinline Char* end() {return Data()+Length();}
	INTRA_NODISCARD constexpr forceinline const Char* begin() const {return Data();}
	INTRA_NODISCARD constexpr forceinline const Char* end() const {return Data()+Length();}

#ifdef INTRA_CONTAINER_STL_FORWARD_COMPATIBILITY
	//! @defgroup String_STL_Interface STL-подобный интерфейс для BList
	//! Этот интерфейс предназначен для совместимости с обобщённым контейнеро-независимым кодом.
	//! Использовать напрямую этот интерфейс не рекомендуется.
	//!@{
	typedef char* iterator;
	typedef const char* const_iterator;
	typedef Char value_type;

	forceinline void push_back(Char c) {operator+=(c);}
	forceinline void reserve(size_t capacity) {Reserve(capacity);}
	forceinline void resize(size_t newLength, Char filler='\0') {SetLength(newLength, filler);}
	forceinline size_t size() const {return Length();}
	forceinline size_t length() const {return Length();}
	forceinline const Char* c_str() {return CStr();} //Не const, в отличие от STL
	forceinline GenericString substr(size_t start, size_t count) const {return operator()(start, start+count);}
	forceinline bool empty() const {return Empty();}
	forceinline Char* data() {return Data();}
	forceinline const Char* data() const {return Data();}

	forceinline GenericString& append(const GenericString& str)
	{*this += str.AsRange();}

	forceinline GenericString& append(const GenericString& str, size_t subpos, size_t sublen)
	{*this += str(subpos, subpos+sublen);}

	forceinline GenericString& append(const Char* s)
	{*this += GenericStringView<const Char>(s);}

	forceinline GenericString& append(const Char* s, size_t n)
	{*this += GenericStringView<const Char>(s, n);}

	GenericString& append(size_t n, Char c)
	{
		size_t oldLen = Length();
		SetLengthUninitialized(oldLen+n);
		Fill(View(oldLen, oldLen+n), c);
	}

	template<class InputIt> forceinline GenericString& append(InputIt firstIt, InputIt endIt)
	{while(firstIt!=endIt) *this += *endIt++;}

	forceinline void clear() {SetLengthUninitialized(0);}
	//!@}
#endif


	//! String formatting.
	//! @param format Строка, содержащая метки <^>, в которые будут подставляться аргументы.
	//! @param () Используйте скобки для передачи параметров и указания их форматирования
	//! @return Прокси-объект для форматирования, неявно преобразующийся к String.
	static forceinline INTRA_NODISCARD StringFormatter<GenericString> Format(GenericStringView<const Char> format=null)
	{return StringFormatter<GenericString>(format);}

	//! Формирование строки как конкатенация строковых представлений аргугментов функции.
	template<typename... Args> static INTRA_NODISCARD forceinline GenericString Concat(Args&&... args)
	{return StringFormatter<GenericString>(null).Arg(Forward<Args>(args)...);}

	template<typename Arg> GenericString& operator<<(Arg&& value)
	{
		// TODO: potential code bloat, can it be reduced?
		const size_t maxLen = MaxLengthOfToString(value);
		SetLengthUninitialized(Length() + maxLen);
		auto bufferRest = Tail(maxLen);
		ToString(bufferRest, Forward<Arg>(value));
		SetLengthUninitialized(Length() - bufferRest.Length());
		return *this;
	}

	INTRA_NODISCARD constexpr forceinline GenericStringView<Char> View()
	{return {Data(), Length()};}
	
	INTRA_NODISCARD constexpr forceinline GenericStringView<Char> View(size_t startIndex, size_t endIndex)
	{
		INTRA_PRECONDITION(startIndex <= endIndex);
		INTRA_PRECONDITION(endIndex <= Length());
		return GenericStringView<Char>::FromPointerRange(Data() + startIndex, Data() + endIndex);
	}

	INTRA_NODISCARD constexpr forceinline GenericStringView<const Char> View() const
	{return {Data(), Length()};}
	
	INTRA_NODISCARD constexpr forceinline GenericStringView<const Char> View(size_t startIndex, size_t endIndex) const
	{
		INTRA_PRECONDITION(startIndex <= endIndex);
		INTRA_PRECONDITION(endIndex <= Length());
		return GenericStringView<Char>::FromPointerRange(Data() + startIndex, Data() + endIndex);
	}

	INTRA_NODISCARD constexpr forceinline bool IsHeapAllocated() const {return (u.m.Capacity & SSO_LONG_BIT_MASK) != 0;}

private:
	struct M
	{
		mutable Char* Data;
		size_t Len;
		size_t Capacity;
	};

	// Before calling constructor object is a string consisting of sizeof(M)/sizeof(Char)-1 '\0' chars
	union
	{
		M m;
		Char buf[sizeof(M)/sizeof(Char)]{};
	} u;

	enum: size_t
	{
		SSO_LE = INTRA_PLATFORM_ENDIANESS == INTRA_PLATFORM_ENDIANESS_LittleEndian,
		SSO_LONG_BIT_MASK = SSO_LE? //Выбираем бит, который будет означать, выделена ли память в куче. Он должен находиться в последнем элементе mBuffer.
			(size_t(1) << (sizeof(size_t)*8-1)): //В little-endian лучше всего подходит старший бит m.Capacity.
			1, //В big-endian лучше всего подходит младший бит m.Capacity.
		SSO_CAPACITY_MASK = ~SSO_LONG_BIT_MASK,
		SSO_CAPACITY_RIGHT_SHIFT = 1-SSO_LE, //В little-endian сдвига нет, а в big-endian нужно убрать младший бит, не относящийся к ёмкости выделенного буфера
		SSO_SHORT_SIZE_SHIFT = 1-SSO_LE, //Когда строка размещается в SSO, в последнем элементе mBuffer содержится обратная длина строки. Но в big-endian младший бит занят, поэтому нужен сдвиг
		SSO_BUFFER_CAPACITY_CHARS = sizeof(M)/sizeof(Char)-1, //В режиме SSO используется все символы буфера объекта кроме последнего, который может быть только терминирующим нулём
		SSO_CAPACITY_FIELD_FOR_EMPTY = SSO_LE? //Значение m.Capacity, соответствующее пустой строке. Нужно для конструктора по уиолчанию
			SSO_BUFFER_CAPACITY_CHARS << (sizeof(size_t)-sizeof(Char))*8:
			SSO_BUFFER_CAPACITY_CHARS << 1
	};

	constexpr forceinline void setShortLength(size_t len) noexcept
	{u.buf[SSO_BUFFER_CAPACITY_CHARS] = Char((SSO_BUFFER_CAPACITY_CHARS - len) << SSO_SHORT_SIZE_SHIFT);}

	constexpr forceinline size_t getShortLength() const noexcept
	{return SSO_BUFFER_CAPACITY_CHARS - (u.buf[SSO_BUFFER_CAPACITY_CHARS] >> SSO_SHORT_SIZE_SHIFT);}

	constexpr forceinline void setLongCapacity(size_t newCapacity) noexcept
	{u.m.Capacity = (newCapacity << SSO_CAPACITY_RIGHT_SHIFT)|SSO_LONG_BIT_MASK;}

	constexpr forceinline size_t getLongCapacity() const noexcept
	{return (u.m.Capacity & SSO_CAPACITY_MASK) >> SSO_CAPACITY_RIGHT_SHIFT;}

	constexpr forceinline void shortLenDecrement()
	{u.buf[SSO_BUFFER_CAPACITY_CHARS] = Char(u.buf[SSO_BUFFER_CAPACITY_CHARS] + (1 << SSO_SHORT_SIZE_SHIFT));}

	constexpr forceinline void shortLenIncrement()
	{u.buf[SSO_BUFFER_CAPACITY_CHARS] = Char(u.buf[SSO_BUFFER_CAPACITY_CHARS] - (1 << SSO_SHORT_SIZE_SHIFT));}

	constexpr forceinline bool emptyShort() const noexcept
	{return u.buf[SSO_BUFFER_CAPACITY_CHARS] == Char(SSO_BUFFER_CAPACITY_CHARS << SSO_SHORT_SIZE_SHIFT);}

	constexpr forceinline void resetToEmptySsoWithoutFreeing() noexcept
	{u.m.Capacity = SSO_CAPACITY_FIELD_FOR_EMPTY;}

	forceinline void freeLongData() noexcept
	{deallocate(u.m.Data, getLongCapacity()*sizeof(Char));}

	constexpr forceinline bool containsView(StringView rhs) const noexcept
	{return Data() <= rhs.Data() && rhs.Data() < End();}

	constexpr forceinline bool sameRange(StringView rhs) const noexcept
	{return Data() == rhs.Data() && Length() == rhs.Length();}

	static forceinline Char* allocate(size_t newCapacityInBytes)
	{return GlobalHeap.Allocate(newCapacityInBytes, INTRA_SOURCE_INFO);}

	static forceinline void deallocate(Char* data, size_t oldCapacityInBytes)
	{GlobalHeap.Free(data, oldCapacityInBytes);}
};

template<typename Char> INTRA_NODISCARD forceinline
GenericString<Char> operator+(GenericString<Char>&& lhs, GenericStringView<const Char> rhs)
{return Move(lhs += rhs);}

template<typename R,
	typename Char = TValueTypeOfAs<R>
> INTRA_NODISCARD forceinline Requires<
	CArrayClass<R> &&
	!CSame<R, GenericStringView<const Char>>,
GenericString<Char>> operator+(GenericString<Char>&& lhs, R&& rhs)
{return Move(lhs += GenericStringView<const Char>(rhs));}


template<typename Char> INTRA_NODISCARD forceinline
GenericString<Char> operator+(GenericString<Char>&& lhs, Char rhs)
{return Move(lhs += rhs);}

#ifdef INTRA_USER_DEFINED_LITERALS_SUPPORT
INTRA_NODISCARD forceinline String operator ""_s(const char* str, size_t len)
{return String(str, len);}

INTRA_NODISCARD forceinline WString operator ""_w(const char16_t* str, size_t len)
{return WString(str, len);}

INTRA_NODISCARD forceinline DString operator ""_d(const char32_t* str, size_t len)
{return DString(str, len);}
#endif

template<typename T, typename... Args> INTRA_NODISCARD forceinline String StringOfConsume(T&& value, Args&&... args)
{return String::Format()(Forward<T>(value), Forward<Args>(args)...);}

template<typename T, typename... Args> INTRA_NODISCARD forceinline String StringOf(const T& value, Args&&... args)
{return String::Format()(value, Forward<Args>(args)...);}

template<typename T, size_t N, typename... Args> INTRA_NODISCARD forceinline String StringOf(T(&value)[N], Args&&... args)
{return String::Format()(value, Forward<Args>(args)...);}

INTRA_NODISCARD forceinline const String& StringOf(const String& value) {return value;}
INTRA_NODISCARD forceinline StringView StringOf(const StringView& value) {return value;}
INTRA_NODISCARD forceinline StringView StringOf(const char* value) {return StringView(value);}
template<size_t N> INTRA_NODISCARD forceinline StringView StringOf(const char(&value)[N]) {return StringView(value);}


template<typename Char> constexpr bool IsTriviallyRelocatable<GenericString<Char>> = true;

namespace D {
template<typename Char> struct TCommonRef_<GenericStringView<Char>, GenericString<Char>> { typedef GenericString<Char> _; };
template<typename Char> struct TCommonRef_<GenericStringView<Char>&, GenericString<Char>> { typedef GenericString<Char> _; };
template<typename Char> struct TCommonRef_<GenericStringView<Char>&&, GenericString<Char>> { typedef GenericString<Char> _; };
template<typename Char> struct TCommonRef_<const GenericStringView<Char>&, GenericString<Char>> { typedef GenericString<Char> _; };

template<typename Char> struct TCommonRef_<GenericStringView<Char>, GenericString<Char>&> { typedef GenericString<Char> _; };
template<typename Char> struct TCommonRef_<GenericStringView<Char>&, GenericString<Char>&> { typedef GenericString<Char> _; };
template<typename Char> struct TCommonRef_<GenericStringView<Char>&&, GenericString<Char>&> { typedef GenericString<Char> _; };
template<typename Char> struct TCommonRef_<const GenericStringView<Char>&, GenericString<Char>&> { typedef GenericString<Char> _; };

template<typename Char> struct TCommonRef_<GenericStringView<Char>, GenericString<Char>&&> { typedef GenericString<Char> _; };
template<typename Char> struct TCommonRef_<GenericStringView<Char>&, GenericString<Char>&&> { typedef GenericString<Char> _; };
template<typename Char> struct TCommonRef_<GenericStringView<Char>&&, GenericString<Char>&&> { typedef GenericString<Char> _; };
template<typename Char> struct TCommonRef_<const GenericStringView<Char>&, GenericString<Char>&&> { typedef GenericString<Char> _; };

template<typename Char> struct TCommonRef_<GenericStringView<Char>, const GenericString<Char>&> { typedef GenericString<Char> _; };
template<typename Char> struct TCommonRef_<GenericStringView<Char>&, const GenericString<Char>&> { typedef GenericString<Char> _; };
template<typename Char> struct TCommonRef_<GenericStringView<Char>&&, const GenericString<Char>&> { typedef GenericString<Char> _; };
template<typename Char> struct TCommonRef_<const GenericStringView<Char>&, const GenericString<Char>&> { typedef GenericString<Char> _; };



template<typename Char> struct TCommonRef_<GenericString<Char>, GenericStringView<Char>> { typedef GenericString<Char> _; };
template<typename Char> struct TCommonRef_<GenericString<Char>, GenericStringView<Char>&> { typedef GenericString<Char> _; };
template<typename Char> struct TCommonRef_<GenericString<Char>, GenericStringView<Char>&&> { typedef GenericString<Char> _; };
template<typename Char> struct TCommonRef_<GenericString<Char>, const GenericStringView<Char>&> { typedef GenericString<Char> _; };

template<typename Char> struct TCommonRef_<GenericString<Char>&, GenericStringView<Char>> { typedef GenericString<Char> _; };
template<typename Char> struct TCommonRef_<GenericString<Char>&, GenericStringView<Char>&> { typedef GenericString<Char> _; };
template<typename Char> struct TCommonRef_<GenericString<Char>&, GenericStringView<Char>&&> { typedef GenericString<Char> _; };
template<typename Char> struct TCommonRef_<GenericString<Char>&, const GenericStringView<Char>&> { typedef GenericString<Char> _; };

template<typename Char> struct TCommonRef_<GenericString<Char>&&, GenericStringView<Char>> { typedef GenericString<Char> _; };
template<typename Char> struct TCommonRef_<GenericString<Char>&&, GenericStringView<Char>&> { typedef GenericString<Char> _; };
template<typename Char> struct TCommonRef_<GenericString<Char>&&, GenericStringView<Char>&&> { typedef GenericString<Char> _; };
template<typename Char> struct TCommonRef_<GenericString<Char>&&, const GenericStringView<Char>&> { typedef GenericString<Char> _; };

template<typename Char> struct TCommonRef_<const GenericString<Char>&, GenericStringView<Char>> { typedef GenericString<Char> _; };
template<typename Char> struct TCommonRef_<const GenericString<Char>&, GenericStringView<Char>&> { typedef GenericString<Char> _; };
template<typename Char> struct TCommonRef_<const GenericString<Char>&, GenericStringView<Char>&&> { typedef GenericString<Char> _; };
template<typename Char> struct TCommonRef_<const GenericString<Char>&, const GenericStringView<Char>&> { typedef GenericString<Char> _; };



template<typename Char> struct TCommonRef_<GenericStringView<const Char>, GenericString<Char>> { typedef GenericString<Char> _; };
template<typename Char> struct TCommonRef_<GenericStringView<const Char>&, GenericString<Char>> { typedef GenericString<Char> _; };
template<typename Char> struct TCommonRef_<GenericStringView<const Char>&&, GenericString<Char>> { typedef GenericString<Char> _; };
template<typename Char> struct TCommonRef_<const GenericStringView<const Char>&, GenericString<Char>> { typedef GenericString<Char> _; };

template<typename Char> struct TCommonRef_<GenericStringView<const Char>, GenericString<Char>&> { typedef GenericString<Char> _; };
template<typename Char> struct TCommonRef_<GenericStringView<const Char>&, GenericString<Char>&> { typedef GenericString<Char> _; };
template<typename Char> struct TCommonRef_<GenericStringView<const Char>&&, GenericString<Char>&> { typedef GenericString<Char> _; };
template<typename Char> struct TCommonRef_<const GenericStringView<const Char>&, GenericString<Char>&> { typedef GenericString<Char> _; };

template<typename Char> struct TCommonRef_<GenericStringView<const Char>, GenericString<Char>&&> { typedef GenericString<Char> _; };
template<typename Char> struct TCommonRef_<GenericStringView<const Char>&, GenericString<Char>&&> { typedef GenericString<Char> _; };
template<typename Char> struct TCommonRef_<GenericStringView<const Char>&&, GenericString<Char>&&> { typedef GenericString<Char> _; };
template<typename Char> struct TCommonRef_<const GenericStringView<const Char>&, GenericString<Char>&&> { typedef GenericString<Char> _; };

template<typename Char> struct TCommonRef_<GenericStringView<const Char>, const GenericString<Char>&> { typedef GenericString<Char> _; };
template<typename Char> struct TCommonRef_<GenericStringView<const Char>&, const GenericString<Char>&> { typedef GenericString<Char> _; };
template<typename Char> struct TCommonRef_<GenericStringView<const Char>&&, const GenericString<Char>&> { typedef GenericString<Char> _; };
template<typename Char> struct TCommonRef_<const GenericStringView<const Char>&, const GenericString<Char>&> { typedef GenericString<Char> _; };



template<typename Char> struct TCommonRef_<GenericString<Char>, GenericStringView<const Char>> { typedef GenericString<Char> _; };
template<typename Char> struct TCommonRef_<GenericString<Char>, GenericStringView<const Char>&> { typedef GenericString<Char> _; };
template<typename Char> struct TCommonRef_<GenericString<Char>, GenericStringView<const Char>&&> { typedef GenericString<Char> _; };
template<typename Char> struct TCommonRef_<GenericString<Char>, const GenericStringView<const Char>&> { typedef GenericString<Char> _; };

template<typename Char> struct TCommonRef_<GenericString<Char>&, GenericStringView<const Char>> { typedef GenericString<Char> _; };
template<typename Char> struct TCommonRef_<GenericString<Char>&, GenericStringView<const Char>&> { typedef GenericString<Char> _; };
template<typename Char> struct TCommonRef_<GenericString<Char>&, GenericStringView<const Char>&&> { typedef GenericString<Char> _; };
template<typename Char> struct TCommonRef_<GenericString<Char>&, const GenericStringView<const Char>&> { typedef GenericString<Char> _; };

template<typename Char> struct TCommonRef_<GenericString<Char>&&, GenericStringView<const Char>> { typedef GenericString<Char> _; };
template<typename Char> struct TCommonRef_<GenericString<Char>&&, GenericStringView<const Char>&> { typedef GenericString<Char> _; };
template<typename Char> struct TCommonRef_<GenericString<Char>&&, GenericStringView<const Char>&&> { typedef GenericString<Char> _; };
template<typename Char> struct TCommonRef_<GenericString<Char>&&, const GenericStringView<const Char>&> { typedef GenericString<Char> _; };

template<typename Char> struct TCommonRef_<const GenericString<Char>&, GenericStringView<const Char>> { typedef GenericString<Char> _; };
template<typename Char> struct TCommonRef_<const GenericString<Char>&, GenericStringView<const Char>&> { typedef GenericString<Char> _; };
template<typename Char> struct TCommonRef_<const GenericString<Char>&, GenericStringView<const Char>&&> { typedef GenericString<Char> _; };
template<typename Char> struct TCommonRef_<const GenericString<Char>&, const GenericStringView<const Char>&> { typedef GenericString<Char> _; };

}


static_assert(CSame<TCommonRef<String, StringView>, String>, "ERROR!");
static_assert(CSequentialContainer<String>, "ERROR!");
static_assert(CDynamicArrayContainer<String>, "ERROR!");
static_assert(CPod<TValueTypeOf<String>>, "ERROR!");

template<typename Char> GenericString<Char> operator+(GenericStringView<const Char> lhs, GenericStringView<const Char> rhs)
{
	GenericString<Char> result;
	result.Reserve(lhs.Length()+rhs.Length());
	result += lhs;
	result += rhs;
	return result;
}

template<typename Char> GenericString<Char> operator+(GenericStringView<const Char> lhs, Char rhs)
{
	GenericString<Char> result;
	result.Reserve(lhs.Length()+1);
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

template<typename S1, typename S2,
	typename Char=TArrayElement<S1>
> forceinline Requires<
	CArrayClass<S1> && CArrayClass<S2> &&
	(CHasData<S1> ||
		CHasData<S2> ||
		CInputRange<S1> ||
		CInputRange<S2> ||
		CArrayType<TRemoveReference<S1>> ||
		CArrayType<TRemoveReference<S2>>) && // To avoid conflicts with STL operator+
	CSame<Char, TArrayElement<S2>>,
GenericString<Char>> operator+(S1&& lhs, S2&& rhs)
{return GenericStringView<const Char>(lhs)+GenericStringView<const Char>(rhs);}

template<typename S1, typename S2,
typename AsS1 = TRangeOfType<S1>,
typename AsS2 = TRangeOfType<S2>,
typename Char = TValueTypeOf<AsS1>>
Requires<
	(CInputRange<AsS1> || CInputRange<AsS2>) &&
	CConsumableRange<AsS1> &&
	CConsumableRange<AsS2> &&
	(!CArrayClass<S1> || !CArrayClass<S2> || !CSame<Char, TArrayElement<AsS2>>),
GenericString<Char>> operator+(S1&& lhs, S2&& rhs)
{
	GenericString<Char> result;
	result.Reserve(LengthOr(lhs) + LengthOr(rhs));
	CopyTo(ForwardAsRange<S1>(lhs), LastAppender(result));
	CopyTo(ForwardAsRange<S2>(rhs), LastAppender(result));
	return result;
}

template<typename R,
	typename AsR = TRangeOfType<R>,
	typename Char = TValueTypeOf<AsR>
> Requires<
	CConsumableRange<AsR> &&
	(!CHas_data<R> || CHasAsRangeMethod<R> || CInputRange<R>) && //To avoid conflicts with STL
	CChar<Char>,
GenericString<Char>> operator+(R&& lhs, Char rhs)
{
	GenericString<Char> result;
	result.Reserve(LengthOr(lhs) + 1);
	result += ForwardAsRange<R>(lhs);
	result += rhs;
	return result;
}

template<typename R, typename Char,
	typename Char2 = TValueTypeOfAs<R>
> Requires<
	CAsConsumableRange<R> &&
	(!CHas_data<R> || CHasAsRangeMethod<R> || CInputRange<R>) && //To avoid conflicts with STL
	CChar<Char2> &&
	CChar<Char>,
GenericString<Char2>> operator+(Char lhs, R&& rhs)
{
	GenericString<Char2> result;
	result.Reserve(1 + LengthOr(rhs));
	result += lhs;
	result += ForwardAsRange<R>(rhs);
	return result;
}

static_assert(CAsConsumableRange<String>, "ERROR!");
static_assert(CAsConsumableRange<const String>, "ERROR!");
static_assert(CSequentialContainer<String>, "ERROR!");
static_assert(CSequentialContainer<const String>, "ERROR!");

static_assert(CSame<TRangeOfType<const String&>, StringView>, "ERROR!");

INTRA_END
