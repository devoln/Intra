#include <Simd/SimdVector.h>

#include "Reduction.h"
#include "Cpp/InfNan.h"
#include "Funal/Op.h"

namespace Intra { namespace Range {

template<> float Minimum(CSpan<float> arr)
{
	if(arr.Empty()) return Cpp::NaN;
	auto ptr = arr.Begin;
	float result;
	if constexpr(Config::TargetMaxSimdLength<float> >= 4)
	{
		auto mini = SimdVectorFilled<float, 4>(*ptr);
		while(arr.End - ptr >= 4)
		{
			const auto v = SimdLoad<4>(ptr);
			mini = SimdSelect(mini, v, mini > v);
			ptr += 4;
		}
		alignas(16) float minis[4];
		SimdStoreAligned(minis, mini);
		result = Funal::Min(Funal::Min(minis[0], minis[1]), Funal::Min(minis[2], minis[3]));
	}
	else result = *ptr++;
	while(ptr < arr.End)
	{
		if(*ptr < result) result = *ptr;
		ptr++;
	}
	return result;
}

template<> float Maximum(CSpan<float> arr)
{
	if(arr == null) return Cpp::NaN;
	auto ptr = arr.Begin;
	float result;
	if constexpr(Config::TargetMaxSimdLength<float> >= 4)
	{
		auto maxi = SimdVectorFilled<float, 4>(*ptr);
		while(arr.End - ptr >= 4)
		{
			const auto v = SimdLoad<4>(ptr);
			maxi = SimdSelect(maxi, v, maxi < v);
			ptr += 4;
		}
		alignas(16) float maxis[4];
		SimdStoreAligned(maxis, maxi);
		result = Funal::Max(Funal::Max(maxis[0], maxis[1]), Funal::Max(maxis[2], maxis[3]));
	}
	else result = *ptr++;
	while(ptr < arr.End)
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
	if constexpr(Config::TargetMaxSimdLength<float> >= 4)
	{
		if(arr.End - arr.Begin >= 4)
		{
			auto mini = SimdLoad<4>(arr.Begin);
			auto maxi = mini;
			const size_t misalignment = size_t(arr.Begin) & 12;
			arr.Begin += misalignment? 4 - (misalignment >> 2): 4;
			while(arr.End - arr.Begin >= 4)
			{
				const auto v = SimdLoadAligned<4>(arr.Begin);
				mini = SimdSelect(mini, v, mini > v);
				maxi = SimdSelect(maxi, v, maxi < v);
				arr.Begin += 4;
			}
			alignas(16) float minis[4];
			alignas(16) float maxis[4];
			SimdStoreAligned(minis, mini);
			SimdStoreAligned(maxis, maxi);
			minimax.first = Funal::Min(Funal::Min(minis[0], minis[1]), Funal::Min(minis[2], minis[3]));
			minimax.second = Funal::Max(Funal::Max(maxis[0], maxis[1]), Funal::Max(maxis[2], maxis[3]));
		}
		else minimax.first = minimax.second = *arr.Begin++;
	}
	else minimax.first = minimax.second = *arr.Begin++;
	while(arr.Begin < arr.End)
	{
		if(*arr.Begin < minimax.first) minimax.first = *arr.Begin;
		if(minimax.second < *arr.Begin) minimax.second = *arr.Begin;
		arr.Begin++;
	}
	return minimax;
}

}}
