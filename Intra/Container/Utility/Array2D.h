#pragma once

#include "Utils/FixedArray.h"

INTRA_BEGIN
inline namespace Container {

template<typename T> class Array2D
{
public:
	forceinline Array2D(null_t=null): mData(), mWidth(0) {}
	
	Array2D(size_t lineWidth, size_t columnHeight):
		mData(lineWidth*columnHeight), mWidth(lineWidth) {}
	
	Array2D(const Array2D& rhs) = default;
	Array2D(Array2D&& rhs) = default;
	Array2D& operator=(const Array2D& rhs) = default;
	Array2D& operator=(Array2D&& rhs) = default;

	forceinline T& operator()(size_t x, size_t y)
	{
		INTRA_DEBUG_ASSERT(x < Width() && y < Height());
		return mData[y*mWidth + x];
	};

	forceinline const T& operator()(size_t x, size_t y) const
	{
		INTRA_DEBUG_ASSERT(x < Width() && y < Height());
		return mData[y*mWidth + x];
	}

	forceinline Span<T> operator[](size_t y)
	{
		INTRA_DEBUG_ASSERT(y < Height());
		return {mData.Data() + y*mWidth, mWidth};
	}

	forceinline CSpan<T> operator[](size_t y) const
	{
		INTRA_DEBUG_ASSERT(y < Height());
		return {mData.Data() + y*mWidth, mWidth};
	}

	forceinline T* Data() {return mData.Data();}
	forceinline const T* Data() const {return mData.Data();}


	forceinline size_t SizeInBytes() const {return mData.SizeInBytes();}

	forceinline FixedArray<T> MoveToLinearArray()
	{
		mWidth = 0;
		return Move(mData);
	}

	forceinline size_t Width() const {return mWidth;}
	forceinline size_t Height() const {return mData.Length() / mWidth;}

private:
	FixedArray<T> mData;
	size_t mWidth;
};
}
INTRA_END
