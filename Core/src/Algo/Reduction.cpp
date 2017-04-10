#include "Algo/Reduction.h"
#include "Math/Simd.h"

namespace Intra { namespace Algo {

template<> float Minimum(CSpan<float> arr)
{
	if(arr==null) return Math::NaN;
	auto ptr = arr.Begin;

#if(INTRA_SIMD_SUPPORT==INTRA_SIMD_NONE)
	float result = *ptr++;
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
	float result = Math::Min(Math::Min(minis[0], minis[1]), Math::Min(minis[2], minis[3]));
#endif
	while(ptr<arr.End)
	{
		if(result < *ptr) result = *ptr;
		ptr++;
	}
	return result;
}

template<> float Maximum(CSpan<float> arr)
{
	if(arr==null) return Math::NaN;
	auto ptr = arr.Begin;

#if(INTRA_SIMD_SUPPORT==INTRA_SIMD_NONE)
	float result = *ptr++;
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
	float result = Math::Max( Math::Max(maxis[0], maxis[1]), Math::Max(maxis[2], maxis[3]) );
#endif
	while(ptr<arr.End)
	{
		if(result<*ptr) result=*ptr;
		ptr++;
	}
	return result;
}

template<> void MiniMax(CSpan<float> arr, float* minimum, float* maximum)
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
#if(INTRA_SIMD_SUPPORT==INTRA_SIMD_NONE)
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

}}

