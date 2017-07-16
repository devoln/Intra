#include "Reduction.h"

#include "Cpp/InfNan.h"

#include "Simd/Simd.h"

#include "Funal/Op.h"

#include <stdio.h>

namespace Intra { namespace Range {

template<> float Minimum(CSpan<float> arr)
{
	if(arr.Empty()) return Cpp::NaN;
	auto ptr = arr.Begin;

#if(INTRA_SIMD_SUPPORT==INTRA_SIMD_NONE)
	float result = *ptr++;
#else
	Simd::float4 mini = Simd::SetFloat4(*ptr);
	while(ptr < arr.End - 3)
	{
		Simd::float4 v = Simd::SetFloat4U(ptr);
		mini = Simd::Min(mini, v);
		ptr += 4;
	}
	float minis[4];
	Simd::GetU(minis, mini);
	float result = Funal::Min(Funal::Min(minis[0], minis[1]), Funal::Min(minis[2], minis[3]));
#endif
	while(ptr < arr.End)
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
	float result = Funal::Max( Funal::Max(maxis[0], maxis[1]), Funal::Max(maxis[2], maxis[3]) );
#endif
	while(ptr<arr.End)
	{
		if(result < *ptr) result = *ptr;
		ptr++;
	}
	return result;
}

template<> Pair<float> MiniMax(CSpan<float> arr)
{
	if(arr.Empty()) return {Cpp::NaN, Cpp::NaN};
	Pair<float> minimax;
#if(INTRA_SIMD_SUPPORT == INTRA_SIMD_NONE)
	minimax.first = minimax.second = *arr.Begin++;
#else
	Simd::float4 mini;
	Simd::float4 maxi;
	if(arr.End - arr.Begin >= 4)
	{
		mini = Simd::SetFloat4U(arr.Begin);
		maxi = Simd::SetFloat4U(arr.Begin);
		if(size_t(arr.Begin) & 12) arr.Begin += 4 - ((size_t(arr.Begin) & 12) >> 2);
		else arr.Begin += 4;
		while(arr.End - arr.Begin >= 4)
		{
			Simd::float4 v = Simd::SetFloat4(arr.Begin);
			mini = Simd::Min(mini, v);
			maxi = Simd::Max(maxi, v);
			arr.Begin += 4;
		}
		union
		{
			Simd::float4 minisAlignment;
			float minis[4];
		};
		Simd::Get(minis, mini);
		union
		{
			Simd::float4 maxisAlignment;
			float maxis[4];
		};
		Simd::Get(maxis, maxi);
		minimax.first = Funal::Min( Funal::Min(minis[0], minis[1]), Funal::Min(minis[2], minis[3]) );
		minimax.second = Funal::Max( Funal::Max(maxis[0], maxis[1]), Funal::Max(maxis[2], maxis[3]) );
	}
	else minimax.first = minimax.second = *arr.Begin++;
#endif
	while(arr.Begin < arr.End)
	{
		if(*arr.Begin < minimax.first) minimax.first = *arr.Begin;
		if(minimax.second < *arr.Begin) minimax.second = *arr.Begin;
		arr.Begin++;
	}
	return minimax;
}

}}

