#pragma once

#include "Cpp/Warnings.h"
#include "Range/Operations.h"
#include "Utils/Op.h"
#include "Range/Mutation/Copy.h"
#include "Concepts/Range.h"
#include "Utils/Span.h"
#include "Concepts/RangeOf.h"

#include "InputRange.h"
#include "FiniteForwardRange.h"

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

template<typename T> struct BidirectionalRange: FiniteForwardRange<T>
{
protected:
	struct Interface: FiniteForwardRange<T>::Interface
	{
		virtual T Last() const = 0;
		virtual void PopLast() = 0;
		virtual void PopLastN(size_t count) = 0;
	};

	template<typename R, typename PARENT> struct ImplFiller: PARENT
	{
		template<typename A> ImplFiller(A&& range):
			PARENT(Cpp::Forward<A>(range)) {}

		size_t Count() const override {return Range::Count(PARENT::OriginalRange);}

		T Last() const override {return PARENT::OriginalRange.Last();}
		void PopLast() override {PARENT::OriginalRange.PopLast();}
		void PopLastN(size_t count) override {Range::PopLastN(PARENT::OriginalRange, count);}
	};

	template<typename R, typename PARENT> using FullImplFiller =
		ImplFiller<R, typename InputRange<T>::TEMPLATE ImplFiller<R, PARENT>>;

	template<typename R> struct WrapperImpl: FullImplFiller<R, Interface>
	{
		typedef FullImplFiller<R, Interface> base;
		template<typename A> WrapperImpl(A&& range): base(Cpp::Forward<A>(range)) {}
		Interface* Clone() const override {return new WrapperImpl(base::OriginalRange);}
	};

private:
	template<typename R> using EnableCondition = Meta::EnableIf<
		Meta::IsConvertible<Concepts::ReturnValueTypeOfAs<R>, T>::_ &&
		Concepts::IsAsBidirectionalRange<R>::_ &&
		!Meta::TypeEqualsIgnoreCVRef<R, BidirectionalRange>::_
	>;

	template<typename R> forceinline static Interface* wrap(R&& range)
	{return new WrapperImpl<Meta::RemoveConstRef<Concepts::RangeOfType<R>>>(Range::Forward<R>(range));}

public:
	typedef Meta::RemoveConstRef<T> value_type;

	forceinline BidirectionalRange(null_t=null) {}

	forceinline BidirectionalRange(BidirectionalRange&& rhs):
		FiniteForwardRange<T>(Cpp::Move(static_cast<FiniteForwardRange<T>&&>(rhs))) {}

	forceinline BidirectionalRange& operator=(BidirectionalRange&& rhs)
	{
		FiniteForwardRange<T>::operator=(Cpp::Move(static_cast<FiniteForwardRange<T>&&>(rhs)));
		return *this;
	}

	forceinline BidirectionalRange& operator=(const BidirectionalRange& range)
	{
		InputRange<T>::mInterface = range.clone();
		return *this;
	}

	forceinline BidirectionalRange(const BidirectionalRange& rhs):
		FiniteForwardRange<T>(rhs.clone()) {}

	template<typename R, typename = EnableCondition<R>>
	forceinline BidirectionalRange(R&& range):
		FiniteForwardRange<T>(wrap(Range::Forward<R>(range))) {}

	template<typename R, typename = EnableCondition<R>>
	forceinline BidirectionalRange& operator=(R&& range)
	{
		InputRange<T>::mInterface = wrap(Range::Forward<R>(range));
		return *this;
	}

	forceinline BidirectionalRange(InitializerList<value_type> arr):
		BidirectionalRange(AsRange(arr)) {}

	forceinline BidirectionalRange& operator=(InitializerList<value_type> arr)
	{
		operator=(AsRange(arr));
		return *this;
	}

	forceinline T Last() const
	{return static_cast<Interface*>(InputRange<T>::mInterface.Ptr())->Last();}

	forceinline void PopLast()
	{static_cast<Interface*>(InputRange<T>::mInterface.Ptr())->PopLast();}

	forceinline void PopLastN(size_t count)
	{static_cast<Interface*>(InputRange<T>::mInterface.Ptr())->PopLastN(count);}

protected:
	BidirectionalRange(typename ForwardRange<T>::Interface* interfacePtr): FiniteForwardRange<T>(interfacePtr) {}
};


#undef TEMPLATE

INTRA_WARNING_POP

}

using Range::BidirectionalRange;

}
