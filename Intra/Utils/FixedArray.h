#pragma once

#include "Cpp/Fundamental.h"
#include "Cpp/Features.h"
#include "Cpp/Warnings.h"
#include "Utils/Debug.h"

namespace Intra { namespace Utils {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename T> struct FixedArray
{
	forceinline FixedArray(null_t=null) noexcept: mData(null) {}
	
	//! Создаёт новый FixedArray, передавая ему во владение диапазон элементов.
	//! Этот метод предполагает, что элементы диапазона выделены как new T[...].
	//! Передавать элементы, выделенные другим способом, запрещено.
	forceinline static FixedArray MakeOwnerOf(Span<T> arr) noexcept
	{
		FixedArray result;
		result.mData = arr;
		return result.mData;
	}

	//! Создаёт новый FixedArray, передавая ему во владение диапазон элементов.
	//! Этот метод предполагает, что элементы диапазона выделены как new T[...].
	//! Передавать элементы, выделенные другим способом, запрещено.
	forceinline static FixedArray MakeOwnerOf(T* ptr, size_t length) noexcept
	{return MakeOwnerOf({ptr, length});}

	//! Конструктор, самостоятельно выделяющий память для length элементов.
	forceinline FixedArray(size_t length): mData(length==0? null: new T[length], length) {}

	forceinline ~FixedArray() noexcept {if(mData.Data()!=null) delete[] mData.Data();}

	forceinline FixedArray(const FixedArray& rhs) = delete;
	FixedArray& operator=(const FixedArray& rhs) = delete;

	forceinline FixedArray(FixedArray&& rhs) noexcept:
		mData(rhs.mData) {rhs.mData=null;}

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

	void SetCount(size_t count)
	{
		FixedArray result(count);
		mData.CopyTo(result);
		operator=(Cpp::Move(result));
	}

	forceinline T* begin() const noexcept {return mData.begin();}
	forceinline T* end() const noexcept {return mData.end();}

	//! Возвращает диапазон элементов, передавая права владения ими вызывающей стороне.
	//! Полученные элементы освобождаются через delete[].
	forceinline Span<T> Release()
	{
		Span<T> result = mData;
		mData = null;
		return result;
	}

	forceinline bool operator==(null_t) const {return mData.Empty();}
	forceinline bool operator!=(null_t) const {return !operator==(null);}

private:
	Span<T> mData;
};

INTRA_WARNING_POP

}
using Utils::FixedArray;

}
