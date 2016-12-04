#pragma once

#include "Range/Concepts.h"

namespace Intra { namespace Algo {

template<typename R1, typename R2> forceinline Meta::EnableIf<
	Range::IsFiniteRange<R1>::_ != Range::IsFiniteRange<R2>::_,
bool> Equals(R1 r1, R2 r2) {(void)r1; (void)r2; return false;}

namespace detail {

template<typename R1, typename R2, typename P> Meta::EnableIf<
	Range::IsForwardRange<R1>::_ && Range::IsForwardRange<R2>::_,
bool> Equals(const R1& r1, const R2& r2, P pred)
{
	R1 range1 = r1;
	R2 range2 = r2;
	while(!range1.Empty() && !range2.Empty())
	{
		if(pred(range1.First(), range2.First()))
		{
			range1.PopFirst();
			range2.PopFirst();
			continue;
		}
		return false;
	}
	return range1.Empty() && range2.Empty();
}

}

template<typename R1, typename R2> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R1>::_ &&
	Range::IsFiniteForwardRange<R2>::_ &&
	!(Range::HasLength<R1>::_ &&
		Range::HasLength<R2>::_),
bool> Equals(const R1& r1, const R2& r2)
{return detail::Equals(r1, r2, Op::Equal);}

template<typename R1, typename R2, typename P> forceinline Meta::EnableIf<
	Range::IsForwardRange<R1>::_ &&
	Range::IsForwardRange<R2>::_ &&
	Range::HasLength<R1>::_ &&
	Range::HasLength<R2>::_,
bool> Equals(const R1& r1, const R2& r2, P pred)
{
	if(r1.Length()!=r2.Length()) return false;
	return detail::Equals(r1, r2, pred);
}

template<typename ArrayRange1, typename ArrayRange2> inline Meta::EnableIf<
	Range::IsInputRangeOfTrivial<ArrayRange1>::_ &&
	Range::ValueTypeEquals<ArrayRange1, ArrayRange2>::_ &&
	Range::IsArrayRange<ArrayRange1>::_ &&
	Range::IsArrayRange<ArrayRange2>::_,
bool> Equals(const ArrayRange1& r1, const ArrayRange2& r2)
{
	if(r1.Length()!=r2.Length()) return false;
	return core::memcmp(r1.Data(), r2.Data(), r1.Length()*sizeof(typename ArrayRange1::value_type))==0;
}


template<typename R1, typename R2, typename P> Meta::EnableIf<
	Range::IsForwardRange<R1>::_ &&
	Range::IsForwardRange<R2>::_,
int> LexCompare(const R1& r1, const R2& r2, P pred = Op::Less)
{
	R1 range1 = r1;
	R2 range2 = r2;
	while(!range1.Empty() && !range2.Empty())
	{
		if(pred(range1.First(), range2.First())) return -1;
		if(pred(range2.First(), range1.First())) return 1;
	}
	if(range1.Empty())
	{
		if(range2.Empty()) return 0;
		return -1;
	}
	return 1;
}

template<typename ArrayRange1, typename ArrayRange2> inline Meta::EnableIf<
	Range::IsInputRangeOfPod<ArrayRange1>::_ &&
	Range::ValueTypeEquals<ArrayRange1, ArrayRange2>::_ &&
	Range::IsArrayRange<ArrayRange1>::_ &&
	Range::IsArrayRange<ArrayRange2>::_,// &&
	//(sizeof(typename ArrayRange1::value_type)==1 || INTRA_PLATFORM_ENDIANESS==INTRA_PLATFORM_ENDIANESS_BigEndian),
int> LexCompare(const ArrayRange1& r1, const ArrayRange2& r2)
{
	size_t minLen = Op::Min(r1.Length(), r2.Length());
	int result = core::memcmp(r1.Data(), r2.Data(), minLen*sizeof(typename ArrayRange1::value_type));
	if(result!=0) return result;
	if(r1.Length()<r2.Length()) return -1;
	if(r1.Length()>r2.Length()) return 1;
	return 0;
}


namespace detail {

template<typename R, typename RW> Meta::EnableIf<
	Range::IsForwardRange<R>::_ &&
	Range::IsFiniteForwardRangeOf<RW, typename R::value_type>::_,
bool> StartsWith(const R& range, const RW& what)
{
	RW whatCopy = what;
	auto temp = range;
	while(!whatCopy.Empty())
	{
		if(temp.Empty()) return false;
		if(temp.First()!=whatCopy.First()) return false;
		temp.PopFirst();
		whatCopy.PopFirst();
	}
	return true;
}

}

template<typename R, typename RW> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRangeOf<RW, typename R::value_type>::_ &&
	!(Range::HasLength<R>::_ &&
		Range::HasLength<RW>::_),
bool> StartsWith(const R& range, const RW& what)
{return detail::StartsWith(range, what);}

template<typename R, typename RW> forceinline Meta::EnableIf<
	Range::IsForwardRangeOf<RW, typename R::value_type>::_ &&
	Range::HasLength<R>::_ &&
	Range::HasLength<RW>::_,
bool> StartsWith(const R& range, const RW& what)
{
	if(range.Length()<what.Length()) return false;
	return detail::StartsWith(range, what);
}

template<typename R, size_t N> forceinline Meta::EnableIf<
	Range::IsForwardRange<R>::_,
bool> StartsWith(const R& range, const typename R::value_type(&rhs)[N])
{return StartsWith(range, Range::AsRange(rhs));}

template<typename R, typename RWs> Meta::EnableIf<
	Range::IsForwardRange<R>::_ &&
	Range::IsRangeOfFiniteForwardRangesOf<RWs, typename R::value_type>::_ && Range::IsFiniteRange<RWs>::_,
bool> StartsWithAnyAdvance(const R& range, RWs& subranges, size_t* oSubrangeIndex=null)
{
	if(oSubrangeIndex!=null) *oSubrangeIndex = 0;
	while(!subranges.Empty())
	{
		if(StartsWith(range, subranges.First())) return true;
		if(oSubrangeIndex!=null) ++*oSubrangeIndex;
		subranges.PopFirst();
	}
	return false;
}

template<typename R, typename RWs> forceinline Meta::EnableIf<
	Range::IsRangeOfFiniteForwardRangesOf<RWs, typename R::value_type>::_ && Range::IsFiniteRange<RWs>::_,
bool> StartsWithAny(const R& range, const RWs& subranges, size_t* ioIndex=null)
{
	RWs subrangesCopy = subranges;
	return StartsWithAnyAdvance(range, subrangesCopy, ioIndex);
}

}}
