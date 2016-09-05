#pragma once

#include "Meta/Mixins.h"
#include "Algorithms/Mixins/InputRangeMixin.h"
#include "Algorithms/Mixins/ForwardRangeMixin.h"
#include "Algorithms/Mixins/BidirectionalRangeMixin.h"
#include "Algorithms/Mixins/RandomAccessRangeMixin.h"
#include "Algorithms/Mixins/ArrayRangeMixin.h"
#include "Algorithms/Mixins/CharRangeMixin.h"

namespace Intra { namespace Range {


template<typename R, typename T, TypeEnum::Type TYPE, bool FINITE> struct RangeMixin;

template<typename R, typename T> struct RangeMixin<R, T, TypeEnum::Input, false>:
	InputRangeMixin<R, T, Meta::SelectType<CharRangeMixin<R, T>, Meta::EmptyType, Meta::IsCharType<T>::_>>
{
	typedef T value_type;
	typedef T return_value_type;
	enum {RangeType = TypeEnum::Input};
	enum: bool {RangeIsFinite = false};
};

template<typename R, typename T> struct RangeMixin<R, T, TypeEnum::Input, true>:
	FiniteInputRangeMixin<R, T,
	RangeMixin<R, T, TypeEnum::Input, false>>
{
	enum: bool {RangeIsFinite = true};

private:
	CycleResult<R> Cycle() const;
};

template<typename R, typename T> struct RangeMixin<R, T, TypeEnum::Forward, false>:
	ForwardRangeMixin<R, T,
	RangeMixin<R, T, TypeEnum::Input, false>>
{
private:
	typedef ForwardRangeMixin<R, T, RangeMixin<R, T, TypeEnum::Input, false>> BaseForward;
public:
	enum {RangeType = TypeEnum::Forward};

	using BaseForward::begin;
	using BaseForward::end;

};

template<typename R, typename T> struct RangeMixin<R, T, TypeEnum::Forward, true>:
	FiniteForwardRangeMixin<R, T,
	FiniteInputRangeMixin<R, T,
	RangeMixin<R, T, TypeEnum::Forward, false>>>
{
private:
	typedef FiniteInputRangeMixin<R, T, RangeMixin<R, T, TypeEnum::Forward, false>> BaseInput;
	typedef FiniteForwardRangeMixin<R, T, BaseInput> BaseForward;
public:
	enum: bool {RangeIsFinite = true};

	using BaseForward::Cycle;
	using BaseInput::FindAdvance;
	using BaseForward::FindAdvance;
	using BaseInput::CountUntilAdvance;
	using BaseForward::CountUntilAdvance;
	using BaseInput::CountUntilAdvanceAny;
	using BaseForward::CountUntilAdvanceAny;
	using BaseInput::Count;
	using BaseForward::Count;
	using BaseInput::CountAdvance;
	using BaseForward::CountAdvance;
	using BaseInput::FindAdvanceAny;
	using BaseForward::FindAdvanceAny;
	using BaseInput::FindAdvanceAnyAdvance;
	using BaseForward::FindAdvanceAnyAdvance;
	using BaseInput::CountUntilAdvanceAnyAdvance;
	using BaseForward::CountUntilAdvanceAnyAdvance;
	using BaseInput::TailAdvance;
	using BaseForward::TailAdvance;
};

template<typename R, typename T> struct RangeMixin<R, T, TypeEnum::Bidirectional, true>:
	BidirectionalRangeMixin<R, T,
	RangeMixin<R, T, TypeEnum::Forward, true>>
{
	enum {RangeType = TypeEnum::Bidirectional};
};

template<typename R, typename T> struct RangeMixin<R, T, TypeEnum::RandomAccess, false>:
	RandomAccessRangeMixin<R, T,
	RangeMixin<R, T, TypeEnum::Forward, false>>
{
private:
	typedef RandomAccessRangeMixin<R, T, RangeMixin<R, T, TypeEnum::Forward, false>> BaseRandomAccess;
public:
	enum {RangeType = TypeEnum::RandomAccess};
	using BaseRandomAccess::Cycle;

};

template<typename R, typename T> struct RangeMixin<R, T, TypeEnum::RandomAccess, true>:
	FiniteRandomAccessRangeMixin<R, T,
	RandomAccessRangeMixin<R, T,
	RangeMixin<R, T, TypeEnum::Bidirectional, true>>>
{
private:
	typedef FiniteRandomAccessRangeMixin<R, T, RandomAccessRangeMixin<R, T, RangeMixin<R, T, TypeEnum::Bidirectional, true>>> BaseRandomAccess;
public:
	enum {RangeType = TypeEnum::RandomAccess};
	enum: bool {RangeIsFinite=true};

	using BaseRandomAccess::Take;
	using BaseRandomAccess::Tail;
	using BaseRandomAccess::TailAdvance;
	using BaseRandomAccess::PopFirstN;
	using BaseRandomAccess::PopFirstExactly;
	using BaseRandomAccess::PopLastN;
	using BaseRandomAccess::PopLastExactly;
	using BaseRandomAccess::Cycle;
	using BaseRandomAccess::operator();

};

template<typename R, typename T> struct RangeMixin<R, T, TypeEnum::Array, true>:
	ArrayRangeMixin<R, T, RangeMixin<R, T, TypeEnum::RandomAccess, true>>
{
private:
	typedef RangeMixin<R, T, TypeEnum::RandomAccess, true> BaseRandomAccess;
	typedef ArrayRangeMixin<R, T, RangeMixin<R, T, TypeEnum::RandomAccess, true>> BaseArray;
public:

	enum {RangeType = TypeEnum::Array};

	using BaseRandomAccess::ReadUntilAdvance;


	template<typename OR> forceinline Meta::EnableIf<
		!IsArrayRangeOfExactly<OR, T>::_ && IsOutputRange<OR>::_
	> CopyAdvanceToAdvance(OR& dst)
	{
		BaseRandomAccess::CopyAdvanceToAdvance(dst);
	}

	template<typename OR> forceinline Meta::EnableIf<
		!IsArrayRangeOfExactly<OR, T>::_ && IsOutputRange<OR>::_
	> CopyAdvanceTo(OR& dst)
	{
		BaseRandomAccess::CopyAdvanceTo(dst);
	}

	template<typename OR> forceinline Meta::EnableIf<
		!IsArrayRangeOfExactly<OR, T>::_ && IsOutputRange<OR>::_
	> CopyTo(const OR& dst) const
	{
		BaseRandomAccess::CopyTo(dst);
	}

	template<typename OR> forceinline Meta::EnableIf<
		!IsArrayRangeOfExactly<OR, T>::_ && IsOutputRange<OR>::_
	> CopyToAdvance(OR& dst) const
	{
		BaseRandomAccess::CopyToAdvance(dst);
	}



	template<typename OR, typename P> forceinline Meta::EnableIf<
		IsOutputRange<OR>::_ && Meta::IsCallable<P, T>::_
	> CopyAdvanceToAdvance(OR& dst, P pred)
	{
		BaseRandomAccess::CopyAdvanceToAdvance(dst, pred);
	}

	template<typename OR, typename P> forceinline Meta::EnableIf<
		IsOutputRange<OR>::_ && Meta::IsCallable<P, T>::_
	> CopyAdvanceTo(const OR& dst, P pred)
	{
		BaseRandomAccess::CopyAdvanceTo(dst, pred);
	}

	template<typename OR, typename P> forceinline Meta::EnableIf<
		IsOutputRange<OR>::_ && Meta::IsCallable<P, T>::_
	> CopyToAdvance(OR& dst, P pred) const
	{
		BaseRandomAccess::CopyToAdvance(dst, pred);
	}

	template<typename OR, typename P> forceinline Meta::EnableIf<
		IsOutputRange<OR>::_ && Meta::IsCallable<P, T>::_
	> CopyTo(const OR& dst, P pred) const
	{
		BaseRandomAccess::CopyTo(dst, pred);
	}



	template<typename DstRange> forceinline Meta::EnableIf<
		IsArrayRangeOfExactly<DstRange, T>::_
	> CopyAdvanceTo(const DstRange& dst)
	{
		BaseArray::CopyAdvanceTo(dst);
	}

	template<typename DstRange> forceinline Meta::EnableIf<
		IsArrayRangeOfExactly<DstRange, T>::_
	> CopyAdvanceToAdvance(DstRange& dst)
	{
		BaseArray::CopyAdvanceToAdvance(dst);
	}

	template<typename DstRange> forceinline Meta::EnableIf<
		IsArrayRangeOfExactly<DstRange, T>::_
	> CopyTo(const DstRange& dst) const
	{
		BaseArray::CopyTo(dst);
	}

	template<typename DstRange> forceinline Meta::EnableIf<
		IsArrayRangeOfExactly<DstRange, T>::_
	> CopyToAdvance(DstRange& dst) const
	{
		BaseArray::CopyToAdvance(dst);
	}




	template<typename RW> forceinline Meta::EnableIf<
		IsArrayRangeOfExactly<RW, T>::_,
	bool> StartsWith(const RW& what) const
	{
		return BaseArray::StartsWith(what);
	}

	template<typename RW> forceinline Meta::EnableIf<
		IsFiniteForwardRangeOf<RW, T>::_ && !IsArrayRangeOfExactly<RW, T>::_,
	bool> StartsWith(const RW& what) const
	{
		return BaseRandomAccess::StartsWith(what);
	}



	template<typename RW> forceinline Meta::EnableIf<
		IsArrayRangeOfExactly<RW, T>::_,
	bool> EndsWith(const RW& what) const
	{
		return BaseArray::EndsWith(what);
	}

	template<typename RW> forceinline Meta::EnableIf<
		IsBidirectionalRangeOf<RW, T>::_ && !IsArrayRangeOfExactly<RW, T>::_,
	bool> EndsWith(const RW& what) const
	{
		return BaseRandomAccess::EndsWith(what);
	}

};

template<typename... RANGES> struct CommonRangeCategoryAnyFinite;
template<typename R0> struct CommonRangeCategoryAnyFinite<R0>
{
	enum: byte {Type=R0::RangeType};
	enum: bool {Finite = R0::RangeIsFinite};
};

template<typename R0, typename R1, typename... RANGES> struct CommonRangeCategoryAnyFinite<R0, R1, RANGES...>
{
	typedef CommonRangeCategoryAnyFinite<R1, RANGES...> Rest;
	enum: byte {Type = Rest::Type? R0::RangeType: Rest::Type};
	enum: byte {Finite = Rest::Finite || R0::RangeIsFinite};
};



template<typename... RANGES> struct CommonRangeCategoryAllFinite;
template<typename R0> struct CommonRangeCategoryAllFinite<R0>
{
	enum: byte {Type = R0::RangeType==TypeEnum::Array? TypeEnum::RandomAccess: R0::RangeType};
	enum: bool {Finite = R0::RangeIsFinite};
};

template<typename R0, typename R1, typename... RANGES> struct CommonRangeCategoryAllFinite<R0, R1, RANGES...>
{
	typedef CommonRangeCategoryAllFinite<R1, RANGES...> Rest;
	enum: byte {Type = Rest::Type? R0::RangeType: Rest::Type};
	enum: byte {Finite = Rest::Finite && R0::RangeIsFinite};
};


template<typename R, typename... RANGES> struct CommonAllFiniteRangeMixin:
	RangeMixin<R,
	Meta::CommonType<typename RANGES::value_type...>,
	CommonRangeCategoryAllFinite<RANGES...>::Type,
	CommonRangeCategoryAllFinite<RANGES...>::Finite> {};

template<typename R, typename... RANGES> struct CommonAnyFiniteRangeMixin:
	RangeMixin<R,
	Meta::CommonType<typename RANGES::value_type...>,
	CommonRangeCategoryAnyFinite<RANGES...>::Type,
	CommonRangeCategoryAnyFinite<RANGES...>::Finite> {};


template<typename R, typename T, typename... RANGES> struct CommonTypedAllFiniteRangeMixin:
	RangeMixin<R,
	T,
	CommonRangeCategoryAllFinite<RANGES...>::Type,
	CommonRangeCategoryAllFinite<RANGES...>::Finite> {};

template<typename R, typename T, typename... RANGES> struct CommonTypedAnyFiniteRangeMixin:
	RangeMixin<R,
	T,
	CommonRangeCategoryAnyFinite<RANGES...>::Type,
	CommonRangeCategoryAnyFinite<RANGES...>::Finite> {};



}



}

