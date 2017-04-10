#pragma once

#include "Range/Generators/Span.h"

namespace Intra { namespace Range {

//!  ласс, ориентированный на использование в качестве выходного диапазона или потока.
//! ¬ отличие от Span запоминает первоначальную позицию,
//! что позвол€ет запросить диапазон всех записанных элементов.
template<typename T> class OutputArrayRange
{
	T* mBegin;
	Span<T> mRight;
public:
	OutputArrayRange(null_t=null): mBegin(null), mRight(null) {}

	template<typename R, typename=Meta::EnableIf<
		IsAsArrayRangeOfExactly<R, T>::_
	>> forceinline OutputArrayRange(R&& dst)
	{
		auto rangeCopy = Range::Forward<R>(dst);
		mBegin = rangeCopy.Data();
		mRight = {mBegin, rangeCopy.Length()};
	}

	//! —бросить поток в начальное состо€ние дл€ перезаписи всех уже записанных элементов.
	void Reset() {mRight.Begin = mBegin;}

	//! ѕолучить диапазон всех записанных данных.
	Span<T> GetWrittenData() const {return {mBegin, mRight.Begin};}

	//! ¬озвращает количество записанных элементов.
	size_t ElementsWritten() const {return size_t(mRight.Begin-mBegin);}
	size_t Position() const {return ElementsWritten();}

	//! ѕереместить элемент в диапазон. »спользуема€ операци€ - присваивание
	void Put(T&& value) {mRight.Put(Meta::Move(value));}

	//! —копировать элемент в диапазон. »спользуема€ операци€ - присваивание
	void Put(const T& value) {mRight.Put(value);}

	//! ћетоды дл€ пр€мого доступа к ещЄ не записанным элементам массива.
	//! ѕолезно дл€ использовани€ с алгоритмами, оптимизированными дл€ массивов.
	T* Data() const {return mRight.Begin;}
	size_t Length() const {return mRight.Length();}
	T& First() const {return mRight.First();}
	void PopFirst() {mRight.PopFirst();}
	T& Last() const {return mRight.Last();}
	void PopLast() {mRight.PopLast();}

	//! Output диапазон переполнен, т.е. свободное место пусто.
	bool Empty() const {return mRight.Empty();}
	size_t PopFirstN(size_t count) {return mRight.PopFirstN(count);}

	//! –авными null считаютс€ только OutputArrayRange, которым присвоено null, или сконструированные по умолчанию.
	//! Ёто поведение отличаетс€ от поведени€ большинства других диапазонов, дл€ которых равенство null означает Empty.
	forceinline bool operator==(null_t) const {return mBegin==null || (mBegin==mRight.Begin && mRight.Empty());}
	forceinline bool operator!=(null_t) const {return !operator==(null);}
};

}
using Range::OutputArrayRange;

}
