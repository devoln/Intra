#pragma once

#include "Cpp/Warnings.h"
#include "Range/Operations.h"
#include "Utils/Op.h"
#include "Range/Mutation/Copy.h"
#include "Concepts/Range.h"
#include "Utils/Span.h"
#include "Concepts/RangeOf.h"

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

template<typename T> struct FiniteInputRange: InputRange<T>
{
	enum: bool {RangeIsFinite = true};

protected:
	typedef typename InputRange<T>::Interface Interface;
	template<typename R> using WrapperImpl = typename InputRange<T>::template WrapperImpl<R>;

private:
	template<typename R> using EnableCondition = Meta::EnableIf<
		Meta::IsConvertible<ReturnValueTypeOfAs<R>, T>::_ &&
		IsAsFiniteInputRange<R>::_ &&
		!Meta::TypeEqualsIgnoreCVRef<R, FiniteInputRange>::_
	>;

	template<typename R> forceinline static Interface* wrap(R&& range)
	{
		typedef Meta::RemoveConstRef<RangeOfType<R>> Range;
		return new WrapperImpl<Range>(AsRange(Cpp::Forward<R>(range)));
	}

public:
	typedef Meta::RemoveConstRef<T> value_type;
	
	forceinline FiniteInputRange(null_t=null) {}

	forceinline FiniteInputRange(FiniteInputRange&& rhs):
		InputRange<T>(Cpp::Move(static_cast<InputRange<T>&&>(rhs))) {}

	FiniteInputRange(const FiniteInputRange& rhs) = delete;

	forceinline FiniteInputRange& operator=(FiniteInputRange&& rhs)
	{
		InputRange<T>::operator=(Cpp::Move(rhs));
		return *this;
	}

	FiniteInputRange& operator=(const FiniteInputRange& rhs) = delete;

	template<typename R, typename = EnableCondition<R>>
	forceinline FiniteInputRange(R&& range):
		InputRange<T>(wrap(Range::Forward<R>(range))) {}

	template<typename R, typename = EnableCondition<R>>
	forceinline FiniteInputRange& operator=(R&& range)
	{
		InputRange<T>::mInterface = wrap(Range::Forward<R>(range));
		return *this;
	}

	forceinline FiniteInputRange(InitializerList<value_type> arr):
		FiniteInputRange(AsRange(arr)) {}

	forceinline FiniteInputRange& operator=(InitializerList<value_type> arr)
	{
		operator=(AsRange(arr));
		return *this;
	}

protected:
	FiniteInputRange(Interface* interfacePtr): InputRange<T>(interfacePtr) {}
};

typedef FiniteInputRange<char> FiniteInputStream;

#undef TEMPLATE

INTRA_WARNING_POP

}

using Range::FiniteInputRange;
using Range::FiniteInputStream;

}
