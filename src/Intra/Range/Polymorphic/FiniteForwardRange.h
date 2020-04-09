#pragma once

#include "Intra/Range/Operations.h"
#include "Intra/Range/Mutation/Copy.h"

#include "Intra/Range/Span.h"
#include "Intra/Functional.h"

#include "Intra/Range/Concepts.h"

#include "InputRange.h"
#include "ForwardRange.h"

INTRA_BEGIN
INTRA_IGNORE_WARNING_COPY_IMPLICITLY_DELETED
INTRA_IGNORE_WARNING_DEFAULT_CONSTRUCTOR_IMPLICITLY_DELETED
INTRA_IGNORE_WARNING_SIGN_CONVERSION
template<typename T> struct FiniteForwardRange: ForwardRange<T>
{
	static constexpr bool IsAnyInstanceFinite = true;
protected:
	struct Interface: ForwardRange<T>::Interface
	{
		virtual size_t Count() const = 0;
	};

	template<typename R, typename PARENT> using FullImplFiller =
		typename InputRange<T>::template ImplFiller<R, PARENT>;

	template<typename R> struct WrapperImpl:
		FullImplFiller<R, Interface>
	{
		template<typename A> WrapperImpl(A&& range):
			FullImplFiller<R, Interface>(Forward<A>(range)) {}

		[[nodiscard]] size_t Count() const override {return FullImplFiller<R, Interface>::OriginalRange.Count();}
		Interface* Clone() const override {return new WrapperImpl(FullImplFiller<R, Interface>::OriginalRange);}
	};

private:
	template<typename R> using EnableCondition = Requires<
		CConvertibleTo<TReturnValueTypeOfAs<R>, T> &&
		CAsFiniteForwardRange<R> &&
		!CSameIgnoreCVRef<R, FiniteForwardRange>
	>;

	template<typename R> static Interface* wrap(R&& range)
	{return new WrapperImpl<TRemoveConstRef<TRangeOfRef<R>>>(ForwardAsRange<R>(range));}

public:
	typedef TRemoveConstRef<T> value_type;

	FiniteForwardRange(decltype(null)=null) {}

	FiniteForwardRange(FiniteForwardRange&& rhs):
		ForwardRange<T>(Move(static_cast<ForwardRange<T>&&>(rhs))) {}

	FiniteForwardRange& operator=(FiniteForwardRange&& rhs)
	{
		ForwardRange<T>::operator=(Move(static_cast<ForwardRange<T>&&>(rhs)));
		return *this;
	}

	FiniteForwardRange(const FiniteForwardRange& rhs):
		ForwardRange<T>(rhs.clone()) {}

	FiniteForwardRange& operator=(const FiniteForwardRange& rhs)
	{
		InputRange<T>::mInterface = rhs.clone();
		return *this;
	}

	template<typename R, typename = EnableCondition<R>>
	FiniteForwardRange(R&& range):
		ForwardRange<T>(wrap(ForwardAsRange<R>(range))) {}

	template<typename R, typename = EnableCondition<R>>
	FiniteForwardRange& operator=(R&& range)
	{
		InputRange<T>::mInterface = wrap(ForwardAsRange<R>(range));
		return *this;
	}

	FiniteForwardRange(InitializerList<value_type> arr):
		FiniteForwardRange(AsRange(arr)) {}

	FiniteForwardRange& operator=(InitializerList<value_type> arr)
	{
		operator=(AsRange(arr));
		return *this;
	}

	[[nodiscard]] index_t Length() const
	{return static_cast<Interface*>(InputRange<T>::mInterface.Ptr())->Count();}

protected:
	FiniteForwardRange(typename ForwardRange<T>::Interface* interfacePtr): ForwardRange<T>(interfacePtr) {}
};

using FiniteForwardStream = FiniteForwardRange<char>;
INTRA_END
