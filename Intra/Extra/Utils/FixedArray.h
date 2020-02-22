#pragma once

#include "Core/Assert.h"
#include "Core/Range/Span.h"

INTRA_BEGIN
/** Dynamic-allocated array of fixed length.
  It does not reserve space and can't add elements.
*/
template<typename T> struct FixedArray
{
	forceinline FixedArray(null_t=null) noexcept: mData(null) {}
	
	/** Creates FixedArray by transferring it Span of preallocated and constructed elements.
	  Warning:
	  This method assumes that ``arr`` is allocated using new T[...] and points to the entire allocation.
	  It is disallowed to pass memory allocated in a different way.
	*/
	forceinline static FixedArray MakeOwnerOf(Owner<Span<T>> arr) noexcept
	{
		FixedArray result;
		result.mData = arr;
		return result.mData;
	}

	/*! @brief Creates FixedArray by transferring it pointer and size of range of preallocated and constructed elements.

	  Warning:
	  This method assumes that range [\p ptr and \p ptr+length) is allocated using new T[...] and points to the entire allocation.
	  It is disallowed to pass memory allocated in a different way.
	*/
	forceinline static FixedArray MakeOwnerOf(T* ptr, size_t length) noexcept
	{return MakeOwnerOf({ptr, length});}

	/*! @brief Construct array of length elements.

	  All elements are value-initialized. POD-types are zeroed,
	  and non-POD types are initialized using their default constructors.
	*/
	forceinline FixedArray(size_t length):
		mData(length == 0? null: new T[length](), length) {}
	
	FixedArray(CSpan<T> values):
		FixedArray(values.Length()) {values.CopyTo(mData);}
	
	forceinline FixedArray(InitializerList<T> values):
		FixedArray(SpanOf(values)) {}

	forceinline ~FixedArray() noexcept {delete[] mData.Data();}

	forceinline FixedArray(const FixedArray& rhs):
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

	forceinline FixedArray& operator=(FixedArray&& rhs) noexcept
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
	forceinline index_t Length() const noexcept {return mData.Length();}
	forceinline bool Empty() const noexcept {return mData.Empty();}
	forceinline T& First() {return mData.First();}
	forceinline const T& First() const {return mData.First();}
	forceinline T& Last() {return mData.Last();}
	forceinline const T& Last() const {return mData.Last();}
	forceinline T& operator[](size_t index) {return mData[index];}
	forceinline const T& operator[](size_t index) const {return mData[index];}

	//! Изменяет размер массива для хранения ровно count элементов.
	//! Всегда приводит к перераспределению памяти, даже в случае уменьшения размера.
	void SetCount(size_t count)
	{
		if(count == Length()) return;
		FixedArray result(count);
		mData.MoveTo(result);
		operator=(Move(result));
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

	forceinline const Span<T>& AsRange() {return mData;}
	forceinline CSpan<T> AsRange() const {return mData;}
	forceinline CSpan<T> AsConstRange() const {return mData;}

	forceinline bool operator==(null_t) const {return mData.Empty();}
	forceinline bool operator!=(null_t) const {return !operator==(null);}

private:
	Owner<Span<T>> mData;
};
INTRA_END
