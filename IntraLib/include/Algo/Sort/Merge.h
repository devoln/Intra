#pragma once

#include "Algo/Op.h"
#include "Range/Concepts.h"
#include "Containers/ForwardDeclarations.h"

namespace Intra { namespace Algo {

namespace D {

/**
* Сортирует массив, используя рекурсивную сортировку слиянием
* up - указатель на массив, который нужно сортировать
* down - указатель на массив с, как минимум, таким же размером как у 'up', используется как буфер
* left - левая граница массива, передайте 0, чтобы сортировать массив с начала
* right - правая граница массива, передайте длину массива - 1, чтобы сортировать массив до последнего элемента
* возвращает: указатель на отсортированный массив. Из-за особенностей работы данной реализации
* отсортированная версия массива может оказаться либо в 'up', либо в 'down'
**/
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

//! Сортировка слиянием
//! Особенности алгоритма:
//! - Лучшее, среднее и худшее время: O(n Log n);
//! - На «почти отсортированных» массивах работает столь же долго, как на хаотичных;
//! - Требует дополнительной памяти по размеру исходного массива.
template<typename ArrRange, typename C = Comparers::Function<Range::ValueTypeOf<ArrRange>>> Meta::EnableIf<
	Range::IsArrayRange<ArrRange>::_ &&
	Range::IsAssignableInputRange<ArrRange>::_
> MergeSort(const ArrRange& range, C comparer = Op::Less<Range::ValueTypeOf<ArrRange>>)
{
	Array<Range::ValueTypeOf<ArrRange>> temp;
	temp.SetCountUninitialized(range.Length());
	auto resultPtr = D::merge_sort_pass(range.Data(), temp.Data(), 0, range.Length()-1, comparer);
	if(resultPtr==range.Data()) return;
	Algo::CopyTo(temp.AsConstRange(), range);
}

}}

