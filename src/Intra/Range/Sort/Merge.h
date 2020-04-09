#pragma once

#include "Intra/Functional.h"
#include "Intra/Concepts.h"
#include "Intra/Range/Concepts.h"

#include "Extra/Container/ForwardDecls.h"

#include "Intra/Range/Mutation/Copy.h"

INTRA_BEGIN
namespace z_D {

/*
 Сортирует массив, используя рекурсивную сортировку слиянием
 up - указатель на массив, который нужно сортировать
 down - указатель на массив с, как минимум, таким же размером как у 'up', используется как буфер
 left - левая граница массива, передайте 0, чтобы сортировать массив с начала
 right - правая граница массива, передайте длину массива - 1, чтобы сортировать массив до последнего элемента
 возвращает: указатель на отсортированный массив. Из-за особенностей работы данной реализации
 отсортированная версия массива может оказаться либо в 'up', либо в 'down'
*/
template<typename T, typename C> constexpr T* merge_sort_pass(T* up, T* down, size_t left, size_t right, C comparer)
{
	if(left == right)
	{
		down[left] = up[left];
		return down;
	}

	size_t middle = (left+right)/2;

	T* const l_buff = merge_sort_pass(up, down, left, middle, comparer);
	T* const r_buff = merge_sort_pass(up, down, middle+1, right, comparer);

	// слияние двух отсортированных половин
	T* const target = l_buff==up? down: up;

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

/** Sort ``range`` using merge sort algorithm and using ``comparer`` predicate.
  1) The best, average and worst time are O(n Log n);
  2) On almost sorted range it is as slow as on randomly ordered range;
  3) Allocates dynamic memory. Size of allocation equals ``range`` length.
*/
template<typename R, typename C = TFLess> Requires<
	!CConst<TArrayElementRefRequired<R>>
> MergeSort(R&& range, C comparer = FLess)
{
	Array<TValueTypeOf<R>> temp;
	temp.SetCountUninitialized(LengthOf(range));
	auto resultPtr = z_D::merge_sort_pass(DataOf(range), DataOf(temp), 0, LengthOf(range)-1, comparer);
	if(resultPtr == DataOf(range)) return;
	CopyTo(temp.AsConstRange(), range);
}
INTRA_END
