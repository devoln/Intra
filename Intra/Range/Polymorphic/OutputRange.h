#pragma once

#include "Utils/Unique.h"
#include "Utils/Op.h"
#include "Utils/Span.h"

#include "Concepts/Range.h"
#include "Concepts/RangeOf.h"
#include "Concepts/IOutput.h"

#include "Range/Operations.h"
#include "Range/Mutation/Copy.h"
#include "Range/Stream/OutputStreamMixin.h"

namespace Intra { namespace Range {

INTRA_WARNING_PUSH
INTRA_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_COPY_MOVE_CONSTRUCT_IMPLICITLY_DELETED
INTRA_WARNING_DISABLE_SIGN_CONVERSION

template<typename T> class OutputRange: public Meta::SelectType<
	OutputStreamMixin<OutputRange<T>, Meta::RemoveConst<T>>,
	Meta::EmptyType, Meta::IsTriviallySerializable<T>::_>
{
public:
	typedef IOutputStream<T> Interface;

	template<typename R> struct WrapperImpl: Interface
	{
		template<typename A> WrapperImpl(A&& range):
			OriginalRange(Cpp::Forward<A>(range)) {}

		bool Full() const final {return Range::EmptyOrFalse(OriginalRange);}
		void Put(const T& value) final {OriginalRange.Put(value);}

		void Put(T&& value) final {OriginalRange.Put(Cpp::Move(value));}

		bool TryPut(const T& value) final
		{
			if(Range::EmptyOrFalse(OriginalRange)) return false;
			OriginalRange.Put(value);
			return true;
		}

		bool TryPut(T&& value) final
		{
			if(Range::EmptyOrFalse(OriginalRange)) return false;
			OriginalRange.Put(Cpp::Move(value));
			return true;
		}

		size_t PutAllAdvance(CSpan<T>& src) final
		{return ReadToAdvance(src, OriginalRange);}

		R OriginalRange;
	};

private:
	template<typename R> using EnableCondition = Meta::EnableIf<
		Concepts::IsAsOutputRangeOf<R, T>::_ &&
		!Meta::IsInherited<R, OutputRange>::_
	>;

	template<typename R> forceinline static Interface* wrap(R&& range)
	{
		return new WrapperImpl<Concepts::RangeOfTypeNoCRef<R&&>>(RangeOf(Cpp::Forward<R>(range)));
	}

public:
	forceinline OutputRange(null_t=null): mInterface(null) {}

	forceinline OutputRange(OutputRange&& rhs):
		mInterface(Cpp::Move(rhs.mInterface)) {}

	forceinline OutputRange& operator=(OutputRange&& rhs)
	{
		mInterface = Cpp::Move(rhs.mInterface);
		return *this;
	}

	OutputRange(const OutputRange& rhs) = delete;

	OutputRange& operator=(const OutputRange& rhs) = delete;

	template<typename R, typename = EnableCondition<R>>
	forceinline OutputRange(R&& range):
		mInterface(wrap(Range::ForwardOutputOf<R, T>(range))) {}

	template<typename R, typename = EnableCondition<R>>
	forceinline OutputRange& operator=(R&& range)
	{
		mInterface = wrap(Range::ForwardOutputOf<R, T>(range));
		return *this;
	}

	forceinline OutputRange(InitializerList<T> arr):
		OutputRange(SpanOf(arr)) {}

	forceinline OutputRange& operator=(InitializerList<T> arr)
	{
		operator=(SpanOf(arr));
		return *this;
	}


	forceinline bool Full() const {return mInterface==null || mInterface->Full();}
	forceinline void Put(const T& value) {mInterface->Put(value);}
	forceinline void Put(T&& value) {mInterface->Put(Cpp::Move(value));}
	forceinline bool TryPut(const T& value) {return mInterface!=null && mInterface->TryPut(value);}
	forceinline bool TryPut(T&& value) {return mInterface!=null && mInterface->TryPut(Cpp::Move(value));}

	forceinline size_t PutAllAdvance(CSpan<T>& dst)
	{
		if(mInterface==null) return 0;
		return mInterface->PutAllAdvance(dst);
	}

	forceinline size_t PutAll(CSpan<T> dst) {return PutAllAdvance(dst);}

	template<typename AR> Meta::EnableIf<
		Concepts::IsArrayRangeOfExactly<AR, T>::_ &&
		!Meta::IsConst<AR>::_,
	size_t> PutAllAdvance(AR& src)
	{
		CSpan<T> srcArr = {src.Data(), src.Length()};
		size_t result = PutAllAdvance(srcArr);
		Range::PopFirstExactly(src, result);
		return result;
	}

protected:
	Unique<Interface> mInterface;
	OutputRange(Interface* interfacePtr): mInterface(interfacePtr) {}
};

typedef OutputRange<char> OutputStream;


INTRA_WARNING_POP

}

using Range::OutputRange;
using Range::OutputStream;

}
