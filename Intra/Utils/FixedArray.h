#pragma once

#include "Cpp/Fundamental.h"
#include "Cpp/Features.h"
#include "Cpp/Warnings.h"
#include "Cpp/InitializerList.h"

#include "Debug.h"
#include "Span.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Utils {

//! ������������ ������ ������������� �����.
//! �� ����������� ����� � �� �������� ������� ��� ������� ���������.
template<typename T> struct FixedArray
{
	forceinline FixedArray(null_t = null) noexcept: mData(null) {}
	
	//! ������ ����� FixedArray, ��������� ��� �� �������� �������� ���������.
	//! ���� ����� ������������, ��� �������� ��������� �������� ��� new T[...].
	//! ���������� ��������, ���������� ������ ��������, ���������.
	forceinline static FixedArray MakeOwnerOf(Span<T> arr) noexcept
	{
		FixedArray result;
		result.mData = arr;
		return result.mData;
	}

	//! ������ ����� FixedArray, ��������� ��� �� �������� �������� ���������.
	//! ���� ����� ������������, ��� �������� ��������� �������� ��� new T[...].
	//! ���������� ��������, ���������� ������ ��������, ���������.
	forceinline static FixedArray MakeOwnerOf(T* ptr, size_t length) noexcept
	{return MakeOwnerOf({ptr, length});}

	//! �����������, �������������� ���������� ������ ��� length ���������.
	//! ��� �������� value-initialized, �� ���� POD ���� ���������������� ������,
	//! � ��� ��-POD ������� � �������� ���������� ������������ �� ���������.
	forceinline FixedArray(size_t length):
		mData(length == 0? null: new T[length](), length) {}
	
	FixedArray(CSpan<T> values):
		FixedArray(values.Length()) {values.CopyTo(mData);}
	
	forceinline FixedArray(InitializerList<T> values):
		FixedArray(SpanOf(values)) {}

	forceinline ~FixedArray() noexcept {delete[] mData.Data();}

	FixedArray(const FixedArray& rhs):
		mData(new T[rhs.Length()], rhs.Length()) {rhs.mData.CopyTo(mData);}

	FixedArray& operator=(const FixedArray& rhs)
	{
		if(this == &rhs) return *this;
		if(Length() < rhs.Length())
		{
			auto oldData = mData.Data();
			mData = {new T[rhs.Length()], rhs.Length()};
			delete[] oldData;
		}
		else mData = mData.TakeExactly(rhs.Length());
		rhs.mData.CopyTo(mData);
		return *this;
	}

	forceinline FixedArray(FixedArray&& rhs) noexcept:
		mData(rhs.mData) {rhs.mData = null;}

	FixedArray& operator=(FixedArray&& rhs) noexcept
	{
		if(this == &rhs) return *this;
		delete[] mData.Data();
		mData = rhs.mData;
		rhs.mData = null;
		return *this;
	}


	forceinline FixedArray& operator=(null_t) noexcept
	{
		delete[] mData.Data();
		mData = null;
		return *this;
	}

	forceinline T* Data() const noexcept {return mData.Data();}
	forceinline size_t Length() const noexcept {return mData.Length();}
	forceinline bool Empty() const noexcept {return mData.Empty();}
	forceinline T& First() const {return mData.First();}
	forceinline T& Last() const {return mData.Last();}
	forceinline T& operator[](size_t index) const {return mData[index];}

	//! �������� ������ ������� ��� �������� ����� count ���������.
	//! ������ �������� � ����������������� ������, ���� � ������ ���������� �������.
	void SetCount(size_t count)
	{
		if(count == Length()) return;
		FixedArray result(count);
		mData.MoveTo(result);
		operator=(Cpp::Move(result));
	}

	forceinline T* begin() const noexcept {return mData.begin();}
	forceinline T* end() const noexcept {return mData.end();}

	//! ���������� �������� ���������, ��������� ����� �������� ��� ���������� �������.
	//! ���������� �������� ������������� ����� delete[].
	forceinline Span<T> Release()
	{
		Span<T> result = mData;
		mData = null;
		return result;
	}

	forceinline const Span<T>& AsRange() {return mData;}
	forceinline CSpan<T> AsRange() const {return mData;}
	forceinline CSpan<T> AsConstRange() const {return mData;}

	forceinline bool operator==(null_t) const {return mData.Empty();}
	forceinline bool operator!=(null_t) const {return !operator==(null);}

private:
	Span<T> mData;
};

}
using Utils::FixedArray;

}

INTRA_WARNING_POP
