#pragma once

#include "Algo/Op.h"
#include "Range/Concepts.h"

namespace Intra { namespace Algo {

namespace D {

template<typename T, typename C> void heap_shift_down(T arr[], size_t i, size_t j, C comparer)
{
	while(i*2+1<j)
	{
		size_t maxNodeId = i*2+2;
		if(i*2+1==j-1 || comparer(arr[i*2+2], arr[i*2+1])) maxNodeId--;

		if(!comparer(arr[i], arr[maxNodeId])) break;

		Meta::Swap(arr[i], arr[maxNodeId]);
		i = maxNodeId;
	}
}

}

//! ������������� ���������� ������� array � ���������� ��������� comparer.
//! �������������� ���������:
//! - ��������������� ���������: O(n Log n);
//! - ����������;
//! - �� ����� ��������������� �������� �������� ��� �� �����, ��� � ��� ��������� ������;
//! - ��� N ������ ���������� ����� ShellSort �������.
template<typename ArrRange, typename C = Comparers::Function<Range::ValueTypeOf<ArrRange>>> Meta::EnableIf<
	Range::IsArrayRange<ArrRange>::_ &&
	Range::IsAssignableInputRange<ArrRange>::_
> HeapSort(const ArrRange& range, C comparer = Op::Less<Range::ValueTypeOf<ArrRange>>)
{
	const size_t count = range.Length();

	//������ ������ ������
	for(size_t i=count/2; i>0; i--)
		D::heap_shift_down(range.Data(), i-1, count, comparer);

	//�������� ������������ (0) ������� ������ � i-� �������
	//���������� ����� 0 ������� �� ���������� ������� � ������
	for(size_t i=count-1; i>0; i--)
	{
		Meta::Swap(range[0], range[i]);
		D::heap_shift_down(range.Data(), 0, i, comparer);
	}
}

}}
