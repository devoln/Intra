#include "Range/Reduction.h"
#include "Simd/Simd.h"
#include "Cpp/InfNan.h"
#include "Utils/Op.h"

namespace Intra { namespace Range {

template<> float Minimum(CSpan<float> arr)
{
	if(arr==null) return Cpp::NaN;
	auto ptr = arr.Begin;

#if(INTRA_SIMD_SUPPORT==INTRA_SIMD_NONE)
	float result = *ptr++;
#else
	Simd::float4 mini = Simd::SetFloat4(*ptr);
	while(ptr<arr.End-3)
	{
		Simd::float4 v = Simd::SetFloat4U(ptr);
		mini = Simd::Min(mini, v);
		ptr += 4;
	}
	float minis[4];
	Simd::GetU(minis, mini);
	float result = Op::Min(Op::Min(minis[0], minis[1]), Op::Min(minis[2], minis[3]));
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
	if(arr==null) return Cpp::NaN;
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
	float result = Op::Max( Op::Max(maxis[0], maxis[1]), Op::Max(maxis[2], maxis[3]) );
#endif
	while(ptr<arr.End)
	{
		if(result < *ptr) result = *ptr;
		ptr++;
	}
	return result;
}

template<> void MiniMax(CSpan<float> arr, float* oMinimum, float* oMaximum)
{
	if(oMinimum == null)
	{
		if(oMaximum != null) *oMaximum = Maximum(arr);
		return;
	}
	if(oMaximum == null)
	{
		*oMinimum = Minimum(arr);
		return;
	}

	if(arr.Empty())
	{
		*oMinimum = *oMaximum = Cpp::NaN;
		return;
	}
	auto ptr = arr.Begin;
#if(INTRA_SIMD_SUPPORT==INTRA_SIMD_NONE)
	*oMaximum = *oMinimum = *ptr++;
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
	*oMinimum = Op::Min( Op::Min(minis[0], minis[1]), Op::Min(minis[2], minis[3]) );
	*oMaximum = Op::Max( Op::Max(maxis[0], maxis[1]), Op::Max(maxis[2], maxis[3]) );
#endif
	while(ptr<arr.End)
	{
		if(*ptr < *oMinimum) *oMinimum = *ptr;
		if(*oMaximum < *ptr) *oMaximum = *ptr;
		ptr++;
	}
}

}}

