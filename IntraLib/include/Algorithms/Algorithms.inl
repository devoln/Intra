#pragma once

#include "Meta/Type.h"

//TODO: алгоритмы пока не учитывают, что диапазоны могут перекрываться!

namespace Intra { namespace Algo {
//Операции между двумя диапазонами:

template<typename T> void Add(ArrayRange<T> dstOp1, ArrayRange<const T> op2)
{
	INTRA_ASSERT(dstOp1.Length()==op2.Length());
	while(!dstOp1.Empty())
	{
		dstOp1.First() += op2.First();
		dstOp1.PopFirst();
		op2.PopFirst();
	}
}

template<typename T> void Add(ArrayRange<T> dest, ArrayRange<const T> op1, ArrayRange<const T> op2)
{
	INTRA_ASSERT(dest.Length()==op1.Length());
	INTRA_ASSERT(op1.Length()==op2.Length());
	while(!dest.Empty())
	{
		dest.First() = op1.First() + op2.First();
		dest.PopFirst();
		op1.PopFirst();
		op2.PopFirst();
	}
}



template<typename T> void Subtract(ArrayRange<T> dstOp1, ArrayRange<const T> op2)
{
	INTRA_ASSERT(dstOp1.Length()==op2.Length());
	while(!dstOp1.Empty())
	{
		dstOp1.First() -= op2.First();
		dstOp1.PopFirst();
		op2.PopFirst();
	}
}

template<typename T> void Subtract(ArrayRange<T> dest, ArrayRange<const T> op1, ArrayRange<const T> op2)
{
	INTRA_ASSERT(dest.Length()==op1.Length());
	INTRA_ASSERT(op1.Length()==op2.Length());
	while(!dest.Empty())
	{
		dest.First() = op1.First() - op2.First();
		dest.PopFirst();
		op1.PopFirst();
		op2.PopFirst();
	}
}



template<typename T> void Multiply(ArrayRange<T> dstOp1, ArrayRange<const T> op2)
{
	INTRA_ASSERT(dstOp1.Length()==op2.Length());
	while(dstOp1.Empty())
	{
		dstOp1.First() *= op2.First();
		dstOp1.PopFirst();
		op2.PopFirst();
	}
}

template<typename T> void Multiply(ArrayRange<T> dest, ArrayRange<const T> op1, ArrayRange<const T> op2)
{
	INTRA_ASSERT(dest.Length()==op1.Length());
	INTRA_ASSERT(op1.Length()==op2.Length());
	while(!dest.Empty())
	{
		dest.First() = op1.First() * op2.First();
		dest.PopFirst();
		op1.PopFirst();
		op2.PopFirst();
	}
}

template<typename T> void Divide(ArrayRange<T> dstOp1, ArrayRange<const T> op2)
{
	INTRA_ASSERT(dstOp1.Length()==op2.Length());
	while(!dstOp1.Empty())
	{
		dstOp1.First() /= op2.First();
		dstOp1.PopFirst();
		op2.PopFirst();
	}
}

template<typename T> void Divide(ArrayRange<T> dest, ArrayRange<const T> op1, ArrayRange<const T> op2)
{
	INTRA_ASSERT(dest.Length()==op1.Length());
	INTRA_ASSERT(op1.Length()==op2.Length());
	while(!dest.Empty())
	{
		dest.First() = op1.First() / op2.First();
		dest.PopFirst();
		op1.PopFirst();
		op2.PopFirst();
	}
}




//Операции между диапазоном и значением:

template<typename T> void Add(ArrayRange<T> dstOp1, T op2)
{
	for(auto& val: dstOp1) val += op2;
}

template<typename T> void Add(ArrayRange<T> dest, ArrayRange<const T> op1, T op2)
{
	while(!dest.Empty())
	{
		dest.First() = op1.First() + op2;
		dest.PopFirst();
		op1.PopFirst();
	}
}

template<typename T> void Multiply(ArrayRange<T> dstOp1, T op2)
{
	for(auto& val: dstOp1) val *= op2;
}

template<typename T> void Multiply(ArrayRange<T> dest, ArrayRange<const T> op1, T op2)
{
	while(!dest.Empty())
	{
		dest.First() = op1.First() * op2;
		dest.PopFirst();
		op1.PopFirst();
	}
}

template<typename T> void Divide(ArrayRange<T> dstOp1, T op2)
{
	while(!dstOp1.Empty())
	{
		dstOp1.First() /= op2;
		dstOp1.PopFirst();
	}
}

template<typename T> void Divide(ArrayRange<T> dest, ArrayRange<const T> op1, T op2)
{
	while(!dest.Empty())
	{
		dest.First() = op1.First() / op2;
		dest.PopFirst();
		op1.PopFirst();
	}
}

template<typename T> void MulAdd(ArrayRange<T> dstOp1, T mul, T add)
{
	while(!dstOp1.Empty())
	{
		dstOp1.First() = dstOp1.First() * mul + add;
		dstOp1.PopFirst();
	}
}

template<typename T> void MulAdd(ArrayRange<T> dest, ArrayRange<const T> op1, T mul, T add)
{
	while(!dest.Empty())
	{
		dest.First() = op1.First() * mul + add;
		dest.PopFirst();
		op1.PopFirst();
	}
}




//Операции аккумулирования:

template<typename T> T Minimum(ArrayRange<const T> arr)
{
	INTRA_ASSERT(!arr.Empty());
	T result = arr.First();
	arr.PopFirst();
	while(!arr.Empty())
	{
		if(arr.First()<result) result = arr.First();
		arr.PopFirst();
	}
	return result;
}

template<typename T> T Maximum(ArrayRange<const T> arr)
{
	INTRA_ASSERT(!arr.Empty());
	T result = arr.First();
	arr.PopFirst();
	while(!arr.Empty())
	{
		if(result<arr.First()) result = arr.First();
		arr.PopFirst();
	}
	return result;
}

template<typename T> void MiniMax(ArrayRange<const T> arr, T* minimum, T* maximum)
{
	if(minimum==null)
	{
		if(maximum!=null) *maximum = Maximum(arr);
		return;
	}
	if(maximum==null)
	{
		*minimum = Minimum(arr);
		return;
	}

	INTRA_ASSERT(!arr.Empty());
	*maximum = *minimum = arr.First();
	arr.PopFirst();

	while(!arr.Empty())
	{
		if(arr.First()<*minimum) *minimum = arr.First();
		if(*maximum<arr.First()) *maximum = arr.First();
		arr.PopFirst();
	}
}

	



template<typename T> void Interleave(ArrayRange<T> dst, ArrayRange<const ArrayRange<const T>> src)
{
	INTRA_ASSERT(!dst.Empty());
	INTRA_ASSERT(!src.Empty());
	const size_t channelCount = src.Length();
	const size_t valuesPerChannel = src.First().Length();
	INTRA_ASSERT(dst.Length() == valuesPerChannel*channelCount);

	for(size_t i=0; i<valuesPerChannel; i++)
	{
		for(size_t c=0; c<channelCount; c++)
		{
			dst.First() = src[c][i];
			dst.PopFirst();
		}
	}
}

template<typename T> void Deinterleave(ArrayRange<const ArrayRange<T>> dst, ArrayRange<const T> src)
{
	INTRA_ASSERT(!dst.Empty());
	INTRA_ASSERT(!src.Empty());
	const size_t channelCount = dst.Length();
	const size_t valuesPerChannel = dst.First().Length();
	INTRA_ASSERT(src.Length() == valuesPerChannel*channelCount);

	for(size_t i=0; i<valuesPerChannel; i++)
		for(size_t c=0; c<channelCount; c++)
		{
			dst[c][i] = src.First();
			src.PopFirst();
		}
}

template<typename To, typename From> void Cast(ArrayRange<To> dst, ArrayRange<const From> src)
{
	INTRA_ASSERT(dst.Length()==src.Length());
	while(!dst.Empty())
	{
		dst.First() = (To)src.First();
		src.PopFirst();
		dst.PopFirst();
	}
}

template<typename To, typename From> Meta::EnableIf<
	Meta::IsFloatType<To>::_ && Meta::IsIntegralType<From>::_
	> CastToNormalized(ArrayRange<To> dst, ArrayRange<const From> src)
{
	INTRA_ASSERT(dst.Length()==src.Length());
	while(!dst.Empty())
	{
		dst.First() = (To)src.First() / (To)Meta::NumericLimits<From>::Max();
		dst.PopFirst();
		src.PopFirst();
	}
}

template<typename To, typename From> Meta::EnableIf<
	Meta::IsFloatType<From>::_ && Meta::IsIntegralType<To>::_
	> CastFromNormalized(ArrayRange<To> dst, ArrayRange<const From> src)
{
	INTRA_ASSERT(dst.Length()==src.Length());
	while(!dst.Empty())
	{
		dst.First() = To(src.First() * Meta::NumericLimits<From>::Max());
		dst.PopFirst();
		src.PopFirst();
	}
}

}}
