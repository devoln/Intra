#pragma once

#include "Cpp/Features.h"
#include "Cpp/Warnings.h"
#include "Cpp/Intrinsics.h"
#include "Cpp/InitializerList.h"

#include "Meta/Type.h"

#include "Debug.h"

#ifndef INTRA_UTILS_NO_CONCEPTS
#include "Concepts/Array.h"
#include "Concepts/Range.h"
#include "Concepts/RangeOf.h"
#endif

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra {

//! Определяется в Range, чтобы работал ADL с алгоритмами, определёнными в Range.
namespace Range {

//! Невладеющая ссылка на массив.
template<typename T> struct Span
{
	typedef Meta::RemoveConst<T> MutT;

	constexpr forceinline Span(null_t=null) noexcept:
		Begin(null), End(null) {}

	constexpr forceinline Span(InitializerList<MutT> list) noexcept:
		Begin(list.begin()), End(list.end()) {}

	template<size_t N> constexpr forceinline Span(T(&arr)[N]) noexcept:
		Begin(arr), End(arr + N - Meta::IsCharType<T>::_) {}

	forceinline Span(T* startPtr, T* endPtr):
		Begin(startPtr), End(endPtr) {INTRA_DEBUG_ASSERT(End >= Begin);}

	constexpr forceinline Span(T* startPtr, size_t length) noexcept:
		Begin(startPtr), End(startPtr+length) {}

	constexpr forceinline Span(const Span& rhs) noexcept:
		Begin(rhs.Begin), End(rhs.End) {}

	template<typename R, typename=Meta::EnableIf<
		Concepts::IsAssignableToArrayOf<R, T>::_
	>> forceinline Span(R&& rhs) noexcept: Span(Concepts::DataOf(rhs), Concepts::LengthOf(rhs)) {}

	constexpr forceinline Span<const T> AsConstRange() const noexcept {return Span<const T>(Begin, End);}
	constexpr forceinline operator Span<const T>() const noexcept {return AsConstRange();}

	forceinline constexpr bool ContainsSubrange(const Span& subrange) const noexcept
	{return Begin <= subrange.Begin && End >= subrange.End;}

	template<typename U> forceinline constexpr bool ContainsAddress(const U* address) const noexcept
	{return size_t(reinterpret_cast<const T*>(address) - Begin) <= Length();}

	forceinline constexpr bool Overlaps(Span<const T> rhs) const noexcept
	{
		return Begin < rhs.End && End > rhs.Begin &&
			!Empty() && !rhs.Empty();
	}

	forceinline constexpr T* begin() const noexcept {return Begin;}
	forceinline constexpr T* end() const noexcept {return End;}

	forceinline size_t Length() const noexcept {return size_t(End - Begin);}
	forceinline constexpr bool Empty() const noexcept {return End <= Begin;}
	forceinline constexpr bool Full() const noexcept {return End <= Begin;}
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
	{return Span(Length() <= count? End: Begin+count, End);}

	forceinline Span<T> DropLast(size_t count=1) const noexcept
	{return Span(Begin, Length() <= count? Begin: End-count);}

	forceinline Span Take(size_t count) const noexcept
	{return Span(Begin, count >= Length()? End: Begin + count);}

	forceinline Span TakeAdvance(size_t count) noexcept
	{
		const Span result = Take(count);
		Begin = result.End;
		return result;
	}

	forceinline Span TakeExactly(size_t count) const
	{
		INTRA_DEBUG_ASSERT(count <= Length());
		return Span(Begin, Begin+count);
	}

	forceinline Span<T> Tail(size_t count) const noexcept
	{return Span(Length()<count? Begin: End-count, End);}

	size_t ReadWrite(Span<MutT>& dst)
	{
		const size_t len = Length()<dst.Length()? Length(): dst.Length();
		for(size_t i=0; i<len; i++) *dst.Begin++ = *Begin++;
		return len;
	}

	size_t MoveAdvanceToAdvance(Span<MutT>& dst) noexcept
	{
		const size_t len = Length() < dst.Length()? Length(): dst.Length();
		for(size_t i=0; i<len; i++) *dst.Begin++ = Cpp::Move(*Begin++);
		return len;
	}

	forceinline size_t CopyTo(Span<MutT> dst) const
	{return Span(*this).ReadWrite(dst);}

	forceinline size_t MoveTo(Span<MutT> dst) const
	{return Span(*this).MoveAdvanceToAdvance(dst);}

	forceinline size_t ReadTo(Span<MutT> dst)
	{return ReadWrite(dst);}

	forceinline size_t MoveAdvanceTo(Span<MutT> dst)
	{return MoveAdvanceToAdvance(dst);}

	forceinline size_t WriteTo(Span<MutT>& dst) const
	{return Span(*this).ReadWrite(dst);}

	forceinline size_t MoveToAdvance(Span<MutT>& dst) const
	{return Span(*this).MoveAdvanceToAdvance(dst);}

	template<typename U=T> forceinline Meta::EnableIf<
		!Meta::IsConst<U>::_
	> Put(const T& v)
	{
		INTRA_DEBUG_ASSERT(!Empty());
		*Begin++ = v;
	}

	template<typename U=T> forceinline Meta::EnableIf<
		!Meta::IsConst<U>::_
	> Put(T&& v)
	{
		INTRA_DEBUG_ASSERT(!Empty());
		*Begin++ = Cpp::Move(v);
	}

	template<typename U=T> forceinline Meta::EnableIf<
		!Meta::IsConst<U>::_,
	Span&> operator<<(const T& v)
	{
		if(!Empty()) *Begin++ = v;
		return *this;
	}

	template<typename U=T> forceinline Meta::EnableIf<
		!Meta::IsConst<U>::_,
	Span&> operator<<(T&& v) noexcept
	{
		if(!Empty()) *Begin++ = Cpp::Move(v);
		return *this;
	}

	template<typename U=T> forceinline Meta::EnableIf<
		!Meta::IsConst<U>::_,
	Span&> operator<<(Span<const T> v) noexcept
	{
		v.ReadWrite(*this);
		return *this;
	}


	//! Сравниваются только указатели, но не содержимое.
	forceinline constexpr bool operator==(const Span& rhs) const noexcept
	{
		return (Begin == rhs.Begin && End == rhs.End) ||
			(Empty() && rhs.Empty());
	}
	forceinline constexpr bool operator!=(const Span& rhs) const noexcept {return !operator==(rhs);}

	forceinline constexpr bool operator==(null_t) const noexcept {return Empty();}
	forceinline constexpr bool operator!=(null_t) const noexcept {return !Empty();}

	forceinline Span& operator=(null_t) noexcept
	{Begin = End = null; return *this;}

	forceinline T& operator[](size_t index) const
	{
		INTRA_DEBUG_ASSERT(index < Length());
		return Begin[index];
	}

	forceinline T& Get(size_t index, T& defaultValue) const
	{
		if(index < Length()) return Begin[index];
		return defaultValue;
	}

	template<typename U=T> forceinline Meta::EnableIf<
		!Meta::IsConst<U>::_,
	const T&> Get(size_t index, const T& defaultValue) const
	{
		if(index < Length()) return Begin[index];
		return defaultValue;
	}

	forceinline T Get(size_t index) const
	{
		if(index < Length()) return Begin[index];
		return T();
	}


	forceinline Span operator()(size_t firstIndex, size_t endIndex) const
	{
		INTRA_DEBUG_ASSERT(endIndex >= firstIndex && endIndex <= Length());
		return Span(Begin + firstIndex, Begin + endIndex);
	}

	constexpr forceinline Span TakeNone() const noexcept {return {Begin, Begin};}


	template<typename U=T> forceinline Meta::EnableIf<
		Meta::IsArithmeticType<U>::_,
	bool> StartsWith(Span<const T> str) const noexcept
	{return Length() >= str.Length() && C::memcmp(Data(), str.Data(), str.Length()*sizeof(T)) == 0;}

	Span Find(T c) const
	{
		Span result = *this;
		while(!result.Empty() && result.First() != c)
			result.PopFirst();
		return result;
	}

	Span Find(Span<const T> arr) const
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

	Span FindBefore(const T& v) const
	{
		Span result = {Begin, Begin};
		while(result.End != End && *result.End != v) result.End++;
		return result;
	}

	forceinline bool Contains(T c) {return !Find(c).Empty();}
	forceinline bool Contains(Span<const T> arr) {return !Find(arr).Empty();}


	template<typename U> constexpr forceinline Span<U> Reinterpret() const noexcept
	{
		typedef U* UPtr;
		return Span<U>(UPtr(Begin), UPtr(End));
	}

	T* Begin;
	T* End;
};

template<typename T> using CSpan = Span<const T>;

template<typename T, size_t N> forceinline constexpr Span<T> Take(T(&arr)[N], size_t n) noexcept {return {arr, n<N? n: N};}

template<typename T> forceinline constexpr Span<T> SpanOfPtr(T* arrPtr, size_t n) noexcept {return {arrPtr, n};}
template<typename T, size_t N> forceinline constexpr Span<T> SpanOfBuffer(T(&arr)[N]) noexcept {return {arr, N};}
template<typename T, size_t N> forceinline constexpr Span<T> SpanOf(T(&arr)[N]) noexcept {return Span<T>(arr);}
template<typename T, size_t N> forceinline constexpr CSpan<T> CSpanOf(const T(&arr)[N]) noexcept {return CSpan<T>(arr);}
template<typename T> forceinline constexpr CSpan<T> SpanOf(InitializerList<T> arr) noexcept {return arr;}
template<typename T> forceinline constexpr CSpan<T> CSpanOf(InitializerList<T> arr) noexcept {return arr;}

template<typename T=byte> forceinline Span<T> SpanOfRaw(void* data, size_t bytes) noexcept {return {static_cast<T*>(data), bytes/sizeof(T)};}
template<typename T=byte> forceinline CSpan<T> SpanOfRaw(const void* data, size_t bytes) noexcept {return {static_cast<const T*>(data), bytes/sizeof(T)};}
template<typename T=byte> forceinline CSpan<T> CSpanOfRaw(const void* data, size_t bytes) noexcept {return SpanOfRaw<T>(data, bytes);}

template<typename T> forceinline Span<T> SpanOfRawElements(void* data, size_t elements) noexcept {return {static_cast<T*>(data), elements};}
template<typename T> forceinline CSpan<T> SpanOfRawElements(const void* data, size_t elements) noexcept {return {static_cast<const T*>(data), elements};}
template<typename T> forceinline CSpan<T> CSpanOfRawElements(const void* data, size_t elements) noexcept {return SpanOfRawElements<T>(data, elements);}

#ifndef INTRA_UTILS_NO_CONCEPTS

template<typename R> forceinline Span<Concepts::ElementTypeOfArrayKeepConstOrDisable<R>> SpanOf(R&& r) noexcept
{return {Concepts::DataOf(r), Concepts::LengthOf(r)};}

template<typename R> forceinline CSpan<Concepts::ElementTypeOfArrayOrDisable<R>> CSpanOf(R&& r) noexcept
{return {Concepts::DataOf(r), Concepts::LengthOf(r)};}

static_assert(Concepts::HasLength<Span<float>>::_, "HasLength error.");
static_assert(Concepts::HasData<Span<int>>::_, "HasData error.");
static_assert(Concepts::IsArrayClass<Span<double>>::_, "IsArrayClass error.");
static_assert(Concepts::IsInputRange<Span<float>>::_, "Not input range???");
static_assert(Concepts::IsForwardRange<Span<float>>::_, "Not forward range???");
static_assert(Concepts::IsInputRange<CSpan<int>>::_, "Not input range???");
static_assert(Concepts::IsForwardRange<CSpan<int>>::_, "Not forward range???");
static_assert(Concepts::IsRandomAccessRange<CSpan<uint>>::_, "Not random access range???");
static_assert(Concepts::IsFiniteRandomAccessRange<CSpan<int>>::_, "Not finite random access range???");
static_assert(Concepts::IsRandomAccessRange<Span<float>>::_, "IsRandomAccessRange error.");
static_assert(Concepts::IsFiniteRandomAccessRange<Span<float>>::_, "IsFiniteRandomAccessRange error.");
static_assert(Meta::TypeEquals<Concepts::ValueTypeOf<Span<float>>, float>::_, "IsArrayRange error.");
static_assert(Concepts::IsArrayRange<Span<float>>::_, "IsArrayRange error.");
static_assert(Concepts::IsArrayRange<CSpan<char>>::_, "IsArrayRange error.");

#endif

}

namespace Utils {
using Range::Span;
using Range::CSpan;
using Range::SpanOf;
using Range::CSpanOf;
using Range::SpanOfPtr;
using Range::SpanOfBuffer;
using Range::SpanOfRaw;
using Range::CSpanOfRaw;
using Range::SpanOfRawElements;
using Range::CSpanOfRawElements;
using Range::Take;
}

using Range::Span;
using Range::CSpan;
using Range::SpanOf;
using Range::CSpanOf;
using Range::SpanOfPtr;
using Range::SpanOfBuffer;
using Range::SpanOfRaw;
using Range::CSpanOfRaw;
using Range::SpanOfRawElements;
using Range::CSpanOfRawElements;
using Range::Take;

#ifndef INTRA_UTILS_NO_CONCEPTS
namespace Concepts {
template<typename R> forceinline Meta::EnableIf<
	!IsInputRange<R>::_ &&
	!HasAsRangeMethod<R>::_,
Span<Meta::RemovePointer<PtrElementTypeOfArrayOrDisable<R>>>> RangeOf(R&& r) noexcept {return SpanOf(r);}

template<typename T, size_t N> forceinline constexpr Span<T> RangeOf(T(&arr)[N]) noexcept {return Span<T>(arr);}
}
#endif

}

INTRA_WARNING_POP
