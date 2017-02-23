#pragma once

#include "Range/Generators/ArrayRange.h"

namespace Intra { namespace Range {

//! Класс, ориентированный на использование в качестве выходного диапазона или потока.
//! В отличие от ArrayRange запоминает первоначальную позицию,
//! что позволяет запросить диапазон всех записанных элементов.
template<typename T> class OutputArrayRange
{
	T* mBegin;
	ArrayRange<T> mRight;
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

	//! Сбросить поток в начальное состояние для перезаписи всех уже записанных элементов.
	void Reset() {mRight.Begin = mBegin;}

	//! Получить диапазон всех записанных данных.
	ArrayRange<T> GetWrittenData() const {return {mBegin, mRight.Begin};}

	//! Возвращает количество записанных элементов.
	size_t ElementsWritten() const {return size_t(mRight.Begin-mBegin);}
	size_t Position() const {return ElementsWritten();}

	//! Переместить элемент в диапазон. Используемая операция - присваивание
	void Put(T&& value) {mRight.Put(Meta::Move(value));}

	//! Скопировать элемент в диапазон. Используемая операция - присваивание
	void Put(const T& value) {mRight.Put(value);}

	//! Методы для прямого доступа к ещё не записанным элементам массива.
	//! Полезно для использования с алгоритмами, оптимизированными для массивов.
	T* Data() const {return mRight.Begin;}
	size_t Length() const {return mRight.Length();}
	T& First() const {return mRight.First();}
	void PopFirst() {mRight.PopFirst();}
	bool Empty() const {return mRight.Empty();}
	size_t PopFirstN(size_t count) {return mRight.PopFirstN(count);}
};

}}
