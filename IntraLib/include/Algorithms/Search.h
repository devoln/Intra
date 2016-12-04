#pragma once

#include "Meta/Type.h"
#include "Range/Concepts.h"

namespace Intra { namespace Algo {

//! ���������� ��������, ���������� �� ����� ��������� ��������� ���� ������ ���������, ������ x.
template<typename R, typename X> Meta::EnableIf<
	Range::IsInputRange<R>::_ &&
	Meta::IsConvertible<X, typename R::value_type>::_,
R&> TrimLeftAdvance(R& range, const X& x)
{
	while(!range.Empty() && range.First()==x)
		range.PopFirst();
	return range;
}
	
//! ��������������� ������� �������� �� ������ ���������, ���� ����������� �������� pred.
//! ��������������� �� ������ ��������, ��� �������� ��� ������� �� ���������.
template<typename R, typename P> Meta::EnableIf<
	Range::IsInputRange<R>::_ &&
	Meta::IsCallable<P, typename R::value_type>::_,
R&> TrimLeftAdvance(R& range, P pred)
{
	while(!range.Empty() && pred(range.First()))
		range.PopFirst();
	return range;
}

//! ���������� ��������, ���������� �� ����� ��������� ��������� ���� ������ ���������, ������ x.
template<typename R, typename X> forceinline Meta::EnableIf<
	Range::IsForwardRange<R>::_ &&
	Meta::IsConvertible<X, typename R::value_type>::_,
R> TrimLeft(const R& range, const X& x)
{
	R result = range;
	return TrimLeftAdvance(result, x);
}

//! ���������� ��������, ���������� �� ����� ��������� ��������� ���� ������ ���������, ��� ������� �������� �������� pred.
template<typename R, typename P> forceinline Meta::EnableIf<
	Range::IsForwardRange<R>::_ &&
	Meta::IsCallable<P, typename R::value_type>::_,
R> TrimLeft(const R& range, P pred)
{
	R result = range;
	return TrimLeftAdvance(result, pred);
}


//! ����� ������ ��������� �������� what � ���� ��������.
//! �������� �������� � null, ���� �������� �� �������. ����� ��������� ����� ����� ���������, ������� � �������, �� ������� ���������� ������ ��������� what.
//! \param what ������� �������.
//! \param ioIndex[inout] ��������� �� �������, ������� ������������� �� ���������� ���������, �������������� ��������� �������. ����� ���� null.
template<typename R, typename X> Meta::EnableIf<
	Range::IsFiniteInputRange<R>::_ && Meta::IsConvertible<X, typename R::value_type>::_,
R&> FindAdvance(R& range, const X& what, size_t* ioIndex=null)
{
	size_t index=0;
	while(!range.Empty() && !(range.First()==what))
	{
		range.PopFirst();
		index++;
	}
	if(ioIndex!=null) *ioIndex += index;
	return range;
}

//! ����� ������ ��������� ��������, ���������������� ���������� �������, � ���� ��������.
//! �������� �������� � null, ���� �������� �� �������. ����� ��������� ����� ����� ���������, ������� � �������, �� ������� ���������� ������ ��������� �������� ��������.
//! \param pred �������, �������� ������ ������������� ������� �������.
//! \param ioIndex[inout] ��������� �� �������, ������� ������������� �� ���������� ���������, �������������� ��������� �������. ����� ���� null.
template<typename R, typename P> Meta::EnableIf<
	Range::IsFiniteInputRange<R>::_ && Meta::IsCallable<P, typename R::value_type>::_,
R&> FindAdvance(R& range, P pred, size_t* ioIndex=null)
{
	size_t index=0;
	while(!range.Empty() && !pred(range.First()))
	{
		range.PopFirst();
		index++;
	}
	if(ioIndex!=null) *ioIndex += index;
	return range;
}


//! ����� ������ ��������� ������ �������� �� ��������� whats � ���� ��������.
//! ������ ����� ��������� ��������� � ���������� �������� ��� ����������� � ������ � ������, ����� ������� �� ������.
//! \param whats �������� ������� ���������. ���� ���� �� ���� ��������� ������, ������ whats ��������������� �� ��������� �������.
//! \param ioIndex[inout] ��������� �� �������, ������� ������������� �� ���������� ���������, �������������� ��������� �������. ����� ���� null.
//! \param oWhatIndex[out] ��������� �� ����������, � ������� ����� ������� ������ ���������� �������� � ��������� whats. ���� ������� �� ��� ������, ����� �������� �������� whats.Count().
//! \return ���������� ������ �� ����.
template<typename R, typename Ws> Meta::EnableIf<
	Range::IsFiniteInputRange<R>::_ &&
	Range::IsFiniteForwardRange<Ws>::_ && Meta::IsConvertible<typename Ws::value_type, typename R::value_type>::_,
R&> FindAdvanceAnyAdvance(R& range, Ws& whats, size_t* ioIndex=null, size_t* oWhatIndex=null)
{
	size_t index=0, whatIndex = whats.Count();
	Ws whatsCopy = whats;
	for(; !range.Empty(); range.PopFirst(), index++)
	{
		whats = whatsCopy;
		whatIndex = 0;
		while(!whats.Empty())
		{
			if(range.First()!=whats.First())
			{
				whats.PopFirst();
				whatIndex++;
				continue;
			}
			goto exit;
		}
	}
exit:
	if(ioIndex!=null) *ioIndex += index;
	if(oWhatIndex!=null) *oWhatIndex = whatIndex;
	return range;
}

//! ����� ������ ��������� ������ �������� �� ��������� whats � ���� ��������.
//! ������ ����� ��������� ��������� � ���������� �������� ��� ����������� � ������ � ������, ����� ������� �� ������.
//! \param whats �������� ������� ���������.
//! \param ioIndex[inout] ��������� �� �������, ������� ������������� �� ���������� ���������, �������������� ��������� �������. ����� ���� null.
//! \param oWhatIndex[out] ��������� �� ����������, � ������� ����� ������� ������ ���������� �������� � ��������� whats. ���� ������� �� ��� ������, ����� �������� �������� whats.Count().
//! \return ���������� ������ �� ����.
template<typename R, typename Ws> forceinline Meta::EnableIf<
	Range::IsFiniteInputRange<R>::_ && Range::IsFiniteForwardRangeOf<Ws, typename R::value_type>::_,
R&> FindAdvanceAny(R& range, const Ws& whats, size_t* ioIndex=null, size_t* oWhatIndex=null)
{
	Ws whatCopy = whats;
	return FindAdvanceAnyAdvance(range, whatCopy, ioIndex, oWhatIndex);
}



//! ��������������� ������� �������� �� ������ ���������, �� ��� ���,
//! ���� �� ���������� �������, ������ x, ��� �������� �� ������ ������.
//! \return ���������� ���������� ���������� ���������.
template<typename R, typename X> forceinline Meta::EnableIf<
	Range::IsFiniteInputRange<R>::_ && Meta::IsConvertible<X, typename R::value_type>::_,
size_t> CountUntilAdvance(R& range, const X& x)
{
	size_t index=0;
	FindAdvance(range, x, &index);
	return index;
}
	
//! ��������������� ������� �������� �� ������ ��������� �� ��� ���,
//! ���� �� ���������� ������� predicate ��� �������� �� ������ ������.
//! \return ���������� ���������� ���������� ���������.
template<typename R, typename P> forceinline Meta::EnableIf<
	Range::IsFiniteInputRange<R>::_ && Meta::IsCallable<P, typename R::value_type>::_,
size_t> CountUntilAdvance(R& range, P predicate)
{
	size_t index=0;
	FindAdvance(range, predicate, &index);
	return index;
}

//! ��������������� ������� �������� �� ������ ��������� �� ��� ���,
//! ���� �� ���������� �������, ������ ������ �� ��������� ��������� whats.
//! \param whats[inout] �������� ������� ���������. ���� ������� ������, ������ ��������� whats ��������� � ���������� ��������.
//! \return ���������� ���������� ���������� ���������.
template<typename R, typename Ws> forceinline Meta::EnableIf<
	Range::IsFiniteInputRange<R>::_ && Range::IsForwardRangeOf<Ws, typename R::value_type>::_,
size_t> CountUntilAdvanceAnyAdvance(R& range, Ws& whats)
{
	size_t index=0;
	FindAdvanceAnyAdvance(range, whats, &index);
	return index;
}

//! ��������������� ������� �������� �� ������ ��������� �� ��� ���,
//! ���� �� ���������� �������, ������ ������ �� ��������� ��������� whats.
//! \param whats[inout] �������� ������� ���������.
//! \return ���������� ���������� ���������� ���������.
template<typename R, typename Ws> forceinline Meta::EnableIf<
	Range::IsFiniteInputRange<R>::_ && Range::IsForwardRangeOf<Ws, typename R::value_type>::_,
size_t> CountUntilAdvanceAny(R& range, const Ws& whats)
{
	size_t index=0;
	FindAdvanceAny(range, whats, &index);
	return index;
}



//! ��������������� ������������� �������� ��������� �� ��� ���,
//! ���� �� ���������� �������, ������ x, ��� �� ����� ��������� ����� ���������.
//! ���������� ���������� ���������� ���������.
template<typename R, typename X> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	Meta::IsConvertible<X, typename R::value_type>::_,
size_t> CountUntil(const R& range, const X& x)
{
	R r = range;
	return CountUntilAdvance(r, x);
}
	
//! ��������������� ������������� �������� ��������� �� ��� ���, ���� ��� ��������
//! �� ���������� ������� predicate ��� �� ����� ��������� ����� ���������.
//! \return ���������� ���������� ���������� ���������.
template<typename R, typename P> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	Meta::IsCallable<P, typename R::value_type>::_,
size_t> CountUntil(const R& range, P predicate)
{
	R r = range;
	return CountUntilAdvance(r, predicate);
}


//! ��������������� ������� �������� ��������� �� ��� ���, ���� �� ���������� �������,
//! ������ x, ��� �� ����� ��������� ����� ���������.
//! \return ���������� �������� ���������� ���������.
template<typename R, typename X> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	Meta::IsConvertible<X, typename R::value_type>::_,
R> ReadUntilAdvance(R& range, const X& x, size_t* ioIndex=null)
{
	auto r = range;
	size_t index = CountUntilAdvance(range, x);
	if(ioIndex!=null) *ioIndex += index;
	return Range::Take(r, index);
}
	
//! ��������������� ������� �������� ��������� �� ��� ���, ���� ��� ��������
//! �� ���������� ������� predicate ��� �� ����� ��������� ����� ���������.
//! ���������� �������� ���������� ���������.
template<typename R, typename P> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	Meta::IsCallable<P, typename R::value_type>::_,
R> ReadUntilAdvance(R& range, P predicate, size_t* ioIndex=null)
{
	auto r = range;
	size_t index = CountUntilAdvance(range, predicate);
	if(ioIndex!=null) *ioIndex += index;
	return Range::Take(r, index);
}

//! ��������������� ������������� �������� ��������� �� ��� ���,
//! ���� �� ���������� �������, ������ x, ��� �� ����� ��������� ����� ���������.
//! ���������� �������� ���������� ���������.
template<typename R, typename X> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	Meta::IsConvertible<X, typename R::value_type>::_,
Range::ResultOfTake<R>> ReadUntil(const R& range, const X& x, size_t* ioIndex=null)
{
	const size_t index = CountUntil(range, x);
	if(ioIndex!=null) *ioIndex += index;
	return Range::Take(range, index);
}
	
//! ��������������� ������������� �������� ��������� �� ��� ���,
//! ���� ��� �������� �� ���������� ������� predicate ��� �� ����� ��������� ����� ���������.
//! ���������� �������� ���������� ���������.
template<typename R, typename P> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	Meta::IsCallable<P, typename R::value_type>::_,
Range::ResultOfTake<R>> ReadUntil(const R& range, P predicate, size_t* ioIndex=null)
{
	size_t index = CountUntil(range, predicate);
	if(ioIndex!=null) *ioIndex += index;
	return Range::Take(range, index);
}


//! ����� ������ ��������� �������� what � ���� ��������.
//! \param what ������� �������.
//! \param ioIndex[inout] ��������� �� �������, ������� ������������� �� ���������� ���������, �������������� ��������� �������. ����� ���� null.
//! \returns null, ���� �������� �� �������. ����� ����� ���������, ������� � �������, �� ������� ���������� ������ ��������� what.
template<typename R, typename X> Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	Meta::IsConvertible<X, typename R::value_type>::_,
R> Find(const R& range, const X& what, size_t* ioIndex=null)
{
	auto result = range;
	FindAdvance(result, what, ioIndex);
	return result;
}

template<typename R, typename X> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	Meta::IsConvertible<X, typename R::value_type>::_,
bool> Contains(const R& range, const X& what) {return !Find(range, what).Empty();}

//! ����� ������ ��������� ��������, ���������������� ���������� �������, � ���� ��������.
//! \param pred �������, �������� ������ ������������� ������� �������.
//! \param ioIndex[inout] ��������� �� �������, ������� ������������� �� ���������� ���������, �������������� ��������� �������. ����� ���� null.
template<typename R, typename P> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	Meta::IsCallable<P, typename R::value_type>::_,
R> Find(const R& range, P pred, size_t* ioIndex=null)
{
	R r = range;
	return FindAdvance(r, pred, ioIndex);
}

template<typename R> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_,
bool> Contains(const R& range, const typename R::value_type& what)
{return !Find(range, what).Empty();}
	

template<typename R, typename Ws> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	Range::IsFiniteForwardRangeOf<Ws, typename R::value_type>::_,
Range::ResultOfTake<R>> ReadUntilAdvanceAny(const R& range, const Ws& whats, size_t* ioIndex=null, size_t* oWhatIndex=null)
{
	R r = range;
	size_t index = CountUntilAdvanceAny(r, whats, oWhatIndex);
	if(ioIndex!=null) *ioIndex += index;
	return Range::Take(range, index);
}

template<typename R, typename Ws> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRangeOf<Ws, typename R::value_type>::_,
Range::ResultOfTake<R>> ReadUntilAny(const R& range, const Ws& whats,
	size_t* ioIndex=null, size_t* oWhatIndex=null)
{
	size_t index = CountUntilAny(range, whats, oWhatIndex);
	if(ioIndex!=null) *ioIndex += index;
	return Range::Take(range, index);
}

//! ����� ������ ��������� ��������� what � ���� ��������.
//! ������ ��������� ��������������� �� ������ ������� ��������� what ��� ��������� � ������, ���� �������� �� �������� what.
//! \param what ������� ��������.
//! \param ioIndex[inout] ��������� �� �������, ������� ������������� �� ���������� ���������, �������������� ��������� �������. ����� ���� null.
//! \returns ���������� ������ �� ����.
template<typename R, typename RW> Meta::EnableIf<
	Range::IsFiniteForwardRangeOf<RW, typename R::value_type>::_,
R&> FindAdvance(R& range, const RW& what, size_t* ioIndex=null)
{
	while(!range.Empty() && !range.StartsWith(what))
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
	Range::IsFiniteForwardRangeOf<RW, typename R::value_type>::_,
R> Find(const R& range, const RW& what, size_t* ioIndex=null)
{
	auto result = range;
	return FindAdvance(result, what, ioIndex);
}

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
	Range::IsFiniteForwardRange<R>::_ &&
	Range::IsFiniteForwardRangeOfFiniteForwardRanges<RWs>::_,
R&> FindAdvanceAnyAdvance(R& range, RWs& subranges, size_t* ioIndex=null, size_t* oSubrangeIndex=null)
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
	Range::IsFiniteForwardRangeOfFiniteForwardRanges<RWs>::_,
size_t> CountUntilAdvanceAnyAdvance(RWs& subranges, size_t* oSubrangeIndex=null)
{
	size_t index = 0;
	FindAdvanceAnyAdvance(subranges, &index, oSubrangeIndex);
	return index;
}

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
	Range::IsFiniteForwardRangeOfFiniteForwardRanges<RWs>::_,
Range::ResultOfTake<R>> ReadUntilAdvanceAnyAdvance(R& range,
	RWs& subranges, size_t* ioIndex, size_t* oSubrangeIndex=null)
{
	auto r = range;
	size_t index = CountUntilAdvanceAnyAdvance(range, subranges, oSubrangeIndex);
	if(ioIndex!=null) *ioIndex += index;
	return Range::Take(r, index);
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
	Range::IsFiniteForwardRangeOfFiniteForwardRanges<RWs>::_,
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
	Range::IsFiniteForwardRangeOfFiniteForwardRanges<RWs>::_,
size_t> CountUntilAdvanceAny(R& range, const RWs& subranges, size_t* oSubrangeIndex=null)
{
	size_t index = 0;
	FindAdvanceAny(range, subranges, &index, oSubrangeIndex);
	return index;
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
template<typename R, typename RWs> Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	Range::IsFiniteForwardRangeOfFiniteForwardRanges<RWs>::_,
Range::ResultOfTake<R>> ReadUntilAdvanceAny(R& range,
	const RWs& subranges, size_t* ioIndex=null, size_t* oSubrangeIndex=null)
{
	auto r = range;
	size_t index = CountUntilAdvanceAny(range, subranges, oSubrangeIndex);
	if(ioIndex!=null) *ioIndex += index;
	return Range::Take(r, index);
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
	Range::IsFiniteForwardRangeOfFiniteForwardRanges<RWs>::_,
R> FindAnyAdvance(const R& range, RWs& subranges, size_t* ioIndex=null, size_t* oSubrangeIndex=null)
{
	R r = range;
	return FindAdvanceAnyAdvance(r, subranges, ioIndex, oSubrangeIndex);
}

//! ����� ���������� ��������, �������������� ������� ��������� ������ ��������� �� ��������� ������������� subranges � ���� ��������.
//! \param subranges[inout] �������� ������� �������������.
//! ����� ������ ���� ������� ������ subranges ��������� � ������� ���������� ��������.
//! ���� ���������� �� �������, subranges ��������� � �������� ���������.
//! \param oSubrangeIndex[out] ��������� �� ����������, � ������� ����� ������� ������
//! ���������� �������� � ��������� subranges. ���� ������� �� ��� ������, ����� �������� �������� subranges.Count().
//! \return ���������� ���������� ���������� ���������.
template<typename R, typename RWs> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	Range::IsFiniteForwardRangeOfFiniteForwardRanges<RWs>::_,
size_t> CountUntilAnyAdvance(const R& range, RWs& subranges, size_t* oSubrangeIndex=null)
{
	size_t index = 0;
	FindAnyAdvance(range, subranges, &index, oSubrangeIndex);
	return index;
}

//! ��������� ���������� ��������, �������������� ������� ��������� ������ ���������
//! �� ��������� ������������� subranges � ���� ��������.
//! \param subranges[inout] �������� ������� �������������.
//! ����� ������ ���� ������� ������ subranges ��������� � ������� ���������� ��������.
//! ���� ���������� �� �������, subranges ��������� � �������� ���������.
//! \param oSubrangeIndex[out] ��������� �� ����������, � ������� ����� ������� ������ ���������� �������� � ��������� subranges. ���� ������� �� ��� ������, ����� �������� �������� subranges.Count().
//! \return ���������� �������� ����������� ���������.
template<typename R, typename RWs> Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	Range::IsFiniteForwardRangeOfFiniteForwardRanges<RWs>::_,
Range::ResultOfTake<R>> ReadUntilAnyAdvance(const R& range,
	RWs& subranges, size_t* ioIndex=null, size_t* oSubrangeIndex=null)
{
	size_t index = CountUntilAnyAdvance(range, subranges, oSubrangeIndex);
	if(ioIndex!=null) *ioIndex += index;
	return Take(range, index);
}



//! ����� ������ ��������� ������ ������������ �� ��������� subranges � ���� ��������.
//! \param subranges ������� ������������.
//! \param ioIndex[inout] ��������� �� �������, ������� ������������� �� ���������� ���������, �������������� ��������� �������. ����� ���� null.
//! \param oWhatIndex[out] ��������� �� ����������, � ������� ����� ������� ������ ���������� ������������
//! � ��������� subranges. ���� ������� �� ��� ������, ����� �������� �������� whats.Count().
template<typename R, typename RWs> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	Range::IsFiniteForwardRangeOfFiniteForwardRanges<RWs>::_,
R> FindAny(const R& range, const RWs& subranges,
	size_t* ioIndex=null, size_t* oWhatIndex=null)
{
	R result = range;
	return FindAdvanceAny(result, subranges, ioIndex, oWhatIndex);
}

//! ����� ���������� ��������, �������������� ������� ��������� ������ ��������� �� ��������� ������������� subranges � ���� ��������.
//! \param subranges[inout] �������� ������� �������������.
//! ����� ������ ���� ������� ������ subranges ��������� � ������� ���������� ��������.
//! ���� ���������� �� �������, subranges ��������� � �������� ���������.
//! \param oSubrangeIndex[out] ��������� �� ����������, � ������� ����� ������� ������
//! ���������� �������� � ��������� subranges. ���� ������� �� ��� ������, ����� �������� �������� subranges.Count().
//! \return ���������� ���������� ���������� ���������.
template<typename R, typename RWs> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	Range::IsFiniteForwardRangeOfFiniteForwardRanges<RWs>::_,
size_t> CountUntilAny(const R& range,
	const RWs& subranges, size_t* oSubrangeIndex=null)
{
	size_t index = 0;
	FindAny(range, subranges, &index, oSubrangeIndex);
	return index;
}

//! ��������� ���������� ��������, �������������� ������� ��������� ������ ���������
//! �� ��������� ������������� subranges � ���� ��������.
//! \param subranges[inout] �������� ������� �������������.
//! ����� ������ ���� ������� ������ subranges ��������� � ������� ���������� ��������.
//! ���� ���������� �� �������, subranges ��������� � �������� ���������.
//! \param oSubrangeIndex[out] ��������� �� ����������, � ������� ����� ������� ������ ���������� �������� � ��������� subranges. ���� ������� �� ��� ������, ����� �������� �������� subranges.Count().
//! \return ���������� �������� ����������� ���������.
template<typename R, typename RWs> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	Range::IsFiniteForwardRangeOfFiniteForwardRanges<RWs>::_,
Range::ResultOfTake<R>> ReadUntilAny(const R& range, const RWs& subranges,
	size_t* ioIndex=null, size_t* oSubrangeIndex=null)
{
	const size_t index = CountUntilAny(range, subranges, oSubrangeIndex);
	if(ioIndex!=null) *ioIndex += index;
	return Range::Take(range, index);
}


//! ����� ���������� ���������, �������������� ������� ��������� ��������� what � ���� ��������.
//! ������ ��������� ��������������� �� ������ ������� ��������� what ��� ��������� � ������, ���� �������� �� �������� what.
//! \param what ������� ��������.
//! \returns ���������� ���������� ���������� ���������.
template<typename R, typename RW> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	Range::IsFiniteForwardRangeOf<RW, typename R::value_type>::_,
size_t> CountUntilAdvance(R& range, const RW& what)
{
	size_t index=0;
	FindAdvance(range, what, &index);
	return index;
}

//! ��������� �������� �� ������ ���������, �������������� ������� ��������� ��������� what � ���� ��������.
//! \param what ������� ��������.
//! \returns ���������� �������� ���������� ���������.
template<typename R, typename RW> Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	Range::IsFiniteForwardRangeOf<RW, typename R::value_type>::_,
Range::ResultOfTake<R>> ReadUntilAdvance(R& range, const RW& what, size_t* ioIndex=null)
{
	R r = range;
	size_t index = CountUntilAdvance(range, what);
	if(ioIndex!=null) *ioIndex += index;
	return Range::Take(r, index);
}

//! ����� ���������� ���������, �������������� ������� ��������� ��������� what � ���� ��������.
//! \param what ������� ��������.
//! \returns ���������� ���������� ���������� ���������.
template<typename R, typename RW> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	Range::IsFiniteForwardRangeOf<RW, typename R::value_type>::_,
size_t> CountUntil(const R& range, const RW& what)
{
	R r = range;
	return CountUntilAdvance(r, what);
}

//! ��������� �������� �� ������ ���������, �������������� ������� ��������� ��������� what � ���� ��������.
//! \param what ������� ��������.
//! \returns ���������� �������� ���������� ���������.
template<typename R, typename RW> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	Range::IsFiniteForwardRangeOf<RW, typename R::value_type>::_,
Range::ResultOfTake<R>> ReadUntil(const R& range,
	const RW& what, size_t* ioIndex=null)
{
	size_t index = CountUntil(range, what);
	if(ioIndex!=null) *ioIndex += index;
	return Range::Take(range, index);
}



	

template<typename R, typename RW> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	(Range::IsFiniteForwardRange<RW>::_ || Range::HasAsRange<RW>::_),
bool> Contains(const R& range, const RW& what)
{return !Find(range, Range::AsRange(what)).Empty();}

template<typename R, size_t N, typename T = typename R::value_type> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_,
bool> Contains(const R& range, const T(&what)[N])
{
	const size_t len = N-size_t(Meta::IsCharType<typename R::value_type>::_);
	return !Contains(range, Range::ArrayRange<const T>(what, len));
}



}}

