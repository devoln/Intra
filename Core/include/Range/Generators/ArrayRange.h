#pragma once

#include "Platform/CppFeatures.h"
#include "Platform/CppWarnings.h"
#include "Platform/Debug.h"
#include "Platform/InitializerList.h"
#include "Range/Concepts.h"

namespace Intra {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Range {

template<typename T> struct ArrayRange
{
	constexpr forceinline ArrayRange(null_t=null):
		Begin(null), End(null) {}

	constexpr forceinline ArrayRange(InitializerList<Meta::RemoveConst<T>> list):
		Begin(list.begin()), End(list.end()) {}

	template<size_t N> forceinline ArrayRange(T(&arr)[N]):
		Begin(arr), End(arr+N) {}

	forceinline ArrayRange(T* startPtr, T* endPtr):
		Begin(startPtr), End(endPtr) {INTRA_ASSERT(End >= Begin);}

	constexpr forceinline ArrayRange(T* startPtr, size_t length):
		Begin(startPtr), End(startPtr+length) {}

	constexpr forceinline ArrayRange(const ArrayRange& rhs):
		Begin(rhs.Begin), End(rhs.End) {}

	forceinline constexpr ArrayRange<const T> AsConstRange() const {return ArrayRange<const T>(Begin, End);}
	forceinline constexpr operator ArrayRange<const T>() const {return AsConstRange();}

	forceinline bool ContainsSubrange(const ArrayRange& subrange) const
	{return Begin<=subrange.Begin && End>=subrange.End;}

	template<typename U> forceinline bool ContainsAddress(const U* address) const
	{return size_t(reinterpret_cast<const T*>(address)-Begin) <= Length();}

	forceinline bool Overlaps(ArrayRange<const T> rhs) const
	{
		return Begin<rhs.End && End>rhs.Begin &&
			!Empty() && !rhs.Empty();
	}

	forceinline constexpr T* begin() const {return Begin;}
	forceinline constexpr T* end() const {return End;}

	forceinline size_t Length() const {return size_t(End-Begin);}
	forceinline constexpr bool Empty() const {return End<=Begin;}
	forceinline constexpr T* Data() const {return Begin;}
	forceinline T& First() const {INTRA_ASSERT(!Empty()); return *Begin;}
	
	forceinline void PopFirst() {INTRA_ASSERT(!Empty()); Begin++;}

	forceinline T& Last() const {INTRA_ASSERT(!Empty()); return *(End-1);}
	forceinline void PopLast() {INTRA_ASSERT(!Empty()); End--;}

	forceinline size_t PopFirstN(size_t count)
	{
		if(count>Length()) count = Length();
		Begin += count;
		return count;
	}

	forceinline void PopFirstExactly(size_t count)
	{INTRA_ASSERT(count <= Length()); Begin += count;}

	forceinline size_t PopBackN(size_t count)
	{
		if(count>Length()) count = Length();
		End -= count;
		return count;
	}

	forceinline void PopBackExactly(size_t count)
	{INTRA_ASSERT(count <= Length()); End -= count;}

	forceinline ArrayRange<T> Drop(size_t count=1) const
	{return ArrayRange(Length()<=count? End: Begin+count, End);}

	forceinline ArrayRange<T> DropBack(size_t count=1) const
	{return ArrayRange(Begin, Length()<=count? Begin: End-count);}

	forceinline ArrayRange<T> Take(size_t count) const
	{return ArrayRange(Begin, count>=Length()? End: Begin+count);}

	forceinline ArrayRange<T> TakeExactly(size_t count) const
	{
		INTRA_ASSERT(count <= Length());
		return ArrayRange(Begin, Begin+count);
	}

	forceinline ArrayRange<T> Tail(size_t count) const
	{return ArrayRange(Length()<count? Begin: End-count, End);}


	void Put(const T& v)
	{
		INTRA_ASSERT(!Empty());
		*Begin++ = v;
	}

	void Put(T&& v)
	{
		INTRA_ASSERT(!Empty());
		*Begin++ = Meta::Move(v);
	}


	//! Сравниваются только указатели, но не содержимое.
	forceinline bool operator==(const ArrayRange& rhs) const
	{
		return (Empty() && rhs.Empty()) ||
			(Begin==rhs.Begin && End==rhs.End);
	}

	forceinline bool operator==(null_t) const {return Empty();}
	forceinline bool operator!=(null_t) const {return !Empty();}

	forceinline ArrayRange& operator=(null_t)
	{Begin=End=null; return *this;}

	forceinline T& operator[](size_t index) const
	{
		INTRA_ASSERT(index<Length());
		return Begin[index];
	}

	forceinline ArrayRange operator()(size_t firstIndex, size_t endIndex) const
	{
		INTRA_ASSERT(endIndex>=firstIndex && Begin+endIndex<=End);
		return ArrayRange(Begin+firstIndex, Begin+endIndex);
	}

	forceinline constexpr ArrayRange TakeNone() const {return {Begin, Begin};}


	template<typename U> constexpr forceinline ArrayRange<U> Reinterpret() const
	{
		typedef U* UPtr;
		return ArrayRange<U>(UPtr(Begin), UPtr(End));
	}

	T* Begin;
	T* End;
};

static_assert(IsInputRange<ArrayRange<float>>::_, "Not input range???");
static_assert(IsForwardRange<ArrayRange<float>>::_, "Not forward range???");
static_assert(IsInputRange<ArrayRange<const int>>::_, "Not input range???");
static_assert(IsForwardRange<ArrayRange<const int>>::_, "Not forward range???");
static_assert(IsRandomAccessRange<ArrayRange<const uint>>::_, "Not random access range???");
static_assert(IsFiniteRandomAccessRange<ArrayRange<const int>>::_, "Not finite random access range???");
static_assert(IsRandomAccessRange<ArrayRange<float>>::_, "IsRandomAccessRange error.");
static_assert(HasLength<ArrayRange<float>>::_, "HasLength error.");
static_assert(IsFiniteRandomAccessRange<ArrayRange<float>>::_, "IsFiniteRandomAccessRange error.");
static_assert(Meta::TypeEquals<ValueTypeOf<ArrayRange<float>>, float>::_, "IsArrayRange error.");
static_assert(IsArrayRange<ArrayRange<float>>::_, "IsArrayRange error.");
static_assert(IsArrayRange<ArrayRange<const char>>::_, "IsArrayRange error.");

template<typename T, size_t N> forceinline Meta::EnableIf<
	!Meta::IsCharType<T>::_,
ArrayRange<T>> AsRange(T(&arr)[N]) {return ArrayRange<T>(arr);}


template<typename T> forceinline ArrayRange<const T> AsRange(InitializerList<T> arr)
{return ArrayRange<const T>(arr);}

}

using Range::ArrayRange;

INTRA_WARNING_POP

}
