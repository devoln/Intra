#pragma once

//TODO: move to Utils

#include "Core/Functional.h"
#include "Core/Range/Concepts.h"
#include "Core/Range/Span.h"
#include "Core/Range/Operations.h"
#include "Core/Range/Mutation/Copy.h"
#include "Core/Range/Stream/OutputStreamMixin.h"

#include "Utils/Unique.h"
#include "Utils/IOutput.h"


INTRA_BEGIN
INTRA_WARNING_DISABLE_COPY_MOVE_CONSTRUCT_IMPLICITLY_DELETED
INTRA_WARNING_DISABLE_SIGN_CONVERSION

template<typename T, typename R> class OutputRangePolymorphicWrapper: public IOutputEx<T>
{
public:
	template<typename A> forceinline OutputRangePolymorphicWrapper(A&& range):
		OriginalRange(Forward<A>(range)) {}

	bool Full() const final {return EmptyOr(OriginalRange);}
	void Put(const T& value) final {OriginalRange.Put(value);}

	void Put(T&& value) final {OriginalRange.Put(Move(value));}

	bool TryPut(const T& value) final
	{
		if(EmptyOr(OriginalRange)) return false;
		OriginalRange.Put(value);
		return true;
	}

	bool TryPut(T&& value) final
	{
		if(EmptyOr(OriginalRange)) return false;
		OriginalRange.Put(Move(value));
		return true;
	}

	size_t PutAllAdvance(CSpan<T>& src) final
	{return ReadWrite(src, OriginalRange);}

	R OriginalRange;
};

template<typename T, typename R> forceinline IOutputEx<T>* WrapOutputRange(R&& range)
{
	return new OutputRangePolymorphicWrapper<T, TRangeOfTypeNoCRef<R&&>>(RangeOf(Forward<R>(range)));
}

template<typename T> class OutputRange: public TSelect<
	OutputStreamMixin<OutputRange<T>, TRemoveConst<T>>,
	EmptyType, CPod<T>>
{
	template<typename R> using EnableCondition = Requires<
		CAsOutputRangeOf<R, T> &&
		!CDerived<R, OutputRange>
	>;

public:
	constexpr forceinline OutputRange(null_t=null) {}

	constexpr forceinline OutputRange(Unique<IOutputEx<T>> stream): Stream(Move(stream)) {}

	constexpr forceinline OutputRange(OutputRange&& rhs):
		Stream(Move(rhs.Stream)) {}

	constexpr forceinline OutputRange& operator=(OutputRange&& rhs)
	{
		Stream = Move(rhs.Stream);
		return *this;
	}

	OutputRange(const OutputRange& rhs) = delete;

	OutputRange& operator=(const OutputRange& rhs) = delete;

	template<typename R, typename = EnableCondition<R>>
	forceinline OutputRange(R&& range):
		Stream(WrapOutputRange<T>(ForwardAsOutputRangeOf<R, T>(range))) {}

	template<typename R, typename = EnableCondition<R>>
	forceinline OutputRange& operator=(R&& range)
	{
		Stream = WrapOutputRange<T>(ForwardAsOutputRangeOf<R, T>(range));
		return *this;
	}

	forceinline OutputRange(InitializerList<T> arr):
		OutputRange(SpanOf(arr)) {}

	forceinline OutputRange& operator=(InitializerList<T> arr)
	{
		operator=(SpanOf(arr));
		return *this;
	}


	forceinline bool Full() const {return Stream == null || Stream->Full();}
	forceinline void Put(const T& value) {Stream->Put(value);}
	forceinline void Put(T&& value) {Stream->Put(Move(value));}
	forceinline bool TryPut(const T& value) {return Stream != null && Stream->TryPut(value);}
	forceinline bool TryPut(T&& value) {return Stream != null && Stream->TryPut(Move(value));}

	forceinline size_t PutAllAdvance(CSpan<T>& dst)
	{
		if(Stream == null) return 0;
		return Stream->PutAllAdvance(dst);
	}

	forceinline size_t PutAll(CSpan<T> dst) {return PutAllAdvance(dst);}

	template<typename AR> Requires<
		CArrayRangeOfExactly<AR, T> &&
		!CConst<AR>,
	size_t> PutAllAdvance(AR& src)
	{
		CSpan<T> srcArr = {src.Data(), src.Length()};
		const size_t result = PutAllAdvance(srcArr);
		PopFirstExactly(src, result);
		return result;
	}

	Unique<IOutputEx<T>> Stream;
};

typedef OutputRange<char> OutputStream;
INTRA_END
