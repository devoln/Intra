#pragma once

#include "Memory/SmartRef.hh"
#include "Range/Operations.h"
#include "Algo/Op.h"
#include "Algo/Mutation/Copy.h"
#include "Range/Concepts.h"
#include "Range/Generators/ArrayRange.h"
#include "Range/AsRange.h"
#include "Range/Stream/OutputStreamMixin.h"

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

template<typename T> struct OutputRange: Meta::SelectType<
	OutputStreamMixin<OutputRange<T>, Meta::RemoveConst<T>>,
	Meta::EmptyType, Meta::IsTriviallySerializable<T>::_>
{
private:
	struct Interface
	{
		virtual ~Interface() {}

		virtual bool Empty() const = 0;
		virtual void Put(const T& value) = 0;

		virtual void Put(T&& value) = 0;

		virtual bool TryPut(const T& value) = 0;
		virtual bool TryPut(T&& value) = 0;

		virtual size_t CopyAdvanceFromAdvance(ArrayRange<const T>& src) = 0;
	};

	template<typename R> struct WrapperImpl: Interface
	{
		template<typename A> WrapperImpl(A&& range):
			OriginalRange(Meta::Forward<A>(range)) {}

		bool Empty() const override final {return Range::EmptyOrFalse(OriginalRange);}
		void Put(const T& value) override final {OriginalRange.Put(value);}

		void Put(T&& value) override final {OriginalRange.Put(Meta::Move(value));}

		bool TryPut(const T& value) override final
		{
			if(Range::EmptyOrFalse(OriginalRange)) return false;
			OriginalRange.Put(value);
			return true;
		}

		bool TryPut(T&& value) override final
		{
			if(Range::EmptyOrFalse(OriginalRange)) return false;
			OriginalRange.Put(Meta::Move(value));
			return true;
		}

		size_t CopyAdvanceFromAdvance(ArrayRange<const T>& src) override final
		{return Algo::CopyAdvanceToAdvance(src, OriginalRange);}

		R OriginalRange;
	};

private:
	template<typename R> using EnableCondition = Meta::EnableIf<
		Meta::IsConvertible<ReturnValueTypeOfAs<R>, T>::_ &&
		IsAsOutputRange<R>::_ &&
		!Meta::TypeEqualsIgnoreCVRef<R, OutputRange>::_
	>;

	template<typename R> forceinline static Interface* wrap(R&& range)
	{
		typedef AsRangeResultNoCRef<R&&> Range;
		return new WrapperImpl<Range>(AsRange(Meta::Forward<R>(range)));
	}

public:
	forceinline OutputRange(null_t=null): mInterface(null) {}

	forceinline OutputRange(OutputRange&& rhs):
		mInterface(Meta::Move(rhs.mInterface)) {}

	forceinline OutputRange& operator=(OutputRange&& rhs)
	{
		mInterface = Meta::Move(rhs.mInterface);
		return *this;
	}

	OutputRange(const OutputRange& rhs) = delete;

	OutputRange& operator=(const OutputRange& rhs) = delete;

	template<typename R, typename = EnableCondition<R>>
	forceinline OutputRange(R&& range):
		mInterface(wrap(Range::Forward<R>(range))) {}

	template<typename R, typename = EnableCondition<R>>
	forceinline OutputRange& operator=(R&& range)
	{
		mInterface = wrap(Range::Forward<R>(range));
		return *this;
	}

	forceinline OutputRange(InitializerList<T> arr):
		OutputRange(AsRange(arr)) {}

	forceinline OutputRange& operator=(InitializerList<T> arr)
	{
		operator=(AsRange(arr));
		return *this;
	}


	forceinline bool Empty() const {return mInterface==null || mInterface->Empty();}
	forceinline void Put(const T& value) {mInterface->Put(value);}
	forceinline void Put(T&& value) {mInterface->Put(Meta::Move(value));}
	forceinline bool TryPut(const T& value) {return mInterface!=null && mInterface->TryPut(value);}
	forceinline bool TryPut(T&& value) {return mInterface!=null && mInterface->TryPut(Meta::Move(value));}

	forceinline size_t CopyAdvanceFromAdvance(ArrayRange<const T>& dst)
	{
		if(mInterface==null) return 0;
		return mInterface->CopyAdvanceFromAdvance(dst);
	}

	template<typename AR> Meta::EnableIf<
		Range::IsArrayRangeOfExactly<AR, T>::_ && !Meta::IsConst<AR>::_,
	size_t> CopyAdvanceFromAdvance(AR& src)
	{
		ArrayRange<const T> srcArr = {src.Data(), src.Length()};
		size_t result = CopyAdvanceFromAdvance(srcArr);
		Range::PopFirstExactly(src, result);
		return result;
	}

protected:
	Memory::UniqueRef<Interface> mInterface;
	InputRange(Interface* interfacePtr): mInterface(interfacePtr) {}
};

typedef OutputRange<char> OutputStream;


#undef TEMPLATE

INTRA_WARNING_POP

}

using Range::OutputRange;
using Range::OutputStream;

}
