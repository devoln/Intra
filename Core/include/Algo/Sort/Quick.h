#pragma once

#include "Algo/Op.h"
#include "Algo/Sort/Insertion.h"
#include "Algo/Sort/Heap.h"
#include "Range/Concepts.h"
#include "Range/Generators/Span.h"
#include "Meta/Pair.h"

namespace Intra { namespace Algo {

using namespace Range::Concepts;

template<typename C, typename R, typename AsR=AsRangeResult<R>> Meta::EnableIf<
	IsArrayRange<AsR>::_ &&
	IsAssignableRange<AsR>::_
> QuickSort(R&& range, C comparer);


template<typename R, typename AsR=AsRangeResult<R>> Meta::EnableIf<
	IsArrayRange<AsR>::_ &&
	IsAssignableRange<AsR>::_
> QuickSort(R&& range)
{
	return QuickSort<Comparers::Function<ValueTypeOf<AsR>>>(
		Range::Forward<R>(range), Op::Less<ValueTypeOf<AsR>>);
}

namespace D {

template<typename T, typename C> void med3(T* first, T* mid, T* last, C comparer)
{
	// sort median of three elements to middle
	if(comparer(*mid, *first))
		Meta::Swap(*mid, *first);
	if(!comparer(*last, *mid)) return;
		
	// swap middle and last, then test first again
	Meta::Swap(*last, *mid);
	if(comparer(*mid, *first))
		Meta::Swap(*mid, *first);
}

template<typename T, typename C> void median(T* first, T* mid, T* last, C comparer)
{
	// sort median element to middle
	if(last-first <= 40)
	{
		med3(first, mid, last, comparer);
		return;
	}

	// median of nine
	const size_t step = size_t(last-first+1)/8;
	med3(first, first+step, first+2*step, comparer);
	med3(mid-step, mid, mid+step, comparer);
	med3(last-2*step, last-step, last, comparer);
	med3(first+step, mid, last-step, comparer);
}

template<typename T, typename C> Pair<T*, T*> unguarded_partition(Span<T> range, C comparer)
{
	// partition [_First, _Last), using _Pred
	T* mid = range.Data()+range.Length()/2;
	median(range.Begin, mid, range.End-1, comparer);
	T* pfirst = mid;
	T* pend = pfirst + 1;

	while(range.Begin<pfirst && !comparer(*(pfirst-1), *pfirst) && !comparer(*pfirst, *(pfirst-1))) pfirst--;
	while(pend<range.End && !comparer(*pend, *pfirst) && !comparer(*pfirst, *pend)) ++pend;

	T* gfirst = pend;
	T* gend = pfirst;

	for(;;)
	{
		// partition
		for(; gfirst<range.End; ++gfirst)
		{
			if(comparer(*pfirst, *gfirst)) continue;
			if(comparer(*gfirst, *pfirst)) break;
			if(pend++ == gfirst) continue;
			Meta::Swap(*(pend-1), *gfirst);
		}

		for(; range.Begin<gend; --gend)
		{
			if(comparer(*(gend-1), *pfirst)) continue;
			if(comparer(*pfirst, *(gend-1))) break;
			if(--pfirst == gend-1) continue;
			Meta::Swap(*pfirst, *(gend-1));
		}

		if(gend==range.Begin && gfirst==range.End)
			return {pfirst, pend};

		if(gend==range.Begin)
		{
			// no room at bottom, rotate pivot upward
			if(pend!=gfirst) Meta::Swap(*pfirst, *pend);
			++pend;
			Meta::Swap(*pfirst++, *gfirst++);
			continue;
		}
		if(gfirst==range.End)
		{
			// no room at top, rotate pivot downward
			if(--gend != --pfirst)
				Meta::Swap(*gend, *pfirst);
			Meta::Swap(*pfirst, *--pend);
			continue;
		}
		Meta::Swap(*gfirst++, *--gend);
	}
}

template<typename T, typename C> void sort_pass(Span<T> range,
	intptr ideal, C comparer, size_t insertionSortThreshold=32)
{
	size_t count;
	while(range.Length()>insertionSortThreshold && ideal>0)
	{
		count = range.Length();

		// divide and conquer by quicksort
		Meta::Pair<T*, T*> mid = unguarded_partition(range, comparer);
		ideal /= 2, ideal += ideal/2;	// allow 1.5 log2(N) divisions

		if(mid.first-range.Begin < range.End-mid.second)
		{
			// loop on second Half
			sort_pass(Span<T>(range.Begin, mid.first), ideal, comparer);
			range.Begin = mid.second;
			continue;
		}

		// loop on first Half
		sort_pass(Span<T>(mid.second, range.End), ideal, comparer);
		range.End = mid.first;
	}
	count = range.Length();

	if(count>insertionSortThreshold)
	{
		HeapSort(range, comparer);
		return;
	}
	if(count>=2) InsertionSort(range, comparer);
}

}


template<typename C, typename R, typename AsR> Meta::EnableIf<
	IsArrayRange<AsR>::_ &&
	IsAssignableRange<AsR>::_
> QuickSort(R&& range, C comparer)
{
	auto rangeCopy = Range::Forward<R>(range);
	Span<ValueTypeOf<AsR>> arr(rangeCopy.Data(), rangeCopy.Length());
	D::sort_pass(arr, intptr(rangeCopy.Length()), comparer);
}

}}

