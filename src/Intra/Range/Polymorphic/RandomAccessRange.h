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
template<typename T> struct RandomAccessRange: ForwardRange<T>
{
protected:
	struct Interface: ForwardRange<T>::Interface
	{
		virtual T OpIndex(Index index) const = 0;
		virtual index_t PopFirstCount(ClampedSize elementsToPop) = 0;
	};

	template<typename R, typename PARENT> using FullImplFiller =
		typename ForwardRange<T>::template FullImplFiller<R, PARENT>;

	template<typename R> struct WrapperImpl: FullImplFiller<R, Interface>
	{
		template<typename A> WrapperImpl(A&& range):
			FullImplFiller<R, Interface>(Forward<A>(range)) {}

		Interface* Clone() const override
		{return new WrapperImpl(FullImplFiller<R, Interface>::OriginalRange);}

		T OpIndex(Index index) const override
		{return FullImplFiller<R, Interface>::OriginalRange[index];}

		index_t PopFirstCount(ClampedSize elementsToPop) override
		{PopFirstCount(FullImplFiller<R, Interface>::OriginalRange, elementsToPop);}
	};

private:
	template<typename R> using EnableCondition = Requires<
		CConvertibleTo<TReturnValueTypeOfAs<R>, T> &&
		CAsRandomAccessRange<R> &&
		!CSameIgnoreCVRef<R, RandomAccessRange>
	>;

	template<typename R> INTRA_FORCEINLINE static Interface* wrap(R&& range)
	{return new WrapperImpl<TRemoveConstRef<R>>(Forward<R>(range));}

public:
	typedef TRemoveConstRef<T> value_type;

	constexpr RandomAccessRange(decltype(null)=null) {}

	constexpr RandomAccessRange(RandomAccessRange&& rhs):
		ForwardRange<T>(Move(static_cast<ForwardRange<T>&&>(rhs))) {}

	constexpr RandomAccessRange& operator=(RandomAccessRange&& rhs)
	{
		InputRange<T>::mInterface = Move(rhs.mInterface);
		return *this;
	}

	RandomAccessRange(const RandomAccessRange& rhs):
		ForwardRange<T>(rhs.clone()) {}

	RandomAccessRange& operator=(const RandomAccessRange& range)
	{
		InputRange<T>::mInterface = range.clone();
		return *this;
	}

	template<typename R, typename = EnableCondition<R>>
	RandomAccessRange(R&& range):
		ForwardRange<T>(wrap(ForwardAsRange<R>(range))) {}

	template<typename R, typename = EnableCondition<R>>
	RandomAccessRange& operator=(R&& range)
	{
		InputRange<T>::mInterface = wrap(ForwardAsRange<R>(range));
		return *this;
	}

	RandomAccessRange(InitializerList<value_type> arr):
		RandomAccessRange(AsRange(arr)) {}

	RandomAccessRange& operator=(InitializerList<value_type> arr)
	{
		operator=(AsRange(arr));
		return *this;
	}


	[[nodiscard]] T operator[](Index index) const
	{return static_cast<Interface*>(InputRange<T>::mInterface.Ptr())->OpIndex(index);}

	index_t PopFirstCount(ClampedSize elementsToPop) override
	{static_cast<Interface*>(InputRange<T>::mInterface.Ptr())->PopFirstCount(elementsToPop);}

protected:
	constexpr RandomAccessRange(typename ForwardRange<T>::Interface* interfacePtr): ForwardRange<T>(interfacePtr) {}
};

#undef TEMPLATE
INTRA_END
