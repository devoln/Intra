#pragma once

#include "Intra/Functional.h"
#include "Intra/Concepts.h"
#include "Intra/Range.h"

#include "IntraX/Container/ForwardDecls.h"

namespace Intra { INTRA_BEGIN
namespace z_D {

template<typename T, typename C> struct MergeSorter
{
	T* SourceBuffer;
	T* ExtraBuffer;
	INTRA_NO_UNIQUE_ADDRESS C Comparer;

	constexpr T* operator()(size_t left, size_t right)
	{
		if(left == right)
		{
			ExtraBuffer[left] = SourceBuffer[left];
			return ExtraBuffer;
		}
		const auto middle = (left + right) / 2;
		const auto lBuff = operator()(left, middle);
		const auto rBuff = operator()(middle + 1, right);

		const auto target = lBuff == SourceBuffer? ExtraBuffer: SourceBuffer;
		auto lCur = left, rCur = middle + 1;
		for(size_t i = left; i <= right; i++)
		{
			if(lCur <= middle && rCur <= right)
				target[i] = Comparer(lBuff[lCur], rBuff[rCur])? lBuff[lCur++]: rBuff[rCur++];
			else target[i] = lCur <= middle? lBuff[lCur++]: rBuff[rCur++];
		}
		return target;
	}
};

}

/** Sort ``range`` using merge sort algorithm and using ``comparer`` predicate.
  1) The best, average and worst time are O(n Log n);
  2) On almost sorted range it is as slow as on randomly ordered range;
  3) Allocates dynamic memory. Size of allocation equals ``range`` length.
*/
template<CConvertibleToSpan R, typename C = decltype(Less)> requires (!CConst<TArrayElementKeepConst<R>>)
void MergeSort(R&& range, C comparer = Less)
{
	Array<TRangeValue<R>> temp;
	temp.SetCountUninitialized(LengthOf(range));
	auto resultPtr = z_D::MergeSortContext{Data(range), Data(temp), comparer}(0, Length(range) - 1);
	if(resultPtr == Data(range)) return;
	CopyTo(range).From(temp);
}
} INTRA_END
