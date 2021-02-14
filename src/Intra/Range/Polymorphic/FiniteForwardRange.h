#pragma once

#include "Intra/Range/Operations.h"
#include "Intra/Range/Mutation/Copy.h"

#include "Intra/Range/Span.h"
#include "Intra/Functional.h"

#include "Intra/Range/Concepts.h"

#include "InputRange.h"
#include "ForwardRange.h"

namespace Intra { INTRA_BEGIN
INTRA_IGNORE_WARN_COPY_IMPLICITLY_DELETED
INTRA_IGNORE_WARN_DEFAULT_CTOR_IMPLICITLY_DELETED
INTRA_IGNORE_WARN_SIGN_CONVERSION
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
		CConvertibleTo<TListValueRef<R>, T> &&
		CFiniteForwardList<R> &&
		!CSameUnqualRef<R, FiniteForwardRange>
	>;

	template<typename R> static Interface* wrap(R&& range)
	{return new WrapperImpl<TRemoveConstRef<TRangeOfRef<R>>>(ForwardAsRange<R>(range));}

public:
	typedef TRemoveConstRef<T> value_type;

	FiniteForwardRange(decltype(nullptr)=nullptr) {}

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
} INTRA_END
