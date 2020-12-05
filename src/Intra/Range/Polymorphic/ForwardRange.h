#pragma once

#include "Intra/Range/Operations.h"
#include "Intra/Functional.h"
#include "Intra/Range/Mutation/Copy.h"
#include "Intra/Range/Concepts.h"
#include "Intra/Range/Span.h"

#include "IntraX/Utils/Unique.h"

#include "InputRange.h"

INTRA_BEGIN
INTRA_IGNORE_WARN_COPY_IMPLICITLY_DELETED
INTRA_IGNORE_WARN_DEFAULT_CTOR_IMPLICITLY_DELETED
INTRA_IGNORE_WARN_SIGN_CONVERSION
template<typename T> struct ForwardRange: InputRange<T>
{
protected:
	struct Interface: InputRange<T>::Interface
	{
		virtual Interface* Clone() const = 0;
	};

	template<typename R, typename PARENT> using FullImplFiller =
		typename InputRange<T>::template ImplFiller<R, PARENT>;

	template<typename R> struct WrapperImpl: FullImplFiller<R, Interface>
	{
		template<typename A> WrapperImpl(A&& range):
			FullImplFiller<R, Interface>(Forward<A>(range)) {}

		Interface* Clone() const override {return new WrapperImpl(FullImplFiller<R, Interface>::OriginalRange);}
	};

	Interface* clone() const {return static_cast<Interface*>(InputRange<T>::mInterface.Ptr())->Clone();}

private:
	template<typename R> using EnableCondition = Requires<
		CConvertibleTo<TListValueRef<R>, T> &&
		CForwardList<R> &&
		!CSameIgnoreCVRef<R, ForwardRange>
	>;

	template<typename R> static Interface* wrap(R&& range)
	{return new WrapperImpl<TRemoveConstRef<TRangeOfRef<R>>>(ForwardAsRange<R>(range));}

public:
	typedef TRemoveConstRef<T> value_type;

	constexpr ForwardRange(decltype(null)=null) {}

	constexpr ForwardRange(ForwardRange&& rhs):
		InputRange<T>(Move(static_cast<InputRange<T>&&>(rhs))) {}

	constexpr ForwardRange& operator=(ForwardRange&& rhs)
	{
		InputRange<T>::operator=(Move(static_cast<InputRange<T>&&>(rhs)));
		return *this;
	}

	ForwardRange(const ForwardRange& rhs):
		InputRange<T>(rhs.clone()) {}

	ForwardRange& operator=(const ForwardRange& rhs)
	{
		InputRange<T>::mInterface = rhs.clone();
		return *this;
	}

	template<typename R, typename = EnableCondition<R>>
	ForwardRange(R&& range):
		InputRange<T>(wrap(ForwardAsRange<R>(range))) {}
	
	template<typename R, typename = EnableCondition<R>>
	ForwardRange& operator=(R&& range)
	{
		InputRange<T>::mInterface = wrap(ForwardAsRange<R>(range));
		return *this;
	}

	ForwardRange(InitializerList<value_type> arr):
		ForwardRange(AsRange(arr)) {}

	ForwardRange& operator=(InitializerList<value_type> arr)
	{
		operator=(AsRange(arr));
		return *this;
	}

	operator Interface&() {return *static_cast<Interface*>(InputRange<T>::mInterface.Ptr());}

protected:
	constexpr ForwardRange(Interface* interfacePtr): InputRange<T>(interfacePtr) {}
};
typedef ForwardRange<char> ForwardStream;
INTRA_END
