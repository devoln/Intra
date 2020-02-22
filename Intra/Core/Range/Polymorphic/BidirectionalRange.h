#pragma once

#include "Core/Range/Span.h"
#include "Core/Functional.h"
#include "Core/Range/Concepts.h"


#include "Core/Range/Operations.h"
#include "Core/Range/Mutation/Copy.h"

#include "InputRange.h"
#include "FiniteForwardRange.h"

INTRA_BEGIN
INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED
INTRA_WARNING_DISABLE_DEFAULT_CONSTRUCTOR_IMPLICITLY_DELETED
INTRA_WARNING_DISABLE_SIGN_CONVERSION

#if defined(_MSC_VER) && !defined(__clang__)
#define TEMPLATE //VS doesn't compile construction typename ...<...>::template ...<...>, но без template компилирует
#else
#define TEMPLATE template //clang required template keyword in typename ...<...>::template ...<...>
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
		template<typename A> constexpr forceinline ImplFiller(A&& range):
			PARENT(Forward<A>(range)) {}

		forceinline size_t Count() const override {return Count(PARENT::OriginalRange);}

		INTRA_NODISCARD T Last() const override {return PARENT::OriginalRange.Last();}
		void PopLast() override {PARENT::OriginalRange.PopLast();}
		void PopLastN(size_t count) override {PopLastN(PARENT::OriginalRange, count);}
	};

	template<typename R, typename PARENT> using FullImplFiller =
		ImplFiller<R, typename InputRange<T>::TEMPLATE ImplFiller<R, PARENT>>;

	template<typename R> struct WrapperImpl: FullImplFiller<R, Interface>
	{
		typedef FullImplFiller<R, Interface> base;
		template<typename A> constexpr forceinline WrapperImpl(A&& range): base(Forward<A>(range)) {}
		Interface* Clone() const override {return new WrapperImpl(base::OriginalRange);}
	};

private:
	template<typename R> using EnableCondition = Requires<
		CConvertible<TReturnValueTypeOfAs<R>, T> &&
		CAsBidirectionalRange<R> &&
		!CSameIgnoreCVRef<R, BidirectionalRange>
	>;

	template<typename R> forceinline static Interface* wrap(R&& range)
	{return new WrapperImpl<TRemoveConstRef<TRangeOfType<R>>>(ForwardAsRange<R>(range));}

public:
	typedef TRemoveConstRef<T> value_type;

	constexpr forceinline BidirectionalRange(null_t=null) {}

	constexpr forceinline BidirectionalRange(BidirectionalRange&& rhs):
		FiniteForwardRange<T>(Move(static_cast<FiniteForwardRange<T>&&>(rhs))) {}

	constexpr forceinline BidirectionalRange& operator=(BidirectionalRange&& rhs)
	{
		FiniteForwardRange<T>::operator=(Move(static_cast<FiniteForwardRange<T>&&>(rhs)));
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
		FiniteForwardRange<T>(wrap(ForwardAsRange<R>(range))) {}

	template<typename R, typename = EnableCondition<R>>
	forceinline BidirectionalRange& operator=(R&& range)
	{
		InputRange<T>::mInterface = wrap(ForwardAsRange<R>(range));
		return *this;
	}

	forceinline BidirectionalRange(InitializerList<value_type> arr):
		BidirectionalRange(AsRange(arr)) {}

	forceinline BidirectionalRange& operator=(InitializerList<value_type> arr)
	{
		operator=(AsRange(arr));
		return *this;
	}

	INTRA_NODISCARD forceinline T Last() const
	{return static_cast<Interface*>(InputRange<T>::mInterface.Ptr())->Last();}

	forceinline void PopLast()
	{static_cast<Interface*>(InputRange<T>::mInterface.Ptr())->PopLast();}

	forceinline void PopLastN(size_t count)
	{static_cast<Interface*>(InputRange<T>::mInterface.Ptr())->PopLastN(count);}

protected:
	BidirectionalRange(typename ForwardRange<T>::Interface* interfacePtr): FiniteForwardRange<T>(interfacePtr) {}
};

#undef TEMPLATE

INTRA_END
