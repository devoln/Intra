#pragma once



#include "Core/Range/Operations.h"
#include "Core/Range/Mutation/Copy.h"

#include "Core/Range/Span.h"
#include "Core/Functional.h"

#include "Core/Range/Concepts.h"


#include "InputRange.h"
#include "ForwardRange.h"

INTRA_BEGIN
INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED
INTRA_WARNING_DISABLE_DEFAULT_CONSTRUCTOR_IMPLICITLY_DELETED
INTRA_WARNING_DISABLE_SIGN_CONVERSION

#if defined(_MSC_VER) && !defined(__clang__)
#define TEMPLATE //VS doesn't compile construction typename ...<...>::template ...<...>, но без template компилирует
#else
#define TEMPLATE template //clang required template keyword in typename ...<...>::template ...<...>
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
			FullImplFiller<R, Interface>(Forward<A>(range)) {}

		INTRA_NODISCARD size_t Count() const override {return FullImplFiller<R, Interface>::OriginalRange.Count();}
		Interface* Clone() const override {return new WrapperImpl(FullImplFiller<R, Interface>::OriginalRange);}
	};

private:
	template<typename R> using EnableCondition = Requires<
		CConvertible<TReturnValueTypeOfAs<R>, T> &&
		CAsFiniteForwardRange<R> &&
		!CSameIgnoreCVRef<R, FiniteForwardRange>
	>;

	template<typename R> forceinline static Interface* wrap(R&& range)
	{return new WrapperImpl<TRemoveConstRef<TRangeOfType<R>>>(ForwardAsRange<R>(range));}

public:
	typedef TRemoveConstRef<T> value_type;

	forceinline FiniteForwardRange(null_t=null) {}

	forceinline FiniteForwardRange(FiniteForwardRange&& rhs):
		ForwardRange<T>(Move(static_cast<ForwardRange<T>&&>(rhs))) {}

	forceinline FiniteForwardRange& operator=(FiniteForwardRange&& rhs)
	{
		ForwardRange<T>::operator=(Move(static_cast<ForwardRange<T>&&>(rhs)));
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
		ForwardRange<T>(wrap(ForwardAsRange<R>(range))) {}

	template<typename R, typename = EnableCondition<R>>
	forceinline FiniteForwardRange& operator=(R&& range)
	{
		InputRange<T>::mInterface = wrap(ForwardAsRange<R>(range));
		return *this;
	}

	forceinline FiniteForwardRange(InitializerList<value_type> arr):
		FiniteForwardRange(AsRange(arr)) {}

	forceinline FiniteForwardRange& operator=(InitializerList<value_type> arr)
	{
		operator=(AsRange(arr));
		return *this;
	}

	INTRA_NODISCARD forceinline index_t Length() const
	{return static_cast<Interface*>(InputRange<T>::mInterface.Ptr())->Count();}

protected:
	FiniteForwardRange(typename ForwardRange<T>::Interface* interfacePtr): ForwardRange<T>(interfacePtr) {}
};

typedef FiniteForwardRange<char> FiniteForwardStream;

#undef TEMPLATE

}
INTRA_END
