#pragma once

#include "Core/Core.h"
#include "Meta/Type.h"
#include "Meta/Tuple.h"
#include "Meta/Preprocessor.h"

namespace Intra { namespace Range {

INTRA_DEFINE_EXPRESSION_CHECKER(HasEmpty, static_cast<bool>(Meta::Val<T>().Empty()));
INTRA_DEFINE_EXPRESSION_CHECKER(HasLength, static_cast<size_t>(Meta::Val<T>().Length()));
INTRA_DEFINE_EXPRESSION_CHECKER(HasFirst, Meta::Val<T>().First());
INTRA_DEFINE_EXPRESSION_CHECKER(HasLast, Meta::Val<T>().Last());
INTRA_DEFINE_EXPRESSION_CHECKER(HasPopFirst, Meta::Val<T>().PopFirst());
INTRA_DEFINE_EXPRESSION_CHECKER(HasPopLast, Meta::Val<T>().PopLast());
INTRA_DEFINE_EXPRESSION_CHECKER(HasSlicing, Meta::Val<T>().opSlice(size_t(), size_t()));
INTRA_DEFINE_EXPRESSION_CHECKER(HasIndex, Meta::Val<T>()[size_t()]);
INTRA_DEFINE_EXPRESSION_CHECKER(HasData, Meta::Val<T>().Data()==static_cast<typename T::value_type*>(null));

INTRA_DEFINE_EXPRESSION_CHECKER(HasPut, Meta::Val<T>().Put(Meta::Val<typename T::value_type>()));
INTRA_DEFINE_EXPRESSION_CHECKER2(HasTypedPut, Meta::Val<T1>().Put(Meta::Val<T2>()),,);

namespace TypeEnum
{
	typedef byte Type;
	enum: Type {Input, Forward, Bidirectional, RandomAccess, Array, Error};
}


template<typename R, typename T, class PARENT> struct OutputRangeMixin;

template<typename R, typename T, class PARENT> struct InputRangeMixin;
template<typename R, typename T, class PARENT> struct FiniteInputRangeMixin;
template<typename R, typename T, class PARENT> struct ForwardRangeMixin;
template<typename R, typename T, class PARENT> struct FiniteForwardRangeMixin;
template<typename R, typename T, class PARENT> struct BidirectionalRangeMixin;
template<typename R, typename T, class PARENT> struct RandomAccessRangeMixin;
template<typename R, typename T, class PARENT> struct FiniteRandomAccessRangeMixin;

template<typename R>
struct IsOutputRange: Meta::TypeFromValue<bool, HasPut<Meta::RemoveConstRef<R>>::_> {};

template<typename R>
struct IsInputRange: Meta::TypeFromValue<bool,
	HasFirst<R>::_ && HasPopFirst<R>::_ && HasEmpty<R>::_
> {};

INTRA_DEFINE_EXPRESSION_CHECKER_WITH_CONDITION(IsForwardRange, Meta::Val<typename T::value_type>(),
	IsInputRange<T>::_ && T::RangeType>=TypeEnum::Forward);

INTRA_DEFINE_EXPRESSION_CHECKER2_WITH_CONDITION(IsInputRangeOf, Meta::Val<typename T1::value_type>(),
	(IsInputRange<T1>::_ && Meta::IsConvertible<typename T1::value_type, T2>::_),,);


template<typename R, typename T>
struct IsForwardRangeOf: Meta::TypeFromValue<bool,
	IsInputRangeOf<R, T>::_ && IsForwardRange<R>::_
> {};

template<typename R>
struct IsBidirectionalRange: Meta::TypeFromValue<bool,
	IsForwardRange<R>::_ && HasLast<R>::_ && HasPopLast<R>::_
	> {};

template<typename R, typename T>
struct IsBidirectionalRangeOf: Meta::TypeFromValue<bool,
	IsBidirectionalRange<R>::_ && IsInputRangeOf<R, T>::_
	> {};

template<typename R>
struct IsRandomAccessRange: Meta::TypeFromValue<bool,
	IsForwardRange<R>::_ && HasIndex<R>::_
> {};

template<typename R>
struct IsRangeHasLength: Meta::TypeFromValue<bool,
	IsInputRange<R>::_ && HasLength<R>::_
> {};

template<typename R>
struct IsSliceableRange: Meta::TypeFromValue<bool,
	IsInputRange<R>::_ && HasSlicing<R>::_
> {};



template<typename R>
struct IsFiniteRandomAccessRange: Meta::TypeFromValue<bool,
	IsRandomAccessRange<R>::_ && HasLength<R>::_
> {};

template<typename R>
struct IsArrayRange: Meta::TypeFromValue<bool,
	IsFiniteRandomAccessRange<R>::_ && HasData<R>::_
> {};

INTRA_DEFINE_EXPRESSION_CHECKER2_WITH_CONDITION(IsArrayRangeOfExactly, Meta::Val<typename T1::value_type>(),
	(IsArrayRange<T1>::_ && Meta::TypeEqualsIgnoreCV<typename T1::value_type, T2>::_),,);

//INTRA_DEFINE_EXPRESSION_CHECKER_WITH_CONDITION(IsFiniteRange, T::RangeIsFinite,
//	IsInputRange<T>::_ && T::RangeIsFinite);

template<class T> struct HasRangeIsFinite
{
    template<bool> struct Helper {};
    template<typename U> static char HelpMe(...);
    template<typename U> static short HelpMe(Helper<U::RangeIsFinite>*);
    enum: bool {_=(sizeof(HelpMe<T>(0))==sizeof(short))};
};

template<typename R, bool=HasRangeIsFinite<R>::_> struct IsFiniteRange: Meta::TypeFromValue<bool, R::RangeIsFinite> {};
template<typename R> struct IsFiniteRange<R, false>: Meta::TypeFromValue<bool, false> {};


INTRA_DEFINE_EXPRESSION_CHECKER_WITH_CONDITION(IsCharRange, Meta::Val<typename T::value_type>(),
	IsInputRange<T>::_ && Meta::IsCharType<typename T::value_type>::_);

INTRA_DEFINE_EXPRESSION_CHECKER_WITH_CONDITION(IsRangeElementAssignable, Meta::Val<typename T::return_value_type>(),
	IsInputRange<T>::_ && Meta::IsLValueReference<typename T::return_value_type>::_);




template<typename R> struct IsInfiniteRange: Meta::TypeFromValue<bool,
	IsInputRange<R>::_ && !IsFiniteRange<R>::_
> {};

template<typename R>
struct IsFiniteInputRange: Meta::TypeFromValue<bool,
	IsInputRange<R>::_ && IsFiniteRange<R>::_
> {};

template<typename R>
struct IsFiniteForwardRange: Meta::TypeFromValue<bool,
	IsForwardRange<R>::_ && IsFiniteRange<R>::_
> {};

INTRA_DEFINE_EXPRESSION_CHECKER_WITH_CONDITION(IsInputRangeOfPod, Meta::Val<typename T::value_type>(),
	(IsInputRange<T>::_ && Meta::IsPod<typename T::value_type>::_));

INTRA_DEFINE_EXPRESSION_CHECKER_WITH_CONDITION(IsInputRangeOfTrivial, Meta::Val<typename T::value_type>(),
	(IsInputRange<T>::_ && Meta::IsAlmostPod<typename T::value_type>::_));

INTRA_DEFINE_EXPRESSION_CHECKER2_WITH_CONDITION(IsFiniteInputRangeOf, Meta::Val<typename T1::value_type>(),
	(IsFiniteInputRange<T1>::_ && Meta::IsConvertible<typename T1::value_type, T2>::_),,);

INTRA_DEFINE_EXPRESSION_CHECKER2_WITH_CONDITION(ValueTypeEquals, (Meta::Val<typename T1::value_type>(), Meta::Val<typename T2::value_type>()),
	(Meta::TypeEquals<typename T1::value_type, typename T2::value_type>::_),,);

INTRA_DEFINE_EXPRESSION_CHECKER_WITH_CONDITION(IsFiniteInputCharRange, Meta::Val<typename T::value_type>(),
	(IsFiniteInputRange<T>::_ && Meta::IsCharType<typename T::value_type>::_));

INTRA_DEFINE_EXPRESSION_CHECKER_WITH_CONDITION(IsFiniteInputNonCharRange, Meta::Val<typename T::value_type>(),
	(IsFiniteInputRange<T>::_ && !Meta::IsCharType<typename T::value_type>::_));

INTRA_DEFINE_EXPRESSION_CHECKER_WITH_CONDITION(IsFiniteForwardNonCharRange, Meta::Val<typename T::value_type>(),
	(IsFiniteForwardRange<T>::_ && !Meta::IsCharType<typename T::value_type>::_));

INTRA_DEFINE_EXPRESSION_CHECKER2_WITH_CONDITION(IsFiniteInputRangeOfExactly, Meta::Val<typename T1::value_type>(),
	(IsFiniteInputRange<T1>::_ && Meta::TypeEquals<typename T1::value_type, T2>::_),,);

INTRA_DEFINE_EXPRESSION_CHECKER2_WITH_CONDITION(IsFiniteForwardRangeOfExactly, Meta::Val<typename T1::value_type>(),
	(IsFiniteForwardRange<T1>::_ && Meta::TypeEquals<typename T1::value_type, T2>::_),,);

INTRA_DEFINE_EXPRESSION_CHECKER2_WITH_CONDITION(IsFiniteForwardRangeOf, Meta::Val<typename T1::value_type>(),
	(IsFiniteForwardRange<T1>::_ && Meta::IsConvertible<typename T1::value_type, T2>::_),,);

INTRA_DEFINE_EXPRESSION_CHECKER_WITH_CONDITION(IsRangeOfRanges, Meta::Val<typename T::value_type>(),
	(IsInputRange<T>::_ && IsInputRange<typename T::value_type>::_));

INTRA_DEFINE_EXPRESSION_CHECKER_WITH_CONDITION(IsRangeOfTuples, Meta::Val<typename T::value_type>(),
	(IsInputRange<T>::_ && Meta::IsTuple<typename T::value_type>::_));

INTRA_DEFINE_EXPRESSION_CHECKER_WITH_CONDITION(IsRangeOfFiniteForwardRanges, Meta::Val<typename T::value_type>(),
	(IsInputRange<T>::_ && IsFiniteForwardRange<typename T::value_type>::_));

INTRA_DEFINE_EXPRESSION_CHECKER2_WITH_CONDITION(IsRangeOfFiniteForwardRangesOf, Meta::Val<typename T1::value_type::value_type>(),
	(IsInputRange<T1>::_ && IsFiniteForwardRange<typename T1::value_type>::_) &&
	(Meta::IsConvertible<typename T1::value_type::value_type, T2>::_),,);



template<typename R>
struct IsForwardRangeOfFiniteForwardRanges: Meta::TypeFromValue<bool,
	IsForwardRange<R>::_ && IsRangeOfFiniteForwardRanges<R>::_
> {};

template<typename R>
struct IsFiniteForwardRangeOfFiniteForwardRanges: Meta::TypeFromValue<bool,
	IsFiniteForwardRange<R>::_ && IsRangeOfFiniteForwardRanges<R>::_
> {};

template<typename R, typename T>
struct IsFiniteInputRangeOfFiniteForwardRangesOf: Meta::TypeFromValue<bool,
	IsFiniteInputRange<R>::_ && IsRangeOfFiniteForwardRangesOf<R, T>::_
> {};



template<typename R0> bool AnyEmpty(const Meta::Tuple<R0>& ranges)
{
	return ranges.first.Empty();
}

template<typename R0, typename R1, typename... Rs> bool AnyEmpty(const Meta::Tuple<R0, R1, Rs...>& ranges)
{
	return ranges.first.Empty() || AnyEmpty(ranges.next);
}



template<typename R0> bool AllEmpty(const Meta::Tuple<R0>& ranges)
{
	return ranges.first.Empty();
}

template<typename R0, typename R1, typename... Rs> bool AllEmpty(const Meta::Tuple<R0, R1, Rs...>& ranges)
{
	return ranges.first.Empty() && AllEmpty(ranges.next);
}


template<typename R0> size_t MinLength(const Meta::Tuple<R0>& ranges)
{
	return ranges.first.Length();
}

template<typename R0, typename R1, typename... Rs> size_t MinLength(const Meta::Tuple<R0, R1, Rs...>& ranges)
{
	return Math::Min(ranges.first.Length(), MinLength(ranges.next));
}


struct Fronter
{
	template<typename T> typename T::return_value_type operator()(const T& range) const {return range.First();}
};

struct Backer
{
	template<typename T> typename T::return_value_type operator()(const T& range) const {return range.Last();}
};

struct Indexer
{
	size_t index;
	template<typename T> typename T::return_value_type operator()(const T& range) const {return range[index];}
};

struct Slicer
{
	size_t start, end;
	template<typename T> decltype(Meta::Val<T>().opSlice(0, 1)) operator()(const T& range) const {return range.opSlice(start, end);}
};

struct PopFronter
{
	template<typename T> void operator()(T& range) const {return range.PopFirst();}
};

struct PopBacker
{
	template<typename T> void operator()(T& range) const {return range.PopLast();}
};

namespace detail {

template<typename R1, typename R2=null_t, typename R3=null_t, typename R4=null_t, typename R5=null_t> struct TupleOfElement;

template<typename R1> struct TupleOfElement<R1>
{typedef Meta::Tuple<typename R1::value_type> _;};

template<typename R1, typename R2> struct TupleOfElement<R1, R2>
{typedef Meta::Tuple<typename R1::value_type, typename R2::value_type> _;};

template<typename R1, typename R2, typename R3> struct TupleOfElement<R1, R2, R3>
{typedef Meta::Tuple<typename R1::value_type, typename R2::value_type, typename R3::value_type> _;};

template<typename R1, typename R2, typename R3, typename R4> struct TupleOfElement<R1, R2, R3, R4>
{typedef Meta::Tuple<typename R1::value_type, typename R2::value_type, typename R3::value_type, typename R4::value_type> _;};

template<typename R1, typename R2, typename R3, typename R4, typename R5> struct TupleOfElement
{typedef Meta::Tuple<typename R1::value_type, typename R2::value_type, typename R3::value_type, typename R4::value_type, typename R5::value_type> _;};



template<typename R1, typename R2=null_t, typename R3=null_t, typename R4=null_t, typename R5=null_t> struct TupleOfReturnElement;

template<typename R1> struct TupleOfReturnElement<R1>
{typedef Meta::Tuple<typename R1::return_value_type> _;};

template<typename R1, typename R2> struct TupleOfReturnElement<R1, R2>
{typedef Meta::Tuple<typename R1::return_value_type, typename R2::return_value_type> _;};

template<typename R1, typename R2, typename R3> struct TupleOfReturnElement<R1, R2, R3>
{typedef Meta::Tuple<typename R1::return_value_type, typename R2::return_value_type, typename R3::return_value_type> _;};

template<typename R1, typename R2, typename R3, typename R4> struct TupleOfReturnElement<R1, R2, R3, R4>
{typedef Meta::Tuple<typename R1::return_value_type, typename R2::return_value_type, typename R3::return_value_type, typename R4::return_value_type> _;};

template<typename R1, typename R2, typename R3, typename R4, typename R5> struct TupleOfReturnElement
{typedef Meta::Tuple<typename R1::return_value_type, typename R2::return_value_type, typename R3::return_value_type, typename R4::return_value_type, typename R5::return_value_type> _;};

}

template<typename... Args> using TupleOfElement = typename detail::TupleOfElement<Args...>::_;
template<typename... Args> using TupleOfReturnElement = typename detail::TupleOfReturnElement<Args...>::_;


template<typename T> struct NullRange
{
	typedef T value_type;
	typedef T return_value_type;
	enum {RangeType=TypeEnum::RandomAccess};
	enum: bool {RangeIsFinite=true};

	T* begin() const {return null;}
	T* end() const {return null;}

	NullRange(null_t=null) {}
	bool Empty() const {return true;}
	size_t Length() const {return 0;}
	size_t Count() const {return 0;}
	T First() const {INTRA_ASSERT(false); return T();}
	T Last() const {INTRA_ASSERT(false); return T();}
	void PopFirst() {INTRA_ASSERT(false);}
	void PopLast() {INTRA_ASSERT(false);}
	T operator[](size_t) {INTRA_ASSERT(false); return T();}
	
	NullRange opSlice(size_t startIndex, size_t endIndex) const
	{
		(void)startIndex; (void)endIndex;
		INTRA_ASSERT(startIndex==0 && endIndex==0);
		return NullRange();
	}

	void Put(const T&) {}
};



template<typename R> forceinline Meta::EnableIf<
	IsInputRange<R>::_ || IsOutputRange<R>::_,
R&> AsRange(R& r) {return r;}

template<typename R> forceinline Meta::EnableIf<
	IsInputRange<R>::_ || IsOutputRange<R>::_,
const R&> AsRange(const R& r) {return r;}

template<typename T> using AsRangeResult = decltype(AsRange(Meta::Val<T>()));

template<typename R> Meta::EnableIf<
	IsInputRange<R>::_,
R&> AsConstRange(R& r) {return r;}

template<typename R> Meta::EnableIf<
	IsInputRange<R>::_,
const R&> AsConstRange(const R& r) {return r;}

template<typename T> decltype(Meta::Val<T>().AsRange()) AsRange(T& v) {return v.AsRange();}
template<typename T> decltype(Meta::Val<T>().AsConstRange()) AsConstRange(T& v) {return v.AsConstRange();}

template<typename T> using AsConstRangeResult = decltype(AsConstRange(Meta::Val<T>()));

INTRA_DEFINE_EXPRESSION_CHECKER(HasAsRange, Meta::Val<T>().AsRange());
INTRA_DEFINE_EXPRESSION_CHECKER(HasAsConstRange, Meta::Val<T>().AsConstRange());


template<typename R> forceinline typename R::iterator begin(const R& r) {return r.begin();}
template<typename R> forceinline typename R::iterator end(const R& r) {return r.end();}

INTRA_DEFINE_EXPRESSION_CHECKER_WITH_CONDITION(IsRangeOfContainers, Meta::Val<typename T::value_type>(),
	(IsInputRange<T>::_ && IsInputRange<AsRangeResult<typename T::value_type>>::_));


}

using Range::AsRange;
using Range::AsConstRange;

}
