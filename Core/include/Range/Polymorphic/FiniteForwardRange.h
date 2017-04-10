#pragma once

#include "Memory/SmartRef.hh"
#include "Range/Operations.h"
#include "Algo/Op.h"
#include "Algo/Mutation/Copy.h"
#include "Range/Concepts.h"
#include "Range/Generators/Span.h"
#include "Range/AsRange.h"

#include "InputRange.h"
#include "ForwardRange.h"

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

template<typename T> struct FiniteForwardRange: ForwardRange<T>
{
	enum: bool {RangeIsFinite = true};
protected:
	struct Interface: ForwardRange<T>::Interface
	{
		virtual size_t Count() const = 0;
	};

	template<typename R, typename PARENT> using FullImplFiller =
		typename InputRange<T>::TEMPLATE ImplFiller<R, PARENT>;

	template<typename R> struct WrapperImpl:
		FullImplFiller<R, Interface>
	{
		template<typename A> WrapperImpl(A&& range):
			FullImplFiller<R, Interface>(Meta::Forward<A>(range)) {}

		size_t Count() const override {return FullImplFiller<R, Interface>::OriginalRange.Count();}
		Interface* Clone() const override {return new WrapperImpl(FullImplFiller<R, Interface>::OriginalRange);}
	};

private:
	template<typename R> using EnableCondition = Meta::EnableIf<
		Meta::IsConvertible<ReturnValueTypeOfAs<R>, T>::_ &&
		IsAsFiniteForwardRange<R>::_ &&
		!Meta::TypeEqualsIgnoreCVRef<R, FiniteForwardRange>::_
	>;

	template<typename R> forceinline static Interface* wrap(R&& range)
	{return new WrapperImpl<Meta::RemoveConstRef<AsRangeResult<R>>>(Range::Forward<R>(range));}

public:
	typedef Meta::RemoveConstRef<T> value_type;

	forceinline FiniteForwardRange(null_t=null) {}

	forceinline FiniteForwardRange(FiniteForwardRange&& rhs):
		ForwardRange<T>(Meta::Move(static_cast<ForwardRange<T>&&>(rhs))) {}

	forceinline FiniteForwardRange& operator=(FiniteForwardRange&& rhs)
	{
		ForwardRange<T>::operator=(Meta::Move(static_cast<ForwardRange<T>&&>(rhs)));
		return *this;
	}

	forceinline FiniteForwardRange(const FiniteForwardRange& rhs):
		ForwardRange<T>(rhs.clone()) {}

	forceinline FiniteForwardRange& operator=(const FiniteForwardRange& rhs)
	{
		InputRange<T>::mInterface = rhs.clone();
		return *this;
	}

	template<typename R, typename = EnableCondition<R>>
	forceinline FiniteForwardRange(R&& range):
		ForwardRange<T>(wrap(Range::Forward<R>(range))) {}

	template<typename R, typename = EnableCondition<R>>
	forceinline FiniteForwardRange& operator=(R&& range)
	{
		InputRange<T>::mInterface = wrap(Range::Forward<R>(range));
		return *this;
	}

	forceinline FiniteForwardRange(InitializerList<value_type> arr):
		FiniteForwardRange(AsRange(arr)) {}

	forceinline FiniteForwardRange& operator=(InitializerList<value_type> arr)
	{
		operator=(AsRange(arr));
		return *this;
	}

	forceinline size_t Length() const
	{return static_cast<Interface*>(InputRange<T>::mInterface.Ptr())->Count();}

protected:
	FiniteForwardRange(typename ForwardRange<T>::Interface* interfacePtr): ForwardRange<T>(interfacePtr) {}
};

typedef FiniteForwardRange<char> FiniteForwardStream;

#undef TEMPLATE

INTRA_WARNING_POP

}

using Range::FiniteForwardRange;
using Range::FiniteForwardStream;

}
