#pragma once

#include "Range/Concepts.h"
#include "Range/Operations.h"
#include "Range/Construction/Take.h"
#include "Core/Debug.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4365)
#endif

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#endif

namespace Intra {namespace Algo {

template<typename R, typename OR> Meta::EnableIf<
	Range::IsFiniteInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	Range::IsOutputRange<OR>::_ &&
	!(Range::IsArrayRange<R>::_ && Range::IsArrayRangeOfExactly<OR, Range::ValueTypeOf<R>>::_)
> CopyAdvanceToAdvance(R&& src, OR&& dst)
{
	while(!src.Empty())
	{
		dst.Put(src.First());
		src.PopFirst();
	}
}

template<typename R, typename OR> Meta::EnableIf<
	Range::IsInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	Range::IsOutputRange<OR>::_ &&
	!(Range::IsArrayRange<R>::_ && Range::IsArrayRangeOfExactly<OR, Range::ValueTypeOf<R>>::_)
> CopyAdvanceToAdvance(R&& src, size_t n, OR&& dst)
{
	while(!src.Empty() && n--!=0)
	{
		dst.Put(src.First());
		src.PopFirst();
	}
}

template<typename R, typename OR> forceinline Meta::EnableIf<
	Range::IsFiniteInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	Range::IsOutputRange<OR>::_ &&
	!(Range::IsArrayRange<R>::_ && Range::IsArrayRangeOfExactly<OR, Range::ValueTypeOf<R>>::_)
> CopyAdvanceTo(R&& src, const OR& dst)
{CopyAdvanceToAdvance(src, OR(dst));}

template<typename R, typename OR> forceinline Meta::EnableIf<
	Range::IsInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	Range::IsOutputRange<OR>::_ &&
	!(Range::IsArrayRange<R>::_ && Range::IsArrayRangeOfExactly<OR, Range::ValueTypeOf<R>>::_)
> CopyAdvanceTo(R&& src, size_t n, const OR& dst)
{CopyAdvanceToAdvance(src, n, OR(dst));}

template<typename R, typename DstRange> forceinline Meta::EnableIf<
	Range::IsArrayRange<R>::_ && !Meta::IsConst<R>::_ &&
	Range::IsArrayRangeOfExactly<DstRange, Range::ValueTypeOf<R>>::_
> CopyAdvanceTo(R&& src, const DstRange& dst)
{
	CopyTo(src, dst);
	Range::PopFirstExactly(src, src.Length());
}

template<typename R, typename DstRange> forceinline Meta::EnableIf<
	Range::IsArrayRange<R>::_ && !Meta::IsConst<R>::_ &&
	Range::IsArrayRangeOfExactly<DstRange, Range::ValueTypeOf<R>>::_
> CopyAdvanceTo(R&& src, size_t n, const DstRange& dst)
{
	CopyTo(Range::Take(src, n), dst);
	Range::PopFirstN(src, n);
}

template<typename R, typename DstRange> forceinline Meta::EnableIf<
	Range::IsArrayRange<R>::_ &&
	Range::IsArrayRangeOfExactly<DstRange, Range::ValueTypeOf<R>>::_
> CopyTo(const R& range, const DstRange& dst)
{
	INTRA_ASSERT(dst.Length()>=range.Length());
	C::memcpy(dst.Data(), range.Data(), range.Length()*sizeof(range.First()));
}

template<typename R, typename OR> forceinline Meta::EnableIf<
	Range::IsArrayRange<R>::_ &&
	Range::IsArrayRangeOfExactly<OR, Range::ValueTypeOf<R>>::_ && !Meta::IsConst<OR>::_
> CopyToAdvance(const R& range, OR&& dst)
{
	CopyTo(range, dst);
	Range::PopFirstExactly(dst, range.Length());
}


template<typename R, typename OR> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	!(Range::IsArrayRange<R>::_ && Range::IsArrayRangeOfExactly<OR, Range::ValueTypeOf<R>>::_) &&
	(Range::IsOutputRange<OR>::_ ||
		Range::HasAsRange<Meta::RemoveConstRef<OR>>::_)
> CopyToAdvance(const R& range, OR&& dst)
{CopyAdvanceToAdvance(R(range), Meta::Forward<OR>(dst));}

template<typename R, typename OR> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	(Range::IsOutputRange<OR>::_ ||
		Range::HasAsRange<OR>::_) &&
	!(Range::IsArrayRange<R>::_ && Range::IsArrayRangeOfExactly<OR, Range::ValueTypeOf<R>>::_)
> CopyTo(const R& range, OR&& dst)
{
	auto dst2 = Range::AsRange(Meta::Forward<OR>(dst));
	CopyToAdvance(range, dst2);
}

template<typename T1, typename T2, size_t N1, size_t N2> forceinline Meta::EnableIf<
	!Meta::IsConst<T2>::_
> CopyTo(const T1(&src)[N1], T2(&dst)[N2])
{CopyTo(ArrayRange<const T1>(src), ArrayRange<T2>(dst));}

template<typename R, typename T, size_t N> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	!Meta::IsConst<T>::_
> CopyTo(const R& src, T(&dst)[N])
{CopyTo(src, ArrayRange<T>(dst));}

template<typename OR, typename T, size_t N> forceinline Meta::EnableIf<
	Range::IsOutputRange<OR>::_
> CopyTo(const T(&src)[N], OR&& dst)
{CopyTo(ArrayRange<const T>(src), dst);}



template<typename R, typename OR, typename P> Meta::EnableIf<
	Range::IsFiniteInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	Range::IsOutputRange<OR>::_ &&
	Meta::IsCallable<P, Range::ValueTypeOf<R>>::_
> CopyAdvanceToAdvance(R&& src, OR&& dst, P pred)
{
	while(!src.Empty())
	{
		auto value = src.First();
		if(pred(value)) dst.Put(value);
		src.PopFirst();
	}
}

template<typename R, typename OR, typename P> forceinline Meta::EnableIf<
	Range::IsFiniteInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	Range::IsOutputRange<OR>::_ &&
	Meta::IsCallable<P, Range::ValueTypeOf<R>>::_
> CopyAdvanceTo(R&& src, const OR& dst, P pred)
{CopyAdvanceToAdvance(src, OR(dst), pred);}

template<typename R, typename OR, typename P> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	Range::IsOutputRange<OR>::_ &&
	Meta::IsCallable<P, Range::ValueTypeOf<R>>::_
> CopyToAdvance(const R& range, OR&& dst, P pred)
{CopyAdvanceToAdvance(R(range), Meta::Forward<OR>(dst), pred);}

template<typename R, typename OR, typename P> forceinline Meta::EnableIf<
	Range::IsFiniteForwardRange<R>::_ &&
	(Range::IsOutputRange<OR>::_ ||
		Range::HasAsRange<OR>::_) &&
	Meta::IsCallable<P, Range::ValueTypeOf<R>>::_
> CopyTo(const R& range, const OR& dst, P pred)
{
	auto dst2 = Range::AsRange(dst);
	CopyToAdvance(range, dst2, pred);
}

template<typename R, typename DstRange> forceinline Meta::EnableIf<
	Range::IsArrayRange<R>::_ && !Meta::IsConst<R>::_ &&
	Range::IsArrayRangeOfExactly<DstRange, Range::ValueTypeOf<R>>::_ && !Meta::IsConst<DstRange>::_
> CopyAdvanceToAdvance(R&& range, DstRange&& dst)
{
	CopyToAdvance(range, dst);
	Range::PopFirstExactly(range, range.Length());
}

}}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif


#ifdef _MSC_VER
#pragma warning(pop)
#endif
