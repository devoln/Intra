#include "Reduction.h"

#include "Cpp/InfNan.h"

#include "Simd/Simd.h"

#include "Funal/Op.h"

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
	auto ptr = arr.Begin;
	Pair<float> minimax;
#if(INTRA_SIMD_SUPPORT == INTRA_SIMD_NONE)
	minimax.first = minimax.second = *ptr++;
#else
	Simd::float4 mini;
	Simd::float4 maxi;
	if(ptr < arr.End - 3)
	{
		mini = Simd::SetFloat4(ptr);
		maxi = Simd::SetFloat4(ptr);
		ptr += 4;
		while(ptr < arr.End - 3)
		{
			Simd::float4 v = Simd::SetFloat4U(ptr);
			mini = Simd::Min(mini, v);
			maxi = Simd::Max(maxi, v);
			ptr += 4;
		}
		float minis[4]; Simd::GetU(minis, mini);
		float maxis[4]; Simd::GetU(maxis, maxi);
		minimax.first = Funal::Min( Funal::Min(minis[0], minis[1]), Funal::Min(minis[2], minis[3]) );
		minimax.second = Funal::Max( Funal::Max(maxis[0], maxis[1]), Funal::Max(maxis[2], maxis[3]) );
	}
	else minimax.first = minimax.second = *ptr++;
#endif
	while(ptr < arr.End)
	{
		if(*ptr < minimax.first) minimax.first = *ptr;
		if(minimax.second < *ptr) minimax.second = *ptr;
		ptr++;
	}
	return minimax;
}

}}

