#pragma once

#include "Algo/Op.h"
#include "Algo/Sort/Insertion.h"
#include "Range/Concepts.h"
#include "Range/ArrayRange.h"

namespace Intra { namespace Algo {

template<typename C, typename ArrRange> Meta::EnableIf<
	Range::IsArrayRange<ArrRange>::_ &&
	Range::IsAssignableInputRange<ArrRange>::_
> QuickSort(const ArrRange& range, C comparer);


template<typename ArrRange> Meta::EnableIf<
	Range::IsArrayRange<ArrRange>::_ &&
	Range::IsAssignableInputRange<ArrRange>::_
> QuickSort(const ArrRange& range)
{
	return QuickSort<Comparers::Function<Range::ValueTypeOf<ArrRange>>>(
		range, Op::Less<Range::ValueTypeOf<ArrRange>>);
}

namespace D
{
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

	template<typename T, typename C> Meta::Pair<T*, T*> unguarded_partition(ArrayRange<T> range, C comparer)
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

	template<typename T, typename C> void sort_pass(ArrayRange<T> range, intptr ideal, C comparer, size_t insertionSortThreshold=32)
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
				sort_pass(ArrayRange<T>(range.Begin, mid.first), ideal, comparer);
				range.Begin = mid.second;
				continue;
			}

			// loop on first Half
			sort_pass(ArrayRange<T>(mid.second, range.End), ideal, comparer);
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


template<typename C, typename ArrRange> Meta::EnableIf<
	Range::IsArrayRange<ArrRange>::_ &&
	Range::IsAssignableInputRange<ArrRange>::_
> QuickSort(const ArrRange& range, C comparer)
{
	D::sort_pass(ArrayRange<Range::ValueTypeOf<ArrRange>>(range.Data(), range.Length()), intptr(range.Length()), comparer);
}

}}

