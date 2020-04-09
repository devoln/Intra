#pragma once

#include "Intra/Range/Operations.h"
#include "Intra/Range/Mutation/Copy.h"

#include "Intra/Range/Span.h"
#include "Intra/Functional.h"

#include "Intra/Range/Concepts.h"


#include "InputRange.h"

INTRA_BEGIN
INTRA_IGNORE_WARNING_COPY_IMPLICITLY_DELETED
INTRA_IGNORE_WARNING_DEFAULT_CONSTRUCTOR_IMPLICITLY_DELETED
INTRA_IGNORE_WARNING_SIGN_CONVERSION

template<typename T> struct FiniteInputRange: InputRange<T>
{
	static constexpr bool IsAnyInstanceFinite = true;
protected:
	using Interface = typename InputRange<T>::Interface;
	template<typename R> using WrapperImpl = typename InputRange<T>::template WrapperImpl<R>;

private:
	template<typename R> using EnableCondition = Requires<
		CConvertibleTo<TReturnValueTypeOfAs<R>, T> &&
		CAsFiniteInputRange<R> &&
		!CSameIgnoreCVRef<R, FiniteInputRange>
	>;

	template<typename R> static Interface* wrap(R&& range)
	{
		typedef TRemoveConstRef<TRangeOfRef<R>> Range;
		return new WrapperImpl<Range>(AsRange(Forward<R>(range)));
	}

public:
	using value_type = TRemoveConstRef<T>;
	
	constexpr FiniteInputRange(decltype(null)=null) {}

	constexpr FiniteInputRange(FiniteInputRange&& rhs):
		InputRange<T>(Move(static_cast<InputRange<T>&&>(rhs))) {}

	FiniteInputRange(const FiniteInputRange& rhs) = delete;

	constexpr FiniteInputRange& operator=(FiniteInputRange&& rhs)
	{
		InputRange<T>::operator=(Move(rhs));
		return *this;
	}

	FiniteInputRange& operator=(const FiniteInputRange& rhs) = delete;

	template<typename R, typename = EnableCondition<R>>
	FiniteInputRange(R&& range):
		InputRange<T>(wrap(ForwardAsRange<R>(range))) {}

	template<typename R, typename = EnableCondition<R>>
	FiniteInputRange& operator=(R&& range)
	{
		InputRange<T>::mInterface = wrap(ForwardAsRange<R>(range));
		return *this;
	}

	FiniteInputRange(InitializerList<value_type> arr):
		FiniteInputRange(AsRange(arr)) {}

	FiniteInputRange& operator=(InitializerList<value_type> arr)
	{
		operator=(AsRange(arr));
		return *this;
	}

protected:
	constexpr FiniteInputRange(Interface* interfacePtr): InputRange<T>(interfacePtr) {}
};

using FiniteInputStream = FiniteInputRange<char>;
INTRA_END
