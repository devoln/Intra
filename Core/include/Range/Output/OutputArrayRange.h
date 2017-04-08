#pragma once

#include "Range/Generators/ArrayRange.h"

namespace Intra { namespace Range {

//! �����, ��������������� �� ������������� � �������� ��������� ��������� ��� ������.
//! � ������� �� ArrayRange ���������� �������������� �������,
//! ��� ��������� ��������� �������� ���� ���������� ���������.
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

	//! �������� ����� � ��������� ��������� ��� ���������� ���� ��� ���������� ���������.
	void Reset() {mRight.Begin = mBegin;}

	//! �������� �������� ���� ���������� ������.
	ArrayRange<T> GetWrittenData() const {return {mBegin, mRight.Begin};}

	//! ���������� ���������� ���������� ���������.
	size_t ElementsWritten() const {return size_t(mRight.Begin-mBegin);}
	size_t Position() const {return ElementsWritten();}

	//! ����������� ������� � ��������. ������������ �������� - ������������
	void Put(T&& value) {mRight.Put(Meta::Move(value));}

	//! ����������� ������� � ��������. ������������ �������� - ������������
	void Put(const T& value) {mRight.Put(value);}

	//! ������ ��� ������� ������� � ��� �� ���������� ��������� �������.
	//! ������� ��� ������������� � �����������, ����������������� ��� ��������.
	T* Data() const {return mRight.Begin;}
	size_t Length() const {return mRight.Length();}
	T& First() const {return mRight.First();}
	void PopFirst() {mRight.PopFirst();}
	T& Last() const {return mRight.Last();}
	void PopLast() {mRight.PopLast();}

	//! Output �������� ����������, �.�. ��������� ����� �����.
	bool Empty() const {return mRight.Empty();}
	size_t PopFirstN(size_t count) {return mRight.PopFirstN(count);}

	//! ������� null ��������� ������ OutputArrayRange, ������� ��������� null, ��� ����������������� �� ���������.
	//! ��� ��������� ���������� �� ��������� ����������� ������ ����������, ��� ������� ��������� null �������� Empty.
	forceinline bool operator==(null_t) const {return mBegin==null || (mBegin==mRight.Begin && mRight.Empty());}
	forceinline bool operator!=(null_t) const {return !operator==(null);}
};

}
using Range::OutputArrayRange;

}
