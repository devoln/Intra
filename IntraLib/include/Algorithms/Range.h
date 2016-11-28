﻿#pragma once

#include "CompilerSpecific/InitializerList.h"
#include "Algorithms/RangeConcept.h"
#include "Algorithms/Mixins/RangeMixins.h"


namespace Intra { namespace Range {

template<typename T> struct CountRange:
	RangeMixin<CountRange<T>, T, TypeEnum::Forward, false>
{
	typedef T value_type;
	typedef const T& return_value_type;

	CountRange(size_t counter=0): Counter(counter) {}
	bool Empty() {return false;}
	const T& First() const {static const T empty; return empty;}
	void PopFirst() {Counter++;}
	void Put(const T&) {Counter++;}

	bool operator==(const CountRange& rhs) const {return Counter==rhs.Counter;}

	size_t Counter;
};




template<typename R> struct ReverseRange:
	RangeMixin<ReverseRange<R>, typename R::value_type,
		(R::RangeType>=TypeEnum::Bidirectional)? R::RangeType: TypeEnum::Error,
		true>
{
	typedef typename R::value_type value_type;
	typedef typename R::return_value_type return_value_type;


	R OriginalRange;

	ReverseRange(null_t=null): OriginalRange(null) {}
	explicit ReverseRange(const R& range): OriginalRange(range) {}

	forceinline bool Empty() const {return OriginalRange.Empty();}
	forceinline return_value_type First() const {return OriginalRange.Last();}
	forceinline void PopFirst() {OriginalRange.PopLast();}
	forceinline return_value_type Last() const {return OriginalRange.First();}
	forceinline void PopLast() {OriginalRange.PopFirst();}
	
	template<typename U=R> forceinline Meta::EnableIf<
		IsFiniteRandomAccessRange<U>::_
	> operator[](size_t index) const {return OriginalRange[Length()-1-index];}

	bool operator==(const ReverseRange& rhs) const {return OriginalRange==rhs.OriginalRange;}

	template<typename U=R> forceinline Meta::EnableIf<
		HasLength<U>::_,
	size_t> Length() const {return OriginalRange.Length();}

	template<typename U=R> forceinline Meta::EnableIf<
		IsFiniteRandomAccessRange<U>::_,
	ReverseRange> opSlice(size_t first, size_t end) const
	{
		return ReverseRange(OriginalRange.opSlice(Length()-end, Length()-first));
	}

	forceinline const R& Retro() const {return OriginalRange;}
};



template<typename T> struct ArrayRange:
	RangeMixin<ArrayRange<T>, Meta::RemoveConst<T>, TypeEnum::Array, true>
{
	typedef T* iterator;
	typedef const T* const_iterator;

	typedef Meta::RemoveConst<T> value_type;
	typedef T& return_value_type;

	constexpr forceinline ArrayRange(null_t=null): Begin(null), End(null) {}

	constexpr forceinline ArrayRange(std::initializer_list<Meta::RemoveConst<T>> list):
		Begin(list.begin()), End(list.end()) {}

	template<size_t len> constexpr forceinline ArrayRange(T(&arr)[len]):
		Begin(arr), End(arr+len) {}

	forceinline ArrayRange(T* startPtr, T* endPtr):
		Begin(startPtr), End(endPtr) {INTRA_ASSERT(End >= Begin);}

	constexpr forceinline ArrayRange(T* startPtr, size_t length):
		Begin(startPtr), End(startPtr+length) {}

	constexpr forceinline ArrayRange(const ArrayRange& rhs):
		Begin(rhs.Begin), End(rhs.End) {}

	//template<typename U=T> constexpr forceinline ArrayRange(const Meta::EnableIf<Meta::IsConst<U>::_, ArrayRange<value_type>>& rhs):
		//Begin(rhs.Begin), End(rhs.End) {}

	forceinline ArrayRange<const T>& AsConstRange() {return *reinterpret_cast<ArrayRange<const T>*>(this);}
	forceinline constexpr const ArrayRange<const T>& AsConstRange() const {return *reinterpret_cast<const ArrayRange<const T>*>(this);}

	forceinline operator ArrayRange<const T>&() {return AsConstRange();}
	forceinline constexpr operator const ArrayRange<const T>&() const {return AsConstRange();}

	forceinline bool ContainsSubrange(const ArrayRange& subrange) const {return Begin<=subrange.Begin && End>=subrange.End;}

	template<typename U> bool ContainsAddress(const U* address) const
	{
		return reinterpret_cast<const T*>(address)>=Begin && reinterpret_cast<const T*>(address)<End;
	}

	bool Overlaps(ArrayRange<const T> rhs) const {return Begin<rhs.End && End>rhs.Begin && !Empty() && !rhs.Empty();}

	forceinline constexpr T* begin() const {return Begin;}
	forceinline constexpr T* end() const {return End;}

	forceinline constexpr size_t Length() const {return size_t(End-Begin);}
	forceinline constexpr bool Empty() const {return End==Begin;}
	forceinline constexpr T* Data() const {return Begin;}
	forceinline T& First() const {INTRA_ASSERT(!Empty()); return *Begin;}
	forceinline void PopFirst() {INTRA_ASSERT(!Empty()); Begin++;}
	forceinline T& Last() const {INTRA_ASSERT(!Empty()); return *(End-1);}
	forceinline void PopLast() {INTRA_ASSERT(!Empty()); End--;}

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
	bool operator==(const ArrayRange& rhs) const
	{
		return (Empty() && rhs.Empty()) ||
			(Begin==rhs.Begin && End==rhs.End);
	}

	forceinline bool operator==(null_t) const {return Empty();}
	forceinline bool operator!=(null_t) const {return !Empty();}

	ArrayRange& operator=(null_t) {Begin=End=null; return *this;}

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


	template<typename U> constexpr ArrayRange<U> Reinterpret() const
	{
		return {reinterpret_cast<U*>(Begin), reinterpret_cast<U*>(End)};
	}

	T* Begin;
	T* End;
};

static_assert(IsInputRange<ArrayRange<float>>::_, "Not input range???");
static_assert(IsForwardRange<ArrayRange<const float>>::_, "Not forward range???");
static_assert(IsBidirectionalRange<ReverseRange<ArrayRange<int>>>::_, "Not bidirectional range???");
static_assert(IsRandomAccessRange<ArrayRange<const uint>>::_, "Not random access range???");
static_assert(IsFiniteRandomAccessRange<ReverseRange<ArrayRange<float>>>::_, "Not finite random access range???");
static_assert(IsFiniteRandomAccessRange<ArrayRange<const int>>::_, "Not finite random access range???");
static_assert(IsRandomAccessRange<ArrayRange<float>>::_, "IsRandomAccessRange error.");
static_assert(HasLength<ArrayRange<float>>::_, "HasLength error.");
static_assert(IsFiniteRandomAccessRange<ArrayRange<float>>::_, "IsFiniteRandomAccessRange error.");
static_assert(IsArrayRange<ArrayRange<float>>::_, "IsArrayRange error.");
static_assert(IsArrayRange<ArrayRange<const char>>::_, "IsArrayRange error.");
static_assert(Meta::TypeEqualsIgnoreCV<const char, typename ArrayRange<char>::value_type>::_, "IsArrayRange error.");

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





template<typename T> struct ValueRange
{
	T Min, Max;
};

}

using Range::ArrayRange;
using Range::AsRange;

namespace Meta {

template<typename T> struct IsTriviallySerializable<ArrayRange<T>>: TypeFromValue<bool, false> {}; 

}

}

