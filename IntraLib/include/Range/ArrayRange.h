#pragma once

#include "CompilerSpecific/InitializerList.h"
#include "Mixins/RangeMixins.h"

namespace Intra { namespace Range {

template<typename T> struct ArrayRange:
	RangeMixin<ArrayRange<T>, Meta::RemoveConst<T>, TypeEnum::Array, true>
{
	typedef T* iterator;
	typedef const T* const_iterator;

	typedef Meta::RemoveConst<T> value_type;
	typedef T& return_value_type;

	constexpr forceinline ArrayRange(null_t=null):
		Begin(null), End(null) {}

	constexpr forceinline ArrayRange(std::initializer_list<value_type> list):
		Begin(list.begin()), End(list.end()) {}

	template<size_t len> constexpr forceinline ArrayRange(T(&arr)[len]):
		Begin(arr), End(arr+len) {}

	forceinline ArrayRange(T* startPtr, T* endPtr):
		Begin(startPtr), End(endPtr) {INTRA_ASSERT(End >= Begin);}

	constexpr forceinline ArrayRange(T* startPtr, size_t length):
		Begin(startPtr), End(startPtr+length) {}

	constexpr forceinline ArrayRange(const ArrayRange& rhs):
		Begin(rhs.Begin), End(rhs.End) {}

	forceinline constexpr ArrayRange<const T> AsConstRange() const {return ArrayRange<const T>(Begin, End);}
	forceinline constexpr operator ArrayRange<const T>() const {return AsConstRange();}

	forceinline bool ContainsSubrange(const ArrayRange& subrange) const {return Begin<=subrange.Begin && End>=subrange.End;}

	template<typename U> forceinline bool ContainsAddress(const U* address) const
	{
		return size_t(reinterpret_cast<const T*>(address)-Begin) <= Length();
	}

	forceinline bool Overlaps(ArrayRange<const T> rhs) const
	{
		return Begin<rhs.End && End>rhs.Begin &&
			!Empty() && !rhs.Empty();
	}

	forceinline constexpr T* begin() const {return Begin;}
	forceinline constexpr T* end() const {return End;}

	forceinline constexpr size_t Length() const {return size_t(End-Begin);}
	forceinline constexpr bool Empty() const {return End==Begin;}
	forceinline constexpr T* Data() const {return Begin;}
	forceinline T& First() const {INTRA_ASSERT(!Empty()); return *Begin;}
	forceinline void PopFirst() {INTRA_ASSERT(!Empty()); Begin++;}
	forceinline T& Last() const {INTRA_ASSERT(!Empty()); return *(End-1);}
	forceinline void PopLast() {INTRA_ASSERT(!Empty()); End--;}

	forceinline void PopBackN(size_t count) {Begin+=count; if(Begin>End) Begin=End;}
	forceinline void PopBackExactly(size_t count) {Begin+=count;}

	forceinline ArrayRange<T> Drop(size_t count=1) const {return ArrayRange(Begin+count>End? End: Begin+count, End);}
	forceinline ArrayRange<T> Take(size_t count) const {return ArrayRange(Begin, Begin+count>End? End: Begin+count);}
	forceinline ArrayRange<T> Tail(size_t count) const {return ArrayRange(End-count<Begin? Begin: End-count, End);}

	template<typename U=T> forceinline Meta::EnableIf<
		Meta::TypeEquals<U, value_type>::_
	> Put(const T& v)
	{
		INTRA_ASSERT(!Empty());
		*Begin++ = v;
	}

	template<typename U=T> forceinline Meta::EnableIf<
		Meta::TypeEquals<U, value_type>::_
	> Put(T&& v)
	{
		INTRA_ASSERT(!Empty());
		*Begin++ = core::move(v);
	}


	//! Сравниваются только указатели, но не содержимое.
	forceinline bool operator==(const ArrayRange& rhs) const
	{
		return (Empty() && rhs.Empty()) ||
			(Begin==rhs.Begin && End==rhs.End);
	}

	forceinline bool operator==(null_t) const {return Empty();}
	forceinline bool operator!=(null_t) const {return !Empty();}

	forceinline ArrayRange& operator=(null_t) {Begin=End=null; return *this;}

	forceinline T& operator[](size_t index) const
	{
		INTRA_ASSERT(index<Length());
		return Begin[index];
	}

	forceinline ArrayRange opSlice(size_t firstIndex, size_t endIndex) const
	{
		INTRA_ASSERT(endIndex>=firstIndex && Begin+endIndex<=End);
		return ArrayRange(Begin+firstIndex, Begin+endIndex);
	}

	forceinline constexpr ArrayRange TakeNone() const {return {Begin, Begin};}


	template<typename U> constexpr forceinline ArrayRange<U> Reinterpret() const
	{
		return ArrayRange<U>(reinterpret_cast<U*>(Begin), reinterpret_cast<U*>(End));
	}

	T* Begin;
	T* End;
};

static_assert(IsInputRange<ArrayRange<float>>::_, "Not input range???");
static_assert(IsForwardRange<ArrayRange<const float>>::_, "Not forward range???");
static_assert(IsRandomAccessRange<ArrayRange<const uint>>::_, "Not random access range???");
static_assert(IsFiniteRandomAccessRange<ArrayRange<const int>>::_, "Not finite random access range???");
static_assert(IsRandomAccessRange<ArrayRange<float>>::_, "IsRandomAccessRange error.");
static_assert(HasLength<ArrayRange<float>>::_, "HasLength error.");
static_assert(IsFiniteRandomAccessRange<ArrayRange<float>>::_, "IsFiniteRandomAccessRange error.");
static_assert(IsArrayRange<ArrayRange<float>>::_, "IsArrayRange error.");
static_assert(IsArrayRange<ArrayRange<const char>>::_, "IsArrayRange error.");

template<typename T, size_t N> Meta::EnableIf<
	!Meta::IsCharType<T>::_,
ArrayRange<T>> AsRange(T(&arr)[N]) {return ArrayRange<T>(arr);}

template<typename T, size_t N> Meta::EnableIf<
	!Meta::IsCharType<T>::_,
ArrayRange<const T>> AsRange(const T(&arr)[N]) {return ArrayRange<const T>(arr);}


template<typename T> ArrayRange<const T> AsRange(std::initializer_list<T> arr) {return ArrayRange<const T>(arr);}


template<typename T, size_t N> Meta::EnableIf<
	!Meta::IsCharType<T>::_,
ArrayRange<T>> AsConstRange(T(&arr)[N]) {return ArrayRange<const T>(arr);}

template<typename T> ArrayRange<const T> AsConstRange(std::initializer_list<T> arr) {return ArrayRange<const T>(arr);}

}

using Range::ArrayRange;
using Range::AsRange;

namespace Meta {

template<typename T> struct IsTriviallySerializable<ArrayRange<T>>: TypeFromValue<bool, false> {}; 

}

}

