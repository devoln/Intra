#pragma once

#include "Range/ForwardDecls.h"
#include "Range/Concepts.h"
#include "Algo/Search/Single.h"
#include "Algo/Search/Subrange.h"
#include "Take.h"

namespace Intra { namespace Range {

//TODO: ����������� ����� TakeUntilAnyResult ��� InputRange


//! ��������� ���������� ��������, �������������� ������� ��������� ������ ���������
//! �� ��������� ������������� subranges � ���� ��������.
//! ������ ����� ��������� ��������� � ���������� ������������ ��� �����������
//! � ������ � ������, ����� �� ���� ����������� �� ������.
//! \param subranges[inout] �������� ������� �������������.
//! ����� ������ ���� ������� ������ subranges ��������� � ������� ���������� ��������.
//! ���� ���������� �� �������, subranges �������� � �������� ���������.
//! \param oSubrangeIndex[out] ��������� �� ����������, � ������� ����� ������� ������ ����������
//! �������� � ��������� subranges. ���� ������� �� ��� ������, ����� �������� �������� subranges.Count().
//! \return ���������� �������� ����������� ���������.
template<typename R, typename RWs> Meta::EnableIf<
	IsFiniteForwardRange<RWs>::_ && !Meta::IsConst<R>::_ &&
	IsFiniteForwardRange<ValueTypeOf<RWs>>::_ && !Meta::IsConst<RWs>::_,
ResultOfTake<R>> TakeUntilAdvanceAnyAdvance(R&& range,
	RWs&& subranges, size_t* ioIndex, size_t* oSubrangeIndex=null)
{
	auto rangeCopy = range;
	size_t index = Algo::CountUntilAdvanceAnyAdvance(range, subranges, oSubrangeIndex);
	if(ioIndex!=null) *ioIndex += index;
	return Take(rangeCopy, index);
}

//! ��������������� ������� �������� ��������� �� ��� ���, ���� ��:
//! 1) ���������� �������, ������ ������ �� �������� �� whats
//! 2) ����� ��������� ����� ���������.
//! ���������� �������� ���������� ���������.
template<class R, class Ws> forceinline Meta::EnableIf<
	IsFiniteForwardRange<R>::_ && !Meta::IsConst<R>::_ &&
	IsFiniteForwardRange<Ws>::_ &&
	ValueTypeIsConvertible<Ws, ValueTypeOf<R>>::_,
ResultOfTake<R>> TakeUntilAdvanceAny(R&& range, const Ws& whats, size_t* ioIndex=null, size_t* oWhatIndex=null)
{
	auto rangeCopy = range;
	size_t index = Algo::CountUntilAdvanceAny(Meta::Forward<R>(range), whats, oWhatIndex);
	if(ioIndex!=null) *ioIndex += index;
	return Take(rangeCopy, index);
}

//! ��������������� ������������� �������� ��������� �� ��� ���, ���� ��:
//! 1) ���������� �������, ������ ������ �� �������� �� whats
//! 2) ����� ��������� ����� ���������.
//! ���������� �������� ���������� ���������.
template<class R, class Ws> forceinline Meta::EnableIf<
	IsFiniteForwardRange<R>::_ &&
	IsFiniteForwardRange<Ws>::_ &&
	ValueTypeIsConvertible<Ws, ValueTypeOf<R>>::_,
ResultOfTake<R>> TakeUntilAny(R&& range, const Ws& whats,
	size_t* ioIndex=null, size_t* oWhatIndex=null)
{
	size_t index = Algo::CountUntilAny(Meta::Forward<R>(range), whats, oWhatIndex);
	if(ioIndex!=null) *ioIndex += index;
	return Take(range, index);
}


//! ��������� ���������� ��������, �������������� ������� ��������� ������ ��������� �� ��������� ������������� subranges � ���� ��������.
//! ������ ����� ��������� ��������� � ���������� ������������ ��� �����������
//! � ������ � ������, ����� �� ���� ����������� �� ������.
//! \param subranges[inout] �������� ������� �������������.
//! ����� ������ ���� ������� ������ subranges ��������� � ������� ���������� ��������.
//! ���� ���������� �� �������, subranges �������� � �������� ���������.
//! \param oSubrangeIndex[out] ��������� �� ����������, � ������� ����� ������� ������ ����������
//! �������� � ��������� subranges. ���� ������� �� ��� ������, ����� �������� �������� subranges.Count().
//! \return ���������� �������� ����������� ���������.
template<class R, class RWs> Meta::EnableIf<
	IsFiniteForwardRange<R>::_ && !Meta::IsConst<RWs>::_ &&
	IsFiniteForwardRange<RWs>::_ &&
	IsFiniteForwardRange<ValueTypeOf<RWs>>::_,
ResultOfTake<R>> TakeUntilAdvanceAny(R&& range,
	const RWs& subranges, size_t* ioIndex=null, size_t* oSubrangeIndex=null)
{
	auto rangeCopy = range;
	size_t index = Algo::CountUntilAdvanceAny(range, subranges, oSubrangeIndex);
	if(ioIndex!=null) *ioIndex += index;
	return Take(rangeCopy, index);
}

//! ��������� ���������� ��������, �������������� ������� ��������� ������ ���������
//! �� ��������� ������������� subranges � ���� ��������.
//! \param subranges[inout] �������� ������� �������������.
//! ����� ������ ���� ������� ������ subranges ��������� � ������� ���������� ��������.
//! ���� ���������� �� �������, subranges ��������� � �������� ���������.
//! \param oSubrangeIndex[out] ��������� �� ����������, � ������� ����� ������� ������ ���������� �������� � ��������� subranges. ���� ������� �� ��� ������, ����� �������� �������� subranges.Count().
//! \return ���������� �������� ����������� ���������.
template<class R, class RWs> Meta::EnableIf<
	IsFiniteForwardRange<R>::_ &&
	IsFiniteForwardRange<RWs>::_ && !Meta::IsConst<RWs>::_ &&
	IsFiniteForwardRange<ValueTypeOf<RWs>>::_,
ResultOfTake<R>> TakeUntilAnyAdvance(const R& range,
	RWs&& subranges, size_t* ioIndex=null, size_t* oSubrangeIndex=null)
{
	size_t index = Algo::CountUntilAnyAdvance(range, subranges, oSubrangeIndex);
	if(ioIndex!=null) *ioIndex += index;
	return Take(range, index);
}

//! ��������� ���������� ��������, �������������� ������� ��������� ������ ���������
//! �� ��������� ������������� subranges � ���� ��������.
//! \param subranges[inout] �������� ������� �������������.
//! ����� ������ ���� ������� ������ subranges ��������� � ������� ���������� ��������.
//! ���� ���������� �� �������, subranges ��������� � �������� ���������.
//! \param oSubrangeIndex[out] ��������� �� ����������, � ������� ����� ������� ������ ���������� �������� � ��������� subranges. ���� ������� �� ��� ������, ����� �������� �������� subranges.Count().
//! \return ���������� �������� ����������� ���������.
template<class R, class RWs> forceinline Meta::EnableIf<
	IsFiniteForwardRange<R>::_ &&
	IsFiniteForwardRange<RWs>::_ &&
	IsFiniteForwardRange<ValueTypeOf<RWs>>::_,
ResultOfTake<R>> TakeUntilAny(const R& range, const RWs& subranges,
	size_t* ioIndex=null, size_t* oSubrangeIndex=null)
{
	const size_t index = CountUntilAny(range, subranges, oSubrangeIndex);
	if(ioIndex!=null) *ioIndex += index;
	return Take(range, index);
}

}}
