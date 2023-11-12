#pragma once

#include "IntraX/Container/ForwardDecls.h"
#include "IntraX/Container/Operations.h"
#include "Intra/Allocator.h"
#include "IntraX/Memory/Memory.h"
#include "StringFormatter.h"

#include "Intra/Misc/RawMemory.h"
#include "Intra/Range/Inserter.h"
#include "Intra/Range/Mutation/Copy.h"
#include "Intra/Range/Mutation/Fill.h"
#include "Intra/Range/Mutation/ReplaceSubrange.h"
#include "Intra/Range/Span.h"
#include "Intra/Range/Special/Unicode.h"
#include "Intra/Range/StringView.h"
#include "Intra/Range/Decorators.h"
#include "Intra/Range/Zip.h"

namespace Intra { INTRA_BEGIN
//INTRA_IGNORE_WARN_SIGN_CONVERSION
/// Class used to create, store and operate strings.
template<typename CodeUnit> class GenericString
{
	ArrayList<CodeUnit> mRawCodeUnits; //TODO: finish
public:
	constexpr GenericString(TUnsafe, const CodeUnit* str):
		GenericString(GenericStringView(Unsafe, str)) {}

	constexpr GenericString() noexcept {setShortLength(0);}

	static constexpr GenericString CreateReserve(Index reservedCapacityInCodePoints)
	{
		GenericString result;
		result.Reserve(reservedCapacityInCodePoints);
		return result;
	}

	constexpr GenericString(GenericStringView<CodePoint> str): mRawCodeUnits(str.RawCodeUnits()) {}

	template<typename R> requires CConstructible<ArrayList<CodeUnit>, R>
	constexpr GenericString(R&& rhs): mRawCodeUnits(INTRA_FWD(rhs))
	{
		INTRA_PRECONDITION(IsValidUnicode(mRawCodeUnits));
	}

	constexpr GenericString(const GenericString& rhs): u(rhs.u)
	{
		if(!IsHeapAllocated()) return;
		u.m.Data = allocate(u.m.Len);
		BitwiseCopy(Unsafe, u.m.Data, rhs.u.m.Data, u.m.Len);
	}
	
	constexpr GenericString(GenericString&& rhs):
		u(rhs.u) {rhs.resetToEmptySsoWithoutFreeing();}
	

	~GenericString()
	{
		//The destructor is the most serious obstacle to String being constexpr
		if(IsHeapAllocated()) freeLongData();
	}

	/// Get UTF-32 character range
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
		CArrayList<R>,
	GenericString&> operator=(R&& rhs)
	{return operator=(GenericStringView<const Char>(rhs));}

	template<typename R> constexpr Requires<
		!CArrayList<R> &&
		CConsumableListOf<R, Char>,
	GenericString&> operator=(R&& rhs)
	{
		SetLengthUninitialized(0); //avoid unnecessary copying on reallocation
		SetLengthUninitialized(Count(rhs));
		CopyTo(ForwardAsRange<R>(rhs), View());
		return *this;
	}

	constexpr GenericString& operator=(decltype(nullptr))
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

	constexpr bool operator==(decltype(nullptr)) const {return Empty();}

	constexpr bool operator==(const GenericStringView<const Char>& rhs) const {return View() == rhs;}

	constexpr bool operator>(const GenericStringView<const Char>& rhs) const {return View() > rhs;}
	constexpr bool operator<(const Char* rhs) const {return View() < rhs;}
	constexpr bool operator<(const GenericStringView<const Char>& rhs) const {return View() < rhs;}


	/// Make sure that `Capacity()` >= `minCapacity`. Otherwise reallocates to at least \p minCapacity.
	constexpr void Reserve(Size minCapacity)
	{
		if(size_t(Capacity()) < size_t(minCapacity))
			Reallocate(size_t(minCapacity) + size_t(Length()) / 2);
	}


	/// Reallocate String to hold `newCapacity` characters.
	void Reallocate(Index newCapacity)
	{
		const bool wasAllocated = IsHeapAllocated();
		if(!wasAllocated && size_t(newCapacity) <= SsoBufferCapacityChars) return;
		const auto len = size_t(Length());
		auto realNewCapacity = Max(size_t(newCapacity), len);
		Char* newData;
		if(realNewCapacity > SsoBufferCapacityChars)
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
		Span(Unsafe, Data(), len)|CopyTo(Span(Unsafe, newData, len));
		if(wasAllocated) deallocate(oldData, oldLongCapacity*sizeof(Char));
		if(newData != u.buf)
		{
			u.m.Data = newData;
			setLongCapacity(realNewCapacity);
			u.m.Len = len;
		}
	}

	/// If there is more than 20% excess capacity of heap allocation, reallocate so that `Length()` == `Capacity()`.
	constexpr void TrimExcessCapacity()
	{if(Capacity() > Length() + (Length() >> 2)) Reallocate(Length());}

	/// Make sure there is enough space to append `minSpace` chars.
	constexpr void RequireSpace(Size minSpace) {Reserve(size_t(Length()) + size_t(minSpace));}

	/** Set string length to `newLen`.

	  If `newLen` < `Length()`, discards last `Length()` - `newLen` code points.
	  otherwise, adds `newLen` - `Length()` copies of `filler`.
	*/
	constexpr void SetLength(Index newLen, Char filler = '\0')
	{
		const auto oldLength = Length();
		SetLengthUninitialized(newLen);
		if(size_t(newLen) <= size_t(oldLength)) return;
		if(filler == '\0') FillZeros(View(oldLength, newLen));
		else Fill(View(oldLength, newLen), filler);
	}

	/** Set string length to ``newLen``.
	  If ``newLen`` < Length(), discards last Length() - newLen code points.
	  otherwise, adds ``newLen`` - Length() uninitialized code points.
	*/
	constexpr void SetLength(Index newLen, TUndefined)
	{
		Reserve(newLen);
		if(IsHeapAllocated()) u.m.Len = size_t(newLen);
		else setShortLength(newLen);
	}

	/// Number of code points this string can hold without memory reallocation.
	constexpr index_t Capacity() const
	{return IsHeapAllocated()? getLongCapacity(): SsoBufferCapacityChars;}

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

	/// Replace all occurences of \p subStr to \p newSubStr.
	[[nodiscard]] static GenericString ReplaceAll(GenericStringView<const Char> str,
		GenericStringView<const Char> subStr, GenericStringView<const Char> newSubStr)
	{
		GenericString result;
		RCounter<Char> counter;
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
		Span<const GenericStringView<const Char>> subStrs,
		Span<const GenericStringView<const Char>> newSubStrs)
	{
		GenericString result;
		CountRange<Char> counter;
		MultiReplaceToAdvance(str, counter, Zip(subStrs, newSubStrs));
		result.SetLengthUninitialized(counter.Counter);
		MultiReplaceTo(str, result, Zip(subStrs, newSubStrs));
		return result;
	}

	constexpr void AddLast(Char c) { operator+=(c); }

	/// String formatting.
	/// @param format String, containing markers <^> where the arguments should be substituted.
	/// @param () Use braces to pass arguments and their formatting options
	/// @return A proxy formatter object implicitly convertible to GenericString.
	[[nodiscard]] static StringFormatter<GenericString> Format(GenericStringView<const Char> format=nullptr)
	{return StringFormatter<GenericString>(format);}

	/// Convert all arguments to string and concatenate them.
	template<typename... Args> [[nodiscard]] static GenericString Concat(Args&&... args)
	{return StringFormatter<GenericString>(nullptr).Arg(Forward<Args>(args)...);}

	template<typename Arg> GenericString& operator<<(Arg&& value)
	{
		// TODO: potential code bloat, can it be reduced?
		const auto maxLen = MaxLengthOfToString(value);
		SetLengthUninitialized(Length() + maxLen);
		auto bufferRest = Tail(maxLen);
		ToString(bufferRest, INTRA_FWD(value));
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

	[[nodiscard]] constexpr bool IsHeapAllocated() const
	{
		return (u.buf[SsoBufferCapacityChars] & SsoLongBitInLastCharMask) != 0;
	}

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

	static constexpr Char SsoLongBitInLastCharMask = Config::TargetIsBigEndian? 1: 1 << (sizeof(Char)*8-1);

	enum: size_t
	{
		SsoLongBitMask = Config::TargetIsBigEndian? //Choosing a bit indicating a heap allocation. It must be in the last element of u.buf.
			1: //for big-endian we choose the lowest bit of m.Capacity.
			(size_t(1) << (sizeof(size_t)*8-1)), //for little-endian we choose the highest bit of m.Capacity.
		SsoCapacityMask = ~SsoLongBitMask,
		SsoCapacityRightShift = Config::TargetIsBigEndian, //There is no shift for little-endian, for big-endian we need to remove the lowest bit
		SsoShortSizeShift = Config::TargetIsBigEndian, //When string is in SSO, in the last element of u.buf we have the inverted string length.
			                            // However, for big-endian the lowest bit is occupied by the heap indicator, so we need to remove it with a right shift
		
		SsoBufferCapacityChars = sizeof(M)/sizeof(Char)-1, //In SSO we use all elements of the buffer except the last one, which is an inverted length, so that it becomes a \0 when buffer is full.
		SsoCapacityFieldForEmpty = Config::TargetIsBigEndian? //m.Capacity value, corresponding to an empty string. Used by the default constructor above.
			SsoBufferCapacityChars << 1:
			SsoBufferCapacityChars << (sizeof(index_t)-sizeof(Char))*8
	};

	constexpr void setShortLength(Index len) noexcept
	{u.buf[SsoBufferCapacityChars] = Char((SsoBufferCapacityChars - size_t(len)) << SsoShortSizeShift);}

	constexpr index_t getShortLength() const noexcept
	{return SsoBufferCapacityChars - (u.buf[SsoBufferCapacityChars] >> SsoShortSizeShift);}

	constexpr void setLongCapacity(index_t newCapacity) noexcept
	{u.m.Capacity = (size_t(newCapacity) << SsoCapacityRightShift)|SsoLongBitMask;}

	constexpr index_t getLongCapacity() const noexcept
	{return (u.m.Capacity & SsoCapacityMask) >> SsoCapacityRightShift;}

	constexpr void shortLenDecrement()
	{u.buf[SsoBufferCapacityChars] = Char(u.buf[SsoBufferCapacityChars] + (1 << SsoShortSizeShift));}

	constexpr void shortLenIncrement()
	{u.buf[SsoBufferCapacityChars] = Char(u.buf[SsoBufferCapacityChars] - (1 << SsoShortSizeShift));}

	constexpr bool emptyShort() const noexcept
	{return u.buf[SsoBufferCapacityChars] == Char(SsoBufferCapacityChars << SsoShortSizeShift);}

	constexpr void resetToEmptySsoWithoutFreeing() noexcept
	{u.m.Capacity = SsoBufferCapacityChars;}

	void freeLongData() noexcept
	{deallocate(u.m.Data, getLongCapacity()*sizeof(Char));}

	constexpr bool containsView(StringView rhs) const noexcept
	{return Data() <= rhs.Data() && rhs.Data() < End();}

	constexpr bool sameRange(StringView rhs) const noexcept
	{return Data() == rhs.Data() && Length() == rhs.Length();}

	static Char* allocate(size_t newCapacityInBytes)
	{return GlobalHeap.Allocate(newCapacityInBytes);}

	static void deallocate(Char* data, size_t oldCapacityInBytes)
	{GlobalHeap.Free(data, oldCapacityInBytes);}
};

template<typename Char> [[nodiscard]]
GenericString<Char> operator+(GenericString<Char>&& lhs, GenericStringView<const Char> rhs)
{return Move(lhs += rhs);}

template<typename R,
	typename Char = TListValue<R>
> [[nodiscard]] Requires<
	CArrayList<R> &&
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
static_assert(CGrowingList<String>);
static_assert(CDynamicArrayContainer<String>);
static_assert(CSame<TRangeValue<String>, char>);
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
	typename Char = TArrayListValue<S1>
> requires CArrayList<S1> && CArrayList<S2> &&
	(CHasData<S1> ||
		CHasData<S2> ||
		CRange<S1> ||
		CRange<S2> ||
		CArrayType<TRemoveReference<S1>> ||
		CArrayType<TRemoveReference<S2>>) && // To avoid conflicts with STL operator+
	CSame<Char, TArrayListValue<S2>>
GenericString<Char> operator+(S1&& lhs, S2&& rhs)
{
	return GenericStringView<const Char>(lhs) + GenericStringView<const Char>(rhs);
}

template<typename S1, typename S2,
	typename AsS1 = TRangeOfRef<S1>,
	typename AsS2 = TRangeOfRef<S2>,
	typename Char = TRangeValue<AsS1>
> requires
	(CRange<AsS1> || CRange<AsS2>) &&
	CConsumableRange<AsS1> &&
	CConsumableRange<AsS2> &&
	(!CArrayList<S1> || !CArrayList<S2> || !CSame<Char, TArrayListValue<AsS2>>)
GenericString<Char> operator+(S1&& lhs, S2&& rhs)
{
	GenericString<Char> result;
	result.Reserve(LengthOpt(lhs).GetOr(0) + LengthOpt(rhs).GetOr(0));
	ForwardAsRange<S1>(lhs)|CopyTo(LastAppender(result));
	ForwardAsRange<S2>(rhs)|CopyTo(LastAppender(result));
	return result;
}

template<typename R, class AsR = TRangeOfRef<R>, typename Char = TRangeValue<AsR>> requires
	CConsumableRange<AsR> &&
	(!CHas_data<R> || CRange<R>) && //To avoid conflicts with STL
	CChar<Char>
GenericString<Char> operator+(R&& lhs, Char rhs)
{
	GenericString<Char> result;
	result.Reserve(LengthOr(lhs) + 1);
	result += ForwardAsRange<R>(lhs);
	result += rhs;
	return result;
}

template<typename R, typename Char, typename Char2 = TListValue<R>> requires
	CConsumableList<R> &&
	(!CHas_data<R> || CRange<R>) && //To avoid conflicts with STL
	CChar<Char2> &&
	CChar<Char>
GenericString<Char2> operator+(Char lhs, R&& rhs)
{
	GenericString<Char2> result;
	result.Reserve(1 + LengthOr(rhs));
	result += lhs;
	result += ForwardAsRange<R>(rhs);
	return result;
}

#if INTRA_CONSTEXPR_TEST
static_assert(CConsumableList<String>);
static_assert(CConsumableList<const String>);
static_assert(CGrowingList<String>);
static_assert(CGrowingList<const String>);

//static_assert(CSame<TRangeOfRef<const String&>, StringView>);
#endif

} INTRA_END
