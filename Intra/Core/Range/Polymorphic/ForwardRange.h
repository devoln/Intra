#pragma once

#include "Core/Range/Operations.h"
#include "Funal/Op.h"
#include "Core/Range/Mutation/Copy.h"
#include "Core/Range/Concepts.h"
#include "Core/Range/Span.h"

#include "Utils/Unique.h"

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
			FullImplFiller<R, Interface>(Forward<A>(range)) {}

		Interface* Clone() const override {return new WrapperImpl(FullImplFiller<R, Interface>::OriginalRange);}
	};

	Interface* clone() const {return static_cast<Interface*>(InputRange<T>::mInterface.Ptr())->Clone();}

private:
	template<typename R> using EnableCondition = Requires<
		CConvertible<TReturnValueTypeOfAs<R>, T> &&
		CAsForwardRange<R> &&
		!CSameIgnoreCVRef<R, ForwardRange>
	>;

	template<typename R> forceinline static Interface* wrap(R&& range)
	{return new WrapperImpl<TRemoveConstRef<TRangeOfType<R>>>(ForwardAsRange<R>(range));}

public:
	typedef TRemoveConstRef<T> value_type;

	constexpr forceinline ForwardRange(null_t=null) {}

	constexpr forceinline ForwardRange(ForwardRange&& rhs):
		InputRange<T>(Move(static_cast<InputRange<T>&&>(rhs))) {}

	INTRA_CONSTEXPR2 forceinline ForwardRange& operator=(ForwardRange&& rhs)
	{
		InputRange<T>::operator=(Move(static_cast<InputRange<T>&&>(rhs)));
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
		InputRange<T>(wrap(ForwardAsRange<R>(range))) {}
	
	template<typename R, typename = EnableCondition<R>>
	forceinline ForwardRange& operator=(R&& range)
	{
		InputRange<T>::mInterface = wrap(ForwardAsRange<R>(range));
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
	constexpr forceinline ForwardRange(Interface* interfacePtr): InputRange<T>(interfacePtr) {}
};

typedef ForwardRange<char> ForwardStream;

#undef TEMPLATE
}
INTRA_END
