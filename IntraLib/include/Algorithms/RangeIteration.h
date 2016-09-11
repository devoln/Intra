#pragma once

#include "Range.h"
#include "Utils/Optional.h"
#include "Meta/Type.h"

namespace Intra { namespace Range {


template<typename R, typename P> struct FilterResult:
	RangeMixin<FilterResult<R, P>, typename R::value_type,
	R::RangeType>=TypeEnum::RandomAccess? TypeEnum::Bidirectional: R::RangeType, R::RangeIsFinite>
{
	typedef typename R::value_type value_type;
	typedef typename R::return_value_type return_value_type;

	forceinline FilterResult(null_t=null): original_range(null), predicate() {}
	forceinline FilterResult(const R& range, P filterPredicate):
		original_range(range), predicate(filterPredicate) {skip_falses_front(original_range, filterPredicate);}

	forceinline return_value_type First() const
	{
		INTRA_ASSERT(!Empty());
		return original_range.First();
	}

	forceinline void PopFirst()
	{
		original_range.PopFirst();
		skip_falses_front(original_range, predicate());
	}

	template<typename U=R> Meta::EnableIf<
		IsBidirectionalRange<U>::_,
	return_value_type> Last() const
	{
		INTRA_ASSERT(!Empty());
		auto&& b = original_range.Last();
		if(predicate()(b)) return b;

		auto copy = original_range;
		copy.PopLast();
		skip_falses_back(copy, predicate());
		return copy.Last();
	}

	template<typename U=R> forceinline Meta::EnableIf<
		IsBidirectionalRange<U>::_
	> PopLast()
	{
		skip_falses_back(original_range, predicate());
		original_range.PopLast();
	}

	forceinline bool Empty() const {return original_range.Empty() || predicate==null;}

	//TODO BUG: FilterResults with same range but different predicates are considered equal!
	forceinline bool operator==(const FilterResult& rhs) const {return original_range==rhs.original_range;}

private:
	static void forceinline skip_falses_front(R& original_range, const P& p)
	{
		while(!original_range.Empty() && !p(original_range.First())) original_range.PopFirst();
	}

	static void forceinline skip_falses_back(R& original_range, const P& p)
	{
		while(!original_range.Empty() && !p(original_range.Last())) original_range.PopLast();
	}

	R original_range;
	Utils::Optional<P> predicate;
};


template<typename R, typename F> struct MapResult:
	RangeMixin<MapResult<R, F>,
		Meta::ResultOf<F, typename R::value_type>,
		R::RangeType, R::RangeIsFinite>
{
	typedef Meta::ResultOf<F, typename R::value_type> value_type;
	typedef Meta::ResultOf<F, typename R::return_value_type> return_value_type;

	forceinline MapResult(null_t=null): OriginalRange(null), Function() {}
	forceinline MapResult(const R& range, F func): OriginalRange(range), Function(func) {}

	forceinline return_value_type First() const {return Function()(OriginalRange.First());}
	forceinline void PopFirst() {OriginalRange.PopFirst();}
	forceinline bool Empty() const {return OriginalRange.Empty() || Function==null;}

	template<typename U=R> forceinline Meta::EnableIf<
		IsBidirectionalRange<U>::_,
	return_value_type> Last() const {return Function()(OriginalRange.Last());}

	template<typename U=R> forceinline Meta::EnableIf<
		IsBidirectionalRange<U>::_
	> PopLast() {OriginalRange.PopLast();}

	template<typename U=R> forceinline Meta::EnableIf<
		IsRandomAccessRange<U>::_,
	return_value_type> operator[](size_t index) const {return Function()(OriginalRange[index]);}

	template<typename U=R> forceinline Meta::EnableIf<
		HasLength<U>::_,
	size_t> Length() const {return OriginalRange.Length();}

	//TODO BUG: MapResults with same range but different functions are considered to be equal!
	forceinline bool operator==(const MapResult& rhs) const {return OriginalRange==rhs.OriginalRange;}

	R OriginalRange;
	Utils::Optional<F> Function;
};

template<typename T, typename... RANGES> struct ChainResult;

template<typename T, typename R0, typename R1, typename... RANGES> struct ChainResult<T, R0, R1, RANGES...>:
	CommonTypedAllFiniteRangeMixin<ChainResult<T, R0, R1, RANGES...>, T, R0, R1, RANGES...>
{
	enum {RangeType = CommonRangeCategoryAllFinite<R0, R1, RANGES...>::Type};
	typedef T value_type;
	typedef Meta::CommonTypeRef<typename R0::return_value_type, typename R1::return_value_type, typename RANGES::return_value_type...> return_value_type;

	forceinline ChainResult(null_t=null) {}

	forceinline ChainResult(R0&& r0, R1&& r1, RANGES&&... ranges):
		range0(core::forward<R0>(r0)), next(core::forward<R1>(r1), core::forward<RANGES>(ranges)...) {}

	forceinline return_value_type First() const
	{
		return range0.Empty()? next.First(): range0.First();
	}

	forceinline void PopFirst()
	{
		if(!range0.Empty()) range0.PopFirst();
		else next.PopFirst();
	}

	forceinline bool Empty() const {return range0.Empty() && next.Empty();}

	template<typename U=R0> forceinline Meta::EnableIf<
		RangeType>=TypeEnum::Bidirectional,
	return_value_type> Last() const {return next.Empty()? range0.Last(): next.Last();}

	template<typename U=R0> forceinline Meta::EnableIf<
		RangeType>=TypeEnum::Bidirectional
	> PopLast()
	{
		if(next.Empty()) range0.PopLast();
		else next.PopLast();
	}


	template<typename U=R0> forceinline Meta::EnableIf<
		HasLength<U>::_ && HasLength<ChainResult<T, R1, RANGES...>>::_,
	size_t> Length() const {return range0.Length()+next.Length();}


	template<typename U=R0> forceinline Meta::EnableIf<
		RangeType>=TypeEnum::RandomAccess,
	return_value_type> operator[](size_t index) const
	{
		size_t len = range0.Length();
		if(index<len) return range0[index];
		return next[index-len];
	}

	template<typename U=R0> forceinline Meta::EnableIf<
		U::RangeType>=TypeEnum::RandomAccess,
	ChainResult> opSlice(size_t start, size_t end) const
	{
		const size_t len = range0.Length();
		return ChainResult(
			range0.opSlice(start>len? len: start, len>end? end: len),
			next.opSlice(start>len? start-len: 0, end>len? end-len: 0)
		);
	}

	forceinline bool operator==(const ChainResult& rhs) const {return range0==rhs.range0 && next==rhs.next;}

private:
	R0 range0;
	ChainResult<T, R1, RANGES...> next;

	forceinline ChainResult(R0&& r0, ChainResult<T, R1, RANGES...>&& ranges):
		range0(core::forward<R0>(r0)), next(core::move(ranges)) {}
};

template<typename T, typename R0> struct ChainResult<T, R0>:
	RangeMixin<ChainResult<T, R0>, T, R0::RangeType, R0::RangeIsFinite>
{
	typedef T value_type;
	typedef typename R0::return_value_type return_value_type;

	forceinline ChainResult(null_t=null) {}

	forceinline ChainResult(R0&& r0): range0(core::forward<R0>(r0)) {}

	forceinline return_value_type First() const
	{
		INTRA_ASSERT(!range0.Empty());
		return range0.First();
	}

	forceinline void PopFirst()
	{
		INTRA_ASSERT(!range0.Empty());
		range0.PopFirst();
	}

	forceinline bool Empty() const {return range0.Empty();}

	template<typename U=R0> forceinline Meta::EnableIf<
		U::RangeType>=TypeEnum::Bidirectional,
	return_value_type> Last() const {return range0.Last();}

	template<typename U=R0> forceinline Meta::EnableIf<
		U::RangeType>=TypeEnum::Bidirectional
	> PopLast()
	{
		range0.PopLast();
	}

	template<typename U=R0> forceinline Meta::EnableIf<
		HasLength<U>::_,
	size_t> Length() const {return range0.Length();}

	template<typename U=R0> forceinline Meta::EnableIf<
		IsRandomAccessRange<U>::_,
	return_value_type> operator[](size_t index) const {return range0[index];}

	template<typename U=R0> forceinline Meta::EnableIf<
		U::RangeType>=TypeEnum::RandomAccess,
	ChainResult> opSlice(size_t start, size_t end) const {return ChainResult(range0.opSlice(start, end));}

	forceinline bool operator==(const ChainResult& rhs) const {return range0==rhs.range0;}


private:
	R0 range0;
};


template<typename R0, typename... RANGES> forceinline
ChainResult<Meta::CommonType<typename R0::value_type, typename RANGES::value_type...>, R0, RANGES...>
Chain(R0&& range0, RANGES&&... ranges)
{
	return {core::forward<R0>(range0), core::forward<RANGES>(ranges)...};
}


template<typename T, typename R0, typename... RANGES> struct RoundRobinResult:
	RangeMixin<RoundRobinResult<T, R0, RANGES...>, T,
	CommonRangeCategoryAllFinite<R0, RANGES...>::Type==TypeEnum::Input? TypeEnum::Input: TypeEnum::Forward,
	CommonRangeCategoryAllFinite<R0, RANGES...>::Finite>
{
	typedef T value_type;
	typedef Meta::CommonTypeRef<typename R0::return_value_type, typename RANGES::return_value_type...> return_value_type;

	forceinline RoundRobinResult(null_t=null): counter(Meta::NumericLimits<decltype(counter)>::Max()) {}

	forceinline RoundRobinResult(R0&& r0, RANGES&&... ranges):
		range0(core::forward<R0>(r0)), next(core::forward<RANGES>(ranges)...),
		counter(r0.Empty()? Meta::NumericLimits<decltype(counter)>::Max(): 0) {}

	return_value_type First() const
	{
		return (!next.before_(counter) && !range0.Empty())?
			range0.First():
			next.First();
	}

	void PopFirst()
	{
		if(!next.before_(counter))
		{
			counter++;
			range0.PopFirst();
			if(range0.Empty())
				counter = Meta::NumericLimits<decltype(counter)>::Max();
			return;
		}
		next.PopFirst();
	}

	forceinline bool Empty() const {return range0.Empty() && next.Empty();}

	forceinline bool operator==(const RoundRobinResult& rhs) const
	{
		return (range0.Empty() && rhs.range0.Empty() ||
			range0==rhs.range0 && counter==rhs.counter) && next==rhs.next;
	}

	template<typename U=R0> forceinline Meta::EnableIf<
		HasLength<U>::_ && HasLength<RoundRobinResult<T, RANGES...>>::_,
	size_t> Length() const {return range0.Length()+next.Length();}

	//template<typename U=R0> forceinline Meta::EnableIf<
		//HasSave<U>::_ && HasSave<RoundRobinResult<T, RANGES...>>::_,
	//RoundRobinResult> Save() const {return {range0.Save(), next.Save()};}

	bool before_(size_t prevCounter) const
	{
		return counter<prevCounter || next.before_(prevCounter);
	}

private:
	R0 range0;
	RoundRobinResult<T, RANGES...> next;
	size_t counter;
};

template<typename T, typename R0> struct RoundRobinResult<T, R0>:
	RangeMixin<RoundRobinResult<T, R0>, T, R0::RangeType, R0::RangeIsFinite>
{
	typedef T value_type;
	typedef typename R0::return_value_type return_value_type;

	forceinline RoundRobinResult(null_t=null):
		counter(Meta::NumericLimits<decltype(counter)>::Max()) {}

	forceinline RoundRobinResult(R0&& r0):
		range0(core::forward<R0>(r0)), counter(r0.Empty()? Meta::NumericLimits<decltype(counter)>::Max(): 0) {}


	forceinline return_value_type First() const
	{
		INTRA_ASSERT(!Empty());
		return range0.First();
	}

	forceinline void PopFirst()
	{
		INTRA_ASSERT(!range0.Empty());
		counter++;
		range0.PopFirst();
		if(range0.Empty())
			counter = Meta::NumericLimits<decltype(counter)>::Max();
	}

	forceinline bool Empty() const {return range0.Empty();}

	forceinline bool operator==(const RoundRobinResult& rhs) const
	{
		return range0.Empty() && rhs.range0.Empty() ||
			range0==rhs.range0 && counter==rhs.counter;
	}

	template<typename U=R0> forceinline Meta::EnableIf<
		HasLength<U>::_,
	size_t> Length() const {return range0.Length();}

	bool before_(size_t prevCounter) const
	{
		return counter<prevCounter;
	}

private:
	R0 range0;
	size_t counter;
};


template<typename R0, typename... RANGES> forceinline
RoundRobinResult<Meta::CommonType<typename R0::value_type, typename RANGES::value_type...>, R0, RANGES...>
RoundRobin(R0&& range0, RANGES&&... ranges)
{
	return {core::forward<R0>(range0), core::forward<RANGES>(ranges)...};
}



template<typename R> struct StrideResult:
	RangeMixin<StrideResult<R>, typename R::value_type,
	R::RangeType!=TypeEnum::Bidirectional || HasLength<R>::_? R::RangeType: TypeEnum::Forward, R::RangeIsFinite>
{
	typedef typename R::value_type value_type;
	typedef typename R::value_type return_value_type;

	forceinline StrideResult(null_t=null): original_range(null), step(0) {}
	forceinline StrideResult(const R& range, size_t strideStep):
		original_range(range), step(strideStep) {INTRA_ASSERT(strideStep!=0); skip_back_odd();}

	template<typename U=R> forceinline Meta::EnableIf<
		HasLength<U>::_ || !U::RangeIsFinite,
		bool> Empty() const {return original_range.Empty();}

	template<typename U=R> forceinline Meta::EnableIf<
		!HasLength<U>::_ && U::RangeIsFinite,
		bool> Empty() const {return original_range.Empty();}


	forceinline return_value_type First() const {return original_range.First();}
	forceinline void PopFirst() {original_range.PopFirstN(step);}

	template<typename U=R> forceinline Meta::EnableIf<
		U::RangeType>=TypeEnum::Bidirectional && HasLength<U>::_,
	return_value_type> Last() const {return original_range.Last();}

	template<typename U=R> forceinline Meta::EnableIf<
		U::RangeType>=TypeEnum::Bidirectional && HasLength<U>::_
	> PopLast() {original_range.PopLastN(step);}

	template<typename U=R> forceinline Meta::EnableIf<
		U::RangeType==TypeEnum::RandomAccess,
	return_value_type> operator[](size_t index) const {return original_range[index*step];}

	template<typename U=R> forceinline Meta::EnableIf<
		U::RangeType==TypeEnum::RandomAccess,
	return_value_type> opSlice(size_t start, size_t end) const {return original_range.opSlice(start*step, end*step);}

	forceinline bool operator==(const StrideResult& rhs) const
	{
		return step==rhs.step && (step==0 || original_range==rhs.original_range);
	}

	template<typename U=R> forceinline Meta::EnableIf<
		HasLength<U>::_ || !U::RangeIsFinite,
	size_t> Length() const {return (original_range.Length()+step-1)/step;}

	StrideResult Stride(size_t strideStep) const
	{
		return StrideResult(original_range, step*strideStep);
	}

private:
	template<typename U=R> forceinline Meta::EnableIf<
		U::RangeType>=TypeEnum::Bidirectional && HasLength<U>::_
	> skip_back_odd()
	{
		size_t len = original_range.Length();
		if(len==0) return;
		original_range.PopLastExactly((len-1) % step);
	}

	template<typename U=R> forceinline Meta::EnableIf<
		U::RangeType<TypeEnum::Bidirectional || !HasLength<U>::_
	> skip_back_odd() {}

	R original_range;
	size_t step;
};

template<typename R> StrideResult<R> Stride(const R& r, size_t step)
{
	return StrideResult<R>(r, step);
}


template<typename... RANGES> struct ZipResult:
	RangeMixin<ZipResult<RANGES...>, TupleOfElement<RANGES...>,
		CommonRangeCategoryAnyFinite<RANGES...>::Type, CommonRangeCategoryAnyFinite<RANGES...>::Finite>
{
	enum {RangeType = CommonRangeCategoryAnyFinite<RANGES...>::Type};
	typedef TupleOfElement<RANGES...> value_type;
	typedef TupleOfReturnElement<RANGES...> return_value_type;
	typedef Meta::Tuple<RANGES...> OriginalRangeTuple;
	OriginalRangeTuple OriginalRanges;

	forceinline ZipResult(null_t=null) {}
	forceinline ZipResult(const RANGES&... ranges): OriginalRanges(ranges...) {}
	forceinline ZipResult(OriginalRangeTuple ranges): OriginalRanges(ranges) {}

	forceinline return_value_type First() const {return OriginalRanges.template TransformEach<Fronter>(Fronter());}
	forceinline void PopFirst() {OriginalRanges.ForEachField(PopFronter());}
	forceinline bool Empty() const {return AnyEmpty(OriginalRanges);}

	template<typename U=OriginalRangeTuple> forceinline
		Meta::EnableIfCompiles<size_t, decltype(MinLength(U()))>
		Length() const {return MinLength(OriginalRanges);}

	template<typename U=ZipResult> Meta::EnableIf<
		U::RangeType>=TypeEnum::RandomAccess,
	return_value_type> operator[](size_t index) const {return OriginalRanges.TransformEach(Indexer{index});}

	template<typename U=ZipResult> Meta::EnableIf<
		U::RangeType>=TypeEnum::RandomAccess,
	ZipResult> opSlice(size_t start, size_t end) const {return OriginalRanges.TransformEach(Slicer{start, end});}

	forceinline bool operator==(const ZipResult& rhs) const {return OriginalRanges==rhs.OriginalRanges;}
};


template<typename R0, typename... RANGES> forceinline
ZipResult<R0, RANGES...> Zip(const R0& range0, const RANGES&... ranges)
{
	return ZipResult<R0, RANGES...>(range0, ranges...);
}




template<size_t N, typename RangeOfTuples> struct UnzipResult:
	RangeMixin<UnzipResult<N, RangeOfTuples>, Meta::TypeListAt<N, typename RangeOfTuples::value_type::TL>,
		RangeOfTuples::RangeType, RangeOfTuples::RangeIsFinite>
{
	typedef Meta::TypeListAt<N, typename RangeOfTuples::value_type::TL> value_type;
	typedef decltype(Meta::Get<N>(Meta::Val<typename RangeOfTuples::value_type>())) return_value_type;
	
	RangeOfTuples OriginalRange;

	forceinline UnzipResult(null_t=null) {}

	forceinline UnzipResult(const RangeOfTuples& rangeOfTuples):
		OriginalRange(rangeOfTuples) {}

	forceinline return_value_type First() const
	{
		return Meta::Get<N>(OriginalRange.First());
	}

	forceinline void PopFirst()
	{
		OriginalRange.PopFirst();
	}

	forceinline bool Empty() const
	{
		return OriginalRange.Empty();
	}

	template<typename U=RangeOfTuples> forceinline Meta::EnableIf<
		HasIndex<U>::_,
	return_value_type> operator[](size_t index) const
	{
		return Meta::Get<N>(OriginalRange[index]);
	}

	template<typename U=RangeOfTuples> forceinline Meta::EnableIf<
		HasSlicing<U>::_,
	UnzipResult> opSlice(size_t start, size_t end) const
	{
		return UnzipResult(OriginalRange.opSlice(start, end));
	}

	template<typename U=RangeOfTuples> forceinline Meta::EnableIf<
		HasLast<U>::_,
	return_value_type> Last() const
	{
		return Meta::Get<N>(OriginalRange.Last());
	}

	template<typename U=RangeOfTuples> forceinline Meta::EnableIf<
		HasPopLast<U>::_> PopLast(size_t start, size_t end) const
	{
		OriginalRange.PopLast();
	}

	forceinline bool operator==(const UnzipResult& rhs) const {return OriginalRange==rhs.OriginalRange;}

};


}}
