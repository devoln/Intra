#include "Algorithms/Algorithms.h"
#include "Range/ArrayRange.h"
#include "Range/StringView.h"

#ifdef INTRA_USE_PDO

#if INTRA_PLATFORM_ARCH==INTRA_PLATFORM_X86 || INTRA_PLATFORM_ARCH==INTRA_PLATFORM_X86_64
#include "Math/Simd.h"
#elif INTRA_PLATFORM_ARCH==INTRA_PLATFORM_ARM
#include "Math/Simd.h"
#undef INTRA_USE_PDO
#else
#undef INTRA_USE_PDO
#endif

#endif

namespace Intra {
using namespace Math;


namespace Algo {

template<> void Add(ArrayRange<float> dstOp1, ArrayRange<const float> op2)
{
	INTRA_ASSERT(dstOp1.Length()==op2.Length());
	auto dst = dstOp1.Begin;
	auto src = op2.Begin;

#if(INTRA_MIN_SIMD_SUPPORT==INTRA_SIMD_NONE)
	while(dst<dstOp1.End-3)
	{
		*dst++ += *src++;
		*dst++ += *src++;
		*dst++ += *src++;
		*dst++ += *src++;
	}
#else
	while(dst<dstOp1.End-3)
	{
		Simd::GetU(dst, Simd::Add(Simd::SetFloat4U(dst), Simd::SetFloat4U(src)));
		dst += 4;
		src += 4;
	}
#endif
	while(dst<dstOp1.End) *dst++ += *src++;
}


template<> void MulAdd(ArrayRange<float> dstOp1, float mul, float add)
{
	auto dst = dstOp1.Begin;

#if(INTRA_MIN_SIMD_SUPPORT==INTRA_SIMD_NONE)
	while(dst<dstOp1.End-3)
	{
		*dst = *dst * mul + add; dst++;
		*dst = *dst * mul + add; dst++;
		*dst = *dst * mul + add; dst++;
		*dst = *dst * mul + add; dst++;
	}
#else
	Simd::float4 addps = Simd::SetFloat4(add);
	Simd::float4 mulps = Simd::SetFloat4(mul);
	while(dst<dstOp1.End-3)
	{
		Simd::float4 v = Simd::SetFloat4U(dst);
		v = Simd::Mul(v, mulps);
		v = Simd::Add(v, addps);
		Simd::GetU(dst, v);
		dst+=4;
	}
#endif

	while(dst<dstOp1.End) *dst = *dst * mul + add, dst++;
}

/*template<> void MulAdd(ArrayRange<float> dest, ArrayRange<const float> op1, float op2, float op3)
{
	auto dst=dest.Begin;
	auto src=op1.Begin;
	while(dst<dest.end) *dst++ = *src++ * op2 + op3;
}*/


template<> float Minimum(ArrayRange<const float> arr)
{
	if(arr==null) return Math::NaN;
	auto ptr = arr.Begin;
	float result;
#if(INTRA_MIN_SIMD_SUPPORT==INTRA_SIMD_NONE)
	result = *ptr++;
#else
	Simd::float4 mini = Simd::SetFloat4(*ptr);
	while(ptr<arr.End-3)
	{
		Simd::float4 v = Simd::SetFloat4U(ptr);
		mini = Simd::Min(mini, v);
		ptr+=4;
	}
	float minis[4];
	Simd::GetU(minis, mini);
	result = Math::Min(Math::Min(minis[0], minis[1]), Math::Min(minis[2], minis[3]));
#endif
	while(ptr<arr.End)
	{
		if(result<*ptr) result=*ptr;
		ptr++;
	}
	return result;
}

template<> float Maximum(ArrayRange<const float> arr)
{
	if(arr==null) return Math::NaN;
	auto ptr = arr.Begin;
	float result;
#if(INTRA_MIN_SIMD_SUPPORT==INTRA_SIMD_NONE)
	result=*ptr++;
#else
	Simd::float4 maxi = Simd::SetFloat4(*ptr);
	while(ptr<arr.End-3)
	{
		Simd::float4 v = Simd::SetFloat4U(ptr);
		maxi = Simd::Max(maxi, v);
		ptr+=4;
	}
	float maxis[4];
	Simd::GetU(maxis, maxi);
	result = Math::Max( Math::Max(maxis[0], maxis[1]), Math::Max(maxis[2], maxis[3]) );
#endif
	while(ptr<arr.End)
	{
		if(result<*ptr) result=*ptr;
		ptr++;
	}
	return result;
}

template<> void MiniMax(ArrayRange<const float> arr, float* minimum, float* maximum)
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

	if(arr==null) {*minimum = *maximum = Math::NaN; return;}
	auto ptr=arr.Begin;
#if(INTRA_MIN_SIMD_SUPPORT==INTRA_SIMD_NONE)
	*maximum = *minimum = *ptr++;
#else
	Simd::float4 mini = Simd::SetFloat4(*ptr);
	Simd::float4 maxi = Simd::SetFloat4(*ptr);
	while(ptr<arr.End-3)
	{
		Simd::float4 v = Simd::SetFloat4U(ptr);
		mini = Simd::Min(mini, v);
		maxi = Simd::Max(maxi, v);
		ptr+=4;
	}
	float minis[4]; Simd::GetU(minis, mini);
	float maxis[4]; Simd::GetU(maxis, maxi);
	*minimum = Math::Min( Math::Min(minis[0], minis[1]), Math::Min(minis[2], minis[3]) );
	*maximum = Math::Max( Math::Max(maxis[0], maxis[1]), Math::Max(maxis[2], maxis[3]) );
#endif
	while(ptr<arr.End)
	{
		if(*ptr<*minimum) *minimum = *ptr;
		if(*maximum<*ptr) *maximum = *ptr;
		ptr++;
	}
}

template<> void Cast(ArrayRange<short> dst, ArrayRange<const float> src)
{
#if(INTRA_MIN_SIMD_SUPPORT>=INTRA_SIMD_SSE && INTRA_PLATFORM_ARCH==INTRA_PLATFORM_X86)
	while(dst.Begin<dst.End-3)
	{
		*(__m64*)dst.Begin = _mm_cvtps_pi16(Simd::SetFloat4U(src.Begin));
		src.Begin+=4; dst.Begin+=4;
	}
	_mm_empty();
#else
	while(dst.Begin<dst.End-3)
	{
		*dst.Begin++ = short(*src.Begin++);
		*dst.Begin++ = short(*src.Begin++);
		*dst.Begin++ = short(*src.Begin++);
		*dst.Begin++ = short(*src.Begin++);
	}
#endif
	while(dst.Begin<dst.End)
	{
		//INTRA_ASSERT(*src>=-32768.0f && *src<=32767.0f);
		*dst.Begin++ = short(*src.Begin++);
	}
}

}}

