#pragma once

#include "Memory/Memory.h"

namespace Intra { namespace Algo {

namespace detail
{
	template<typename T, typename ExtractKeyFunc, size_t RadixLog> void radixSortPass(
		ArrayRange<T> arr, ArrayRange<T> temp, size_t radixOffset, ExtractKeyFunc extractKey)
	{
		enum: size_t {Radix = 1 << RadixLog, RadixMask = Radix-1};
		const size_t count = arr.Count();

		int c[Radix] = {0};
		for(size_t j = 0; j<count; j++)
		{
			const auto key = extractKey(arr[j]);
			const size_t keyRadix = (key >> radixOffset) & RadixMask;
			c[keyRadix]++;
		}
		for(size_t j=1; j<Radix; j++) c[j] += c[j-1];
		for(size_t j = count-1; j!=Meta::NumericLimits<size_t>::Max(); j--)
		{
			const auto key = extractKey(arr[j]);
			const size_t keyRadix = (key >> radixOffset) & RadixMask;
			const size_t tempIndex = --c[keyRadix];
			temp[tempIndex] = arr[j];
		}
	}
}

template<typename T, typename ExtractKeyFunc, size_t RadixLog> void RadixSort(ArrayRange<T> arr, ExtractKeyFunc extractKey)
{
	enum: size_t {KeyBits = sizeof(extractKey(Meta::Val<T>()))*8};

	Array<T> tempBuffer;
	tempBuffer.SetCountUninitialized(arr.Count());
	ArrayRange<T> dst = arr, temp = tempBuffer;
	size_t shift = 0;
	for(size_t i=0; i<KeyBits/RadixLog; i++)
	{
		detail::radixSortPass<T, ExtractKeyFunc, RadixLog>(dst, temp, shift, extractKey);
		shift += RadixLog;
		core::swap(dst, temp);
	}
	if(shift<KeyBits)
	{
		detail::radixSortPass<T, ExtractKeyFunc, KeyBits % RadixLog>(dst, temp, shift, extractKey);
		core::swap(dst, temp);
	}
	if(arr==dst) return;
	temp.CopyTo(arr);
}




namespace detail
{
#if INTRA_DISABLED
	template<typename T, typename C> void q_sort_pass(T* a, size_t left, size_t right, C comparer)
	{
		if(left>=right) return;

		size_t i = left, j = right+1;
		T pivot = a[left];
		for(;;)
		{
			do i++; while(i<=right && comparer(a[i], pivot));
			do j--; while(j>=left && comparer(pivot, a[j]));
			if(i>=j) break;
			core::swap(a[i], a[j]);
		}

		a[left] = a[j];
		a[j] = pivot;

		q_sort_pass(a, left, j-1, comparer);
		q_sort_pass(a, j+1, right, comparer);
	}
#endif

	template<typename T, typename C> void med3(T* first, T* mid, T* last, C comparer)
	{
		// sort median of three elements to middle
		if(comparer(*mid, *first))
			core::swap(*mid, *first);
		if(!comparer(*last, *mid)) return;
		
		// swap middle and last, then test first again
		core::swap(*last, *mid);
		if(comparer(*mid, *first))
			core::swap(*mid, *first);
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
		const size_t step = (last-first+1)/8;
		med3(first, first+step, first+2*step, comparer);
		med3(mid-step, mid, mid+step, comparer);
		med3(last-2*step, last-step, last, comparer);
		med3(first+step, mid, last-step, comparer);
	}

	template<typename T, typename C> core::pair<T*, T*> unguarded_partition(ArrayRange<T> range, C comparer)
	{
		// partition [_First, _Last), using _Pred
		T* mid = range.Data()+range.Count()/2;
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
				core::swap(*(pend-1), *gfirst);
			}

			for(; range.Begin<gend; --gend)
			{
				if(comparer(*(gend-1), *pfirst)) continue;
				if(comparer(*pfirst, *(gend-1))) break;
				if(--pfirst == gend-1) continue;
				core::swap(*pfirst, *(gend-1));
			}

			if(gend==range.Begin && gfirst==range.End)
				return {pfirst, pend};

			if(gend==range.Begin)
			{
				// no room at bottom, rotate pivot upward
				if(pend!=gfirst) core::swap(*pfirst, *pend);
				++pend;
				core::swap(*pfirst++, *gfirst++);
				continue;
			}
			if(gfirst==range.End)
			{
				// no room at top, rotate pivot downward
				if(--gend != --pfirst)
					core::swap(*gend, *pfirst);
				core::swap(*pfirst, *--pend);
				continue;
			}
			core::swap(*gfirst++, *--gend);
		}
	}

	template<typename T, typename C> void sort_pass(ArrayRange<T> range, intptr ideal, C comparer, size_t insertionSortThreshold=32)
	{
		size_t count;
		while(range.Length()>insertionSortThreshold && ideal>0)
		{
			count = range.Count();

			// divide and conquer by quicksort
			core::pair<T*, T*> mid = unguarded_partition(range, comparer);
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
		count = range.Count();

		if(count>insertionSortThreshold)
		{
			HeapSort(range, comparer);
			return;
		}
		if(count>=2) InsertionSort(range, comparer);
	}
}


template<typename ArrRange, typename C> Meta::EnableIf<
	Range::IsArrayRange<ArrRange>::_ &&
	Range::IsRangeElementAssignable<ArrRange>::_
> QuickSort(const ArrRange& range, C comparer)
{
	detail::sort_pass(ArrayRange<typename ArrRange::value_type>(range.Data(), range.Length()), range.Length(), comparer);
}





namespace detail
{
	template<typename T, typename C> void heap_shift_down(T arr[], size_t i, size_t j, C comparer)
	{
		while(i*2+1<j)
		{
			size_t maxNodeId = i*2+2;
			if(i*2+1==j-1 || comparer(arr[i*2+2], arr[i*2+1])) maxNodeId--;

			if(!comparer(arr[i], arr[maxNodeId])) break;

			core::swap(arr[i], arr[maxNodeId]);
			i = maxNodeId;
		}
	}
}


template<typename ArrRange, typename C> Meta::EnableIf<
	Range::IsArrayRange<ArrRange>::_ &&
	Range::IsRangeElementAssignable<ArrRange>::_
> HeapSort(const ArrRange& range, C comparer)
{
	const size_t count = range.Count();

	//Строим дерево поиска
	for(size_t i=count/2; i>0; i--)
		detail::heap_shift_down(range.Data(), i-1, count, comparer);

	//Забираем максимальный (0) элемент дерева в i-ю позицию
	//Перемещаем новый 0 элемент на правильную позицию в дереве
	for(size_t i=count-1; i>0; i--)
	{
		core::swap(range[0], range[i]);
		detail::heap_shift_down(range.Data(), 0, i, comparer);
	}
}



/**
* Сортирует массив, используя рекурсивную сортировку слиянием
* up - указатель на массив, который нужно сортировать
* down - указатель на массив с, как минимум, таким же размером как у 'up', используется как буфер
* left - левая граница массива, передайте 0, чтобы сортировать массив с начала
* right - правая граница массива, передайте длину массива - 1, чтобы сортировать массив до последнего элемента
* возвращает: указатель на отсортированный массив. Из-за особенностей работы данной реализации
* отсортированная версия массива может оказаться либо в 'up', либо в 'down'
**/
namespace detail
{
	template<typename T, typename C> T* merge_sort_pass(T* up, T* down, size_t left, size_t right, C comparer)
	{
		if(left == right)
		{
			down[left] = up[left];
			return down;
		}

		size_t middle = (left+right)/2;

		T* l_buff = merge_sort_pass(up, down, left, middle, comparer);
		T* r_buff = merge_sort_pass(up, down, middle+1, right, comparer);

		// слияние двух отсортированных половин
		T* target = l_buff==up? down: up;

		//size_t width = right-left;
		size_t l_cur = left;
		size_t r_cur = middle + 1;
		for(size_t i=left; i<=right; i++)
		{
			if(l_cur<=middle && r_cur<=right)
			{
				if(comparer(l_buff[l_cur], r_buff[r_cur]))
				{
					target[i] = l_buff[l_cur++];
					continue;
				}
				target[i] = r_buff[r_cur++];
				continue;
			}
			if(l_cur <= middle)
			{
				target[i] = l_buff[l_cur++];
				continue;
			}

			target[i] = r_buff[r_cur++];
		}
		return target;
	}
}

template<typename ArrRange, typename C> Meta::EnableIf<
	Range::IsArrayRange<ArrRange>::_ &&
	Range::IsRangeElementAssignable<ArrRange>::_
> MergeSort(const ArrRange& range, C comparer)
{
	const size_t count = range.Count();
	Array<typename ArrRange::value_type> temp;
	temp.SetCount(count);
	auto resultPtr = detail::merge_sort_pass(range.Data(), temp.Data(), 0, count-1, comparer);
	if(resultPtr==range.Data()) return;
	Memory::CopyAssign(range, temp.AsConstRange());
}


}}
