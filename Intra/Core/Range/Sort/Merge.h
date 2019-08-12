﻿#pragma once

#include "Funal/Op.h"

#include "Core/CArray.h"
#include "Core/Range/Concepts.h"

#include "Container/ForwardDecls.h"

#include "Core/Range/Mutation/Copy.h"

INTRA_BEGIN
inline namespace Range {

namespace D {

/*
 Сортирует массив, используя рекурсивную сортировку слиянием
 up - указатель на массив, который нужно сортировать
 down - указатель на массив с, как минимум, таким же размером как у 'up', используется как буфер
 left - левая граница массива, передайте 0, чтобы сортировать массив с начала
 right - правая граница массива, передайте длину массива - 1, чтобы сортировать массив до последнего элемента
 возвращает: указатель на отсортированный массив. Из-за особенностей работы данной реализации
 отсортированная версия массива может оказаться либо в 'up', либо в 'down'
*/
template<typename T, typename C> INTRA_CONSTEXPR2 T* merge_sort_pass(T* up, T* down, size_t left, size_t right, C comparer)
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
template<typename R, typename C = Funal::TLess> Requires<
	!CConst<TRefArrayElementRequire<R>>
> MergeSort(R&& range, C comparer = Less)
{
	Array<TValueTypeOf<R>> temp;
	temp.SetCountUninitialized(LengthOf(range));
	auto resultPtr = D::merge_sort_pass(DataOf(range), DataOf(temp), 0, LengthOf(range)-1, comparer);
	if(resultPtr == DataOf(range)) return;
	CopyTo(temp.AsConstRange(), range);
}

}
INTRA_END
