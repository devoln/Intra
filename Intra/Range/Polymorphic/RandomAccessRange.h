#pragma once

#include "Cpp/Warnings.h"
#include "Range/Operations.h"
#include "Utils/Op.h"
#include "Range/Mutation/Copy.h"
#include "Concepts/Range.h"
#include "Utils/Span.h"
#include "Concepts/RangeOf.h"

#include "InputRange.h"
#include "ForwardRange.h"

namespace Intra { namespace Range {

INTRA_WARNING_PUSH
INTRA_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED
INTRA_WARNING_DISABLE_DEFAULT_CONSTRUCTOR_IMPLICITLY_DELETED
INTRA_WARNING_DISABLE_SIGN_CONVERSION

#if defined(_MSC_VER) && !defined(__clang__)
#define TEMPLATE //VS не хочет компилировать конструкцию typename ...<...>::template ...<...>, но без template компилирует
#else
#define TEMPLATE template //clang требует слово template в typename ...<...>::template ...<...>
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
			FullImplFiller<R, Interface>(Cpp::Forward<A>(range)) {}

		Interface* Clone() const override
		{return new WrapperImpl(FullImplFiller<R, Interface>::OriginalRange);}

		T OpIndex(size_t index) const override
		{return FullImplFiller<R, Interface>::OriginalRange[index];}
	};

private:
	template<typename R> using EnableCondition = Meta::EnableIf<
		Meta::IsConvertible<ReturnValueTypeOfAs<R>, T>::_ &&
		IsAsRandomAccessRange<R>::_ &&
		!Meta::TypeEqualsIgnoreCVRef<R, RandomAccessRange>::_
	>;

	template<typename R> forceinline static Interface* wrap(R&& range)
	{return new WrapperImpl<Meta::RemoveConstRef<R>>(Cpp::Forward<R>(range));}

public:
	typedef Meta::RemoveConstRef<T> value_type;

	forceinline RandomAccessRange(null_t=null) {}

	forceinline RandomAccessRange(RandomAccessRange&& rhs):
		ForwardRange<T>(Cpp::Move(static_cast<ForwardRange<T>&&>(rhs))) {}

	forceinline RandomAccessRange& operator=(RandomAccessRange&& rhs)
	{
		InputRange<T>::mInterface = Cpp::Move(rhs.mInterface);
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
		ForwardRange<T>(wrap(Range::Forward<R>(range))) {}

	template<typename R, typename = EnableCondition<R>>
	forceinline RandomAccessRange& operator=(R&& range)
	{
		InputRange<T>::mInterface = wrap(Range::Forward<R>(range));
		return *this;
	}

	forceinline RandomAccessRange(InitializerList<value_type> arr):
		RandomAccessRange(AsRange(arr)) {}

	forceinline RandomAccessRange& operator=(InitializerList<value_type> arr)
	{
		operator=(AsRange(arr));
		return *this;
	}


	forceinline T operator[](size_t index) const
	{return static_cast<Interface*>(InputRange<T>::mInterface.Ptr())->OpIndex(index);}

	RandomAccessRange Drop(size_t count=1)
	{
		auto result = *this;
		result.PopFirstN(count);
		return result;
	}

	RTake<RandomAccessRange> operator()(size_t start, size_t end) const
	{
		INTRA_DEBUG_ASSERT(end>=start);
		return Take(Drop(start), end-start);
	}

protected:
	RandomAccessRange(typename ForwardRange<T>::Interface* interfacePtr): ForwardRange<T>(interfacePtr) {}
};


#undef TEMPLATE

INTRA_WARNING_POP

}

using Range::RandomAccessRange;

}
