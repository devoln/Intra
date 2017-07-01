#pragma once

#include "Range/Operations.h"
#include "Funal/Op.h"
#include "Range/Mutation/Copy.h"
#include "Concepts/Range.h"
#include "Utils/Span.h"
#include "Concepts/RangeOf.h"
#include "Utils/Unique.h"

#include "InputRange.h"

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

template<typename T> struct ForwardRange: InputRange<T>
{
protected:
	struct Interface: InputRange<T>::Interface
	{
		virtual Interface* Clone() const = 0;
	};

	template<typename R, typename PARENT> using FullImplFiller =
		typename InputRange<T>::TEMPLATE ImplFiller<R, PARENT>;

	template<typename R> struct WrapperImpl: FullImplFiller<R, Interface>
	{
		template<typename A> WrapperImpl(A&& range):
			FullImplFiller<R, Interface>(Cpp::Forward<A>(range)) {}

		Interface* Clone() const override {return new WrapperImpl(FullImplFiller<R, Interface>::OriginalRange);}
	};

	Interface* clone() const {return static_cast<Interface*>(InputRange<T>::mInterface.Ptr())->Clone();}

private:
	template<typename R> using EnableCondition = Meta::EnableIf<
		Meta::IsConvertible<Concepts::ReturnValueTypeOfAs<R>, T>::_ &&
		Concepts::IsAsForwardRange<R>::_ &&
		!Meta::TypeEqualsIgnoreCVRef<R, ForwardRange>::_
	>;

	template<typename R> forceinline static Interface* wrap(R&& range)
	{
		typedef Meta::RemoveConstRef<Concepts::RangeOfType<R>> Range;
		return new WrapperImpl<Range>(AsRange(Cpp::Forward<R>(range)));
	}

public:
	typedef Meta::RemoveConstRef<T> value_type;

	forceinline ForwardRange(null_t=null) {}

	forceinline ForwardRange(ForwardRange&& rhs):
		InputRange<T>(Cpp::Move(static_cast<InputRange<T>&&>(rhs))) {}

	forceinline ForwardRange& operator=(ForwardRange&& rhs)
	{
		InputRange<T>::operator=(Cpp::Move(static_cast<InputRange<T>&&>(rhs)));
		return *this;
	}

	forceinline ForwardRange(const ForwardRange& rhs):
		InputRange<T>(rhs.clone()) {}

	forceinline ForwardRange& operator=(const ForwardRange& rhs)
	{
		InputRange<T>::mInterface = rhs.clone();
		return *this;
	}

	template<typename R, typename = EnableCondition<R>>
	forceinline ForwardRange(R&& range):
		InputRange<T>(wrap(Range::Forward<R>(range))) {}
	
	template<typename R, typename = EnableCondition<R>>
	forceinline ForwardRange& operator=(R&& range)
	{
		InputRange<T>::mInterface = wrap(Range::Forward<R>(range));
		return *this;
	}

	forceinline ForwardRange(InitializerList<value_type> arr):
		ForwardRange(AsRange(arr)) {}

	forceinline ForwardRange& operator=(InitializerList<value_type> arr)
	{
		operator=(AsRange(arr));
		return *this;
	}

	forceinline operator Interface&() {return *static_cast<Interface*>(InputRange<T>::mInterface.Ptr());}

protected:
	ForwardRange(Interface* interfacePtr): InputRange<T>(interfacePtr) {}
};

typedef ForwardRange<char> ForwardStream;

#undef TEMPLATE

INTRA_WARNING_POP

}

using Range::ForwardRange;
using Range::ForwardStream;

}
