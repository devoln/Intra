#pragma once

#include "Platform/CppWarnings.h"
#include "Platform/CppFeatures.h"
#include "Range/Generators/Span.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Range {

//! �����, ��������������� �� ������������� � �������� ��������� ��������� ��� ������.
//! � ������� �� Span ���������� �������������� �������,
//! ��� ��������� ��������� �������� ���� ���������� ���������.
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

	//! �������� ����� � ��������� ��������� ��� ���������� ���� ��� ���������� ���������.
	forceinline void Reset() noexcept {mRight.Begin = mBegin;}

	//! �������� �������� ���� ���������� ������.
	constexpr forceinline Span<T> GetWrittenData() const noexcept {return {mBegin, mRight.Begin};}

	//! ���������� ���������� ���������� ���������.
	constexpr forceinline size_t ElementsWritten() const noexcept {return size_t(mRight.Begin-mBegin);}
	constexpr forceinline size_t Position() const noexcept {return ElementsWritten();}

	//! ����������� ������� � ��������. ������������ �������� - ������������
	void Put(T&& value) {mRight.Put(Meta::Move(value));}

	//! ����������� ������� � ��������. ������������ �������� - ������������
	void Put(const T& value) {mRight.Put(value);}

	//! ������ ��� ������� ������� � ��� �� ���������� ��������� �������.
	//! ������� ��� ������������� � �����������, ����������������� ��� ��������.
	constexpr forceinline T* Data() const noexcept {return mRight.Begin;}
	constexpr forceinline size_t Length() const noexcept {return mRight.Length();}
	T& First() const {return mRight.First();}
	void PopFirst() {mRight.PopFirst();}
	T& Last() const {return mRight.Last();}
	void PopLast() {mRight.PopLast();}

	//! Output �������� ����������, �.�. ��������� ����� �����.
	constexpr forceinline bool Empty() const noexcept {return mRight.Empty();}
	forceinline size_t PopFirstN(size_t count) noexcept {return mRight.PopFirstN(count);}

	//! ������� null ��������� ������ OutputArrayRange, ������� ��������� null, ��� ����������������� �� ���������.
	//! ��� ��������� ���������� �� ��������� ����������� ������ ����������, ��� ������� ��������� null �������� Empty.
	constexpr forceinline bool operator==(null_t) const noexcept
	{return mBegin==null || (mBegin==mRight.Begin && mRight.Empty());}
	
	constexpr forceinline bool operator!=(null_t) const noexcept {return !operator==(null);}
};

}
using Range::OutputArrayRange;

}

INTRA_WARNING_POP
