#pragma once

#include "Cpp/Warnings.h"

#include "Meta/Pair.h"

#include "Utils/Op.h"
#include "Utils/Span.h"

#include "Concepts/Array.h"
#include "Concepts/Range.h"

#include "Range/Sort/Insertion.h"
#include "Range/Sort/Heap.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Range {

namespace D {

template<typename T, typename C> void med3(T* first, T* mid, T* last, C comparer)
{
	// sort median of three elements to middle
	if(comparer(*mid, *first))
		Cpp::Swap(*mid, *first);
	if(!comparer(*last, *mid)) return;
		
	// swap middle and last, then test first again
	Cpp::Swap(*last, *mid);
	if(comparer(*mid, *first))
		Cpp::Swap(*mid, *first);
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
	T* const mid = range.Data()+range.Length()/2;
	median(range.Begin, mid, range.End-1, comparer);
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
		for(; gfirst<range.End; ++gfirst)
		{
			if(comparer(*pfirst, *gfirst)) continue;
			if(comparer(*gfirst, *pfirst)) break;
			if(pend++ == gfirst) continue;
			Cpp::Swap(*(pend-1), *gfirst);
		}

		for(; range.Begin<gend; --gend)
		{
			if(comparer(*(gend-1), *pfirst)) continue;
			if(comparer(*pfirst, *(gend-1))) break;
			if(--pfirst == gend-1) continue;
			Cpp::Swap(*pfirst, *(gend-1));
		}

		if(gend==range.Begin && gfirst==range.End)
			return {pfirst, pend};

		if(gend==range.Begin)
		{
			// no room at bottom, rotate pivot upward
			if(pend!=gfirst) Cpp::Swap(*pfirst, *pend);
			++pend;
			Cpp::Swap(*pfirst++, *gfirst++);
			continue;
		}
		if(gfirst==range.End)
		{
			// no room at top, rotate pivot downward
			if(--gend != --pfirst)
				Cpp::Swap(*gend, *pfirst);
			Cpp::Swap(*pfirst, *--pend);
			continue;
		}
		Cpp::Swap(*gfirst++, *--gend);
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


template<typename C, typename R> Meta::EnableIf<
	Concepts::IsAssignableArrayClass<R>::_
> QuickSort(R&& range, C comparer)
{
	auto arr = SpanOf(range);
	D::sort_pass(arr, intptr(arr.Length()), comparer);
}

template<typename R> Meta::EnableIf<
	Concepts::IsAssignableArrayClass<R>::_
> QuickSort(R&& range)
{
	return QuickSort<Comparers::Function<Concepts::ElementTypeOfArray<R>>>(
		Range::Forward<R>(range), Op::Less<Concepts::ElementTypeOfArray<R>>);
}

}}

INTRA_WARNING_POP
