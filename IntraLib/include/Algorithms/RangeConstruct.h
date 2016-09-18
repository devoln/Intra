#pragma once

#include "Algorithms/Range.h"
#include "Utils/Optional.h"


namespace Intra { namespace Range {

template<typename T, typename S> struct IotaResult:
	RangeMixin<IotaResult<T, S>, T, TypeEnum::RandomAccess, false>
{
	typedef T value_type;
	typedef value_type return_value_type;

	T Begin;
	S Step;

	forceinline IotaResult(null_t=null): Begin(0), Step(0) {}
	forceinline IotaResult(T begin, S step): Begin(begin), Step(step) {}

	forceinline T First() const {return Begin;}
	forceinline void PopFirst() {Begin+=Step;}
	forceinline bool Empty() const {return Step==0;}
	forceinline T operator[](size_t index) const {return Begin+Step*index;}

	forceinline bool operator==(const IotaResult<T, S>& rhs) const
	{
		return (Begin==rhs.Begin || Step==0) && Step==rhs.Step;
	}

	forceinline TakeResult<IotaResult> opSlice(size_t start, size_t end) const
	{
		INTRA_ASSERT(start<=end);
		return TakeResult<IotaResult>(IotaResult(Begin+Step*start, Step), end-start);
	}
};



template<typename R> struct TakeResult:
	RangeMixin<TakeResult<R>, typename R::value_type,
		R::RangeType==TypeEnum::Bidirectional? TypeEnum::Forward: R::RangeType, true>
{
	typedef typename R::value_type value_type;
	typedef typename R::return_value_type return_value_type;

	forceinline TakeResult(null_t=null): r(null), len(0) {}
	forceinline TakeResult(const R& range, size_t count): r(range) {set_len(count);}

	template<typename U=R> forceinline Meta::EnableIf<
		HasLength<U>::_ || IsInfiniteRange<U>::_,
	bool> Empty() const {return len==0;}

	template<typename U=R> forceinline Meta::EnableIf<
		!HasLength<U>::_ && IsFiniteRange<U>::_,
	bool> Empty() const {return len==0 || r.Empty();}


	forceinline return_value_type First() const {INTRA_ASSERT(!Empty()); return r.First();}
	forceinline void PopFirst() {r.PopFirst(); len--;}
	
	template<typename U=R> forceinline Meta::EnableIf<
		IsBidirectionalRange<U>::_,
	return_value_type> Last() const {INTRA_ASSERT(!Empty()); return r[len-1];}

	template<typename U=R> forceinline Meta::EnableIf<
		IsBidirectionalRange<U>::_
	> PopLast() {len--;}

	template<typename U=R> forceinline Meta::EnableIf<
		IsRandomAccessRange<U>::_,
	return_value_type> operator[](size_t index) const {return r[index];}


	forceinline bool operator==(const TakeResult& rhs) const
	{
		return len==rhs.len && (len==0 || r==rhs.r);
	}

	template<typename U=R> forceinline Meta::EnableIf<
		HasLength<U>::_ || IsInfiniteRange<U>::_,
	size_t> Length() const {return len;}

	TakeResult Take(size_t count) const
	{
		if(count>len) count=len;
		return TakeResult(r, count);
	}

	template<typename U=R, typename = Meta::EnableIf<
		U::RangeType==TypeEnum::RandomAccess
		>> forceinline 
	decltype(Meta::Val<U>().opSlice(1, 2)) opSlice(size_t startIndex, size_t endIndex) const
	{
		INTRA_ASSERT(startIndex <= endIndex);
		INTRA_ASSERT(endIndex <= len);
		return r.opSlice(startIndex, endIndex);
	}

private:
	R r;
	size_t len;

	template<typename U=R> forceinline Meta::EnableIf<
		HasLength<U>::_
	> set_len(size_t maxLen)
	{
		len = r.Length();
		if(len>maxLen) len=maxLen;
	}

	template<typename U=R> forceinline Meta::EnableIf<
		!HasLength<U>::_
	> set_len(size_t maxLen)
	{
		len = maxLen;
	}
};

template<typename T, typename S> forceinline
TakeResult<IotaResult<T, S>> Iota(T begin, T end, S step)
{
	return IotaResult<T, S>(begin, step).Take((end-begin+step-1)/step);
}


static_assert(IsInputRange<IotaResult<int, int>>::_, "Not input range???");
static_assert(IsForwardRange<decltype(Iota(1, 2, 3))>::_, "Not forward range???");




static_assert(!IsBidirectionalRange<IotaResult<int, int>>::_, "Is bidirectional range???");
static_assert(IsRandomAccessRange<IotaResult<int, int>>::_, "Not random access range???");
static_assert(!IsFiniteRandomAccessRange<IotaResult<int, int>>::_, "Is finite random access range???");



template<typename R> struct CycleResult:
	RangeMixin<CycleResult<R>, typename R::value_type,
		R::RangeType==TypeEnum::Bidirectional? TypeEnum::Forward: R::RangeType, false>
{
	typedef typename R::value_type value_type;
	typedef typename R::return_value_type return_value_type;

	forceinline CycleResult(null_t=null): counter(0) {}
	forceinline CycleResult(const R& range): original_range(range), offset_range(range), counter(0) {}

	forceinline bool Empty() const {return original_range.Empty();}
	forceinline return_value_type First() const {return offset_range.First();}

	forceinline void PopFirst()
	{
		offset_range.PopFirst();
		counter++;
		if(!offset_range.Empty()) return;
		offset_range=original_range;
		counter=0;
	}

	template<typename U=R> forceinline Meta::EnableIf<
		IsRandomAccessRange<U>::_,
	return_value_type> operator[](size_t index) const {return original_range[(index+counter) % original_range.Length()];}

	forceinline bool operator==(const CycleResult& rhs) const
	{
		return offset_range==rhs.offset_range && original_range==rhs.original_range;
	}

private:
	R original_range, offset_range;
	size_t counter;
};

template<typename R> struct CycleRandomResult:
	RangeMixin<CycleRandomResult<R>, typename R::value_type,
		R::RangeType>=TypeEnum::RandomAccess && HasLength<R>::_? TypeEnum::RandomAccess: TypeEnum::Error, false>
{
	typedef typename R::value_type value_type;
	typedef typename R::return_value_type return_value_type;

	forceinline CycleRandomResult(null_t=null): counter(0) {}
	forceinline CycleRandomResult(const R& range): original_range(range), counter(0) {}

	forceinline bool Empty() const {return original_range.Empty();}

	forceinline return_value_type First() const {return original_range[counter];}

	forceinline void PopFirst()
	{
		counter++;
		if(counter==original_range.Length()) counter=0;
	}

	forceinline return_value_type operator[](size_t index) const {return original_range[(index+counter) % original_range.Length()];}

	forceinline bool operator==(const CycleRandomResult& rhs) const
	{
		return original_range==rhs.original_range && counter==rhs.counter;
	}

	forceinline TakeResult<CycleRandomResult<R>> opSlice(size_t startIndex, size_t endIndex) const
	{
		INTRA_ASSERT(startIndex <= endIndex);
		CycleRandomResult<R> result(original_range);
		result.counter = (counter+startIndex) % original_range.Length();
		return result.Take(endIndex-startIndex);
	}

private:
	R original_range;
	size_t counter;
};

template<typename T, size_t N> forceinline
CycleRandomResult<ArrayRange<const T>> Cycle(const T(&arr)[N])
{
	return ArrayRange<const T>(arr).Cycle();
}

template<typename T> struct RepeatResult:
	RangeMixin<RepeatResult<T>, T, TypeEnum::RandomAccess, false>
{
	typedef T value_type;
	typedef const T& return_value_type;

	RepeatResult(null_t=null): value(null) {}
	RepeatResult(T&& val): value(core::move(val)) {}
	RepeatResult(const T& val): value(val) {}

	forceinline bool Empty() const {return value==null;}
	forceinline const T& First() const {return value();}

	forceinline void PopFirst() {}
	const T& operator[](size_t) const {return value();}

	forceinline bool operator==(const RepeatResult& rhs) const
	{
		if(value==null && rhs.value==null) return true;
		if(value==null && rhs.value!=null || value!=null && rhs.value==null) return false;
		return value()==rhs.value();
	}
	
private:
	Utils::Optional<T> value;
};

template<typename T> RepeatResult<T> Repeat(T&& val)
{
	return RepeatResult<T>(core::forward<T>(val));
}

template<typename T> TakeResult<RepeatResult<T>> Repeat(T&& val, size_t n)
{
	return RepeatResult<T>(core::forward<T>(val)).Take(n);
}



template<typename F> struct GenerateResult:
	RangeMixin<GenerateResult<F>, Meta::RemoveConstRef<Meta::ResultOf<F>>, TypeEnum::Input, false>
{
	typedef Meta::RemoveConstRef<Meta::ResultOf<F>> value_type;
	typedef const value_type& return_value_type;

	GenerateResult(null_t=null): func(null) {}
	GenerateResult(F function): func(function) {front = func()();}

	forceinline bool Empty() const {return func==null;}
	forceinline return_value_type First() const {return front;}

	forceinline void PopFirst() {front = func()();}

	forceinline bool operator==(const GenerateResult& rhs) const
	{
		return func==null && rhs.func==null;
	}

private:
	Utils::Optional<F> func;
	value_type front;
};

template<typename F> GenerateResult<F> Generate(F&& func)
{
	return GenerateResult<F>(core::forward<F>(func));
}


template<typename R1, typename R2> struct ChooseResult:
	CommonAllFiniteRangeMixin<ChooseResult<R1, R2>, R1, R2>
{
	typedef Meta::CommonType<typename R1::value_type, typename R2::value_type> value_type;
	typedef Meta::CommonTypeRef<typename R1::return_value_type, typename R2::return_value_type> return_value_type;

	ChooseResult(null_t=null): range1(null), range2(null) {}
	ChooseResult(R1&& ifFalseRange, R2&& ifTrueRange, bool condition):
		range1(condition? null: core::forward<R1>(ifFalseRange)),
		range2(condition? core::forward<R2>(ifTrueRange): null) {}

	forceinline bool Empty() const
	{
		return range1.Empty() && range2.Empty();
	}

	forceinline return_value_type First() const
	{
		return range1.Empty()? range2.First(): range1.First();
	}

	forceinline void PopFirst()
	{
		if(!range1.Empty())
		{
			range1.PopFirst();
			return;
		}
		INTRA_ASSERT(!Empty());
		range2.PopFirst();
	}

	template<typename U=R1> forceinline Meta::EnableIf<
		IsBidirectionalRange<U>::_ && IsBidirectionalRange<R2>::_,
	return_value_type> Last() const
	{
		return range1.Empty()? range2.Last(): range1.Last();
	}

	template<typename U=R1> forceinline Meta::EnableIf<
		IsBidirectionalRange<U>::_ && IsBidirectionalRange<R2>::_
	> PopLast()
	{
		if(!range1.Empty())
		{
			range1.PopLast();
			return;
		}
		INTRA_ASSERT(!Empty());
		range2.PopLast();
	}

	template<typename U=R1> forceinline Meta::EnableIf<
		IsRandomAccessRange<U>::_ && IsRandomAccessRange<R2>::_,
	return_value_type> operator[](size_t index) const
	{
		return range1.Empty()? range2[index]: range1[index];
	}

	template<typename U=R1> forceinline Meta::EnableIf<
		IsFiniteRandomAccessRange<U>::_ && IsFiniteRandomAccessRange<R2>::_,
	ChooseResult> opSlice(size_t start, size_t end) const
	{
		return ChooseResult(
			range1.Empty()? null: range1.opSlice(start, end),
			range1.Empty()? null: range2.opSlice(start, end));
	}

	forceinline bool operator==(const ChooseResult& rhs) const
	{
		return range1==rhs.range1 && range2==rhs.range2;
	}

private:
	R1 range1;
	R2 range2;
};

template<typename R1, typename R2> forceinline
ChooseResult<R1, R2> Choose(R1&& ifFalseRange, R2&& ifTrueRange, bool condition)
{
	return ChooseResult<R1, R2>(core::forward<R1>(ifFalseRange), core::forward<R2>(ifTrueRange), condition);
}



template<typename ValueRangeType, typename IndexRangeType> struct IndexedResult:
	RangeMixin<
		IndexedResult<ValueRangeType, IndexRangeType>,
		typename ValueRangeType::value_type,
		IndexRangeType::RangeType, IndexRangeType::RangeIsFinite>
{
	typedef typename ValueRangeType::value_type value_type;
	typedef typename ValueRangeType::return_value_type return_value_type;

	IndexedResult(null_t=null): ValueRange(null), IndexRange(null) {}
	IndexedResult(const ValueRangeType& valueRange, const IndexRangeType& indexRange):
		ValueRange(valueRange), IndexRange(indexRange) {}
	IndexedResult(ValueRangeType&& valueRange, IndexRangeType&& indexRange):
		ValueRange(core::move(valueRange)), IndexRange(core::move(indexRange)) {}

	forceinline bool Empty() const {return IndexRange.Empty();}

	forceinline return_value_type First() const
	{
		INTRA_ASSERT(!Empty());
		return ValueRange[IndexRange.First()];
	}

	forceinline void PopFirst()
	{
		INTRA_ASSERT(!Empty());
		IndexRange.PopFirst();
	}

	template<typename U=IndexRangeType> forceinline Meta::EnableIf<
		IsBidirectionalRange<U>::_,
	return_value_type> Last() const
	{
		INTRA_ASSERT(!Empty());
		return ValueRange[IndexRange.Last()];
	}

	template<typename U=IndexRangeType> forceinline Meta::EnableIf<
		IsBidirectionalRange<U>::_
	> PopLast()
	{
		INTRA_ASSERT(!Empty());
		IndexRange.PopLast();
	}

	template<typename U=IndexRangeType> forceinline Meta::EnableIf<
		IsRandomAccessRange<U>::_,
	return_value_type> operator[](size_t index) const {return ValueRange[IndexRange[index]];}

	template<typename U=IndexRangeType> forceinline Meta::EnableIf<
		IsRandomAccessRange<U>::_,
	IndexedResult> opSlice(size_t start, size_t end) const
	{
		return IndexedResult(ValueRange, IndexRange.opSlice(start, end));
	}

	template<typename U=IndexRangeType> forceinline Meta::EnableIf<
		HasLength<U>::_,
	size_t> Length() const {return IndexRange.Length();}


	forceinline bool operator==(const IndexedResult& rhs) const
	{
		return ValueRange==rhs.ValueRange && IndexRange==rhs.IndexRange;
	}

	ValueRangeType ValueRange;
	IndexRangeType IndexRange;
};


template<typename ValueRangeType, typename IndexRangeType> forceinline Meta::EnableIf<
	IsRandomAccessRange<ValueRangeType>::_ && IsInputRange<IndexRangeType>::_,
IndexedResult<ValueRangeType, IndexRangeType>> Indexed(ValueRangeType&& valueRange, IndexRangeType&& indexRange)
{
	return IndexedResult<ValueRangeType, IndexRangeType>(
		core::forward<ValueRangeType>(valueRange),
		core::forward<IndexRangeType>(indexRange));
}




template<typename Rs> struct FirstTransversalResult:
	RangeMixin<FirstTransversalResult<Rs>, typename Rs::value_type::value_type,
		(Rs::RangeType>TypeEnum::Bidirectional)? TypeEnum::Bidirectional: Rs::RangeType, Rs::RangeIsFinite>
{
	typedef typename Rs::value_type::value_type value_type;
	typedef typename Rs::value_type::return_value_type return_value_type;

	FirstTransversalResult(null_t=null) {}
	FirstTransversalResult(const Rs& rangeOfRanges):
		ranges(rangeOfRanges) {skip_empty();}

	forceinline bool Empty() const {return ranges.Empty();}

	forceinline return_value_type First() const {return ranges.First().First();}

	forceinline void PopFirst()
	{
		ranges.PopFirst();
		skip_empty();
	}

	template<typename U=Rs> forceinline Meta::EnableIf<
		IsBidirectionalRange<U>::_,
	return_value_type> Last() const
	{
		while(ranges.Last().Empty()) ranges.PopLast();
		return ranges.Last().First();
	}

	template<typename U=Rs> forceinline Meta::EnableIf<
		IsBidirectionalRange<U>::_
	> PopLast()
	{
		INTRA_ASSERT(!Empty());
		while(ranges.Last().Empty()) ranges.PopLast();
		ranges.PopLast();
	}

	forceinline bool operator==(const FirstTransversalResult& rhs) const {return ranges==rhs.ranges;}

private:
	Rs ranges;

	void skip_empty()
	{
		while(!Empty() && ranges.First().Empty()) ranges.PopFirst();
	}
};



}}


