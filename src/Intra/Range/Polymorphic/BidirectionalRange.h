#pragma once

#include "Intra/Range/Span.h"
#include "Intra/Functional.h"
#include "Intra/Range/Concepts.h"


#include "Intra/Range/Operations.h"
#include "Intra/Range/Mutation/Copy.h"

#include "InputRange.h"
#include "FiniteForwardRange.h"

namespace Intra { INTRA_BEGIN
INTRA_IGNORE_WARN_COPY_IMPLICITLY_DELETED
INTRA_IGNORE_WARN_DEFAULT_CTOR_IMPLICITLY_DELETED
INTRA_IGNORE_WARN_SIGN_CONVERSION

template<typename T> struct BidirectionalRange: FiniteForwardRange<T>
{
protected:
	struct Interface: FiniteForwardRange<T>::Interface
	{
		virtual T Last() const = 0;
		virtual void PopLast() = 0;
		virtual void PopLastCount(size_t count) = 0;
	};

	template<typename R, typename PARENT> struct ImplFiller: PARENT
	{
		template<typename A> constexpr ImplFiller(A&& range):
			PARENT(Forward<A>(range)) {}

		index_t Count() const override {return Count(PARENT::OriginalRange);}

		[[nodiscard]] T Last() const override {return PARENT::OriginalRange.Last();}
		void PopLast() override {PARENT::OriginalRange.PopLast();}
		void PopLastCount(size_t count) override {PopLastCount(PARENT::OriginalRange, count);}
	};

	template<typename R, typename PARENT> using FullImplFiller =
		ImplFiller<R, typename InputRange<T>::template ImplFiller<R, PARENT>>;

	template<typename R> struct WrapperImpl: FullImplFiller<R, Interface>
	{
		typedef FullImplFiller<R, Interface> base;
		template<typename A> constexpr WrapperImpl(A&& range): base(Forward<A>(range)) {}
		Interface* Clone() const override {return new WrapperImpl(base::OriginalRange);}
	};

private:
	template<typename R> using EnableCondition = Requires<
		CConvertibleTo<TListValueRef<R>, T> &&
		CBidirectionalList<R> &&
		!CSameUnqualRef<R, BidirectionalRange>
	>;

	template<typename R> INTRA_FORCEINLINE static Interface* wrap(R&& range)
	{return new WrapperImpl<TRemoveConstRef<TRangeOfRef<R>>>(ForwardAsRange<R>(range));}

public:
	typedef TRemoveConstRef<T> value_type;

	constexpr BidirectionalRange(decltype(nullptr)=nullptr) {}

	constexpr BidirectionalRange(BidirectionalRange&& rhs):
		FiniteForwardRange<T>(Move(static_cast<FiniteForwardRange<T>&&>(rhs))) {}

	constexpr BidirectionalRange& operator=(BidirectionalRange&& rhs)
	{
		FiniteForwardRange<T>::operator=(Move(static_cast<FiniteForwardRange<T>&&>(rhs)));
		return *this;
	}

	INTRA_FORCEINLINE BidirectionalRange& operator=(const BidirectionalRange& range)
	{
		InputRange<T>::mInterface = range.clone();
		return *this;
	}

	INTRA_FORCEINLINE BidirectionalRange(const BidirectionalRange& rhs):
		FiniteForwardRange<T>(rhs.clone()) {}

	template<typename R, typename = EnableCondition<R>>
	INTRA_FORCEINLINE BidirectionalRange(R&& range):
		FiniteForwardRange<T>(wrap(ForwardAsRange<R>(range))) {}

	template<typename R, typename = EnableCondition<R>>
	INTRA_FORCEINLINE BidirectionalRange& operator=(R&& range)
	{
		InputRange<T>::mInterface = wrap(ForwardAsRange<R>(range));
		return *this;
	}

	INTRA_FORCEINLINE BidirectionalRange(InitializerList<value_type> arr):
		BidirectionalRange(AsRange(arr)) {}

	INTRA_FORCEINLINE BidirectionalRange& operator=(InitializerList<value_type> arr)
	{
		operator=(AsRange(arr));
		return *this;
	}

	[[nodiscard]] INTRA_FORCEINLINE T Last() const
	{return static_cast<Interface*>(InputRange<T>::mInterface.Ptr())->Last();}

	INTRA_FORCEINLINE void PopLast()
	{static_cast<Interface*>(InputRange<T>::mInterface.Ptr())->PopLast();}

	INTRA_FORCEINLINE void PopLastCount(size_t count)
	{static_cast<Interface*>(InputRange<T>::mInterface.Ptr())->PopLastCount(count);}

protected:
	BidirectionalRange(typename ForwardRange<T>::Interface* interfacePtr): FiniteForwardRange<T>(interfacePtr) {}
};

#undef TEMPLATE

} INTRA_END
