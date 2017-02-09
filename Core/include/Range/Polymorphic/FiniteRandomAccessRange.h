﻿#pragma once

#include "Memory/SmartRef.hh"
#include "Range/Operations.h"
#include "Algo/Op.h"
#include "Algo/Mutation/Copy.h"
#include "Range/Concepts.h"
#include "Range/Generators/ArrayRange.h"
#include "Range/AsRange.h"

#include "InputRange.h"
#include "BidirectionalRange.h"

namespace Intra { namespace Range {

INTRA_WARNING_PUSH
INTRA_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED
INTRA_WARNING_DISABLE_DEFAULT_CONSTRUCTOR_IMPLICITLY_DELETED
INTRA_WARNING_DISABLE_SIGN_CONVERSION

#if defined(_MSC_VER) && !defined(__clang__)
#define TEMPLATE //VS не хочет компилировать конструкцию typename ...<...>::template ...<...>, но без template компилирует
#else
#define TEMPLATE template //clang требует слово template в typename ...<...>::template ...<...>
#endif

template<typename T> struct FiniteRandomAccessRange: BidirectionalRange<T>
{
protected:
	typedef typename InputRange<T>::value_type value_type;
	struct Interface: BidirectionalRange<T>::Interface
	{
		virtual T OpIndex(size_t index) const = 0;
		virtual Interface* OpSlice(size_t start, size_t end) const = 0;
	};

	template<typename R, typename PARENT> using FullImplFiller = 
		typename BidirectionalRange<T>::TEMPLATE FullImplFiller<R, PARENT>;

	template<typename R> struct WrapperImpl: FullImplFiller<R, Interface>
	{
		template<typename A> WrapperImpl(A&& range): FullImplFiller<R, Interface>(Meta::Forward<A>(range)) {}

		WrapperImpl* Clone() const override {return new WrapperImpl(FullImplFiller<R, Interface>::OriginalRange);}
		T OpIndex(size_t index) const override {return FullImplFiller<R, Interface>::OriginalRange[index];}
		
		Interface* OpSlice(size_t start, size_t end) const override
		{return new WrapperImpl(FullImplFiller<R, Interface>::OriginalRange(start, end));}

		size_t Count() const override {return Range::Count(FullImplFiller<R, Interface>::OriginalRange);}
	};

private:
	template<typename R> using EnableCondition = Meta::EnableIf<
		Meta::IsConvertible<ReturnValueTypeOfAs<R>, T>::_ &&
		IsAsFiniteRandomAccessRange<R>::_ &&
		!Meta::TypeEqualsIgnoreCVRef<R, FiniteRandomAccessRange>::_
	>;

	template<typename R> forceinline static Interface* wrap(R&& range)
	{return new WrapperImpl<Meta::RemoveConstRef<AsRangeResult<R>>>(Range::Forward<R>(range));}

public:
	forceinline FiniteRandomAccessRange(null_t=null) {}

	forceinline FiniteRandomAccessRange(FiniteRandomAccessRange&& rhs):
		BidirectionalRange<T>(Meta::Move(static_cast<BidirectionalRange<T>&&>(rhs))) {}

	forceinline FiniteRandomAccessRange& operator=(FiniteRandomAccessRange&& rhs)
	{
		BidirectionalRange<T>::operator=(Meta::Move(static_cast<BidirectionalRange<T>&&>(rhs)));
		return *this;
	}

	forceinline FiniteRandomAccessRange(const FiniteRandomAccessRange& rhs):
		BidirectionalRange<T>(rhs.clone()) {}

	forceinline FiniteRandomAccessRange& operator=(const FiniteRandomAccessRange& rhs)
	{
		InputRange<T>::mInterface = rhs.clone();
		return *this;
	}

	template<typename R, typename = EnableCondition<R>>
	forceinline FiniteRandomAccessRange(R&& range):
		BidirectionalRange<T>(wrap(Range::Forward<R>(range))) {}

	template<typename R, typename = EnableCondition<R>>
	forceinline FiniteRandomAccessRange& operator=(R&& range)
	{
		InputRange<T>::mInterface = wrap(Range::Forward<R>(range));
		return *this;
	}

	forceinline FiniteRandomAccessRange(InitializerList<Meta::RemoveConst<value_type>> arr):
		FiniteRandomAccessRange(AsRange(arr)) {}

	forceinline FiniteRandomAccessRange& operator=(InitializerList<Meta::RemoveConst<value_type>> arr)
	{
		operator=(AsRange(arr));
		return *this;
	}

	forceinline T operator[](size_t index) const
	{return static_cast<Interface*>(InputRange<T>::mInterface.Ptr())->OpIndex(index);}

	forceinline FiniteRandomAccessRange operator()(size_t start, size_t end) const
	{return FiniteRandomAccessRange(static_cast<Interface*>(InputRange<T>::mInterface.Ptr())->OpSlice(start, end));}

protected:
	FiniteRandomAccessRange(Interface* interfacePtr): BidirectionalRange<T>(interfacePtr) {}
};

#undef TEMPLATE

INTRA_WARNING_POP

}

using Range::FiniteRandomAccessRange;
}
