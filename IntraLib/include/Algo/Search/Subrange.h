#pragma once

#include "Range/Concepts.h"
#include "Range/Construction/Take.h"
#include "Range/Iteration/Transversal.h"
#include "Single.h"
#include "Algo/Comparison.h"
#include "Platform/CppWarnings.h"

namespace Intra { namespace Algo {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

//! ����� ������ ��������� ��������� what � ���� ��������.
//! ������ ��������� ��������������� �� ������ ������� ��������� what ��� ��������� � ������, ���� �������� �� �������� what.
//! \param what ������� ��������.
//! \param ioIndex[inout] ��������� �� �������, ������� ������������� �� ���������� ���������, �������������� ��������� �������. ����� ���� null.
//! \returns ���������� ������ �� ����.
template<typename R, typename RW> Meta::EnableIf<
	Range::IsFiniteForwardRange<RW>::_ && !Meta::IsConst<R>::_ &&
	Range::ValueTypeIsConvertible<RW, Range::ValueTypeOf<R>>::_,
R&> FindAdvance(R&& range, const RW& what, size_t* ioIndex=null)
{
	while(!range.Empty() && !StartsWith(range, what))
	{
		range.PopFirst();
		if(ioIndex!=null) ++*ioIndex;
		FindAdvance(range, what.First(), ioIndex);
	}
	return range;
}


//! ����� ������ ��������� ��������� what � ���� ��������.
//! \param what ������� ��������.
//! \param ioIndex[inout] ��������� �� �������, ������� ������������� �� ����������
//! ���������, �������������� ��������� �������. ����� ���� null.
//! \returns null, ���� �������� �� �������. ����� ����� ���������, ������� � �������, �� ������� ���������� ������ ��������� what.
template<typename R, typename RW> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	Range::IsFiniteForwardRange<RW>::_ &&
	Range::ValueTypeIsConvertible<RW, Range::ValueTypeOf<R>>::_,
R> Find(const R& range, const RW& what, size_t* ioIndex=null)
{return FindAdvance(R(range), what, ioIndex);}

//! ����� ������ ��������� ������ ��������� �� ��������� ������������� subranges � ���� ��������.
//! ������ ����� ��������� ��������� � ���������� ������������ ��� �����������
//! � ������ � ������, ����� �� ���� �� ������������� �� ������.
//! \param subranges[inout] �������� ������� �������������.
//! ����� ������ ���� ������� ������ subranges ��������� � ������� ���������� ��������.
//! ���� ���������� �� �������, subranges �������� � �������� ���������.
//! \param ioIndex[inout] ��������� �� �������, ������� ������������� �� ����������
//! ���������, �������������� ��������� �������. ����� ���� null.
//! \param oSubrangeIndex[out] ��������� �� ����������, � ������� ����� ������� ������ ����������
//! �������� � ��������� whats. ���� ������� �� ��� ������, ����� �������� �������� whats.Count().
//! \return ���������� ������ �� ����.
template<typename R, typename RWs> Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ && !Meta::IsConst<R>::_ &&
	Range::IsFiniteForwardRange<RWs>::_ && !Meta::IsConst<RWs>::_ &&
	Range::IsFiniteForwardRange<Range::ValueTypeOf<RWs>>::_,
R&> FindAdvanceAnyAdvance(R&& range, RWs&& subranges, size_t* ioIndex=null, size_t* oSubrangeIndex=null)
{
	RWs subrangesCopy = subranges;
	while(!range.Empty() && !StartsWithAnyAdvance(range, subranges, oSubrangeIndex))
	{
		subranges = subrangesCopy;
		range.PopFirst();
		if(ioIndex!=null) ++*ioIndex;
		FindAdvanceAny(range, Range::FirstTransversal(subranges), ioIndex);
	}
	return range;
}

//! ����� ���������� ��������, �������������� ������� ��������� ������ ���������
//! �� ��������� ������������� subranges � ���� ��������.
//! ������ ����� ��������� ��������� � ���������� ������������ ��� �����������
//! � ������ � ������, ����� �� ���� ����������� �� ������.
//! \param subranges[inout] �������� ������� �������������.
//! ����� ������ ���� ������� ������ subranges ��������� � ������� ���������� ��������.
//! ���� ���������� �� �������, subranges �������� � �������� ���������.
//! \param oSubrangeIndex[out] ��������� �� ����������, � ������� ����� ������� ������ ����������
//! �������� � ��������� subranges. ���� ������� �� ��� ������, ����� �������� �������� subranges.Count().
//! \return ���������� ���������� ���������� ���������.
template<typename R, typename RWs> Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	Range::IsFiniteForwardRange<RWs>::_ &&
	Range::IsFiniteForwardRange<Range::ValueTypeOf<RWs>>::_,
size_t> CountUntilAdvanceAnyAdvance(RWs& subranges, size_t* oSubrangeIndex=null)
{
	size_t index = 0;
	FindAdvanceAnyAdvance(subranges, &index, oSubrangeIndex);
	return index;
}


//! ����� ������ ��������� ������ ��������� �� ��������� ������������� subranges � ���� ��������.
//! ������ ����� ��������� ��������� � ���������� �������� ��� ����������� � ������ � ������, ����� ������� �� ������.
//! \param subranges �������� ������� �������������.
//! \param ioIndex[inout] ��������� �� �������, ������� ������������� ��
//! ���������� ���������, �������������� ��������� �������. ����� ���� null.
//! \param oSubrangeIndex[out] ��������� �� ����������, � ������� ����� ������� ������ ����������
//! �������� � ��������� whats. ���� ������� �� ��� ������, ����� �������� �������� whats.Count().
//! \return ���������� ������ �� ����.
template<typename R, typename RWs> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	Range::IsFiniteForwardRange<RWs>::_ &&
	Range::IsFiniteForwardRange<Range::ValueTypeOf<RWs>>::_,
R&> FindAdvanceAny(R& range, const RWs& subranges, size_t* ioIndex=null, size_t* oSubrangeIndex=null)
{
	auto subrangesCopy = subranges;
	return FindAdvanceAnyAdvance(range, subrangesCopy, ioIndex, oSubrangeIndex);
}

//! ����� ���������� ��������, �������������� ������� ��������� ������ ��������� �� ��������� ������������� subranges � ���� ��������.
//! ������ ����� ��������� ��������� � ���������� ������������ ��� �����������
//! � ������ � ������, ����� �� ���� ����������� �� ������.
//! \param subranges[inout] �������� ������� �������������.
//! \param oSubrangeIndex[out] ��������� �� ����������, � ������� ����� ������� ������ ����������
//! �������� � ��������� subranges. ���� ������� �� ��� ������, ����� �������� �������� subranges.Count().
//! \return ���������� ���������� ���������� ���������.
template<typename R, typename RWs> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	Range::IsFiniteForwardRange<RWs>::_ &&
	Range::IsFiniteForwardRange<Range::ValueTypeOf<RWs>>::_,
size_t> CountUntilAdvanceAny(R& range, const RWs& subranges, size_t* oSubrangeIndex=null)
{
	size_t index = 0;
	FindAdvanceAny(range, subranges, &index, oSubrangeIndex);
	return index;
}



//! ����� ������ ��������� ������ ��������� �� ��������� ������������� subranges � ���� ��������.
//! \param subranges[inout] �������� ������� �������������.
//! ����� ������ ���� ������� ������ subranges ��������� � ������� ���������� ��������.
//! ���� ���������� �� �������, subranges �������� � �������� ���������.
//! \param ioIndex[inout] ��������� �� �������, ������� �������������
//! �� ���������� ���������, �������������� ��������� �������. ����� ���� null.
//! \param oSubrangeIndex[out] ��������� �� ����������, � ������� ����� ������� ������ ���������� �������� � ��������� whats. ���� ������� �� ��� ������, ����� �������� �������� whats.Count().
//! \return ���������� ��������, ���������� �� ����� ��������� ���� ��������� �� ������� ��������� ������ �� ������� ����������.
template<typename R, typename RWs> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	Range::IsFiniteForwardRange<RWs>::_ && !Meta::IsConst<RWs>::_ &&
	Range::IsFiniteForwardRange<Range::ValueTypeOf<RWs>>::_,
R> FindAnyAdvance(const R& range, RWs&& subranges, size_t* ioIndex=null, size_t* oSubrangeIndex=null)
{return FindAdvanceAnyAdvance(R(range), subranges, ioIndex, oSubrangeIndex);}

//! ����� ���������� ��������, �������������� ������� ��������� ������ ��������� �� ��������� ������������� subranges � ���� ��������.
//! \param subranges[inout] �������� ������� �������������.
//! ����� ������ ���� ������� ������ subranges ��������� � ������� ���������� ��������.
//! ���� ���������� �� �������, subranges ��������� � �������� ���������.
//! \param oSubrangeIndex[out] ��������� �� ����������, � ������� ����� ������� ������
//! ���������� �������� � ��������� subranges. ���� ������� �� ��� ������, ����� �������� �������� subranges.Count().
//! \return ���������� ���������� ���������� ���������.
template<typename R, typename RWs> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	Range::IsFiniteForwardRange<RWs>::_ && !Meta::IsConst<RWs>::_ &&
	Range::IsFiniteForwardRange<Range::ValueTypeOf<RWs>>::_,
size_t> CountUntilAnyAdvance(const R& range, RWs&& subranges, size_t* oSubrangeIndex=null)
{
	size_t index = 0;
	FindAnyAdvance(range, subranges, &index, oSubrangeIndex);
	return index;
}



//! ����� ������ ��������� ������ ������������ �� ��������� subranges � ���� ��������.
//! \param subranges ������� ������������.
//! \param ioIndex[inout] ��������� �� �������, ������� ������������� �� ���������� ���������, �������������� ��������� �������. ����� ���� null.
//! \param oWhatIndex[out] ��������� �� ����������, � ������� ����� ������� ������ ���������� ������������
//! � ��������� subranges. ���� ������� �� ��� ������, ����� �������� �������� whats.Count().
template<typename R, typename RWs> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	Range::IsFiniteForwardRange<RWs>::_ &&
	Range::IsFiniteForwardRange<Range::ValueTypeOf<RWs>>::_,
R> FindAny(const R& range, const RWs& subranges,
	size_t* ioIndex=null, size_t* oWhatIndex=null)
{return FindAdvanceAny(R(range), subranges, ioIndex, oWhatIndex);}

//! ����� ���������� ��������, �������������� ������� ��������� ������ ��������� �� ��������� ������������� subranges � ���� ��������.
//! \param subranges[inout] �������� ������� �������������.
//! ����� ������ ���� ������� ������ subranges ��������� � ������� ���������� ��������.
//! ���� ���������� �� �������, subranges ��������� � �������� ���������.
//! \param oSubrangeIndex[out] ��������� �� ����������, � ������� ����� ������� ������
//! ���������� �������� � ��������� subranges. ���� ������� �� ��� ������, ����� �������� �������� subranges.Count().
//! \return ���������� ���������� ���������� ���������.
template<typename R, typename RWs> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	Range::IsFiniteForwardRange<RWs>::_ &&
	Range::IsFiniteForwardRange<Range::ValueTypeOf<RWs>>::_,
size_t> CountUntilAny(const R& range,
	const RWs& subranges, size_t* oSubrangeIndex=null)
{
	size_t index = 0;
	FindAny(range, subranges, &index, oSubrangeIndex);
	return index;
}




//! ����� ���������� ���������, �������������� ������� ��������� ��������� what � ���� ��������.
//! ������ ��������� ��������������� �� ������ ������� ��������� what ��� ��������� � ������, ���� �������� �� �������� what.
//! \param what ������� ��������.
//! \returns ���������� ���������� ���������� ���������.
template<typename R, typename RW> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ && !Meta::IsConst<R>::_ &&
	Range::IsFiniteForwardRange<RW>::_ &&
	Range::ValueTypeIsConvertible<RW, Range::ValueTypeOf<R>>::_,
size_t> CountUntilAdvance(R&& range, const RW& what)
{
	size_t index=0;
	FindAdvance(range, what, &index);
	return index;
}

//! ����� ���������� ���������, �������������� ������� ��������� ��������� what � ���� ��������.
//! \param what ������� ��������.
//! \returns ���������� ���������� ���������� ���������.
template<typename R, typename RW> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	Range::IsFiniteForwardRange<RW>::_ &&
	Range::ValueTypeIsConvertible<RW, Range::ValueTypeOf<R>>::_,
size_t> CountUntil(const R& range, const RW& what)
{return CountUntilAdvance(R(range), what);}
	

template<typename R, typename RW> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	(Range::IsFiniteForwardRange<RW>::_ || Range::HasAsRange<RW>::_),
bool> Contains(const R& range, const RW& what)
{return !Find(range, Range::AsRange(what)).Empty();}

template<typename R, size_t N, typename T = Range::ValueTypeOf<R>> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_,
bool> Contains(const R& range, const T(&what)[N])
{
	const size_t len = N-size_t(Meta::IsCharType<Range::ValueTypeOf<R>>::_);
	return !Contains(range, Range::ArrayRange<const T>(what, len));
}

template<typename R, typename RW> Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ && !Meta::IsConst<R>::_ &&
	Range::IsFiniteForwardRange<RW>::_,
size_t> CountAdvance(R&& range, const RW& what)
{
	size_t result=0;
	size_t whatCount = Range::Count(what);
	while(FindAdvance(range, what), !range.Empty())
	{
		Range::PopFirstExactly(range, whatCount);
		result++;
	}
	return result;
}

template<typename R, typename RW> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	Range::IsFiniteForwardRange<RW>::_,
size_t> Count(const R& range, const RW& what)
{return CountAdvance(R(range), what);}

INTRA_WARNING_POP

}}
