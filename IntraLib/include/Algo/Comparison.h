#pragma once

#include "Algo/Op.h"
#include "Range/Concepts.h"

namespace Intra { namespace Algo {

template<typename R1, typename R2> forceinline Meta::EnableIf<
	Range::IsFiniteRange<R1>::_ != Range::IsFiniteRange<R2>::_,
bool> Equals(R1 r1, R2 r2) {(void)r1; (void)r2; return false;}

namespace D {

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
{return D::Equals(r1, r2, Op::Equal);}

template<typename R1, typename R2, typename P> forceinline Meta::EnableIf<
	Range::IsForwardRange<R1>::_ &&
	Range::IsForwardRange<R2>::_ &&
	Range::HasLength<R1>::_ &&
	Range::HasLength<R2>::_,
bool> Equals(const R1& r1, const R2& r2, P pred)
{
	if(r1.Length()!=r2.Length()) return false;
	return D::Equals(r1, r2, pred);
}

template<typename R1, typename R2> inline Meta::EnableIf<
	Range::IsInputRange<R1>::_ && Meta::IsAlmostPod<Range::ValueTypeOf<R1>>::_ &&
	Range::ValueTypeEquals<R1, R2>::_ &&
	Range::IsArrayRange<R1>::_ &&
	Range::IsArrayRange<R2>::_,
bool> Equals(const R1& r1, const R2& r2)
{
	if(r1.Length()!=r2.Length()) return false;
	return C::memcmp(r1.Data(), r2.Data(), r1.Length()*sizeof(Range::ValueTypeOf<R1>))==0;
}

template<typename T1, typename T2, size_t N1, size_t N2> forceinline
bool Equals(T1(&arr1)[N1], T2(&arr2)[N2])
{return Equals(ArrayRange<T1>(arr1), ArrayRange<T2>(arr2));}

template<typename R, typename T, size_t N> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_,
bool> Equals(const R& range, T(&arr)[N])
{return Equals(range, ArrayRange<T>(arr));}

template<typename R, typename T, size_t N> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_,
bool> Equals(T(&arr)[N], const R& range)
{return Equals(ArrayRange<T>(arr), range);}


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
	int result = C::memcmp(r1.Data(), r2.Data(), minLen*sizeof(r1.First()));
	if(result!=0) return result;
	if(r1.Length()<r2.Length()) return -1;
	if(r1.Length()>r2.Length()) return 1;
	return 0;
}


namespace D {

template<typename R, typename RW> Meta::EnableIf<
	Range::IsForwardRange<R>::_ &&
	Range::IsFiniteForwardRange<RW>::_ &&
	Range::ValueTypeIsConvertible<R, Range::ValueTypeOf<RW>>::_,
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
	Range::IsFiniteForwardRange<RW>::_ && Range::ValueTypeIsConvertible<RW, Range::ValueTypeOf<R>>::_ &&
	!(Range::HasLength<R>::_ && Range::HasLength<RW>::_),
bool> StartsWith(const R& range, const RW& what)
{return D::StartsWith(range, what);}

template<typename R, typename RW> forceinline Meta::EnableIf<
	Range::IsForwardRange<R>::_ &&
	Range::IsForwardRange<RW>::_ &&
	Range::ValueTypeIsConvertible<RW, Range::ValueTypeOf<R>>::_ &&
	!(Range::HasData<R>::_ && Range::HasData<RW>::_ && Meta::IsAlmostPod<Range::ValueTypeOf<R>>::_) &&
	Range::HasLength<R>::_ && Range::HasLength<RW>::_,
bool> StartsWith(const R& range, const RW& what)
{
	if(range.Length()<what.Length()) return false;
	return D::StartsWith(range, what);
}

template<typename R, size_t N> forceinline Meta::EnableIf<
	Range::IsArrayRange<R>::_,
bool> StartsWith(const R& range, const Range::ValueTypeOf<R>(&rhs)[N])
{return StartsWith(range, Range::AsRange(rhs));}

template<typename R, typename RW> forceinline Meta::EnableIf<
	Range::IsArrayRange<R>::_ &&
	Range::IsArrayRangeOfExactly<RW, Range::ValueTypeOf<R>>::_ &&
	Meta::IsAlmostPod<Range::ValueTypeOf<R>>::_,
bool> StartsWith(const R& range, const RW& what)
{
	if(range.Length()<what.Length()) return false;
	return C::memcmp(range.Data(), what.Data(), what.Length()*sizeof(what.First()))==0;
}

template<typename R, typename RWs> Meta::EnableIf<
	Range::IsForwardRange<R>::_ &&
	Range::IsFiniteRange<RWs>::_ &&
	Range::IsFiniteForwardRange<Range::ValueTypeOf<RWs>>::_ &&
	Range::ValueTypeIsConvertible<Range::ValueTypeOf<RWs>, Range::ValueTypeOf<R>>::_,
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
	Range::IsFiniteRange<RWs>::_ &&
	Range::IsFiniteForwardRange<Range::ValueTypeOf<RWs>>::_ &&
	Range::ValueTypeIsConvertible<Range::ValueTypeOf<RWs>, Range::ValueTypeOf<R>>::_,
bool> StartsWithAny(const R& range, const RWs& subranges, size_t* ioIndex=null)
{
	RWs subrangesCopy = subranges;
	return StartsWithAnyAdvance(range, subrangesCopy, ioIndex);
}


namespace D {

template<typename R, typename RW> Meta::EnableIf<
	Range::IsBidirectionalRange<R>::_ &&
	Range::IsBidirectionalRange<RW>::_ &&
	Range::ValueTypeIsConvertible<RW, Range::ValueTypeOf<R>>::_,
bool> EndsWith(const R& range, const RW& what)
{
	auto rangeCopy = range;
	auto whatCopy = what;
	while(!whatCopy.Empty())
	{
		if(rangeCopy.Empty()) return false;
		if(rangeCopy.Last()!=whatCopy.Last()) return false;
		rangeCopy.PopLast();
		whatCopy.PopLast();
	}
	return true;
}

}

template<typename R, typename RW> forceinline Meta::EnableIf<
	Range::IsArrayRange<R>::_ &&
	Range::IsArrayRange<RW>::_ && Range::ValueTypeIsConvertible<RW, Range::ValueTypeOf<R>>::_,
bool> EndsWith(const R& range, const RW& what)
{
	if(range.Length()<what.Length()) return false;
	return C::memcmp(range.Data()+range.Length()-what.Length(),
		what.Data(), what.Length()*sizeof(what.First()))==0;
}

template<typename R, size_t N> forceinline Meta::EnableIf<
	Range::IsArrayRange<R>::_,
bool> EndsWith(const R& range, const Range::ValueTypeOf<R>(&rhs)[N])
{return EndsWith(range, Range::AsRange(rhs));}

template<typename R, typename RW> forceinline Meta::EnableIf<
	Range::IsBidirectionalRange<R>::_ &&
	Range::IsBidirectionalRange<RW>::_ &&
	Range::ValueTypeIsConvertible<RW, Range::ValueTypeOf<R>>::_ &&
	!(Range::HasLength<R>::_ && Range::HasLength<RW>::_),
bool> EndsWith(const R& range, const RW& what)
{return D::EndsWith(what);}

template<typename R, typename RW> forceinline Meta::EnableIf<
	Range::IsBidirectionalRange<R>::_ &&
	Range::IsBidirectionalRange<RW>::_ &&
	Range::ValueTypeIsConvertible<RW, Range::ValueTypeOf<R>>::_ &&
	Range::HasLength<R>::_ && Range::HasLength<RW>::_ &&
	!(Range::HasData<R>::_ && Range::HasData<RW>::_),
bool> EndsWith(const R& range, const RW& what)
{
	if(range.Length()<what.Length()) return false;
	return D::EndsWith(what);
}

template<typename R, size_t N> forceinline Meta::EnableIf<
	Range::IsBidirectionalRange<R>::_,
bool> EndsWith(const R& range, const Range::ValueTypeOf<R>(&rhs)[N])
{return EndsWith(range, Range::AsRange(rhs));}


}}
