#pragma once

#include "Container/Sequential/Array.h"

namespace Intra {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename T> class Array2D
{
public:
	Array2D(null_t=null): data(), width(0) {}
	
	Array2D(size_t lineWidth, size_t columnHeight):
		data(), width(lineWidth) {data.SetCount(lineWidth*columnHeight);}
	
	Array2D(const Array2D& rhs): data(rhs.data), width(rhs.width) {}
	Array2D(Array2D&& rhs): data(Cpp::Move(rhs.data)), width(rhs.width) {}

	forceinline T& operator()(size_t x, size_t y)
	{
		INTRA_DEBUG_ASSERT(x<Width() && y<Height());
		return data[y*width+x];
	};

	forceinline const T& operator()(size_t x, size_t y) const
	{
		INTRA_DEBUG_ASSERT(x<Width() && y<Height());
		return data[y*width+x];
	}

	forceinline T* Data() {return data.Data();}
	forceinline const T* Data() const {return data.Data();}

	Array2D& operator=(const Array2D& rhs)
	{
		width = rhs.width;
		data = rhs.data;
		return *this;
	}

	Array2D& operator=(Array2D&& rhs)
	{
		width = rhs.width;
		data = Cpp::Move(rhs.data);
		return *this;
	}

	forceinline size_t SizeInBytes() const {return data.SizeInBytes();}
	Array<T> MoveToLinearArray() {width = 0; return Cpp::Move(data);}
	//ByteBuffer MoveToByteBuffer() {width = 0; return data.MoveToByteBuffer();}

	forceinline size_t Width() const {return width;}
	forceinline size_t Height() const {return data.Count()/width;}

private:
	Array<T> data;
	size_t width;
};

INTRA_WARNING_POP

}
