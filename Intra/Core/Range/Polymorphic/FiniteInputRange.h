#pragma once

#include "Core/Range/Operations.h"
#include "Core/Range/Mutation/Copy.h"

#include "Core/Range/Span.h"
#include "Funal/Op.h"

#include "Core/Range/Concepts.h"


#include "InputRange.h"

INTRA_BEGIN
namespace Range {
INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED
INTRA_WARNING_DISABLE_DEFAULT_CONSTRUCTOR_IMPLICITLY_DELETED
INTRA_WARNING_DISABLE_SIGN_CONVERSION

#if defined(_MSC_VER) && !defined(__clang__)
#define TEMPLATE //VS doesn't compile construction typename ...<...>::template ...<...>, но без template компилирует
#else
#define TEMPLATE template //clang required template keyword in typename ...<...>::template ...<...>
#endif

template<typename T> struct FiniteInputRange: InputRange<T>
{
	enum: bool {RangeIsFinite = true};

protected:
	typedef typename InputRange<T>::Interface Interface;
	template<typename R> using WrapperImpl = typename InputRange<T>::template WrapperImpl<R>;

private:
	template<typename R> using EnableCondition = Requires<
		CConvertible<TReturnValueTypeOfAs<R>, T> &&
		CAsFiniteInputRange<R> &&
		!CSameIgnoreCVRef<R, FiniteInputRange>
	>;

	template<typename R> forceinline static Interface* wrap(R&& range)
	{
		typedef TRemoveConstRef<TRangeOfType<R>> Range;
		return new WrapperImpl<Range>(AsRange(Forward<R>(range)));
	}

public:
	typedef TRemoveConstRef<T> value_type;
	
	constexpr forceinline FiniteInputRange(null_t=null) {}

	constexpr forceinline FiniteInputRange(FiniteInputRange&& rhs):
		InputRange<T>(Move(static_cast<InputRange<T>&&>(rhs))) {}

	FiniteInputRange(const FiniteInputRange& rhs) = delete;

	INTRA_CONSTEXPR2 forceinline FiniteInputRange& operator=(FiniteInputRange&& rhs)
	{
		InputRange<T>::operator=(Move(rhs));
		return *this;
	}

	FiniteInputRange& operator=(const FiniteInputRange& rhs) = delete;

	template<typename R, typename = EnableCondition<R>>
	forceinline FiniteInputRange(R&& range):
		InputRange<T>(wrap(ForwardAsRange<R>(range))) {}

	template<typename R, typename = EnableCondition<R>>
	forceinline FiniteInputRange& operator=(R&& range)
	{
		InputRange<T>::mInterface = wrap(ForwardAsRange<R>(range));
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
	constexpr forceinline FiniteInputRange(Interface* interfacePtr): InputRange<T>(interfacePtr) {}
};

typedef FiniteInputRange<char> FiniteInputStream;

#undef TEMPLATE

}
INTRA_END
