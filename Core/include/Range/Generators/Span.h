#pragma once

#include "Platform/CppFeatures.h"
#include "Platform/CppWarnings.h"
#include "Platform/Debug.h"
#include "Platform/InitializerList.h"
#include "Range/Concepts.h"
#include "Meta/Type.h"

namespace Intra {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Range {

template<typename T> struct Span
{
	constexpr forceinline Span(null_t=null) noexcept:
		Begin(null), End(null) {}

	constexpr forceinline Span(InitializerList<Meta::RemoveConst<T>> list) noexcept:
		Begin(list.begin()), End(list.end()) {}

	template<size_t N> constexpr forceinline Span(T(&arr)[N]) noexcept:
		Begin(arr), End(arr+N-Meta::IsCharType<T>::_) {}

	template<size_t N> static constexpr forceinline Span FromBuffer(T(&arr)[N]) noexcept {return {arr, N};}

	forceinline Span(T* startPtr, T* endPtr):
		Begin(startPtr), End(endPtr) {INTRA_DEBUG_ASSERT(End >= Begin);}

	constexpr forceinline Span(T* startPtr, size_t length) noexcept:
		Begin(startPtr), End(startPtr+length) {}

	constexpr forceinline Span(const Span& rhs) noexcept:
		Begin(rhs.Begin), End(rhs.End) {}

	template<typename R, typename=Meta::EnableIf<
		IsArrayClass<R>::_ &&
		(Meta::TypeEqualsIgnoreRef<ReturnValueTypeOfArray<R>, T>::_ ||
			Meta::TypeEqualsIgnoreRef<ReturnValueTypeOfArray<R>, Meta::RemoveConst<T>>::_)
	>> forceinline Span(R&& rhs) noexcept: Span(rhs.Data(), rhs.Length()) {}

	constexpr forceinline Span<const T> AsConstRange() const noexcept {return CSpan<T>(Begin, End);}
	constexpr forceinline operator Span<const T>() const noexcept {return AsConstRange();}

	forceinline constexpr bool ContainsSubrange(const Span& subrange) const noexcept
	{return Begin<=subrange.Begin && End>=subrange.End;}

	template<typename U> forceinline constexpr bool ContainsAddress(const U* address) const noexcept
	{return size_t(reinterpret_cast<const T*>(address)-Begin) <= Length();}

	forceinline constexpr bool Overlaps(Span<const T> rhs) const noexcept
	{
		return Begin<rhs.End && End>rhs.Begin &&
			!Empty() && !rhs.Empty();
	}

	forceinline constexpr T* begin() const noexcept {return Begin;}
	forceinline constexpr T* end() const noexcept {return End;}

	forceinline size_t Length() const noexcept {return size_t(End-Begin);}
	forceinline constexpr bool Empty() const noexcept {return End <= Begin;}
	forceinline constexpr T* Data() const noexcept {return Begin;}

	forceinline T& First() const {INTRA_DEBUG_ASSERT(!Empty()); return *Begin;}
	forceinline void PopFirst() {INTRA_DEBUG_ASSERT(!Empty()); Begin++;}
	forceinline T& Next() {INTRA_DEBUG_ASSERT(!Empty()); return *Begin++;}

	forceinline T& Last() const {INTRA_DEBUG_ASSERT(!Empty()); return *(End-1);}
	forceinline void PopLast() {INTRA_DEBUG_ASSERT(!Empty()); End--;}

	forceinline size_t PopFirstN(size_t count) noexcept
	{
		if(count>Length()) count = Length();
		Begin += count;
		return count;
	}

	forceinline void PopFirstExactly(size_t count)
	{
		INTRA_DEBUG_ASSERT(count <= Length());
		Begin += count;
	}

	forceinline size_t PopLastN(size_t count) noexcept
	{
		if(count>Length()) count = Length();
		End -= count;
		return count;
	}

	forceinline void PopLastExactly(size_t count)
	{
		INTRA_DEBUG_ASSERT(count <= Length());
		End -= count;
	}

	forceinline Span<T> Drop(size_t count=1) const noexcept
	{return Span(Length()<=count? End: Begin+count, End);}

	forceinline Span<T> DropBack(size_t count=1) const noexcept
	{return Span(Begin, Length()<=count? Begin: End-count);}

	forceinline Span<T> Take(size_t count) const noexcept
	{return Span(Begin, count>=Length()? End: Begin+count);}

	forceinline Span<T> TakeExactly(size_t count) const
	{
		INTRA_DEBUG_ASSERT(count <= Length());
		return Span(Begin, Begin+count);
	}

	forceinline Span<T> Tail(size_t count) const noexcept
	{return Span(Length()<count? Begin: End-count, End);}


	template<typename U=T> forceinline Meta::EnableIf<
		!Meta::IsConst<U>::_
	> Put(const T& v)
	{
		INTRA_DEBUG_ASSERT(!Empty());
		*Begin++ = v;
	}

	template<typename U=T> Meta::EnableIf<
		!Meta::IsConst<U>::_
	> Put(T&& v)
	{
		INTRA_DEBUG_ASSERT(!Empty());
		*Begin++ = Meta::Move(v);
	}


	//! Сравниваются только указатели, но не содержимое.
	forceinline constexpr bool operator==(const Span& rhs) const noexcept
	{
		return  (Begin==rhs.Begin && End==rhs.End) || (Empty() && rhs.Empty());
	}

	forceinline constexpr bool operator==(null_t) const noexcept {return Empty();}
	forceinline constexpr bool operator!=(null_t) const noexcept {return !Empty();}

	forceinline Span& operator=(null_t) noexcept
	{Begin = End = null; return *this;}

	forceinline T& operator[](size_t index) const
	{
		INTRA_DEBUG_ASSERT(index < Length());
		return Begin[index];
	}

	forceinline Span operator()(size_t firstIndex, size_t endIndex) const
	{
		INTRA_DEBUG_ASSERT(endIndex >= firstIndex && endIndex <= Length());
		return Span(Begin+firstIndex, Begin+endIndex);
	}

	constexpr forceinline Span TakeNone() const noexcept {return {Begin, Begin};}


	template<typename U> constexpr forceinline Span<U> Reinterpret() const noexcept
	{
		typedef U* UPtr;
		return Span<U>(UPtr(Begin), UPtr(End));
	}

	T* Begin;
	T* End;
};

template<typename T> using CSpan = Span<const T>;

static_assert(IsInputRange<Span<float>>::_, "Not input range???");
static_assert(IsForwardRange<Span<float>>::_, "Not forward range???");
static_assert(IsInputRange<CSpan<int>>::_, "Not input range???");
static_assert(IsForwardRange<CSpan<int>>::_, "Not forward range???");
static_assert(IsRandomAccessRange<CSpan<uint>>::_, "Not random access range???");
static_assert(IsFiniteRandomAccessRange<CSpan<int>>::_, "Not finite random access range???");
static_assert(IsRandomAccessRange<Span<float>>::_, "IsRandomAccessRange error.");
static_assert(HasLength<Span<float>>::_, "HasLength error.");
static_assert(IsFiniteRandomAccessRange<Span<float>>::_, "IsFiniteRandomAccessRange error.");
static_assert(Meta::TypeEquals<ValueTypeOf<Span<float>>, float>::_, "IsArrayRange error.");
static_assert(IsArrayRange<Span<float>>::_, "IsArrayRange error.");
static_assert(IsArrayRange<CSpan<char>>::_, "IsArrayRange error.");

template<typename T, size_t N> forceinline constexpr Span<T> AsRange(T(&arr)[N]) noexcept {return Span<T>(arr);}
template<typename T> forceinline constexpr CSpan<T> AsRange(InitializerList<T> arr) noexcept {return CSpan<T>(arr);}
template<typename T> forceinline constexpr Span<T> Take(T* arrPtr, size_t n) noexcept {return Span<T>(arrPtr, n);}

}

using Range::Span;
using Range::CSpan;

INTRA_WARNING_POP

}
