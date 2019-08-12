#pragma once

#include "Core/Type.h"
#include "Core/CArray.h"
#include "Core/Range/Concepts.h"
#include "Core/Assert.h"

INTRA_CORE_RANGE_BEGIN
//! Non-owning reference to array.
template<typename T> struct Span
{
	typedef TRemoveConst<T> MutT;

	constexpr forceinline Span() = default;
	constexpr forceinline Span(null_t) noexcept {}

	constexpr forceinline Span(InitializerList<MutT> list) noexcept:
		Begin(list.begin()), End(list.end()) {}

	template<size_t N> constexpr forceinline Span(T(&arr)[N]) noexcept:
		Begin(arr), End(arr + N - (CChar<T>? 1: 0)) {}

	static constexpr forceinline Span FromPointerRange(T* startPtr, T* endPtr)
	{
		INTRA_PRECONDITION(endPtr >= startPtr);
		Span result;
		result.Begin = startPtr;
		result.End = endPtr;
		return result;
	}

	constexpr forceinline Span(T* startPtr, size_t length) noexcept:
		Begin(startPtr), End(startPtr+length) {}

	template<typename R, typename = Requires<
		CAssignableToArrayOf<R, T>
	>> constexpr forceinline Span(R&& rhs) noexcept: Span(DataOf(rhs), LengthOf(rhs)) {}

	INTRA_NODISCARD constexpr forceinline Span<const T> AsConstRange() const noexcept {return Span<const T>(Begin, End);}
	INTRA_NODISCARD constexpr forceinline operator Span<const T>() const noexcept {return AsConstRange();}

	INTRA_NODISCARD constexpr forceinline bool ContainsSubrange(const Span& subrange) const noexcept
	{return Begin <= subrange.Begin && End >= subrange.End;}

	template<typename U> INTRA_NODISCARD forceinline bool ContainsAddress(const U* address) const noexcept
	{return size_t(reinterpret_cast<const T*>(address) - Begin) < Length();}

	INTRA_NODISCARD constexpr forceinline bool Overlaps(Span<const T> rhs) const noexcept
	{
		return Begin < rhs.End && End > rhs.Begin &&
			!Empty() && !rhs.Empty();
	}

	INTRA_NODISCARD constexpr forceinline T* begin() const noexcept {return Begin;}
	INTRA_NODISCARD constexpr forceinline T* end() const noexcept {return End;}

	INTRA_NODISCARD constexpr forceinline index_t Length() const noexcept {return size_t(End - Begin);}
	INTRA_NODISCARD constexpr forceinline size_t SizeInBytes() const noexcept {return Length() * sizeof(T);}
	INTRA_NODISCARD constexpr forceinline bool Empty() const noexcept {return End == Begin;}
	INTRA_NODISCARD constexpr forceinline bool Full() const noexcept {return End == Begin;}
	INTRA_NODISCARD constexpr forceinline T* Data() const noexcept {return Begin;}

	INTRA_NODISCARD constexpr forceinline T& First() const {return INTRA_PRECONDITION(!Empty()), *Begin;}
	INTRA_CONSTEXPR2 forceinline void PopFirst() { INTRA_PRECONDITION(!Empty()); Begin++;}
	INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline T& Next() { INTRA_PRECONDITION(!Empty()); return *Begin++;}

	INTRA_NODISCARD constexpr forceinline T& Last() const {return INTRA_PRECONDITION(!Empty()), *(End-1);}
	INTRA_CONSTEXPR2 forceinline void PopLast() {INTRA_PRECONDITION(!Empty()); End--;}

	INTRA_CONSTEXPR2 forceinline size_t PopFirstN(size_t count) noexcept
	{
		if(count > Length()) count = Length();
		Begin += count;
		return count;
	}

	INTRA_CONSTEXPR2 forceinline void PopFirstExactly(size_t count)
	{
		INTRA_DEBUG_ASSERT(count <= Length());
		Begin += count;
	}

	INTRA_CONSTEXPR2 forceinline size_t PopLastN(size_t count) noexcept
	{
		if(count>Length()) count = Length();
		End -= count;
		return count;
	}

	INTRA_CONSTEXPR2 forceinline void PopLastExactly(size_t count)
	{
		INTRA_PRECONDITION(count <= Length());
		End -= count;
	}

	INTRA_NODISCARD constexpr forceinline Span Drop(size_t count=1) const noexcept
	{return Span(Length() <= count? End: Begin+count, End);}

	INTRA_NODISCARD constexpr forceinline Span DropLast(size_t count=1) const noexcept
	{return Span(Begin, Length() <= count? Begin: End-count);}

	INTRA_NODISCARD constexpr forceinline Span Take(size_t count) const noexcept
	{return Span(Begin, count >= Length()? End: Begin + count);}

	INTRA_CONSTEXPR2 forceinline Span TakeAdvance(size_t count) noexcept
	{
		const Span result = Take(count);
		Begin = result.End;
		return result;
	}

	/*!
	  @returns \p count elements from the beginning of this Span.
	  Requires count <= Length()
	*/
	INTRA_NODISCARD constexpr forceinline Span TakeExactly(index_t count) const
	{
		return INTRA_PRECONDITION(count <= Length()),
			Span(Begin, count);
	}

	//! @returns at most \p count elements from the end.
	INTRA_NODISCARD constexpr forceinline Span<T> Tail(size_t count) const noexcept
	{return Span(Length() < count? Begin: End-count, End);}

	INTRA_CONSTEXPR2 size_t ReadWrite(Span<MutT>& dst)
	{
		const size_t len = Length() < dst.Length()? Length(): dst.Length();
		for(size_t i = 0; i < len; i++) *dst.Begin++ = *Begin++;
		return len;
	}

	INTRA_CONSTEXPR2 size_t MoveAdvanceToAdvance(Span<MutT>& dst) noexcept
	{
		const size_t len = Length() < dst.Length()? Length(): dst.Length();
		for(size_t i = 0; i < len; i++) *dst.Begin++ = Move(*Begin++);
		return len;
	}

	INTRA_CONSTEXPR2 forceinline size_t CopyTo(Span<MutT> dst) const
	{return Span(*this).ReadWrite(dst);}

	INTRA_CONSTEXPR2 forceinline size_t MoveTo(Span<MutT> dst) const
	{return Span(*this).MoveAdvanceToAdvance(dst);}

	INTRA_CONSTEXPR2 forceinline size_t ReadTo(Span<MutT> dst)
	{return ReadWrite(dst);}

	INTRA_CONSTEXPR2 forceinline size_t MoveAdvanceTo(Span<MutT> dst)
	{return MoveAdvanceToAdvance(dst);}

	INTRA_CONSTEXPR2 forceinline size_t WriteTo(Span<MutT>& dst) const
	{return Span(*this).ReadWrite(dst);}

	INTRA_CONSTEXPR2 forceinline size_t MoveToAdvance(Span<MutT>& dst) const
	{return Span(*this).MoveAdvanceToAdvance(dst);}

	template<typename U=T> INTRA_CONSTEXPR2 forceinline Requires<
		CAssignable<T&, const U&>
	> Put(const T& v)
	{
		INTRA_PRECONDITION(!Empty());
		*Begin++ = v;
	}

	template<typename U=T> INTRA_CONSTEXPR2 forceinline Requires<
		CAssignable<T&, U&&>
	> Put(T&& v)
	{
		INTRA_PRECONDITION(!Empty());
		*Begin++ = Move(v);
	}

	template<typename U=T> INTRA_CONSTEXPR2 forceinline Requires<
		!CConst<U>,
	Span&> operator<<(const T& v)
	{
		if(!Empty()) *Begin++ = v;
		return *this;
	}

	template<typename U=T> INTRA_CONSTEXPR2 forceinline Requires<
		!CConst<U>,
	Span&> operator<<(T&& v) noexcept
	{
		if(!Empty()) *Begin++ = Move(v);
		return *this;
	}

	template<typename U=T> INTRA_CONSTEXPR2 forceinline Requires<
		!CConst<U>,
	Span&> operator<<(Span<const T> v) noexcept
	{
		v.ReadWrite(*this);
		return *this;
	}


	//! Сравниваются только указатели, но не содержимое.
	INTRA_NODISCARD constexpr forceinline bool operator==(const Span& rhs) const noexcept
	{
		return (Begin == rhs.Begin && End == rhs.End) ||
			(Empty() && rhs.Empty());
	}
	INTRA_NODISCARD constexpr forceinline bool operator!=(const Span& rhs) const noexcept {return !operator==(rhs);}

	INTRA_NODISCARD constexpr forceinline bool operator==(null_t) const noexcept {return Empty();}
	INTRA_NODISCARD constexpr forceinline bool operator!=(null_t) const noexcept {return !Empty();}

	INTRA_CONSTEXPR2 forceinline Span& operator=(null_t) noexcept
	{Begin = End = null; return *this;}

	INTRA_NODISCARD constexpr forceinline T& operator[](size_t index) const
	{
		return INTRA_PRECONDITION(index < Length()),
			Begin[index];
	}

	INTRA_NODISCARD constexpr forceinline T& Get(size_t index, T& defaultValue) const
	{return index < Length()? Begin[index]: defaultValue;}

	template<typename U=T> INTRA_NODISCARD forceinline Requires<
		!CConst<U>,
	const T&> Get(size_t index, const U& defaultValue) const
	{return index < Length()? Begin[index]: defaultValue;}

	INTRA_NODISCARD constexpr forceinline T Get(size_t index) const
	{return index < Length()? Begin[index]: T();}

	//! Make slice of Span.
	//! @returns slice containing elements with indices [firstIndex; endIndex).
	INTRA_NODISCARD constexpr forceinline Span operator()(size_t firstIndex, size_t endIndex) const
	{
		return INTRA_PRECONDITION(endIndex >= firstIndex && endIndex <= Length()),
			Span(Begin + firstIndex, Begin + endIndex);
	}

	INTRA_NODISCARD constexpr forceinline Span TakeNone() const noexcept {return {Begin, Begin};}

	//TODO: use Core/Range/Comparison.h
	template<typename U=T> INTRA_NODISCARD constexpr forceinline Requires<
		CArithmetic<U>,
	bool> StartsWith(Span<const T> str) const noexcept
	{return Length() >= str.Length() && Memory::BitsEqual(Data(), str.Data(), str.Length());}

	INTRA_NODISCARD INTRA_CONSTEXPR2 Span Find(const T& c) const
	{
		Span result = *this;
		while(!result.Empty() && result.First() != c)
			result.PopFirst();
		return result;
	}

	INTRA_NODISCARD INTRA_CONSTEXPR2 Span Find(Span<const T> arr) const
	{
		if(arr.Empty()) return *this;
		Span result = *this;
		while(!result.Empty() && !result.StartsWith(arr))
		{
			result.PopFirst();
			result = result.Find(arr.First());
		}
		return result;
	}

	INTRA_CONSTEXPR2 Span FindBefore(const T& v) const
	{
		Span result = {Begin, Begin};
		while(result.End != End && *result.End != v) result.End++;
		return result;
	}

	INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline bool Contains(const T& c) noexcept {return !Find(c).Empty();}
	INTRA_NODISCARD INTRA_CONSTEXPR2 forceinline bool Contains(Span<const T> arr) noexcept {return !Find(arr).Empty();}


	template<typename U> INTRA_NODISCARD forceinline Span<U> Reinterpret() const noexcept
	{
		typedef U* UPtr;
		return Span<U>(UPtr(Begin), UPtr(End));
	}

	T* Begin = null;
	T* End = null;
};

template<typename T> using CSpan = Span<const T>;

template<typename T, size_t N> constexpr forceinline Span<T> Take(T(&arr)[N], size_t n) noexcept {return {arr, n<N? n: N};}

template<typename T> constexpr forceinline Span<T> SpanOfPtr(T* arrPtr, size_t n) noexcept {return {arrPtr, n};}
template<typename T, size_t N> constexpr forceinline Span<T> SpanOfBuffer(T(&arr)[N]) noexcept {return {arr, N};}
template<typename T, size_t N> constexpr forceinline Span<T> SpanOf(T(&arr)[N]) noexcept {return Span<T>(arr);}
template<typename T, size_t N> constexpr forceinline CSpan<T> CSpanOf(const T(&arr)[N]) noexcept {return CSpan<T>(arr);}
template<typename T> constexpr forceinline CSpan<T> SpanOf(InitializerList<T> arr) noexcept {return arr;}
template<typename T> constexpr forceinline CSpan<T> CSpanOf(InitializerList<T> arr) noexcept {return arr;}

template<typename T=byte> forceinline Span<T> SpanOfRaw(void* data, size_t bytes) noexcept {return {static_cast<T*>(data), bytes/sizeof(T)};}
template<typename T=byte> forceinline CSpan<T> SpanOfRaw(const void* data, size_t bytes) noexcept {return {static_cast<const T*>(data), bytes/sizeof(T)};}
template<typename T=byte> forceinline CSpan<T> CSpanOfRaw(const void* data, size_t bytes) noexcept {return SpanOfRaw<T>(data, bytes);}

template<typename T> forceinline Span<T> SpanOfRawElements(void* data, size_t elements) noexcept {return {static_cast<T*>(data), elements};}
template<typename T> forceinline CSpan<T> SpanOfRawElements(const void* data, size_t elements) noexcept {return {static_cast<const T*>(data), elements};}
template<typename T> forceinline CSpan<T> CSpanOfRawElements(const void* data, size_t elements) noexcept {return SpanOfRawElements<T>(data, elements);}

template<typename R> constexpr forceinline Span<TArrayElementKeepConstRequired<R>> SpanOf(R&& r) noexcept
{return {DataOf(r), LengthOf(r)};}

template<typename R> constexpr forceinline CSpan<TArrayElementRequired<R>> CSpanOf(R&& r) noexcept
{return {DataOf(r), LengthOf(r)};}

template<typename R> constexpr forceinline Requires<
	!CInputRange<R> &&
	!CHasAsRangeMethod<R>,
Span<TArrayElementKeepConstRequired<R>>> RangeOf(R&& r) noexcept {return SpanOf(r);}

template<typename T, size_t N> INTRA_NODISCARD constexpr forceinline Span<T> RangeOf(T(&arr)[N]) noexcept {return Span<T>(arr);}


/** This class is supposed to be used as an output range or a stream.

  Unlike Span, it remembers its original position,
  which allows to request the range of all written elements.
  It also can be considered as an input range of not yet written elements.
*/
template<typename T> class SpanOutput: public Span<T>
{
	T* mBegin = null;
	
	// Don't directly modify base Span!
	using Span<T>::Begin;
	using Span<T>::End;
	using Span<T>::operator=;
	using Span<T>::operator==;
	using Span<T>::operator!=;
public:
	SpanOutput() = default;
	constexpr forceinline SpanOutput(null_t) noexcept {}
	SpanOutput(const SpanOutput&) = default;
	SpanOutput(SpanOutput&&) = default;

	template<typename R, typename = Requires<
		CArrayClassOfExactly<R, T>
	>> constexpr forceinline SpanOutput(R&& dst): Span(dst), mBegin(Data()) {}

	//! Reset this range to its original state to overwrite all elements written earlier.
	INTRA_CONSTEXPR2 forceinline void Reset() noexcept {Span::Begin = mBegin;}

	//! Get a range of all written data.
	INTRA_NODISCARD constexpr forceinline Span<T> WrittenRange() const noexcept {return {mBegin, Span::Begin};}

	//! @return Number of elements written after last Reset() or construction.
	INTRA_NODISCARD constexpr forceinline size_t Position() const noexcept {return index_t(Span::Begin - mBegin);}
	
	INTRA_NODISCARD constexpr forceinline bool operator==(null_t) const noexcept {return Empty();}
	INTRA_NODISCARD constexpr forceinline bool operator!=(null_t) const noexcept {return !Empty();}
};
INTRA_CORE_RANGE_END
