#pragma once

#include "Intra/Functional.h"
#include "Intra/Range/Concepts.h"
#include "Intra/Range/Mutation/Copy.h"
#include "Intra/Range/Span.h"
#include "Intra/Range/Operations.h"

#include "InputRange.h"
#include "BidirectionalRange.h"

INTRA_BEGIN
INTRA_IGNORE_WARN_COPY_IMPLICITLY_DELETED
INTRA_IGNORE_WARN_DEFAULT_CTOR_IMPLICITLY_DELETED
INTRA_IGNORE_WARN_SIGN_CONVERSION

template<typename T> struct FiniteRandomAccessRange: BidirectionalRange<T>
{
protected:
	struct Interface: BidirectionalRange<T>::Interface
	{
		virtual T OpIndex(size_t index) const = 0;
		virtual index_t PopFirstCount(ClampedSize elementsToPop) = 0;
		virtual index_t PopLastCount(ClampedSize elementsToPop) = 0;
	};

	template<typename R, typename PARENT> using FullImplFiller = 
		typename BidirectionalRange<T>::template FullImplFiller<R, PARENT>;

	template<typename R> struct WrapperImpl: FullImplFiller<R, Interface>
	{
		template<typename A> WrapperImpl(A&& range): FullImplFiller<R, Interface>(Forward<A>(range)) {}

		WrapperImpl* Clone() const override {return new WrapperImpl(FullImplFiller<R, Interface>::OriginalRange);}
		T OpIndex(Index index) const override {return FullImplFiller<R, Interface>::OriginalRange[index];}
		
		index_t PopFirstCount(ClampedSize elementsToPop) override
		{PopFirstCount(FullImplFiller<R, Interface>::OriginalRange, elementsToPop);}

		index_t PopLastCount(ClampedSize elementsToPop) override
		{PopLastCount(FullImplFiller<R, Interface>::OriginalRange, elementsToPop);}

		index_t Count() const override {return Count(FullImplFiller<R, Interface>::OriginalRange);}
	};

private:
	template<typename R> using EnableCondition = Requires<
		CConvertibleTo<TListValueRef<R>, T> &&
		CFiniteRandomAccessList<R> &&
		!CSameIgnoreCVRef<R, FiniteRandomAccessRange>
	>;

	template<typename R> static Interface* wrap(R&& range)
	{return new WrapperImpl<TRemoveConstRef<TRangeOfRef<R>>>(ForwardAsRange<R>(range));}

public:
	using value_type = TRemoveConstRef<T>;

	constexpr FiniteRandomAccessRange(decltype(null)=null) {}

	constexpr FiniteRandomAccessRange(FiniteRandomAccessRange&& rhs):
		BidirectionalRange<T>(Move(static_cast<BidirectionalRange<T>&&>(rhs))) {}

	constexpr FiniteRandomAccessRange& operator=(FiniteRandomAccessRange&& rhs)
	{
		BidirectionalRange<T>::operator=(Move(static_cast<BidirectionalRange<T>&&>(rhs)));
		return *this;
	}

	FiniteRandomAccessRange(const FiniteRandomAccessRange& rhs):
		BidirectionalRange<T>(rhs.clone()) {}

	FiniteRandomAccessRange& operator=(const FiniteRandomAccessRange& rhs)
	{
		InputRange<T>::mInterface = rhs.clone();
		return *this;
	}

	template<typename R, typename = EnableCondition<R>>
	FiniteRandomAccessRange(R&& range):
		BidirectionalRange<T>(wrap(ForwardAsRange<R>(range))) {}

	template<typename R, typename = EnableCondition<R>>
	FiniteRandomAccessRange& operator=(R&& range)
	{
		InputRange<T>::mInterface = wrap(ForwardAsRange<R>(range));
		return *this;
	}

	FiniteRandomAccessRange(InitializerList<value_type> arr):
		FiniteRandomAccessRange(AsRange(arr)) {}

	FiniteRandomAccessRange& operator=(InitializerList<value_type> arr)
	{
		operator=(AsRange(arr));
		return *this;
	}

	[[nodiscard]] T operator[](Index index) const
	{return static_cast<Interface*>(InputRange<T>::mInterface.Ptr())->OpIndex(index);}

	index_t PopFirstCount(ClampedSize elementsToPop) override
	{static_cast<Interface*>(InputRange<T>::mInterface.Ptr())->PopFirstCount(elementsToPop);}

	index_t PopLastCount(ClampedSize elementsToPop) override
	{static_cast<Interface*>(InputRange<T>::mInterface.Ptr())->PopLastCount(elementsToPop);}

protected:
	constexpr FiniteRandomAccessRange(typename ForwardRange<T>::Interface* interfacePtr):
		BidirectionalRange<T>(interfacePtr) {}
};
INTRA_END
