#pragma once

#include "Core/Core.h"
#include "Meta/Type.h"
#include "Range/Concepts.h"


namespace Intra { namespace Range {

template<typename R> struct RetroResult;

template<typename R, typename T, class PARENT> struct BidirectionalRangeMixin: PARENT
{
private:
	forceinline const R& me() const {return *static_cast<const R*>(this);}
	forceinline R& me() {return *static_cast<R*>(this);}

	template<typename RW> Meta::EnableIf<
		IsBidirectionalRangeOf<RW, T>::_,
	bool> ends_with(RW& what) const
	{
		auto temp = me();
		while(!what.Empty())
		{
			if(temp.Empty()) return false;
			if(temp.Last()!=what.Last()) return false;
			temp.PopLast();
			what.PopLast();
		}
		return true;
	}
public:
	template<typename U=R> forceinline decltype(Meta::Val<U>().Retro().begin()) rbegin() const {return me().Retro().begin();}
	template<typename U=R> forceinline decltype(Meta::Val<U>().Retro().end()) rend() const {return me().Retro().end();}

	forceinline RetroResult<R> Retro() const {return RetroResult<R>(me());}

	forceinline size_t PopLastN(size_t n)
	{
		for(size_t i=0; i<n; i++)
		{
			if(me().Empty()) return i;
			me().PopLast();
		}
		return n;
	}

	forceinline void PopLastExactly(size_t elementsToPop)
	{
		for(size_t i=0; i<elementsToPop; i++) me().PopLast();
	}

	forceinline R DropBack() const
	{
		if(me().Empty()) return me();
		auto result = me();
		result.PopLast();
		return result;
	}

	forceinline R DropBack(size_t n) const
	{
		auto result = me();
		result.PopLastN(n);
		return result;
	}

	forceinline R DropBackExactly() const
	{
		auto result = me();
		result.PopLast();
		return result;
	}

	forceinline R DropBackExactly(size_t n) const
	{
		auto result = me();
		result.PopLastExactly(n);
		return result;
	}

	template<typename IndexRange> R Remove(IndexRange indices)
	{
		if(indices.Empty()) return me();
		INTRA_ASSERT(indices.IsSorted([](size_t a, size_t b){return a<b;}));
		size_t nextIndex = indices.First();
		indices.PopFirst();
		auto range = me();
		R dst = range.Drop(nextIndex), src = dst;
		for(size_t i=nextIndex; !src.Empty(); i++)
		{
			if(i==nextIndex)
			{
				nextIndex = indices.First();
				indices.PopFirst();
				src.PopFirst();
				range.PopLast();
				continue;
			}
			dst.First() = src.First();
			src.PopFirst();
			dst.PopFirst();
		}
		return range;
	}

	template<typename P> Meta::EnableIf<
		Meta::IsCallable<P, T>::_,
	R> Remove(P pred)
	{
		R range = me();
		R dst = range, src = dst;
		bool somethingRemoved = false;
		for(size_t i=0; !src.Empty(); i++)
		{
			if(pred(src.First()))
			{
				src.PopFirst();
				somethingRemoved = true;
				range.PopLast();
				continue;
			}
			if(somethingRemoved) dst.First() = src.First();
			src.PopFirst();
			dst.PopFirst();
		}
		return range;
	}


	template<typename RW, typename U=R> Meta::EnableIf<
		IsBidirectionalRangeOf<RW, T>::_ &&
		!(HasLength<U>::_ && HasLength<RW>::_),
	bool> EndsWith(const RW& what) const
	{
		return ends_with(what);
	}

	template<typename RW, typename U=R> Meta::EnableIf<
		IsBidirectionalRangeOf<RW, T>::_ &&
		HasLength<U>::_ && HasLength<RW>::_,
	bool> EndsWith(const RW& what) const
	{
		if(me().Length()<what.Length()) return false;
		return ends_with(what);
	}

	template<size_t N> bool EndsWith(const T(&rhs)[N]) {return me().EndsWith(AsRange(rhs));}


	//! Возвращает диапазон, полученный из этого диапазона удалением всех последних символов, равных x.
	template<typename X> Meta::EnableIf<
		Meta::IsConvertible<X, T>::_
	> TrimRightAdvance(X x)
	{
		while(!me().Empty() && me().Last()==x)
			me().PopLast();
	}

	//! Возвращает диапазон, полученный из этого диапазона удалением всех последних символов, для которых выполнен предикат pred.
	template<typename P> Meta::EnableIf<
		Meta::IsCallable<P, T>::_
	> TrimRightAdvance(P pred)
	{
		while(!me().Empty() && pred(me().Last()))
			me().PopLast();
	}

	//! Возвращает диапазон, полученный из этого диапазона удалением всех первых последних символов, равных x.
	template<typename X> Meta::EnableIf<
		Meta::IsConvertible<X, T>::_
	> TrimAdvance(X x) {TrimLeftAdvance(x); TrimRightAdvance(x);}

	//! Возвращает диапазон, полученный из этого диапазона удалением всех первых последних символов, равных x.
	template<typename P> Meta::EnableIf<
		Meta::IsCallable<P, T>::_
	> TrimAdvance(P pred) {TrimLeftAdvance(pred); TrimRightAdvance(pred);}


	//! Возвращает диапазон, полученный из этого диапазона удалением всех последних символов, равных x.
	template<typename X> Meta::EnableIf<
		Meta::IsConvertible<X, T>::_,
	R> TrimRight(X x) const
	{
		R result = me();
		result.TrimLeftAdvance(x);
		return result;
	}

	//! Возвращает диапазон, полученный из этого диапазона удалением всех последних символов, для которых выполнен предикат pred.
	template<typename P> forceinline Meta::EnableIf<
		Meta::IsCallable<P, T>::_,
	R> TrimRight(P pred) const
	{
		R result = me();
		result.TrimRightAdvance(pred);
		return result;
	}

	//! Возвращает диапазон, полученный из этого диапазона удалением всех первых последних символов, равных x.
	template<typename X> forceinline Meta::EnableIf<
		Meta::IsConvertible<X, T>::_,
	R> Trim(X x) const {return me().TrimLeft(x).TrimRight(x);}

	//! Возвращает диапазон, полученный из этого диапазона удалением всех первых последних символов, равных x.
	template<typename P> forceinline Meta::EnableIf<
		Meta::IsCallable<P, T>::_,
	R> Trim(P pred) const {return me().TrimLeft(pred).TrimRight(pred);}
};




}}


