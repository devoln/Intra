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
	constexpr forceinline Span(null_t=null):
		Begin(null), End(null) {}

	constexpr forceinline Span(InitializerList<Meta::RemoveConst<T>> list):
		Begin(list.begin()), End(list.end()) {}

	template<size_t N> forceinline Span(T(&arr)[N]):
		Begin(arr), End(arr+N) {}

	forceinline Span(T* startPtr, T* endPtr):
		Begin(startPtr), End(endPtr) {INTRA_DEBUG_ASSERT(End >= Begin);}

	constexpr forceinline Span(T* startPtr, size_t length):
		Begin(startPtr), End(startPtr+length) {}

	constexpr forceinline Span(const Span& rhs):
		Begin(rhs.Begin), End(rhs.End) {}

	template<typename R, typename=Meta::EnableIf<
		IsArrayClass<R>::_ && Meta::TypeEqualsIgnoreCV<ValueTypeOfArray<R>, T>::_
	>> forceinline Span(R&& rhs):
		Span(rhs.Data(), rhs.Length()) {}

	forceinline constexpr CSpan<T> AsConstRange() const {return CSpan<T>(Begin, End);}
	forceinline constexpr operator CSpan<T>() const {return AsConstRange();}

	forceinline bool ContainsSubrange(const Span& subrange) const
	{return Begin<=subrange.Begin && End>=subrange.End;}

	template<typename U> forceinline bool ContainsAddress(const U* address) const
	{return size_t(reinterpret_cast<const T*>(address)-Begin) <= Length();}

	forceinline bool Overlaps(CSpan<T> rhs) const
	{
		return Begin<rhs.End && End>rhs.Begin &&
			!Empty() && !rhs.Empty();
	}

	forceinline constexpr T* begin() const {return Begin;}
	forceinline constexpr T* end() const {return End;}

	forceinline size_t Length() const {return size_t(End-Begin);}
	forceinline constexpr bool Empty() const {return End<=Begin;}
	forceinline constexpr T* Data() const {return Begin;}

	forceinline T& First() const {INTRA_DEBUG_ASSERT(!Empty()); return *Begin;}
	forceinline void PopFirst() {INTRA_DEBUG_ASSERT(!Empty()); Begin++;}

	forceinline T& Last() const {INTRA_DEBUG_ASSERT(!Empty()); return *(End-1);}
	forceinline void PopLast() {INTRA_DEBUG_ASSERT(!Empty()); End--;}

	forceinline size_t PopFirstN(size_t count)
	{
		if(count>Length()) count = Length();
		Begin += count;
		return count;
	}

	forceinline void PopFirstExactly(size_t count)
	{INTRA_DEBUG_ASSERT(count <= Length()); Begin += count;}

	forceinline size_t PopLastN(size_t count)
	{
		if(count>Length()) count = Length();
		End -= count;
		return count;
	}

	forceinline void PopLastExactly(size_t count)
	{INTRA_DEBUG_ASSERT(count <= Length()); End -= count;}

	forceinline Span<T> Drop(size_t count=1) const
	{return Span(Length()<=count? End: Begin+count, End);}

	forceinline Span<T> DropBack(size_t count=1) const
	{return Span(Begin, Length()<=count? Begin: End-count);}

	forceinline Span<T> Take(size_t count) const
	{return Span(Begin, count>=Length()? End: Begin+count);}

	forceinline Span<T> TakeExactly(size_t count) const
	{
		INTRA_DEBUG_ASSERT(count <= Length());
		return Span(Begin, Begin+count);
	}

	forceinline Span<T> Tail(size_t count) const
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
	forceinline bool operator==(const Span& rhs) const
	{
		return (Empty() && rhs.Empty()) ||
			(Begin==rhs.Begin && End==rhs.End);
	}

	forceinline bool operator==(null_t) const {return Empty();}
	forceinline bool operator!=(null_t) const {return !Empty();}

	forceinline Span& operator=(null_t)
	{Begin=End=null; return *this;}

	forceinline T& operator[](size_t index) const
	{
		INTRA_DEBUG_ASSERT(index<Length());
		return Begin[index];
	}

	forceinline Span operator()(size_t firstIndex, size_t endIndex) const
	{
		INTRA_DEBUG_ASSERT(endIndex>=firstIndex && Begin+endIndex<=End);
		return Span(Begin+firstIndex, Begin+endIndex);
	}

	forceinline constexpr Span TakeNone() const {return {Begin, Begin};}


	template<typename U> constexpr forceinline Span<U> Reinterpret() const
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

template<typename T, size_t N> forceinline Meta::EnableIf<
	!Meta::IsCharType<T>::_,
Span<T>> AsRange(T(&arr)[N]) {return Span<T>(arr);}


template<typename T> forceinline CSpan<T> AsRange(InitializerList<T> arr)
{return CSpan<T>(arr);}

template<typename T> forceinline Span<T> Take(T* arrPtr, size_t n) {return Span<T>(arrPtr, n);}

}

using Range::Span;
using Range::CSpan;

INTRA_WARNING_POP

}
