#pragma once

#include "Intra/Container/Tuple.h"
#include "Intra/Functional.h"
#include "Intra/Range/Span.h"
#include "Intra/Concepts.h"
#include "Intra/Range/Concepts.h"
#include "Intra/Range/Sort/Insertion.h"
#include "Intra/Range/Sort/Heap.h"

INTRA_BEGIN
namespace z_D {

template<typename T, typename C> void med3(T* first, T* mid, T* last, C comparer)
{
	// sort median of three elements to middle
	if(comparer(*mid, *first))
		Swap(*mid, *first);
	if(!comparer(*last, *mid)) return;
		
	// swap middle and last, then test first again
	Swap(*last, *mid);
	if(comparer(*mid, *first))
		Swap(*mid, *first);
}

template<typename T, typename C> void median(T* first, T* mid, T* last, C comparer)
{
	// sort median element to middle
	if(last - first <= 40)
	{
		med3(first, mid, last, comparer);
		return;
	}

	// median of nine
	const size_t step = size_t(last - first + 1)/8;
	med3(first, first + step, first + 2*step, comparer);
	med3(mid - step, mid, mid + step, comparer);
	med3(last - 2*step, last - step, last, comparer);
	med3(first + step, mid, last - step, comparer);
}

template<typename T, typename C> Tuple<T*, T*> unguarded_partition(Span<T> range, C comparer)
{
	// partition [_First, _Last), using _Pred
	T* const mid = range.Data() + range.Length()/2;
	median(range.Begin, mid, range.End - 1, comparer);
	T* pfirst = mid;
	T* pend = pfirst + 1;

	while(range.Begin < pfirst &&
		!comparer(*(pfirst - 1), *pfirst) &&
		!comparer(*pfirst, *(pfirst - 1))) pfirst--;
	while(pend < range.End &&
		!comparer(*pend, *pfirst) &&
		!comparer(*pfirst, *pend)) ++pend;

	T* gfirst = pend;
	T* gend = pfirst;

	for(;;)
	{
		// partition
		for(; gfirst < range.End; ++gfirst)
		{
			if(comparer(*pfirst, *gfirst)) continue;
			if(comparer(*gfirst, *pfirst)) break;
			if(pend++ == gfirst) continue;
			Swap(*(pend-1), *gfirst);
		}

		for(; range.Begin < gend; --gend)
		{
			if(comparer(*(gend - 1), *pfirst)) continue;
			if(comparer(*pfirst, *(gend - 1))) break;
			if(--pfirst == gend - 1) continue;
			Swap(*pfirst, *(gend - 1));
		}

		if(gend == range.Begin && gfirst == range.End)
			return {pfirst, pend};

		if(gend == range.Begin)
		{
			// no room at bottom, rotate pivot upward
			if(pend != gfirst) Swap(*pfirst, *pend);
			++pend;
			Swap(*pfirst++, *gfirst++);
			continue;
		}
		if(gfirst == range.End)
		{
			// no room at top, rotate pivot downward
			if(--gend != --pfirst)
				Swap(*gend, *pfirst);
			Swap(*pfirst, *--pend);
			continue;
		}
		Swap(*gfirst++, *--gend);
	}
}

template<typename T, typename C> void sort_pass(Span<T> range,
	index_t ideal, C comparer, size_t insertionSortThreshold = 32)
{
	size_t count;
	while(range.Length() > insertionSortThreshold && ideal > 0)
	{
		count = range.Length();

		// divide and conquer by quicksort
		Tuple<T*, T*> mid = unguarded_partition(range, comparer);
		ideal /= 2, ideal += ideal/2;	// allow 1.5 log2(N) divisions

		if(get<0>(mid) - range.Begin < range.End - get<1>(mid))
		{
			// loop on second Half
			sort_pass(Span<T>::FromPointerRange(range.Begin, get<0>(mid)), ideal, comparer);
			range.Begin = get<1>(mid);
			continue;
		}

		// loop on first Half
		sort_pass(Span<T>::FromPointerRange(get<1>(mid), range.End), ideal, comparer);
		range.End = get<0>(mid);
	}
	count = range.Length();

	if(count > insertionSortThreshold)
	{
		HeapSort(range, comparer);
		return;
	}
	if(count >= 2) InsertionSort(range, comparer);
}

}


template<typename C, typename R> Requires<
	CAssignableArrayClass<R>
> QuickSort(R&& range, C comparer)
{
	auto arr = SpanOf(range);
	z_D::sort_pass(arr, index_t(arr.Length()), comparer);
}

template<typename R> Requires<
	CAssignableArrayClass<R>
> QuickSort(R&& range)
{
	return QuickSort<TFLess>(ForwardAsRange<R>(range), FLess);
}
INTRA_END
