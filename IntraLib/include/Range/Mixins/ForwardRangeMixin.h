#pragma once

#include "Core/Core.h"
#include "Meta/Type.h"
#include "Meta/Tuple.h"
#include "Range/Concepts.h"
#include "Algorithms/Operations.h"
#include "Algorithms/Comparison.h"
#include "Algorithms/Search.h"


namespace Intra { namespace Range {

template<typename T> struct ArrayRange;

template<typename R> struct CycleResult;
template<typename... RANGES> struct ZipResult;
template<size_t N, typename RangeOfRanges> struct UnzipResult;


template<typename R> struct ForwardRangeIterator
{
	typedef typename R::value_type value_type;
	typedef typename R::return_value_type return_value_type;
	typedef return_value_type& reference;
	typedef value_type* pointer;
	typedef intptr difference_type;

	forceinline ForwardRangeIterator(null_t=null) {}
	forceinline ForwardRangeIterator(const R& range): Range(range) {}
	forceinline ForwardRangeIterator& operator++() {Range.PopFirst(); return *this;}
	forceinline ForwardRangeIterator operator++(int) {auto copy = Range; Range.PopFirst(); return copy;}
	forceinline return_value_type operator*() const {return Range.First();}

	forceinline bool operator==(const ForwardRangeIterator& rhs) const {return Range==rhs.Range;}
	forceinline bool operator!=(const ForwardRangeIterator& rhs) const {return !operator==(rhs);}
	forceinline bool operator==(null_t) const {return Range.Empty();}
	forceinline bool operator!=(null_t) const {return !Range.Empty();}

	R Range;
};


template<typename R, typename T, class PARENT> struct ForwardRangeMixin: PARENT
{
	enum: TypeEnum::Type {RangeType = TypeEnum::Forward};
	enum: bool {RangeIsFinite = false};
private:
	forceinline const R& me() const {return *static_cast<const R*>(this);}
	forceinline R& me() {return *static_cast<R*>(this);}

public:
	forceinline ForwardRangeIterator<R> begin() {return ForwardRangeIterator<R>(me());}
	forceinline ForwardRangeIterator<R> begin() const {return ForwardRangeIterator<R>(me());}
	forceinline ForwardRangeIterator<R> end() const {return null;}


	template<typename RW, typename U=R> forceinline Meta::EnableIf<
		IsFiniteForwardRangeOf<RW, T>::_ &&
		!(HasLength<U>::_ &&
			HasLength<RW>::_),
	bool> StartsWith(const RW& what) const
	{return Algo::StartsWith(what);}

	template<typename RW, typename U=R> forceinline Meta::EnableIf<
		IsForwardRangeOf<RW, T>::_ &&
		HasLength<U>::_ &&
		HasLength<RW>::_,
	bool> StartsWith(const RW& what) const
	{return Algo::StartsWith(me(), what);}

	template<size_t N> forceinline
	bool StartsWith(const T(&rhs)[N])
	{return Algo::StartsWith(me(), rhs);}

	template<typename RWs> Meta::EnableIf<
		IsRangeOfFiniteForwardRangesOf<RWs, T>::_ && IsFiniteRange<RWs>::_,
	bool> StartsWithAnyAdvance(RWs& subranges, size_t* oSubrangeIndex=null) const
	{return Algo::StartsWithAnyAdvance(me(), subranges, oSubrangeIndex);}

	template<typename RWs> forceinline Meta::EnableIf<
		IsRangeOfFiniteForwardRangesOf<RWs, T>::_ && IsFiniteRange<RWs>::_,
	bool> StartsWithAny(const RWs& subranges, size_t* ioIndex=null) const
	{return Algo::StartsWithAny(me(), subranges, ioIndex);}


	//! Возвращает диапазон, полученный из этого диапазона удалением всех первых элементов, равных x.
	template<typename X> forceinline Meta::EnableIf<
		Meta::IsConvertible<X, T>::_,
	R> TrimLeft(const X& x) const {return Algo::TrimLeft(me(), x);}

	//! Возвращает диапазон, полученный из этого диапазона удалением всех первых элементов, для которых выполнен предикат pred.
	template<typename P> forceinline Meta::EnableIf<
		Meta::IsCallable<P, T>::_,
	R> TrimLeft(P pred) const {return Algo::TrimLeft(me(), pred);}


	forceinline R Drop() const
	{
		if(me().Empty()) return me();
		auto range = me();
		range.PopFirst();
		return range;
	}

	forceinline R Drop(size_t n) const
	{
		auto range = me();
		range.PopFirstN(n);
		return range;
	}

	forceinline R DropExactlyOne() const
	{
		auto range = me();
		range.PopFirst();
		return range;
	}

	forceinline R DropExactly(size_t n) const
	{
		auto range = me();
		range.PopFirstExactly(n);
		return range;
	}


	template<typename U=R> forceinline typename U::return_value_type AtIndex(size_t i) const {return me().Drop(i).First();}
};

template<typename R, typename T, class PARENT> struct FiniteForwardRangeMixin: PARENT
{
private:
	forceinline const R& me() const {return *static_cast<const R*>(this);}
	forceinline R& me() {return *static_cast<R*>(this);}
	
public:
	forceinline CycleResult<R> Cycle() const {return CycleResult<R>(me());}

	template<typename F, typename S> forceinline S Reduce(const F& func, const S& seed) const
	{
		R range = me();
		return range.ReduceAdvance(func, seed);
	}

	template<typename F> forceinline Meta::ResultOf<F, T, T> Reduce(F func) const
	{
		R range = me();
		return range.ReduceAdvance(func);
	}

	//! Удаляет из диапазона все элементы кроме последних n элементов.
	//! \return Возвращает ссылку на себя.
	template<typename U=R> Meta::EnableIf<
		!HasLength<U>::_,
	R&> TailAdvance(size_t n)
	{
		R temp = me();
		temp.PopFirstN(n);
		while(!temp.Empty())
		{
			temp.PopFirst();
			me().PopFirst();
		}
		return me();
	}

	//! Возвращает диапазон, содержащий последние n элементов данного диапазона.
	forceinline R Tail(size_t n) const
	{
		R range = me();
		return range.TailAdvance(n);
	}

	forceinline void Fill(const T& value) const
	{Algo::Fill(me(), value);}

	template<typename PatternRange, typename U=R> forceinline Meta::EnableIf<
		(IsForwardRange<PatternRange>::_ || IsInfiniteRange<PatternRange>::_) &&
		IsRangeElementAssignable<U>::_
	> FillPattern(const PatternRange& pattern) const
	{Algo::FillPattern(me(), pattern);}

	template<typename F, typename U=R> forceinline Meta::EnableIf<
		IsRangeElementAssignable<U>::_ &&
		Meta::IsCallable<F, T&>::_
	> Transform(F f) const {Algo::Transform(me(), f);}

	template<typename ResultRange, typename F> forceinline Meta::EnableIf<
		IsOutputRange<ResultRange>::_ && Meta::IsCallable<F, T&>::_
	> TransformToAdvance(ResultRange& output, F f) const
	{Algo::TransformToAdvance(me(), output, f);}

	template<typename ResultRange, typename F> forceinline Meta::EnableIf<
		IsOutputRange<ResultRange>::_ && Meta::IsCallable<F, T&>::_
	> TransformTo(const ResultRange& output, F f) const
	{return Algo::TransformTo(me(), output, f);}


	template<typename OR> forceinline Meta::EnableIf<
		(IsOutputRange<OR>::_ || HasAsRange<Meta::RemoveConstRef<OR>>::_) && !Meta::IsConst<OR>::_
	> CopyToAdvance(OR&& dst) const
	{Algo::CopyToAdvance(me(), core::forward<OR>(dst));}

	template<typename OR> forceinline Meta::EnableIf<
		IsOutputRange<OR>::_ || HasAsRange<OR>::_
	> CopyTo(OR&& dst) const
	{return Algo::CopyTo(me(), core::forward<OR>(dst));}


	template<typename OR, typename P> forceinline Meta::EnableIf<
		IsOutputRange<OR>::_ && Meta::IsCallable<P, T>::_ && !Meta::IsConst<OR>::_
	> CopyToAdvance(OR&& dst, P pred) const
	{return Algo::CopyToAdvance(me(), core::forward<OR>(dst), pred);}


	template<typename OR, typename P> forceinline Meta::EnableIf<
		(IsOutputRange<OR>::_ || HasAsRange<OR>::_) && Meta::IsCallable<P, T>::_
	> CopyTo(const OR& dst, P pred) const
	{return Algo::CopyTo(me(), dst, pred);}


	//! Возвращает количество элементов в диапазоне. Имеет вычислительную сложность O(length).
	template<typename U=R> forceinline Meta::EnableIf<
		!HasLength<U>::_,
	size_t> Count() const
	{
		R range = me();
		return range.CountAdvance();
	}

	//! Возвращает количество содержащихся в диапазоне элементов, равных x.
	template<typename X> forceinline Meta::EnableIf<
		Meta::IsConvertible<X, T>::_,
	size_t> Count(X x) const
	{
		R range = me();
		return range.CountAdvance(x);
	}

	//! Возвращает количество содержащихся в диапазоне элементов, для которых выполнено условие pred.
	template<typename P> forceinline Meta::EnableIf<
		Meta::IsCallable<P, T>::_,
	size_t> Count(P pred) const
	{
		R range = me();
		return range.CountAdvance(pred);
	}


	//! Последовательно просматривает элементы диапазона до тех пор,
	//! пока не встретится элемент, равный x, или не будет достигнут конец диапазона.
	//! Возвращает количество пройденных элементов.
	template<typename X> forceinline Meta::EnableIf<
		Meta::IsConvertible<X, T>::_,
	size_t> CountUntil(const X& x) const
	{return Algo::CountUntil(me(), x);}
	
	//! Последовательно просматривает элементы диапазона до тех пор, пока для элемента
	//! не выполнится условие predicate или не будет достигнут конец диапазона.
	//! \return Возвращает количество пройденных элементов.
	template<typename P> forceinline Meta::EnableIf<
		Meta::IsCallable<P, T>::_,
	size_t> CountUntil(P predicate) const
	{return Algo::CountUntil(me(), predicate);}


	//! Последовательно удаляет элементы диапазона до тех пор, пока не встретится элемент,
	//! равный x, или не будет достигнут конец диапазона.
	//! \return Возвращает диапазон пройденных элементов.
	template<typename X> forceinline Meta::EnableIf<
		Meta::IsConvertible<X, T>::_,
	R> ReadUntilAdvance(const X& x, size_t* ioIndex=null)
	{return Algo::ReadUntilAdvance(me(), x, ioIndex);}
	
	//! Последовательно удаляет элементы диапазона до тех пор, пока для элемента
	//! не выполнится условие predicate или не будет достигнут конец диапазона.
	//! Возвращает диапазон пройденных элементов.
	template<typename P> forceinline Meta::EnableIf<
		Meta::IsCallable<P, T>::_,
	R> ReadUntilAdvance(P predicate, size_t* ioIndex=null)
	{return Algo::ReadUntilAdvance(me(), predicate, ioIndex);}

	//! Последовательно просматривает элементы диапазона до тех пор,
	//! пока не встретится элемент, равный x, или не будет достигнут конец диапазона.
	//! Возвращает диапазон пройденных элементов.
	template<typename X, typename U=R> forceinline Meta::EnableIf<
		Meta::IsConvertible<X, T>::_,
	ResultOfTake<U>> ReadUntil(const X& x, size_t* ioIndex=null) const
	{return Algo::ReadUntil(me(), x, ioIndex);}
	
	//! Последовательно просматривает элементы диапазона до тех пор,
	//! пока для элемента не выполнится условие predicate или не будет достигнут конец диапазона.
	//! Возвращает диапазон пройденных элементов.
	template<typename P, typename U=R> forceinline Meta::EnableIf<
		Meta::IsCallable<P, T>::_,
	ResultOfTake<U>> ReadUntil(P predicate, size_t* ioIndex=null) const
	{return Algo::ReadUntil(me(), predicate, ioIndex);}


	//! Найти первое вхождение элемента what в этот диапазон.
	//! \param what Искомый элемент.
	//! \param ioIndex[inout] Указатель на счётчик, который увеличивается на количество элементов, предшествующих найденной позиции. Может быть null.
	//! \returns null, если значение не найдено. Часть этого диапазона, начиная с позиции, на которой начинается первое вхождение what.
	template<typename X> Meta::EnableIf<
		Meta::IsConvertible<X, T>::_,
	R> Find(const X& what, size_t* ioIndex=null) const
	{return Algo::Find(me(), what, ioIndex);}

	template<typename X> Meta::EnableIf<
		Meta::IsConvertible<X, T>::_,
	bool> Contains(const X& what) const {return Algo::Contains(me(), what);}

	//! Найти первое вхождение элемента, удовлетворяющего некоторому условию, в этот диапазон.
	//! \param pred Условие, которому должен удовлетворять искомый элемент.
	//! \param ioIndex[inout] Указатель на счётчик, который увеличивается на количество элементов, предшествующих найденной позиции. Может быть null.
	template<typename P> Meta::EnableIf<
		Meta::IsCallable<P, T>::_,
	R> Find(P pred, size_t* ioIndex=null) const
	{return Algo::Find(me(), pred, ioIndex);}

	bool Contains(const T& what) const {return Algo::Contains(me(), what);}
	

	template<typename Ws, typename U=R> forceinline Meta::EnableIf<
		IsFiniteForwardRangeOf<Ws, T>::_,
	ResultOfTake<U>> ReadUntilAdvanceAny(const Ws& whats, size_t* ioIndex=null, size_t* oWhatIndex=null)
	{return Algo::ReadUntilAdvanceAny(me(), whats, ioIndex, oWhatIndex);}

	template<typename Ws, typename U=R> forceinline Meta::EnableIf<
		IsFiniteForwardRangeOf<Ws, T>::_,
	ResultOfTake<U>> ReadUntilAny(const Ws& whats, size_t* ioIndex=null, size_t* oWhatIndex=null) const
	{return Algo::ReadUntilAny(me(), whats, ioIndex, oWhatIndex);}

	//! Найти первое вхождение диапазона what в этот диапазон.
	//! Начало диапазона устанавливается на начало первого вхождения what или совпадает с концом, если диапазон не содержит what.
	//! \param what Искомый диапазон.
	//! \param ioIndex[inout] Указатель на счётчик, который увеличивается на количество элементов, предшествующих найденной позиции. Может быть null.
	//! \returns Возвращает ссылку на себя.
	template<typename RW> Meta::EnableIf<
		IsFiniteForwardRangeOf<RW, T>::_,
	R&> FindAdvance(const RW& what, size_t* ioIndex=null)
	{return Algo::FindAdvance(me(), what, ioIndex);}


	//! Найти первое вхождение диапазона what в этот диапазон.
	//! \param what Искомый диапазон.
	//! \param ioIndex[inout] Указатель на счётчик, который увеличивается на количество
	//! элементов, предшествующих найденной позиции. Может быть null.
	//! \returns null, если значение не найдено. Часть этого диапазона, начиная с позиции, на которой начинается первое вхождение what.
	template<typename RW> forceinline Meta::EnableIf<
		IsFiniteForwardRangeOf<RW, T>::_,
	R> Find(const RW& what, size_t* ioIndex=null) const
	{return Algo::Find(me(), what, ioIndex);}

	//! Найти первое вхождение любого диапазона из диапазона поддиапазонов subranges в этот диапазон.
	//! Начало этого диапазона смещается к найденному поддиапазону или совмещается
	//! с концом в случае, когда ни один из поддиапазонов не найден.
	//! \param subranges[inout] Диапазон искомых поддиапазонов.
	//! После вызова этой функции начало subranges смещается к первому совпавшему элементу.
	//! Если совпадений не найдено, subranges окажется в исходном состоянии.
	//! \param ioIndex[inout] Указатель на счётчик, который увеличивается на количество
	//! элементов, предшествующих найденной позиции. Может быть null.
	//! \param oSubrangeIndex[out] Указатель на переменную, в которую будет записан индекс найденного
	//! элемента в диапазоне whats. Если элемент не был найден, будет записано значение whats.Count().
	//! \return Возвращает ссылку на себя.
	template<typename RWs> Meta::EnableIf<
		IsFiniteForwardRangeOfFiniteForwardRanges<RWs>::_,
	R&> FindAdvanceAnyAdvance(RWs& subranges, size_t* ioIndex=null, size_t* oSubrangeIndex=null)
	{return Algo::FindAdvanceAnyAdvance(me(), subranges, ioIndex, oSubrangeIndex);}

	//! Найти количество символов, предшествующих первому вхождению любого диапазона
	//! из диапазона поддиапазонов subranges в этот диапазон.
	//! Начало этого диапазона смещается к найденному поддиапазону или совмещается
	//! с концом в случае, когда ни один поддиапазон не найден.
	//! \param subranges[inout] Диапазон искомых поддиапазонов.
	//! После вызова этой функции начало subranges смещается к первому совпавшему элементу.
	//! Если совпадений не найдено, subranges окажется в исходном состоянии.
	//! \param oSubrangeIndex[out] Указатель на переменную, в которую будет записан индекс найденного
	//! элемента в диапазоне subranges. Если элемент не был найден, будет записано значение subranges.Count().
	//! \return Возвращает количество пройденных элементов.
	template<typename RWs> Meta::EnableIf<
		IsFiniteForwardRangeOfFiniteForwardRanges<RWs>::_,
	size_t> CountUntilAdvanceAnyAdvance(RWs& subranges, size_t* oSubrangeIndex=null)
	{return Algo::CountUntilAdvanceAnyAdvance(me(), subranges, oSubrangeIndex);}

	//! Прочитать количество символов, предшествующих первому вхождению любого диапазона
	//! из диапазона поддиапазонов subranges в этот диапазон.
	//! Начало этого диапазона смещается к найденному поддиапазону или совмещается
	//! с концом в случае, когда ни один поддиапазон не найден.
	//! \param subranges[inout] Диапазон искомых поддиапазонов.
	//! После вызова этой функции начало subranges смещается к первому совпавшему элементу.
	//! Если совпадений не найдено, subranges окажется в исходном состоянии.
	//! \param oSubrangeIndex[out] Указатель на переменную, в которую будет записан индекс найденного
	//! элемента в диапазоне subranges. Если элемент не был найден, будет записано значение subranges.Count().
	//! \return Возвращает диапазон прочитанных элементов.
	template<typename RWs, typename U=R> Meta::EnableIf<
		IsFiniteForwardRangeOfFiniteForwardRanges<RWs>::_,
	ResultOfTake<U>> ReadUntilAdvanceAnyAdvance(RWs& subranges, size_t* ioIndex, size_t* oSubrangeIndex=null)
	{return Algo::ReadUntilAdvanceAnyAdvance(me(), subranges, ioIndex, oSubrangeIndex);}


	//! Найти первое вхождение любого диапазона из диапазона поддиапазонов subranges в этот диапазон.
	//! Начало этого диапазона смещается к найденному элементу или совмещается с концом в случае, когда элемент не найден.
	//! \param subranges Диапазон искомых поддиапазонов.
	//! \param ioIndex[inout] Указатель на счётчик, который увеличивается на
	//! количество элементов, предшествующих найденной позиции. Может быть null.
	//! \param oSubrangeIndex[out] Указатель на переменную, в которую будет записан индекс найденного
	//! элемента в диапазоне whats. Если элемент не был найден, будет записано значение whats.Count().
	//! \return Возвращает ссылку на себя.
	template<typename RWs> forceinline Meta::EnableIf<
		IsFiniteForwardRangeOfFiniteForwardRanges<RWs>::_,
	R&> FindAdvanceAny(const RWs& subranges, size_t* ioIndex=null, size_t* oSubrangeIndex=null)
	{return Algo::FindAdvanceAny(me(), subranges, ioIndex, oSubrangeIndex);}

	//! Найти количество символов, предшествующих первому вхождению любого диапазона из диапазона поддиапазонов subranges в этот диапазон.
	//! Начало этого диапазона смещается к найденному поддиапазону или совмещается
	//! с концом в случае, когда ни один поддиапазон не найден.
	//! \param subranges[inout] Диапазон искомых поддиапазонов.
	//! \param oSubrangeIndex[out] Указатель на переменную, в которую будет записан индекс найденного
	//! элемента в диапазоне subranges. Если элемент не был найден, будет записано значение subranges.Count().
	//! \return Возвращает количество пройденных элементов.
	template<typename RWs> forceinline Meta::EnableIf<
		IsFiniteForwardRangeOfFiniteForwardRanges<RWs>::_,
	size_t> CountUntilAdvanceAny(const RWs& subranges, size_t* oSubrangeIndex=null)
	{return Algo::CountUntilAdvanceAny(me(), subranges, oSubrangeIndex);}

	//! Прочитать количество символов, предшествующих первому вхождению любого диапазона из диапазона поддиапазонов subranges в этот диапазон.
	//! Начало этого диапазона смещается к найденному поддиапазону или совмещается
	//! с концом в случае, когда ни один поддиапазон не найден.
	//! \param subranges[inout] Диапазон искомых поддиапазонов.
	//! После вызова этой функции начало subranges смещается к первому совпавшему элементу.
	//! Если совпадений не найдено, subranges окажется в исходном состоянии.
	//! \param oSubrangeIndex[out] Указатель на переменную, в которую будет записан индекс найденного
	//! элемента в диапазоне subranges. Если элемент не был найден, будет записано значение subranges.Count().
	//! \return Возвращает диапазон прочитанных элементов.
	template<typename RWs, typename U=R> Meta::EnableIf<
		IsFiniteForwardRangeOfFiniteForwardRanges<RWs>::_,
	ResultOfTake<U>> ReadUntilAdvanceAny(const RWs& subranges, size_t* ioIndex=null, size_t* oSubrangeIndex=null)
	{return Algo::ReadUntilAdvanceAny(me(), subranges, ioIndex, oSubrangeIndex);}



	//! Найти первое вхождение любого диапазона из диапазона поддиапазонов subranges в этот диапазон.
	//! \param subranges[inout] Диапазон искомых поддиапазонов.
	//! После вызова этой функции начало subranges смещается к первому совпавшему элементу.
	//! Если совпадений не найдено, subranges окажется в исходном состоянии.
	//! \param ioIndex[inout] Указатель на счётчик, который увеличивается
	//! на количество элементов, предшествующих найденной позиции. Может быть null.
	//! \param oSubrangeIndex[out] Указатель на переменную, в которую будет записан индекс найденного элемента в диапазоне whats. Если элемент не был найден, будет записано значение whats.Count().
	//! \return Возвращает диапазон, полученный из этого удалением всех элементов до первого вхождения любого из искомых диапазонов.
	template<typename RWs> forceinline Meta::EnableIf<
		IsFiniteForwardRangeOfFiniteForwardRanges<RWs>::_,
	R> FindAnyAdvance(RWs& subranges, size_t* ioIndex=null, size_t* oSubrangeIndex=null) const
	{return Algo::FindAnyAdvance(me(), subranges, ioIndex, oSubrangeIndex);}

	//! Найти количество символов, предшествующих первому вхождению любого диапазона из диапазона поддиапазонов subranges в этот диапазон.
	//! \param subranges[inout] Диапазон искомых поддиапазонов.
	//! После вызова этой функции начало subranges смещается к первому совпавшему элементу.
	//! Если совпадений не найдено, subranges останется в исходном состоянии.
	//! \param oSubrangeIndex[out] Указатель на переменную, в которую будет записан индекс
	//! найденного элемента в диапазоне subranges. Если элемент не был найден, будет записано значение subranges.Count().
	//! \return Возвращает количество пройденных элементов.
	template<typename RWs> forceinline Meta::EnableIf<
		IsFiniteForwardRangeOfFiniteForwardRanges<RWs>::_,
	size_t> CountUntilAnyAdvance(RWs& subranges, size_t* oSubrangeIndex=null) const
	{return Algo::CountUntilAnyAdvance(me(), subranges, oSubrangeIndex);}

	//! Прочитать количество символов, предшествующих первому вхождению любого диапазона
	//! из диапазона поддиапазонов subranges в этот диапазон.
	//! \param subranges[inout] Диапазон искомых поддиапазонов.
	//! После вызова этой функции начало subranges смещается к первому совпавшему элементу.
	//! Если совпадений не найдено, subranges останется в исходном состоянии.
	//! \param oSubrangeIndex[out] Указатель на переменную, в которую будет записан индекс найденного элемента в диапазоне subranges. Если элемент не был найден, будет записано значение subranges.Count().
	//! \return Возвращает диапазон прочитанных элементов.
	template<typename RWs, typename U=R> Meta::EnableIf<
		IsFiniteForwardRangeOfFiniteForwardRanges<RWs>::_,
	ResultOfTake<U>> ReadUntilAnyAdvance(RWs& subranges, size_t* ioIndex=null, size_t* oSubrangeIndex=null) const
	{return Algo::ReadUntilAnyAdvance(me(), subranges, ioIndex, oSubrangeIndex);}



	//! Найти первое вхождение любого поддиапазона из диапазона subranges в этот диапазон.
	//! \param subranges Искомые поддиапазоны.
	//! \param ioIndex[inout] Указатель на счётчик, который увеличивается на количество элементов, предшествующих найденной позиции. Может быть null.
	//! \param oWhatIndex[out] Указатель на переменную, в которую будет записан индекс найденного поддиапазона
	//! в диапазоне subranges. Если элемент не был найден, будет записано значение whats.Count().
	template<typename RWs> forceinline Meta::EnableIf<
		IsFiniteForwardRangeOfFiniteForwardRanges<RWs>::_,
	R> FindAny(const RWs& subranges, size_t* ioIndex=null, size_t* oWhatIndex=null) const
	{return Algo::FindAny(me(), subranges, ioIndex, oWhatIndex);}

	//! Найти количество символов, предшествующих первому вхождению любого диапазона из диапазона поддиапазонов subranges в этот диапазон.
	//! \param subranges[inout] Диапазон искомых поддиапазонов.
	//! После вызова этой функции начало subranges смещается к первому совпавшему элементу.
	//! Если совпадений не найдено, subranges останется в исходном состоянии.
	//! \param oSubrangeIndex[out] Указатель на переменную, в которую будет записан индекс
	//! найденного элемента в диапазоне subranges. Если элемент не был найден, будет записано значение subranges.Count().
	//! \return Возвращает количество пройденных элементов.
	template<typename RWs> forceinline Meta::EnableIf<
		IsFiniteForwardRangeOfFiniteForwardRanges<RWs>::_,
	size_t> CountUntilAny(const RWs& subranges, size_t* oSubrangeIndex=null) const
	{return Algo::CountUntilAny(me(), subranges, oSubrangeIndex);}

	//! Прочитать количество символов, предшествующих первому вхождению любого диапазона
	//! из диапазона поддиапазонов subranges в этот диапазон.
	//! \param subranges[inout] Диапазон искомых поддиапазонов.
	//! После вызова этой функции начало subranges смещается к первому совпавшему элементу.
	//! Если совпадений не найдено, subranges останется в исходном состоянии.
	//! \param oSubrangeIndex[out] Указатель на переменную, в которую будет записан индекс найденного элемента в диапазоне subranges. Если элемент не был найден, будет записано значение subranges.Count().
	//! \return Возвращает диапазон прочитанных элементов.
	template<typename RWs, typename U=R> forceinline Meta::EnableIf<
		IsFiniteForwardRangeOfFiniteForwardRanges<RWs>::_,
	ResultOfTake<U>> ReadUntilAny(const RWs& subranges, size_t* ioIndex=null, size_t* oSubrangeIndex=null) const
	{return Algo::ReadUntilAny(me(), subranges, ioIndex, oSubrangeIndex);}



	//! Найти количество элементов, предшествующих первому вхождению диапазона what в этот диапазон.
	//! Начало диапазона устанавливается на начало первого вхождения what или совпадает с концом, если диапазон не содержит what.
	//! \param what Искомый диапазон.
	//! \returns Возвращает количество пройденных элементов.
	template<typename RW> forceinline Meta::EnableIf<
		IsFiniteForwardRangeOf<RW, T>::_,
	size_t> CountUntilAdvance(const RW& what)
	{return Algo::CountUntilAdvance(me(), what);}

	//! Прочитать элементы из начала диапазона, предшествующие первому вхождению диапазона what в этот диапазон.
	//! \param what Искомый диапазон.
	//! \returns Возвращает диапазон пройденных элементов.
	template<typename RW, typename U=R> Meta::EnableIf<
		IsFiniteForwardRangeOf<RW, T>::_,
	ResultOfTake<U>> ReadUntilAdvance(const RW& what, size_t* ioIndex=null)
	{return Algo::ReadUntilAdvance(me(), what, ioIndex);}

	//! Найти количество элементов, предшествующих первому вхождению диапазона what в этот диапазон.
	//! \param what Искомый диапазон.
	//! \returns Возвращает количество пройденных элементов.
	template<typename RW> forceinline Meta::EnableIf<
		IsFiniteForwardRangeOf<RW, T>::_,
	size_t> CountUntil(const RW& what) const
	{return Algo::CountUntil(me(), what);}

	//! Прочитать элементы из начала диапазона, предшествующие первому вхождению диапазона what в этот диапазон.
	//! \param what Искомый диапазон.
	//! \returns Возвращает диапазон пройденных элементов.
	template<typename RW, typename U=R> forceinline Meta::EnableIf<
		IsFiniteForwardRangeOf<RW, T>::_,
	ResultOfTake<U>> ReadUntil(const RW& what, size_t* ioIndex=null) const
	{return Algo::ReadUntil(me(), what, ioIndex);}



	

	template<typename RW> forceinline Meta::EnableIf<
		IsFiniteForwardRange<RW>::_ || HasAsRange<RW>::_,
	bool> Contains(const RW& what) const {return Algo::Contains(me(), what);}

	template<size_t N> forceinline bool Contains(const T(&what)[N]) const
	{return Algo::Contains(me(), what);}

	template<typename P=bool(*)(const T&, const T&)> bool IsSorted(P comparer=&Op::Less<T>) const
	{
		if(me().Empty()) return true;
		R range = me();
		T prev, cur = range.First();
		range.PopFirst();
		while(!range.Empty())
		{
			prev = core::move(cur);
			cur = range.First();
			if(comparer(cur, prev)) return false;
			range.PopFirst();
		}
		return true;
	}

	template<typename RW> Meta::EnableIf<
		IsFiniteForwardRange<RW>::_,
	size_t> CountAdvance(const RW& what)
	{
		size_t result=0;
		size_t whatCount = what.Count();
		while(me().FindAdvance(what), !me().Empty())
		{
			me().PopFirstExactly(whatCount);
			result++;
		}
		return result;
	}

	template<typename RW> forceinline Meta::EnableIf<
		IsFiniteForwardRange<RW>::_,
	size_t> Count(const RW& what) const
	{
		R range = me();
		return range.CountAdvance(what);
	}



	template<typename ResultRange, typename ReplacementRange> Meta::EnableIf<
		IsOutputRange<ResultRange>::_ &&
		IsFiniteForwardRange<ReplacementRange>::_,
	ResultOfTake<ResultRange>> MultiReplaceToAdvance(ResultRange& dstBuffer,
		const ReplacementRange& replacementSubranges) const
	{
		size_t index = 0;
		R src = me();
		ResultRange resultStart = dstBuffer;
		size_t substrIndex = 0;
		while(src.ReadUntilAdvanceAny(Unzip<0>(replacementSubranges), &index, &substrIndex).CopyToAdvance(dstBuffer), !src.Empty())
		{
			auto&& replacement = replacementSubranges.AtIndex(substrIndex);
			Meta::Get<1>(replacement).CopyToAdvance(dstBuffer);
			src.PopFirstExactly(Meta::Get<0>(replacement).Count());
			index += Meta::Get<1>(replacement).Count();
		}
		return resultStart.Take(index);
	}

	template<typename ResultRange, typename ReplacementRange> forceinline Meta::EnableIf<
		IsOutputRange<ResultRange>::_ &&
		IsFiniteForwardRange<ReplacementRange>::_,
	ResultOfTake<ResultRange>> MultiReplaceTo(const ResultRange& dstBuffer,
		const ReplacementRange& replacementSubranges) const
	{
		ResultRange dstRangeCopy = dstBuffer;
		return MultiReplaceToAdvance(dstRangeCopy, replacementSubranges);
	}




	template<typename OpeningBracketRange,
		typename ClosingBracketRange,
		typename StopTokenRange,
		typename CommentBlockRangePairRange,
		typename RecursiveCommentBlockRangePairRange,
		typename U=R>

	Meta::EnableIf<
		IsFiniteForwardRange<OpeningBracketRange>::_ &&
		IsFiniteForwardRange<ClosingBracketRange>::_ &&
		IsFiniteForwardRange<StopTokenRange>::_ &&
		IsFiniteForwardRange<CommentBlockRangePairRange>::_ &&
		IsFiniteForwardRange<RecursiveCommentBlockRangePairRange>::_,

	ResultOfTake<U>> ReadRecursiveBlockAdvance(int& counter, size_t* ioIndex,
		OpeningBracketRange openingBracket, ClosingBracketRange closingBracket, StopTokenRange stopToken,
		CommentBlockRangePairRange commentBlocks, RecursiveCommentBlockRangePairRange recursiveCommentBlocks)
	{
		R start = me();
		size_t index = 0;
		const size_t openingBracketLen = openingBracket.Count();
		const size_t closingBracketLen = closingBracket.Count();
		const size_t stopTokenLen = stopToken.Count();
		while(!me().Empty() && counter!=0)
		{
			if(openingBracketLen!=0 && me().StartsWith(openingBracket))
			{
				counter++;
				me().PopFirstExactly(openingBracketLen);
				index += openingBracketLen;
				continue;
			}

			if(closingBracketLen!=0 && me().StartsWith(closingBracket))
			{
				counter--;
				me().PopFirstExactly(closingBracketLen);
				index += closingBracketLen;
				continue;
			}

			if(stopTokenLen!=0 && me().StartsWith(stopToken))
			{
				me().PopFirstExactly(stopTokenLen);
				index += stopTokenLen;
				break;
			}

			bool commentFound = false;
			for(auto& commentBlock: commentBlocks)
			{
				commentFound = me().StartsWith(Meta::Get<0>(commentBlock));
				if(!commentFound) continue;

				const size_t commentBlockOpeningLen = Meta::Get<0>(commentBlock).Count();
				const size_t commentBlockClosingLen = Meta::Get<1>(commentBlock).Count();
				me().PopFirstN(commentBlockOpeningLen);
				index += commentBlockOpeningLen;
				me().FindAdvance(Meta::Get<1>(commentBlock), &index);
				me().PopFirstN(commentBlockClosingLen);
				index += commentBlockClosingLen;
				break;
			}
			if(commentFound) continue;

			for(auto& commentBlock: recursiveCommentBlocks)
			{
				commentFound = me().StartsWith(Meta::Get<0>(commentBlock));
				if(!commentFound) continue;

				int commentCounter = 1;
				ReadRecursiveBlockAdvance(commentCounter, &index,
					Meta::Get<0>(commentBlock), Meta::Get<1>(commentBlock), NullRange<T>(),
					NullRange<Meta::Tuple<NullRange<T>, NullRange<T>>>(),
					NullRange<Meta::Tuple<NullRange<T>, NullRange<T>>>());
				break;
			}
			if(commentFound) continue;

			me().PopFirst();
		}
		if(ioIndex!=null) *ioIndex += index;
		return start.Take(index);
	}




	template<typename ResultRange, typename EntryStartRange, typename EntryEndRange,
		typename SubstitutionRangeOfTupleOfRanges, typename UnknownSubstitutionRange, typename U=R> Meta::EnableIf<
		
		IsOutputRange<ResultRange>::_ && IsForwardRange<ResultRange>::_ &&
		IsFiniteForwardRange<EntryStartRange>::_ && IsFiniteForwardRange<EntryEndRange>::_ &&
		IsFiniteForwardRange<SubstitutionRangeOfTupleOfRanges>::_ &&
		IsFiniteForwardRange<UnknownSubstitutionRange>::_,

	ResultOfTake<U>> StringSubstitute(ResultRange& dstBuffer,
		EntryStartRange entryStart, EntryEndRange entryEnd,
		SubstitutionRangeOfTupleOfRanges substitutions, UnknownSubstitutionRange unknownSubstitution)
	{
		R src = me();
		size_t index = 0;
		ResultRange resultBufferStart = dstBuffer;
		while(src.ReadUntilAdvance(entryStart, &index).CopyToAdvance(dstBuffer), !src.Empty())
		{
			src.PopFirstExactly(entryStart.Count());
			int counter = 1;
			auto entryStr = src.ReadRecursiveBlockAdvance(counter, &index, entryStart, entryEnd, null, null, null);
			if(counter>0)
			{
				INTRA_ASSERT(src.Empty());
				entryStr.CopyToAdvance(dstBuffer);
				index += entryStr.Count();
				break;
			}
			entryStr.PopLastExactly(entryEnd.Count());
			auto substituionsCopy = substitutions;
			substituionsCopy.FindAdvance(Meta::TupleElementEquals<0>(entryStr));
			if(substituionsCopy.Empty())
			{
				unknownSubstitution.CopyToAdvance(dstBuffer);
				index += unknownSubstitution.Count();
			}
			else
			{
				substituionsCopy.First().CopyToAdvance(dstBuffer);
				index += substituionsCopy.First().Count();
			}
		}
		return resultBufferStart.Take(index);
	}



};



}}

