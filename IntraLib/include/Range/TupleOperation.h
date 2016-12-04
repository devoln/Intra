#pragma once

namespace Intra { namespace Range {

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
{typedef Meta::Tuple<typename R1::value_type, typename R2::value_type,
	typename R3::value_type, typename R4::value_type> _;};

template<typename R1, typename R2, typename R3, typename R4, typename R5> struct TupleOfElement
{typedef Meta::Tuple<typename R1::value_type, typename R2::value_type,
	typename R3::value_type, typename R4::value_type, typename R5::value_type> _;};



template<typename R1, typename R2=null_t, typename R3=null_t, typename R4=null_t, typename R5=null_t> struct TupleOfReturnElement;

template<typename R1> struct TupleOfReturnElement<R1>
{typedef Meta::Tuple<typename R1::return_value_type> _;};

template<typename R1, typename R2> struct TupleOfReturnElement<R1, R2>
{typedef Meta::Tuple<typename R1::return_value_type, typename R2::return_value_type> _;};

template<typename R1, typename R2, typename R3> struct TupleOfReturnElement<R1, R2, R3>
{typedef Meta::Tuple<typename R1::return_value_type, typename R2::return_value_type, typename R3::return_value_type> _;};

template<typename R1, typename R2, typename R3, typename R4> struct TupleOfReturnElement<R1, R2, R3, R4>
{typedef Meta::Tuple<typename R1::return_value_type, typename R2::return_value_type,
	typename R3::return_value_type, typename R4::return_value_type> _;};

template<typename R1, typename R2, typename R3, typename R4, typename R5> struct TupleOfReturnElement
{typedef Meta::Tuple<typename R1::return_value_type, typename R2::return_value_type,
	typename R3::return_value_type, typename R4::return_value_type, typename R5::return_value_type> _;};

}

template<typename... Args> using TupleOfElement = typename detail::TupleOfElement<Args...>::_;
template<typename... Args> using TupleOfReturnElement = typename detail::TupleOfReturnElement<Args...>::_;

}}
