#pragma once

#include "Extra/Container/ForwardDecls.h"
#include "Extra/Container/Operations.hh"
#include "Extra/Memory/Allocator.hh"
#include "Extra/Memory/Memory.h"
#include "Intra/Assert.h"
#include "Intra/Container/Concepts.h"
#include "Intra/Misc/RawMemory.h"
#include "Intra/Range/Inserter.h"
#include "Intra/Range/Mutation/Copy.h"
#include "Intra/Range/Mutation/Fill.h"
#include "Intra/Range/Mutation/ReplaceSubrange.h"
#include "Intra/Range/Span.h"
#include "Intra/Range/Special/Unicode.h"
#include "Intra/Range/StringView.h"
#include "Intra/Range/TakeUntil.h"
#include "Intra/Range/Zip.h"
#include "StringFormatter.h"

EXTRA_BEGIN
//INTRA_IGNORE_WARNING_SIGN_CONVERSION
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
	constexpr GenericString(const Char* str):
		GenericString(str, (str == null)? 0: Misc::CStringLength(str)) {}

	constexpr GenericString(decltype(null)=null) noexcept {setShortLength(0);}

	constexpr static GenericString CreateReserve(Index reservedCapacity)
	{
		GenericString result;
		result.Reserve(reservedCapacity);
		return result;
	}

	constexpr explicit GenericString(const Char* str, Index strLength)
	{
		SetLengthUninitialized(strLength);
		Misc::BitwiseCopyUnsafe(Data(), str, strLength);
	}

	constexpr explicit GenericString(const Char* startPtr, const Char* endPtr):
		GenericString(startPtr, endPtr - startPtr) {}
	
	constexpr explicit GenericString(Index initialLength, Char filler): GenericString(null)
	{SetLength(initialLength, filler);}

	template<typename R = StringView, typename AsR = TRangeOfRef<R>,
	typename = Requires<
		CConsumableRangeOf<AsR, Char> &&
		CChar<TValueTypeOf<AsR>>
	>> constexpr GenericString(R&& rhs)
	{
		setShortLength(0); //avoid unnecessary copying of default zeros on allocation
		SetLengthUninitialized(Count(rhs));
		CopyTo(ForwardAsRange<R>(rhs), View());
	}

	constexpr GenericString(const GenericString& rhs): u(rhs.u)
	{
		if(!IsHeapAllocated()) return;
		u.m.Data = allocate(u.m.Len);
		Misc::BitwiseCopyUnsafe(u.m.Data, rhs.u.m.Data, u.m.Len);
	}
	
	constexpr GenericString(GenericString&& rhs):
		u(rhs.u) {rhs.resetToEmptySsoWithoutFreeing();}
	

	~GenericString()
	{
		//The destructor is the most serious obstacle to String being constexpr
		if(IsHeapAllocated()) freeLongData();
	}

	//! Get UTF-32 character range
	//UTF8 ByChar() const {return UTF8(Data(), Length());}

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
		INTRA_PRECONDITION(!containsView(rhs));
		SetLengthUninitialized(0); //avoid unnecessary copying on reallocation
		SetLengthUninitialized(rhs.Length());
		Misc::BitwiseCopyUnsafe(Data(), rhs.Data(), rhs.Length());
		return *this;
	}

	constexpr GenericString& operator=(const GenericString& rhs)
	{return operator=(rhs.View());}

	constexpr GenericString& operator=(const Char* rhs)
	{return operator=(GenericStringView<const Char>(rhs));}
	
	template<typename R> constexpr Requires<
		CArrayClass<R>,
	GenericString&> operator=(R&& rhs)
	{return operator=(GenericStringView<const Char>(rhs));}

	template<typename R> constexpr Requires<
		!CArrayClass<R> &&
		CAsConsumableRangeOf<R, Char>,
	GenericString&> operator=(R&& rhs)
	{
		SetLengthUninitialized(0); //avoid unnecessary copying on reallocation
		SetLengthUninitialized(Count(rhs));
		CopyTo(ForwardAsRange<R>(rhs), View());
		return *this;
	}

	constexpr GenericString& operator=(decltype(null))
	{
		if(IsHeapAllocated()) freeLongData();
		resetToEmptySsoWithoutFreeing();
		return *this;
	}
	///@}

	constexpr GenericString& operator+=(StringFormatter<GenericString>& rhs) {return operator+=(*rhs);}
	constexpr GenericString& operator+=(Char rhs)
	{
		const auto oldLen = Length();
		const bool wasAllocated = IsHeapAllocated();
		if(!wasAllocated && size_t(oldLen) < SSO_BUFFER_CAPACITY_CHARS)
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

	constexpr bool operator==(decltype(null)) const {return Empty();}

	constexpr bool operator==(const GenericStringView<const Char>& rhs) const {return View() == rhs;}

	constexpr bool operator>(const GenericStringView<const Char>& rhs) const {return View() > rhs;}
	constexpr bool operator<(const Char* rhs) const {return View() < rhs;}
	constexpr bool operator<(const GenericStringView<const Char>& rhs) const {return View() < rhs;}


	//! Make sure that Capacity() >= \p minCapacity. Otherwise reallocates to at least \p minCapacity.
	constexpr void Reserve(Size minCapacity)
	{
		if(size_t(Capacity()) < size_t(minCapacity))
			Reallocate(size_t(minCapacity) + size_t(Length()) / 2);
	}


	//! Reallocate String to hold \p newCapacity characters.
	void Reallocate(Index newCapacity)
	{
		const bool wasAllocated = IsHeapAllocated();
		if(!wasAllocated && size_t(newCapacity) <= SSO_BUFFER_CAPACITY_CHARS) return;
		const auto len = size_t(Length());
		auto realNewCapacity = FMax(size_t(newCapacity), len);
		Char* newData;
		if(realNewCapacity > SSO_BUFFER_CAPACITY_CHARS)
		{
			size_t newCapacityInBytes = realNewCapacity*sizeof(Char);
			newData = allocate(newCapacityInBytes);
			realNewCapacity = newCapacityInBytes / sizeof(Char);
		}
		else
		{
			newData = u.buf;
			setShortLength(len);
		}
		Char* const oldData = u.m.Data;
		const auto oldLongCapacity = getLongCapacity();
		CopyTo(SpanOfPtr(Data(), len), SpanOfPtr(newData, len));
		if(wasAllocated) deallocate(oldData, oldLongCapacity*sizeof(Char));
		if(newData != u.buf)
		{
			u.m.Data = newData;
			setLongCapacity(realNewCapacity);
			u.m.Len = len;
		}
	}

	//! If there is more than 20% excess capacity of heap allocation, reallocate so that Length() == Capacity().
	constexpr void TrimExcessCapacity()
	{if(Capacity() > Length() + (Length() >> 2)) Reallocate(Length());}

	//! Make sure there is enough space to append minSpace chars.
	constexpr void RequireSpace(Size minSpace) {Reserve(size_t(Length()) + size_t(minSpace));}

	//! Set string length to \p newLen.
	/*!
	  If newLen < Length(), discards last Length() - newLen code points.
	  otherwise, adds newLen - Length() copies of \p filler.
	*/
	constexpr void SetLength(Index newLen, Char filler = '\0')
	{
		const auto oldLength = Length();
		SetLengthUninitialized(newLen);
		if(size_t(newLen) <= size_t(oldLength)) return;
		if(filler == '\0') FillZeros(View(oldLength, newLen));
		else Fill(View(oldLength, newLen), filler);
	}

	constexpr void SetCount(Index newLen, Char filler = '\0') { SetLength(newLen, filler); }

	/** Set string length to ``newLen``.
	  If ``newLen`` < Length(), discards last Length() - newLen code points.
	  otherwise, adds ``newLen`` - Length() uninitialized code points.
	*/
	constexpr void SetLengthUninitialized(Index newLen)
	{
		Reserve(newLen);
		if(IsHeapAllocated()) u.m.Len = size_t(newLen);
		else setShortLength(newLen);
	}

	//! Same as SetLengthUnitialized, used by template detection code.
	//! @see SetLengthUninitialized
	constexpr void SetCountUninitialized(Index newLen)
	{SetLengthUninitialized(newLen);}

	//! Number of code points this string can hold without memory reallocation.
	constexpr index_t Capacity() const
	{return IsHeapAllocated()? getLongCapacity(): SSO_BUFFER_CAPACITY_CHARS;}

	constexpr Char& operator[](Index index)
	{
		INTRA_PRECONDITION(size_t(index) < size_t(Length()));
		return Data()[size_t(index)];
	}

	constexpr const Char& operator[](Index index) const
	{
		INTRA_PRECONDITION(size_t(index) < size_t(Length()));
		return Data()[size_t(index)];
	}

	[[nodiscard]] constexpr bool Empty() const noexcept
	{return IsHeapAllocated()? u.m.Len == 0: emptyShort();}

	constexpr void PopLast()
	{
		INTRA_PRECONDITION(!Empty());
		if(IsHeapAllocated()) u.m.Len--;
		else shortLenDecrement();
	}

	[[nodiscard]] constexpr index_t Length() const
	{return IsHeapAllocated()? u.m.Len: getShortLength();}

	[[nodiscard]] constexpr Char& First()
	{
		INTRA_PRECONDITION(!Empty());
		return *Data();
	}

	[[nodiscard]] constexpr Char First() const
	{
		INTRA_PRECONDITION(!Empty());
		return *Data();
	}

	[[nodiscard]] constexpr Char& Last()
	{
		INTRA_PRECONDITION(!Empty());
		return Data()[size_t(Length()-1)];
	}
	
	[[nodiscard]] constexpr Char Last() const
	{
		INTRA_PRECONDITION(!Empty());
		return Data()[size_t(Length()-1)];
	}

	/**
	  @name Views to internal data
	  @return StringView containing all string characters.
	  @warning
	  The returned range becomes invalid after any modification to this string or its destruction.
	  Use this method carefully with temporary objects:
	  auto range = String("a").View(); //Error: range is dangling.
	  foo(String("a").View()); //OK: the returned StringView is valid before foo returns.
	**/
	///@{
	[[nodiscard]] constexpr Char* Data() {return IsHeapAllocated()? u.m.Data: u.buf;}
	[[nodiscard]] constexpr const Char* Data() const {return IsHeapAllocated()? u.m.Data: u.buf;}
	[[nodiscard]] constexpr Char* End() {return Data()+Length();}
	[[nodiscard]] constexpr const Char* End() const {return Data()+Length();}

	[[nodiscard]] const Char* CStr()
	{
		const auto len = Length();
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
	[[nodiscard]] static GenericString ReplaceAll(GenericStringView<const Char> str,
		GenericStringView<const Char> subStr, GenericStringView<const Char> newSubStr)
	{
		GenericString result;
		CountRange<Char> counter;
		MultiReplaceToAdvance(str, counter, RangeOf({TupleL(subStr, newSubStr)}));
		result.SetLengthUninitialized(counter.Counter);
		MultiReplaceToAdvance(str, result.View(), RangeOf({TupleL(subStr, newSubStr)}));
		return result;
	}

	[[nodiscard]] static GenericString ReplaceAll(GenericStringView<const Char> str, Char c, Char newc)
	{
		if(c == newc) return str;
		GenericString result;
		result.SetLengthUninitialized(str.Length());
		for(size_t i = 0; i < size_t(str.Length()); i++)
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

	constexpr void AddLast(Char c) { operator+=(c); }

	[[nodiscard]] constexpr Char* begin() { return Data(); }
	[[nodiscard]] constexpr Char* end() { return Data() + Length(); }
	[[nodiscard]] constexpr const Char* begin() const { return Data(); }
	[[nodiscard]] constexpr const Char* end() const { return Data() + Length(); }

#ifdef INTRA_CONTAINER_STL_FORWARD_COMPATIBILITY
	//! @defgroup String_STL_Interface STL-like interface for String
	//! For compatibility with generic container-independent code.
	//! It is not recommended to use this interface directly.
	//!@{
	typedef char* iterator;
	typedef const char* const_iterator;
	typedef Char value_type;

	void push_back(Char c) { operator+=(c); }
	void reserve(size_t capacity) { Reserve(capacity); }
	void resize(size_t newLength, Char filler = '\0') { SetLength(newLength, filler); }
	size_t size() const { return size_t(Length()); }
	size_t length() const { return size_t(Length()); }
	const Char* c_str() { return CStr(); } // Non-const, while const in STL
	GenericString substr(size_t start, size_t count) const
	{
		return Slice(start, start + count);
	}
	bool empty() const { return Empty(); }
	Char* data() { return Data(); }
	const Char* data() const { return Data(); }

	GenericString& append(const GenericString& str) { *this += str.View(); }

	GenericString& append(const GenericString& str, size_t subpos, size_t sublen)
	{
		*this += str.View(index_t(subpos), index_t(subpos + sublen));
	}

	GenericString& append(const Char* s) { *this += GenericStringView<const Char>(s); }

	GenericString& append(const Char* s, size_t n)
	{
		*this += GenericStringView<const Char>(s, index_t(n));
	}

	GenericString& append(size_t n, Char c)
	{
		const auto oldLen = Length();
		SetLengthUninitialized(oldLen + index_t(n));
		Fill(Drop(oldLen), c);
	}

	template<class InputIt> GenericString& append(InputIt firstIt, InputIt endIt)
	{
		while(firstIt != endIt) *this += *endIt++;
	}

	void clear() {SetLengthUninitialized(0);}
	//!@}
#endif


	//! String formatting.
	//! @param format String, containing markers <^> where the arguments should be substituted.
	//! @param () Use braces to pass arguments and their formatting options
	//! @return A proxy formatter object implicitly convertible to GenericString.
	[[nodiscard]] static StringFormatter<GenericString> Format(GenericStringView<const Char> format=null)
	{return StringFormatter<GenericString>(format);}

	//! Convert all arguments to string and concatenate them.
	template<typename... Args> [[nodiscard]] static GenericString Concat(Args&&... args)
	{return StringFormatter<GenericString>(null).Arg(Forward<Args>(args)...);}

	template<typename Arg> GenericString& operator<<(Arg&& value)
	{
		// TODO: potential code bloat, can it be reduced?
		const auto maxLen = MaxLengthOfToString(value);
		SetLengthUninitialized(Length() + maxLen);
		auto bufferRest = Tail(maxLen);
		ToString(bufferRest, Forward<Arg>(value));
		SetLengthUninitialized(Length() - bufferRest.Length());
		return *this;
	}

	[[nodiscard]] constexpr GenericStringView<Char> View()
	{return GenericStringView<Char>::FromPointerAndLength(Data(), Length());}
	
	[[nodiscard]] constexpr GenericStringView<Char> View(
		Index startIndex, Index endIndex)
	{
		INTRA_PRECONDITION(size_t(startIndex) <= size_t(endIndex));
		INTRA_PRECONDITION(size_t(endIndex) <= size_t(Length()));
		return GenericStringView<Char>::FromPointerRange(Data() + size_t(startIndex), Data() + size_t(endIndex));
	}

	[[nodiscard]] constexpr GenericStringView<const Char> View() const
	{return GenericStringView<const Char>(*this);}
	
	[[nodiscard]] constexpr GenericStringView<const Char> View(Index startIndex, Index endIndex) const
	{
		INTRA_PRECONDITION(size_t(startIndex) <= size_t(endIndex));
		INTRA_PRECONDITION(size_t(endIndex) <= size_t(Length()));
		return View().Drop(startIndex).TakeExactly(size_t(endIndex) - size_t(startIndex));
	}

	[[nodiscard]] constexpr bool IsHeapAllocated() const {return (u.m.Capacity & SSO_LONG_BIT_MASK) != 0;}

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
		SSO_LE = TargetByteOrder == ByteOrder::LittleEndian,
		SSO_LONG_BIT_MASK = SSO_LE? //Choosing a bit indicating a heap allocation. It must be in the last element of u.buf.
			(size_t(1) << (sizeof(size_t)*8-1)): //for little-endian we choose the highest bit of m.Capacity.
			1, //for big-endian we choose the lowest bit of m.Capacity.
		SSO_CAPACITY_MASK = ~SSO_LONG_BIT_MASK,
		SSO_CAPACITY_RIGHT_SHIFT = 1-SSO_LE, //There is no shift for little-endian, for big-endian we need to remove the lowest bit
		SSO_SHORT_SIZE_SHIFT = 1-SSO_LE, //When string is in SSO, in the last element of u.buf we have the inverted string length.
			                            // However, for big-endian the lowest bit is occupied by the heap indicator, so we need to remove it with a right shift
		
		SSO_BUFFER_CAPACITY_CHARS = sizeof(M)/sizeof(Char)-1, //In SSO we use all elements of the buffer except the last one, which is the inverted length, so that it becomes a \0 when buffer is full.
		SSO_CAPACITY_FIELD_FOR_EMPTY = SSO_LE? //m.Capacity value, corresponding to an empty string. Used by the default constructor above.
			SSO_BUFFER_CAPACITY_CHARS << (sizeof(index_t)-sizeof(Char))*8:
			SSO_BUFFER_CAPACITY_CHARS << 1
	};

	constexpr void setShortLength(Index len) noexcept
	{u.buf[SSO_BUFFER_CAPACITY_CHARS] = Char((SSO_BUFFER_CAPACITY_CHARS - size_t(len)) << SSO_SHORT_SIZE_SHIFT);}

	constexpr index_t getShortLength() const noexcept
	{return SSO_BUFFER_CAPACITY_CHARS - (u.buf[SSO_BUFFER_CAPACITY_CHARS] >> SSO_SHORT_SIZE_SHIFT);}

	constexpr void setLongCapacity(index_t newCapacity) noexcept
	{u.m.Capacity = (size_t(newCapacity) << SSO_CAPACITY_RIGHT_SHIFT)|SSO_LONG_BIT_MASK;}

	constexpr index_t getLongCapacity() const noexcept
	{return (u.m.Capacity & SSO_CAPACITY_MASK) >> SSO_CAPACITY_RIGHT_SHIFT;}

	constexpr void shortLenDecrement()
	{u.buf[SSO_BUFFER_CAPACITY_CHARS] = Char(u.buf[SSO_BUFFER_CAPACITY_CHARS] + (1 << SSO_SHORT_SIZE_SHIFT));}

	constexpr void shortLenIncrement()
	{u.buf[SSO_BUFFER_CAPACITY_CHARS] = Char(u.buf[SSO_BUFFER_CAPACITY_CHARS] - (1 << SSO_SHORT_SIZE_SHIFT));}

	constexpr bool emptyShort() const noexcept
	{return u.buf[SSO_BUFFER_CAPACITY_CHARS] == Char(SSO_BUFFER_CAPACITY_CHARS << SSO_SHORT_SIZE_SHIFT);}

	constexpr void resetToEmptySsoWithoutFreeing() noexcept
	{u.m.Capacity = SSO_CAPACITY_FIELD_FOR_EMPTY;}

	void freeLongData() noexcept
	{deallocate(u.m.Data, getLongCapacity()*sizeof(Char));}

	constexpr bool containsView(StringView rhs) const noexcept
	{return Data() <= rhs.Data() && rhs.Data() < End();}

	constexpr bool sameRange(StringView rhs) const noexcept
	{return Data() == rhs.Data() && Length() == rhs.Length();}

	static Char* allocate(size_t newCapacityInBytes)
	{return GlobalHeap.Allocate(newCapacityInBytes, INTRA_SOURCE_INFO);}

	static void deallocate(Char* data, size_t oldCapacityInBytes)
	{GlobalHeap.Free(data, oldCapacityInBytes);}
};

template<typename Char> [[nodiscard]]
GenericString<Char> operator+(GenericString<Char>&& lhs, GenericStringView<const Char> rhs)
{return Move(lhs += rhs);}

template<typename R,
	typename Char = TValueTypeOfAs<R>
> [[nodiscard]] Requires<
	CArrayClass<R> &&
	!CSame<R, GenericStringView<const Char>>,
GenericString<Char>> operator+(GenericString<Char>&& lhs, R&& rhs)
{return Move(lhs += GenericStringView<const Char>(rhs));}


template<typename Char> [[nodiscard]]
GenericString<Char> operator+(GenericString<Char>&& lhs, Char rhs)
{return Move(lhs += rhs);}

[[nodiscard]] String operator ""_s(const char* str, size_t len)
{return String(str, index_t(len));}

[[nodiscard]] GenericString<char16_t> operator ""_w(const char16_t* str, size_t len)
{return GenericString<char16_t>(str, index_t(len));}

[[nodiscard]] GenericString<char32_t> operator ""_d(const char32_t* str, size_t len)
{return GenericString<char32_t>(str, index_t(len));}

template<typename T, typename... Args> [[nodiscard]] String StringOfConsume(T&& value, Args&&... args)
{return String::Format()(Forward<T>(value), Forward<Args>(args)...);}

template<typename T, typename... Args> [[nodiscard]] String StringOf(const T& value, Args&&... args)
{return String::Format()(value, Forward<Args>(args)...);}

template<typename T, size_t N, typename... Args> [[nodiscard]] String StringOf(T(&value)[N], Args&&... args)
{return String::Format()(value, Forward<Args>(args)...);}

// TODO: unsafe, may cause dangling refs
[[nodiscard]] const String& StringOf(const String& value) {return value;}
[[nodiscard]] StringView StringOf(const StringView& value) {return value;}
[[nodiscard]] StringView StringOf(const char* value) {return StringView(value);}
template<size_t N> [[nodiscard]] StringView StringOf(const char(&value)[N]) {return StringView(value);}


template<typename Char> constexpr bool IsTriviallyRelocatable<GenericString<Char>> = true;

#if INTRA_CONSTEXPR_TEST
static_assert(CSame<TCommonRef<String, StringView>, String>);
static_assert(CSequentialContainer<String>);
static_assert(CDynamicArrayContainer<String>);
static_assert(CSame<TValueTypeOf<String>, char>);
#endif

template<typename Char>
GenericString<Char> operator+(GenericStringView<const Char> lhs, GenericStringView<const Char> rhs)
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
> Requires<
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
typename AsS1 = TRangeOfRef<S1>,
typename AsS2 = TRangeOfRef<S2>,
typename Char = TValueTypeOf<AsS1>>
Requires<
	(CInputRange<AsS1> || CInputRange<AsS2>) &&
	CConsumableRange<AsS1> &&
	CConsumableRange<AsS2> &&
	(!CArrayClass<S1> || !CArrayClass<S2> || !CSame<Char, TArrayElement<AsS2>>),
GenericString<Char>> operator+(S1&& lhs, S2&& rhs)
{
	GenericString<Char> result;
	result.Reserve(LengthOpt(lhs).GetOr(0) + LengthOpt(rhs).GetOr(0));
	CopyTo(ForwardAsRange<S1>(lhs), LastAppender(result));
	CopyTo(ForwardAsRange<S2>(rhs), LastAppender(result));
	return result;
}

template<typename R,
	typename AsR = TRangeOfRef<R>,
	typename Char = TValueTypeOf<AsR>
> Requires<
	CConsumableRange<AsR> &&
	(!CHas_data<R> || CInputRange<R>) && //To avoid conflicts with STL
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
	(!CHas_data<R> || CInputRange<R>) && //To avoid conflicts with STL
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

#if INTRA_CONSTEXPR_TEST
static_assert(CAsConsumableRange<String>);
static_assert(CAsConsumableRange<const String>);
static_assert(CSequentialContainer<String>);
static_assert(CSequentialContainer<const String>);

//static_assert(CSame<TRangeOfRef<const String&>, StringView>);
#endif

EXTRA_END
