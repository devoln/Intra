#pragma once

#include "Core/Range/Span.h"
#include "Core/Functional.h"

#include "Core/Range/Operations.h"
#include "Core/Range/Mutation/Copy.h"

#include "Core/Range/Concepts.h"


#include "InputRange.h"
#include "BidirectionalRange.h"

INTRA_BEGIN
INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED
INTRA_WARNING_DISABLE_DEFAULT_CONSTRUCTOR_IMPLICITLY_DELETED
INTRA_WARNING_DISABLE_SIGN_CONVERSION

#if defined(_MSC_VER) && !defined(__clang__)
#define TEMPLATE //VS doesn't compile construction typename ...<...>::template ...<...>, но без template компилирует
#else
#define TEMPLATE template //clang required template keyword in typename ...<...>::template ...<...>
#endif

template<typename T> struct FiniteRandomAccessRange: BidirectionalRange<T>
{
protected:
	struct Interface: BidirectionalRange<T>::Interface
	{
		virtual T OpIndex(size_t index) const = 0;
		virtual Interface* OpSlice(size_t start, size_t end) const = 0;
	};

	template<typename R, typename PARENT> using FullImplFiller = 
		typename BidirectionalRange<T>::TEMPLATE FullImplFiller<R, PARENT>;

	template<typename R> struct WrapperImpl: FullImplFiller<R, Interface>
	{
		template<typename A> WrapperImpl(A&& range): FullImplFiller<R, Interface>(Forward<A>(range)) {}

		WrapperImpl* Clone() const override {return new WrapperImpl(FullImplFiller<R, Interface>::OriginalRange);}
		T OpIndex(size_t index) const override {return FullImplFiller<R, Interface>::OriginalRange[index];}
		
		Interface* OpSlice(size_t start, size_t end) const override
		{return new WrapperImpl(FullImplFiller<R, Interface>::OriginalRange(start, end));}

		size_t Count() const override {return Count(FullImplFiller<R, Interface>::OriginalRange);}
	};

private:
	template<typename R> using EnableCondition = Requires<
		CConvertible<TReturnValueTypeOfAs<R>, T> &&
		CAsFiniteRandomAccessRange<R> &&
		!CSameIgnoreCVRef<R, FiniteRandomAccessRange>
	>;

	template<typename R> forceinline static Interface* wrap(R&& range)
	{return new WrapperImpl<TRemoveConstRef<TRangeOfType<R>>>(ForwardAsRange<R>(range));}

public:
	typedef TRemoveConstRef<T> value_type;

	constexpr forceinline FiniteRandomAccessRange(null_t=null) {}

	constexpr forceinline FiniteRandomAccessRange(FiniteRandomAccessRange&& rhs):
		BidirectionalRange<T>(Move(static_cast<BidirectionalRange<T>&&>(rhs))) {}

	constexpr forceinline FiniteRandomAccessRange& operator=(FiniteRandomAccessRange&& rhs)
	{
		BidirectionalRange<T>::operator=(Move(static_cast<BidirectionalRange<T>&&>(rhs)));
		return *this;
	}

	forceinline FiniteRandomAccessRange(const FiniteRandomAccessRange& rhs):
		BidirectionalRange<T>(rhs.clone()) {}

	forceinline FiniteRandomAccessRange& operator=(const FiniteRandomAccessRange& rhs)
	{
		InputRange<T>::mInterface = rhs.clone();
		return *this;
	}

	template<typename R, typename = EnableCondition<R>>
	forceinline FiniteRandomAccessRange(R&& range):
		BidirectionalRange<T>(wrap(ForwardAsRange<R>(range))) {}

	template<typename R, typename = EnableCondition<R>>
	forceinline FiniteRandomAccessRange& operator=(R&& range)
	{
		InputRange<T>::mInterface = wrap(ForwardAsRange<R>(range));
		return *this;
	}

	forceinline FiniteRandomAccessRange(InitializerList<value_type> arr):
		FiniteRandomAccessRange(AsRange(arr)) {}

	forceinline FiniteRandomAccessRange& operator=(InitializerList<value_type> arr)
	{
		operator=(AsRange(arr));
		return *this;
	}

	INTRA_NODISCARD forceinline T operator[](size_t index) const
	{return static_cast<Interface*>(InputRange<T>::mInterface.Ptr())->OpIndex(index);}

	INTRA_NODISCARD forceinline FiniteRandomAccessRange operator()(size_t start, size_t end) const
	{return FiniteRandomAccessRange(static_cast<Interface*>(InputRange<T>::mInterface.Ptr())->OpSlice(start, end));}

protected:
	constexpr forceinline FiniteRandomAccessRange(typename ForwardRange<T>::Interface* interfacePtr):
		BidirectionalRange<T>(interfacePtr) {}
};

#undef TEMPLATE
INTRA_END
