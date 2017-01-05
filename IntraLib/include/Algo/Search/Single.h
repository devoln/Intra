#pragma once

#include "Platform/CppWarnings.h"
#include "Meta/Operators.h"
#include "Range/Concepts.h"
#include "Range/Construction/Take.h"

namespace Intra { namespace Algo {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

//! ����� ������ ��������� �������� what � ���� ��������.
//! �������� �������� � null, ���� �������� �� �������.
//! ����� ��������� ����� ����� ���������, ������� � �������, �� ������� ���������� ������ ��������� what.
//! \param what ������� �������.
//! \param ioIndex[inout] ��������� �� �������, ������� ������������� �� ���������� ���������, �������������� ��������� �������. ����� ���� null.
template<class R, typename X> Meta::EnableIf<
	Range::IsFiniteInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	!Meta::IsCallable<X, Range::ValueTypeOf<R>>::_ &&
	!(Range::IsFiniteForwardRange<X>::_ &&
	Range::ValueTypeIsConvertible<X, Range::ValueTypeOf<R>>::_),
R&&> FindAdvance(R&& range, const X& what, size_t* ioIndex=null)
{
	size_t index = 0;
	while(!range.Empty() && !(range.First()==what))
	{
		range.PopFirst();
		index++;
	}
	if(ioIndex!=null) *ioIndex += index;
	return Meta::Forward<R>(range);
}

//! ����� ������ ��������� ��������, ���������������� ���������� �������, � ���� ��������.
//! �������� �������� � null, ���� �������� �� �������.
//! ����� ��������� ����� ����� ���������, ������� � �������, �� ������� ���������� ������ ��������� �������� ��������.
//! \param pred �������, �������� ������ ������������� ������� �������.
//! \param ioIndex[inout] ��������� �� �������, ������� ������������� �� ���������� ���������, �������������� ��������� �������. ����� ���� null.
template<class R, typename P> Meta::EnableIf<
	Range::IsFiniteInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	Meta::IsCallable<P, Range::ValueTypeOf<R>>::_,
R&&> FindAdvance(R&& range, P pred, size_t* ioIndex=null)
{
	size_t index=0;
	while(!range.Empty() && !pred(range.First()))
	{
		range.PopFirst();
		index++;
	}
	if(ioIndex!=null) *ioIndex += index;
	return Meta::Forward<R>(range);
}


//! ����� ������ ��������� ������ �������� �� ��������� whats � ���� ��������.
//! ������ ����� ��������� ��������� � ���������� �������� ��� ����������� � ������ � ������, ����� ������� �� ������.
//! \param whats �������� ������� ���������. ���� ���� �� ���� ��������� ������,
//! ������ whats ��������������� �� ��������� �������. ����� whats ���������� ������
//! \param ioIndex[inout] ��������� �� �������, ������� ������������� �� ���������� ���������, �������������� ��������� �������. ����� ���� null.
//! \param oWhatIndex[out] ��������� �� ����������, � ������� ����� ������� ������ ���������� �������� � ��������� whats. ���� ������� �� ��� ������, ����� �������� �������� whats.Count().
//! \return ���������� ������ �� ����.
template<class R, class Ws> Meta::EnableIf<
	Range::IsFiniteInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	Range::IsFiniteForwardRange<Ws>::_ && !Meta::IsConst<Ws>::_ &&
	Range::ValueTypeIsConvertible<Ws, Range::ValueTypeOf<R>>::_,
R&&> FindAdvanceAnyAdvance(R&& range, Ws&& whats,
	size_t* ioIndex=null, size_t* oWhatIndex=null)
{
	size_t index=0, whatIndex = Range::Count(whats);
	Ws whatsCopy = Meta::Forward<Ws>(whats);
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
	return Meta::Forward<R>(range);
}

//! ����� ������ ��������� ������ �������� �� ��������� whats � ���� ��������.
//! ������ ����� ��������� ��������� � ���������� �������� ��� ����������� � ������ � ������, ����� ������� �� ������.
//! \param whats �������� ������� ���������.
//! \param ioIndex[inout] ��������� �� �������, ������� ������������� �� ���������� ���������, �������������� ��������� �������. ����� ���� null.
//! \param oWhatIndex[out] ��������� �� ����������, � ������� ����� ������� ������ ���������� �������� � ��������� whats. ���� ������� �� ��� ������, ����� �������� �������� whats.Count().
//! \return ���������� ������ �� ����.
template<class R, class Ws> forceinline Meta::EnableIf<
	Range::IsFiniteInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	Range::IsFiniteForwardRange<Ws>::_ &&
	Range::ValueTypeIsConvertible<Ws, Range::ValueTypeOf<R>>::_,
R&> FindAdvanceAny(R&& range, const Ws& whats, size_t* ioIndex=null, size_t* oWhatIndex=null)
{return FindAdvanceAnyAdvance(range, Ws(whats), ioIndex, oWhatIndex);}


//! ��������������� ������� ��� �������� �� ���������, ����������� ���������� ���������, ������ x.
template<class R, typename X> Meta::EnableIf<
	Range::IsInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	!Meta::IsCallable<X, Range::ValueTypeOf<R>>::_ &&
	Meta::HasOpEquals<Range::ReturnValueTypeOf<R>, const X&>::_,
size_t> CountAdvance(R&& range, const X& x)
{
	size_t result=0;
	while(!range.Empty())
	{
		if(range.First()==x) result++;
		range.PopFirst();
	}
	return result;
}


//! ��������������� ������� ��� �������� �� ���������, ����������� ���������� ���������, ��� ������� ��������� ������� pred.
template<class R, typename P> Meta::EnableIf<
	Range::IsInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	Meta::IsCallable<P, Range::ValueTypeOf<R>>::_,
size_t> CountAdvance(R&& range, P pred)
{
	size_t result=0;
	while(!range.Empty())
	{
		if(pred(range.First())) result++;
		range.PopFirst();
	}
	return result;
}


//! ��������������� ������� �������� �� ������ ���������, �� ��� ���, ���� ��:
//! 1) ���������� �������, ��� �������� ���������� �������� valOrPred, ���� ��� ��������;
//! 2) ���������� �������, ������ valOrPred, ���� ��� �� ��������;
//! 3) ����� ��������� ����� ���������.
//! \return ���������� ���������� ���������� ���������.
template<class R, typename X> forceinline Meta::EnableIf<
	Range::IsFiniteInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	!(Range::IsFiniteForwardRange<X>::_ &&
	Range::ValueTypeIsConvertible<X, Range::ValueTypeOf<R>>::_),
size_t> CountUntilAdvance(R&& range, const X& valOrPred)
{
	size_t index=0;
	FindAdvance(range, valOrPred, &index);
	return index;
}

//! ��������������� ������� �������� �� ������ ��������� �� ��� ���,
//! ���� �� ���������� �������, ������ ������ �� ��������� ��������� whats.
//! \param whats[inout] �������� ������� ���������. ���� ������� ������, ������ ��������� whats ��������� � ���������� ��������.
//! \return ���������� ���������� ���������� ���������.
template<class R, class Ws> forceinline Meta::EnableIf<
	Range::IsFiniteInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	Range::IsForwardRange<Ws>::_ && !Meta::IsConst<Ws>::_ &&
	Range::ValueTypeIsConvertible<Ws, Range::ValueTypeOf<R>>::_,
size_t> CountUntilAdvanceAnyAdvance(R&& range, Ws&& whats)
{
	size_t index=0;
	FindAdvanceAnyAdvance(range, whats, &index);
	return index;
}

//! ��������������� ������� �������� �� ������ ��������� �� ��� ���,
//! ���� �� ���������� �������, ������ ������ �� ��������� ��������� whats.
//! \param whats[inout] �������� ������� ���������.
//! \return ���������� ���������� ���������� ���������.
template<class R, class Ws> forceinline Meta::EnableIf<
	Range::IsFiniteInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	Range::IsForwardRange<Ws>::_ &&
	Range::ValueTypeIsConvertible<Ws, Range::ValueTypeOf<R>>::_,
size_t> CountUntilAdvanceAny(R&& range, const Ws& whats)
{
	size_t index=0;
	FindAdvanceAny(range, whats, &index);
	return index;
}



//! ��������������� ������������� �������� ��������� �� ��� ���,
//! ���� �� ���������� �������, ��� �������� ���� �������� �������� valueOrPred,
//! ���� ������ valueOrPred, ���� ��� �� ��������, ��� �� ����� ��������� ����� ���������.
//! ���������� ���������� ���������� ���������.
template<class R, typename X> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	(Meta::IsConvertible<X, Range::ValueTypeOf<R>>::_ ||
		Meta::IsCallable<X, Range::ValueTypeOf<R>>::_),
size_t> CountUntil(R&& range, const X& valueOrPred)
{return CountUntilAdvance(Meta::RemoveConstRef<R>(Meta::Forward<R>(range)), valueOrPred);}


//! ����� ������ ��������� �������� what � ���� ��������.
//! \param what ������� �������.
//! \param ioIndex[inout] ��������� �� �������, ������� ������������� �� ���������� ���������, �������������� ��������� �������. ����� ���� null.
//! \returns null, ���� �������� �� �������. ����� ����� ���������, ������� � �������, �� ������� ���������� ������ ��������� what.
template<class R, typename X> Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	Meta::IsConvertible<X, Range::ValueTypeOf<R>>::_,
Meta::RemoveConstRef<R>> Find(R&& range, const X& what, size_t* ioIndex=null)
{
	auto result = Meta::Forward<R>(range);
	FindAdvance(result, what, ioIndex);
	return result;
}

template<class R, typename X> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	Meta::IsConvertible<X, Range::ValueTypeOf<R>>::_,
bool> Contains(R&& range, const X& what)
{return !Find(Meta::Forward<R>(range), what).Empty();}

//! ����� ������ ��������� ��������, ���������������� ���������� �������, � ���� ��������.
//! \param pred �������, �������� ������ ������������� ������� �������.
//! \param ioIndex[inout] ��������� �� �������, ������� ������������� �� ���������� ���������, �������������� ��������� �������. ����� ���� null.
template<class R, typename P> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	Meta::IsCallable<P, Range::ValueTypeOf<R>>::_,
Meta::RemoveConstRef<R>> Find(R&& range, P pred, size_t* ioIndex=null)
{return FindAdvance(Meta::RemoveConstRef<R>(Meta::Forward<R>(range)), pred, ioIndex);}

INTRA_WARNING_POP

}}
