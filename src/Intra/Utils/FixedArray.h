#pragma once

#include <Intra/Core.h>
#include <Intra/Range.h>

namespace Intra { INTRA_BEGIN
/** Dynamic-allocated array of fixed length.
  It does not reserve space and can't add elements.
*/
template<typename T, CRawAllocator A> struct FixedArray
{
	FixedArray() = default;
	INTRA_FORCEINLINE constexpr FixedArray(TUnsafe, Unique<T[], A> ptr, Size size) noexcept: mData(INTRA_MOVE(ptr)), mSize(size) {}
	
	/** Creates FixedArray by transferring it Span of preallocated and constructed elements.
	  Warning:
	  This method assumes that ``arr`` is allocated using new T[...] and points to the entire allocation.
	  It is disallowed to pass memory allocated in a different way.
	*/
	static FixedArray MakeOwnerOf(Owner<Span<T>> arr) noexcept
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
	static FixedArray MakeOwnerOf(T* ptr, Index length) noexcept
	{return MakeOwnerOf({ptr, length});}

	/*! @brief Construct array of length elements.

	  All elements are value-initialized. POD-types are zeroed,
	  and non-POD types are initialized using their default constructors.
	*/
	FixedArray(Index length):
		mData(SpanOfPtr(size_t(length) == 0? nullptr: new T[size_t(length)](), length)) {}
	
	FixedArray(Span<const T> values):
		FixedArray(values.Length()) {values.CopyTo(mData);}
	
	FixedArray(InitializerList<T> values):
		FixedArray(SpanOf(values)) {}

	~FixedArray() noexcept {delete[] mData.Data();}

	FixedArray(const FixedArray& rhs):
		mData(SpanOfPtr(new T[rhs.Length()], rhs.Length())) {rhs.mData.CopyTo(mData);}

	FixedArray& operator=(const FixedArray& rhs)
	{
		if(this == &rhs) return *this;
		if(Length() < rhs.Length())
		{
			auto oldData = mData.Data();
			mData = SpanOfPtr(new T[rhs.Length()], rhs.Length());
			delete[] oldData;
		}
		else mData = mData.TakeExactly(rhs.Length());
		rhs.mData.CopyTo(mData);
		return *this;
	}

	FixedArray(FixedArray&& rhs) noexcept:
		mData(rhs.mData) {rhs.mData = nullptr;}

	FixedArray& operator=(FixedArray&& rhs) noexcept
	{
		if(this == &rhs) return *this;
		delete[] mData.Data();
		mData = rhs.mData;
		rhs.mData = nullptr;
		return *this;
	}


	FixedArray& operator=(decltype(nullptr)) noexcept
	{
		delete[] mData.Data();
		mData = nullptr;
		return *this;
	}

	T* Data() const noexcept {return mData.Data();}
	auto Length() const noexcept {return mData.Length();}
	bool Empty() const noexcept {return mData.Empty();}
	T& First() {return mData.First();}
	const T& First() const {return mData.First();}
	T& Last() {return mData.Last();}
	const T& Last() const {return mData.Last();}
	T& operator[](Index index) {return mData[index];}
	const T& operator[](NonNegative<size_t> index) const {return mData[index];}

	/// Resize the array to store exactly \p count elements.
	/// Always causes heap reallocation when \p count != Count().
	void SetCount(Index count)
	{
		if(count == Length()) return;
		FixedArray result(count);
		mData.MoveTo(result);
		operator=(Move(result));
	}

	T* begin() const noexcept {return mData.begin();}
	T* end() const noexcept {return mData.end();}

	/// @return internal range transferring data ownership to the caller.
	/// Caller is responsible for deletion using ``delete[] returnedSpan.Data()``.
	Owner<Span<T>> ReleaseOwnership()
	{
		Span<T> result = mData;
		mData = {};
		return mData.release();
	}

	[[nodiscard]] operator bool const {return mData.Empty();}

private:
	Unique<T[]> mData;
	size_t mSize = 0;
};
} INTRA_END