#pragma once

#include "Core/Core.h"
#include "Meta/Type.h"
#include "Algorithms/RangeConcept.h"

namespace Intra { namespace Range {

template<typename R, typename P> struct FilterResult;
template<typename R, typename F> struct MapResult;
template<typename R> struct TakeResult;
template<typename R> struct StrideResult;
template<typename Rs> struct FirstTransversalResult;

namespace detail {

template<typename R> struct ResultOfTake {typedef Meta::SelectType<R, TakeResult<R>, IsRandomAccessRange<R>::_> _;};
template<typename R> struct ResultOfTake<TakeResult<R>> {typedef TakeResult<R> _;};

}

template<typename R> using ResultOfTake = typename detail::ResultOfTake<R>::_;

template<typename R> struct InputRangeIterator
{
	typedef typename R::value_type value_type;
	typedef typename R::return_value_type return_value_type;
	typedef return_value_type& reference;
	typedef value_type* pointer;
	typedef intptr difference_type;

	forceinline InputRangeIterator(null_t=null): Range(emptyR) {}
	forceinline InputRangeIterator(R& range): Range(range) {}

	forceinline InputRangeIterator& operator++() {Range.PopFirst(); return *this;}
	forceinline return_value_type operator*() const {return Range.First();}

	forceinline bool operator==(const InputRangeIterator& rhs) const {return &Range==&rhs.Range;}
	forceinline bool operator!=(const InputRangeIterator& rhs) const {return !operator==(rhs);}
	forceinline bool operator==(null_t) const {return Range.Empty();}
	forceinline bool operator!=(null_t) const {return !operator==(null);}

	R& Range;
protected:
	static R emptyR;
};
template<typename R> R InputRangeIterator<R>::emptyR;


template<typename R, typename T, class PARENT> struct InputRangeMixin: PARENT
{
private:
	forceinline const R& me() const {return *static_cast<const R*>(this);}
	forceinline R& me() {return *static_cast<R*>(this);}
public:
	forceinline InputRangeIterator<R> begin() {return InputRangeIterator<R>(me());}
	forceinline InputRangeIterator<R> begin() const {return InputRangeIterator<R>(me());}
	forceinline InputRangeIterator<R> end() const {return null;}

	forceinline TakeResult<R> Take(size_t count) const {return TakeResult<R>(me(), count);}

	template<typename P> forceinline FilterResult<R, P> Filter(P predicate) {return FilterResult<R, P>(me(), predicate);}

	template<typename F> forceinline MapResult<R, F> Map(F func) {return MapResult<R,F>(me(), func);}

	template<typename U=R> Meta::EnableIf<
		IsRangeOfRanges<U>::_,
	FirstTransversalResult<R>> FirstTransversal() const {return FirstTransversalResult<R>(me());}


	//Этот метод будет скрыт, если это конечный диапазон, класса не выше InputRange
	forceinline const R& Cycle() const {return me();}

	forceinline size_t PopFirstN(size_t n)
	{
		for(size_t i=0; i<n; i++)
		{
			if(me().Empty()) return i;
			me().PopFirst();
		}
		return n;
	}

	forceinline void PopFirstExactly(size_t elementsToPop)
	{
		while(elementsToPop --> 0) me().PopFirst();
	}

	forceinline StrideResult<R> Stride(size_t step) const
	{
		return StrideResult<R>(me(), step);
	}


	//! Возвращает диапазон, полученный из этого диапазона удалением всех первых элементов, равных x.
	template<typename X> Meta::EnableIf<
		Meta::IsConvertible<X, T>::_,
	R&> TrimLeftAdvance(const X& x)
	{
		while(!me().Empty() && me().First()==x)
			me().PopFirst();
		return me();
	}
	
	//! Последовательно удаляет элементы из начала диапазона, пока выполняется предикат pred.
	//! Останавливается на первом элементе, для которого это условие не выполнено.
	template<typename P> Meta::EnableIf<
		Meta::IsCallable<P, T>::_,
	R&> TrimLeftAdvance(P pred)
	{
		while(!me().Empty() && pred(me().First()))
			me().PopFirst();
		return me();
	}

	bool operator!=(const R& rhs) const {return !(me()==rhs);}
};



template<typename R, typename T, class PARENT> struct FiniteInputRangeMixin: PARENT
{
private:
	forceinline const R& me() const {return *static_cast<const R*>(this);}
	forceinline R& me() {return *static_cast<R*>(this);}
public:
	template<typename F, typename S> S ReduceAdvance(const F& func, const S& seed)
	{
		auto result = seed;
		while(!me().Empty())
		{
			result = func(result, me().First());
			me().PopFirst();
		}
		return result;
	}

	template<typename F> Meta::ResultOf<F, T, T> ReduceAdvance(F func)
	{
		auto r = me();
		Meta::ResultOf<F, T, T> seed = r.First();
		r.PopFirst();
		return r.Reduce(func, seed);
	}


	//! Найти первое вхождение элемента what в этот диапазон.
	//! Изменяет диапазон в null, если значение не найдено. Иначе оставляет часть этого диапазона, начиная с позиции, на которой начинается первое вхождение what.
	//! \param what Искомый элемент.
	//! \param ioIndex[inout] Указатель на счётчик, который увеличивается на количество элементов, предшествующих найденной позиции. Может быть null.
	template<typename X> Meta::EnableIf<
		Meta::IsConvertible<X, T>::_,
	R&> FindAdvance(const X& what, size_t* ioIndex=null)
	{
		size_t index=0;
		while(!me().Empty() && !(me().First()==what))
		{
			me().PopFirst();
			index++;
		}
		if(ioIndex!=null) *ioIndex += index;
		return me();
	}

	//! Найти первое вхождение элемента, удовлетворяющего некоторому условию, в этот диапазон.
	//! Изменяет диапазон в null, если значение не найдено. Иначе оставляет часть этого диапазона, начиная с позиции, на которой начинается первое вхождение искомого элемента.
	//! \param pred Условие, которому должен удовлетворять искомый элемент.
	//! \param ioIndex[inout] Указатель на счётчик, который увеличивается на количество элементов, предшествующих найденной позиции. Может быть null.
	template<typename P> Meta::EnableIf<
		Meta::IsCallable<P, T>::_,
	R&> FindAdvance(P pred, size_t* ioIndex=null)
	{
		size_t index=0;
		while(!me().Empty() && !pred(me().First()))
		{
			me().PopFirst();
			index++;
		}
		if(ioIndex!=null) *ioIndex += index;
		return me();
	}


	//! Найти первое вхождение любого элемента из диапазона whats в этот диапазон.
	//! Начало этого диапазона смещается к найденному элементу или совмещается с концом в случае, когда элемент не найден.
	//! \param whats Диапазон искомых элементов. Если один из этих элементов найден, начало whats устанавливается на найденный элемент.
	//! \param ioIndex[inout] Указатель на счётчик, который увеличивается на количество элементов, предшествующих найденной позиции. Может быть null.
	//! \param oWhatIndex[out] Указатель на переменную, в которую будет записан индекс найденного элемента в диапазоне whats. Если элемент не был найден, будет записано значение whats.Count().
	//! \return Возвращает ссылку на себя.
	template<typename Ws> Meta::EnableIf<
		IsFiniteForwardRange<Ws>::_,
	R&> FindAdvanceAnyAdvance(Ws& whats, size_t* ioIndex=null, size_t* oWhatIndex=null)
	{
		size_t index=0, whatIndex = whats.Count();
		Ws whatsCopy = whats;
		for(; !me().Empty(); me().PopFirst(), index++)
		{
			whats = whatsCopy;
			whatIndex = 0;
			while(!whats.Empty())
			{
				if(me().First()!=whats.First())
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
		return me();
	}

	//! Найти первое вхождение любого элемента из диапазона whats в этот диапазон.
	//! Начало этого диапазона смещается к найденному элементу или совмещается с концом в случае, когда элемент не найден.
	//! \param whats Диапазон искомых элементов.
	//! \param ioIndex[inout] Указатель на счётчик, который увеличивается на количество элементов, предшествующих найденной позиции. Может быть null.
	//! \param oWhatIndex[out] Указатель на переменную, в которую будет записан индекс найденного элемента в диапазоне whats. Если элемент не был найден, будет записано значение whats.Count().
	//! \return Возвращает ссылку на себя.
	template<typename Ws> forceinline Meta::EnableIf<
		IsFiniteForwardRangeOf<Ws, T>::_,
	R&> FindAdvanceAny(const Ws& whats, size_t* ioIndex=null, size_t* oWhatIndex=null)
	{
		Ws whatCopy = whats;
		return FindAdvanceAnyAdvance(whatCopy, ioIndex, oWhatIndex);
	}



	//! Последовательно удаляет элементы из начала диапазона, до тех пор,
	//! пока не встретится элемент, равный x, или диапазон не станет пустым.
	//! \return Возвращает количество пройденных элементов.
	template<typename X> forceinline Meta::EnableIf<
		Meta::IsConvertible<X, T>::_,
	size_t> CountUntilAdvance(const X& x)
	{
		size_t index=0;
		me().FindAdvance(x, &index);
		return index;
	}
	
	//! Последовательно удаляет элементы из начала диапазона до тех пор,
	//! пока не выполнится условие predicate или диапазон не станет пустым.
	//! \return Возвращает количество пройденных элементов.
	template<typename P> forceinline Meta::EnableIf<
		Meta::IsCallable<P, T>::_,
	size_t> CountUntilAdvance(P predicate)
	{
		size_t index=0;
		me().FindAdvance(predicate, &index);
		return index;
	}

	//! Последовательно удаляет элементы из начала диапазона до тех пор,
	//! пока не встретится элемент, равный любому из элементов диапазона whats.
	//! \param whats[inout] Диапазон искомых элементов. Если элемент найден, начало диапазона whats смещается к найденному элементу.
	//! \return Возвращает количество пройденных элементов.
	template<typename Ws> forceinline Meta::EnableIf<
		IsForwardRangeOf<Ws, T>::_,
	size_t> CountUntilAdvanceAnyAdvance(Ws& whats)
	{
		size_t index=0;
		me().FindAdvanceAnyAdvance(whats, &index);
		return index;
	}

	//! Последовательно удаляет элементы из начала диапазона до тех пор,
	//! пока не встретится элемент, равный любому из элементов диапазона whats.
	//! \param whats[inout] Диапазон искомых элементов.
	//! \return Возвращает количество пройденных элементов.
	template<typename Ws> forceinline Meta::EnableIf<
		IsForwardRangeOf<Ws, T>::_,
	size_t> CountUntilAdvanceAny(const Ws& whats)
	{
		size_t index=0;
		me().FindAdvanceAny(whats, &index);
		return index;
	}


	//! Удаляет из диапазона все элементы кроме последних n элементов.
	//! \return Возвращает ссылку на себя.
	template<typename U=R> forceinline Meta::EnableIf<
		HasLength<U>::_,
	R&> TailAdvance(size_t n)
	{
		if(me().Length()>n) me().PopFirstExactly(me().Length()-n);
		return me();
	}


	//! Возвращает количество элементов в диапазоне, последовательно удаляя все его элементы. Совпадает с WalkLengthAdvance().
	size_t CountAdvance()
	{
		size_t length = 0;
		while(!me().Empty())
		{
			length++;
			me().PopFirst();
		}
		return length;
	}

	//! Возвращает количество элементов в диапазоне. Имеет вычислительную сложность O(1).
	template<typename U=R> forceinline Meta::EnableIf<
		HasLength<U>::_,
	size_t> Count() const {return me().Length();}


	//! Последовательно удаляет все элементы из диапазона, подсчитывая количество элементов, равных x.
	template<typename X> Meta::EnableIf<
		Meta::IsConvertible<X, T>::_,
	size_t> CountAdvance(X x)
	{
		size_t result=0;
		while(!me().Empty())
		{
			if(me().First()==x) result++;
			me().PopFirst();
		}
		return result;
	}


	//! Последовательно удаляет все элементы из диапазона, подсчитывая количество элементов, для которых выполнено условие pred.
	template<typename P> Meta::EnableIf<
		Meta::IsCallable<P, T>::_,
	size_t> CountAdvance(P pred)
	{
		size_t result=0;
		while(!me().Empty())
		{
			if(pred(me().First())) result++;
			me().PopFirst();
		}
		return result;
	}


	template<typename OR> Meta::EnableIf<
		IsOutputRange<OR>::_ && !Meta::IsConst<OR>::_
	> CopyAdvanceToAdvance(OR&& dst)
	{
		while(!me().Empty())
		{
			dst.Put(me().First());
			me().PopFirst();
		}
	}

	template<typename OR> forceinline Meta::EnableIf<
		IsOutputRange<OR>::_
	> CopyAdvanceTo(const OR& dst)
	{
		OR dst2 = dst;
		me().CopyAdvanceToAdvance(dst2);
	}

	template<typename OR, typename P> Meta::EnableIf<
		IsOutputRange<OR>::_ && Meta::IsCallable<P, T>::_ && !Meta::IsConst<OR>::_
	> CopyAdvanceToAdvance(OR&& dst, P pred)
	{
		while(!me().Empty())
		{
			auto value = me().First();
			if(pred(value)) dst.Put(value);
			me().PopFirst();
		}
	}

	template<typename OR, typename P> forceinline Meta::EnableIf<
		IsOutputRange<OR>::_ && Meta::IsCallable<P, T>::_
	> CopyAdvanceTo(const OR& dst, P pred)
	{
		auto dst2 = dst;
		me().CopyAdvanceToAdvance(dst2, pred);
	}

	void FillAdvance(const T& value)
	{
		while(!me().Empty())
		{
			me().First() = value;
			me().PopFirst();
		}
	}

	

	template<typename PatternRange, typename U=R> Meta::EnableIf<
		(IsForwardRange<PatternRange>::_ || IsInfiniteRange<PatternRange>::_) && IsRangeElementAssignable<U>::_
	> FillPatternAdvance(const PatternRange& pattern)
	{
		auto patternCopy = pattern.Cycle();
		while(!me().Empty())
		{
			auto& ref = me().First();
			ref = patternCopy.First();
			me().PopFirst();
			patternCopy.PopFirst();
		}
	}

	

	template<typename F, typename U=R> Meta::EnableIf<
		IsRangeElementAssignable<U>::_ && Meta::IsCallable<F, T&>::_
	> TransformAdvance(F f)
	{
		while(!me().Empty())
		{
			auto& v = me().First();
			v = f(v);
			me().PopFirst();
		}
	}

	template<typename ResultRange, typename F> Meta::EnableIf<
		IsOutputRange<ResultRange>::_ && Meta::IsCallable<F, T&>::_
	> TransformAdvanceToAdvance(ResultRange& output, F f)
	{
		while(!me().Empty())
		{
			output.Put(f(me().First()));
			me().PopFirst();
		}
	}

	template<typename ResultRange, typename F> Meta::EnableIf<
		IsOutputRange<ResultRange>::_ && Meta::IsCallable<F, T&>::_
	> TransformAdvanceTo(const ResultRange& output, F f)
	{
		ResultRange outputCopy = output;
		me().TransformAdvanceToAdvance(outputCopy, f);
	}

};



}}

