#pragma once

#include "Core/Range/Operations.h"
#include "Core/Range/Mutation/Copy.h"

#include "Core/Range/Span.h"
#include "Funal/Op.h"

#include "Core/Range/Concepts.h"


#include "InputRange.h"
#include "ForwardRange.h"

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

template<typename T> struct RandomAccessRange: ForwardRange<T>
{
protected:
	struct Interface: ForwardRange<T>::Interface
	{
		virtual T OpIndex(size_t index) const = 0;
	};

	template<typename R, typename PARENT> using FullImplFiller =
		typename ForwardRange<T>::template FullImplFiller<R, PARENT>;

	template<typename R> struct WrapperImpl: FullImplFiller<R, Interface>
	{
		template<typename A> WrapperImpl(A&& range):
			FullImplFiller<R, Interface>(Forward<A>(range)) {}

		Interface* Clone() const override
		{return new WrapperImpl(FullImplFiller<R, Interface>::OriginalRange);}

		T OpIndex(size_t index) const override
		{return FullImplFiller<R, Interface>::OriginalRange[index];}
	};

private:
	template<typename R> using EnableCondition = Requires<
		CConvertible<TReturnValueTypeOfAs<R>, T> &&
		CAsRandomAccessRange<R> &&
		!CSameIgnoreCVRef<R, RandomAccessRange>
	>;

	template<typename R> forceinline static Interface* wrap(R&& range)
	{return new WrapperImpl<TRemoveConstRef<R>>(Forward<R>(range));}

public:
	typedef TRemoveConstRef<T> value_type;

	constexpr forceinline RandomAccessRange(null_t=null) {}

	INTRA_CONSTEXPR2 forceinline RandomAccessRange(RandomAccessRange&& rhs):
		ForwardRange<T>(Move(static_cast<ForwardRange<T>&&>(rhs))) {}

	INTRA_CONSTEXPR2 forceinline RandomAccessRange& operator=(RandomAccessRange&& rhs)
	{
		InputRange<T>::mInterface = Move(rhs.mInterface);
		return *this;
	}

	forceinline RandomAccessRange(const RandomAccessRange& rhs):
		ForwardRange<T>(rhs.clone()) {}

	forceinline RandomAccessRange& operator=(const RandomAccessRange& range)
	{
		InputRange<T>::mInterface = range.clone();
		return *this;
	}

	template<typename R, typename = EnableCondition<R>>
	forceinline RandomAccessRange(R&& range):
		ForwardRange<T>(wrap(ForwardAsRange<R>(range))) {}

	template<typename R, typename = EnableCondition<R>>
	forceinline RandomAccessRange& operator=(R&& range)
	{
		InputRange<T>::mInterface = wrap(ForwardAsRange<R>(range));
		return *this;
	}

	forceinline RandomAccessRange(InitializerList<value_type> arr):
		RandomAccessRange(AsRange(arr)) {}

	forceinline RandomAccessRange& operator=(InitializerList<value_type> arr)
	{
		operator=(AsRange(arr));
		return *this;
	}


	INTRA_NODISCARD forceinline T operator[](size_t index) const
	{return static_cast<Interface*>(InputRange<T>::mInterface.Ptr())->OpIndex(index);}

	INTRA_NODISCARD RandomAccessRange Drop(size_t count=1)
	{
		auto result = *this;
		result.PopFirstN(count);
		return result;
	}

	INTRA_NODISCARD RTake<RandomAccessRange> operator()(size_t start, size_t end) const
	{
		INTRA_DEBUG_ASSERT(end >= start);
		return Take(Drop(start), end-start);
	}

protected:
	constexpr forcienline RandomAccessRange(typename ForwardRange<T>::Interface* interfacePtr): ForwardRange<T>(interfacePtr) {}
};


#undef TEMPLATE
}
INTRA_END
