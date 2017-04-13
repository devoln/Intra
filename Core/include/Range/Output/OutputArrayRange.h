#pragma once

#include "Platform/CppWarnings.h"
#include "Platform/CppFeatures.h"
#include "Range/Generators/Span.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Range {

//!  ласс, ориентированный на использование в качестве выходного диапазона или потока.
//! ¬ отличие от Span запоминает первоначальную позицию,
//! что позвол€ет запросить диапазон всех записанных элементов.
template<typename T> class OutputArrayRange
{
	T* mBegin;
	Span<T> mRight;
public:
	constexpr forceinline OutputArrayRange(null_t=null) noexcept: mBegin(null), mRight(null) {}

	template<typename R, typename=Meta::EnableIf<
		IsAsArrayRangeOfExactly<R, T>::_
	>> forceinline OutputArrayRange(R&& dst)
	{
		auto rangeCopy = Range::Forward<R>(dst);
		mBegin = rangeCopy.Data();
		mRight = {mBegin, rangeCopy.Length()};
	}

	//! —бросить поток в начальное состо€ние дл€ перезаписи всех уже записанных элементов.
	forceinline void Reset() noexcept {mRight.Begin = mBegin;}

	//! ѕолучить диапазон всех записанных данных.
	constexpr forceinline Span<T> GetWrittenData() const noexcept {return {mBegin, mRight.Begin};}

	//! ¬озвращает количество записанных элементов.
	constexpr forceinline size_t ElementsWritten() const noexcept {return size_t(mRight.Begin-mBegin);}
	constexpr forceinline size_t Position() const noexcept {return ElementsWritten();}

	//! ѕереместить элемент в диапазон. »спользуема€ операци€ - присваивание
	void Put(T&& value) {mRight.Put(Meta::Move(value));}

	//! —копировать элемент в диапазон. »спользуема€ операци€ - присваивание
	void Put(const T& value) {mRight.Put(value);}

	//! ћетоды дл€ пр€мого доступа к ещЄ не записанным элементам массива.
	//! ѕолезно дл€ использовани€ с алгоритмами, оптимизированными дл€ массивов.
	constexpr forceinline T* Data() const noexcept {return mRight.Begin;}
	constexpr forceinline size_t Length() const noexcept {return mRight.Length();}
	T& First() const {return mRight.First();}
	void PopFirst() {mRight.PopFirst();}
	T& Last() const {return mRight.Last();}
	void PopLast() {mRight.PopLast();}

	//! Output диапазон переполнен, т.е. свободное место пусто.
	constexpr forceinline bool Empty() const noexcept {return mRight.Empty();}
	forceinline size_t PopFirstN(size_t count) noexcept {return mRight.PopFirstN(count);}

	//! –авными null считаютс€ только OutputArrayRange, которым присвоено null, или сконструированные по умолчанию.
	//! Ёто поведение отличаетс€ от поведени€ большинства других диапазонов, дл€ которых равенство null означает Empty.
	constexpr forceinline bool operator==(null_t) const noexcept
	{return mBegin==null || (mBegin==mRight.Begin && mRight.Empty());}
	
	constexpr forceinline bool operator!=(null_t) const noexcept {return !operator==(null);}
};

}
using Range::OutputArrayRange;

}

INTRA_WARNING_POP
