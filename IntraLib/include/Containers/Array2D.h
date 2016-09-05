#pragma once

#include "Array.h"

namespace Intra {

template<typename T> class Array2D
{
public:
	Array2D(null_t=null): width(0) {}
	Array2D(size_t width, size_t height): width(width) {data.SetCount(width*height);}
	Array2D(const Array2D& rhs): width(rhs.width), data(rhs.data) {}

	forceinline T& operator()(size_t x, size_t y)
	{
		INTRA_ASSERT(x<Width() && y<Height());
		return data[y*width+x];
	};

	forceinline const T& operator()(size_t x, size_t y) const
	{
		INTRA_ASSERT(x<Width() && y<Height());
		return data[y*Width()+x];
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
		data = core::move(rhs.data);
		return *this;
	}

	forceinline size_t SizeInBytes() const {return data.SizeInBytes();}
	Array<T> MoveToLinearArray() {width = 0; return core::move(data);}
	//ByteBuffer MoveToByteBuffer() {width = 0; return data.MoveToByteBuffer();}

	forceinline size_t Width() const {return width;}
	forceinline size_t Height() const {return data.Count()/width;}

private:
	Array<T> data;
	size_t width;
};

//DEFINE_AS_TRIV_MOVABLE1(Array2D<T1>);

}
