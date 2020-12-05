#pragma once

#include "Intra/Functional.h"
#include "Intra/Range/Concepts.h"
#include "Intra/Range/Span.h"
#include "Intra/Range/Operations.h"
#include "Intra/Range/Mutation/Copy.h"
#include "Intra/Range/Stream/OutputStreamMixin.h"

#include "IntraX/Utils/Unique.h"
#include "Intra/Range/Polymorphic/IOutput.h"


INTRA_BEGIN
INTRA_IGNORE_WARN_COPY_MOVE_CONSTRUCT_IMPLICITLY_DELETED
INTRA_IGNORE_WARN_SIGN_CONVERSION

template<typename T, typename R> class OutputRangePolymorphicWrapper: public IOutputEx<T>
{
public:
	template<typename A> OutputRangePolymorphicWrapper(A&& range):
		OriginalRange(Forward<A>(range)) {}

	bool Full() const final {return EmptyOpt(OriginalRange).GetOr(false);}
	void Put(const T& value) final {OriginalRange.Put(value);}

	void Put(T&& value) final {OriginalRange.Put(Move(value));}

	bool TryPut(const T& value) final
	{
		if(EmptyOpt(OriginalRange).GetOr(false)) return false;
		OriginalRange.Put(value);
		return true;
	}

	bool TryPut(T&& value) final
	{
		if(EmptyOpt(OriginalRange).GetOr(false)) return false;
		OriginalRange.Put(Move(value));
		return true;
	}

	size_t PutAllAdvance(CSpan<T>& src) final
	{return ReadWrite(src, OriginalRange);}

	R OriginalRange;
};

template<typename T, typename R> inline IOutputEx<T>* WrapOutputRange(R&& range)
{
	return new OutputRangePolymorphicWrapper<T, TRangeOf<R&&>>(RangeOf(Forward<R>(range)));
}

template<typename T> class OutputRange: public TSelect<
	OutputStreamMixin<OutputRange<T>, TRemoveConst<T>>,
	EmptyType, CChar<T>>
{
	template<typename R> using EnableCondition = Requires<
		CAsOutputRangeOf<R, T> &&
		!CDerived<R, OutputRange>
	>;

public:
	constexpr OutputRange(decltype(null)=null) {}

	constexpr OutputRange(Unique<IOutputEx<T>> stream): Stream(Move(stream)) {}

	constexpr OutputRange(OutputRange&& rhs):
		Stream(Move(rhs.Stream)) {}

	constexpr OutputRange& operator=(OutputRange&& rhs)
	{
		Stream = Move(rhs.Stream);
		return *this;
	}

	OutputRange(const OutputRange& rhs) = delete;

	OutputRange& operator=(const OutputRange& rhs) = delete;

	template<typename R, typename = EnableCondition<R>>
	OutputRange(R&& range):
		Stream(WrapOutputRange<T>(ForwardAsOutputRangeOf<R, T>(range))) {}

	template<typename R, typename = EnableCondition<R>>
	OutputRange& operator=(R&& range)
	{
		Stream = WrapOutputRange<T>(ForwardAsOutputRangeOf<R, T>(range));
		return *this;
	}

	OutputRange(InitializerList<T> arr):
		OutputRange(SpanOf(arr)) {}

	OutputRange& operator=(InitializerList<T> arr)
	{
		operator=(SpanOf(arr));
		return *this;
	}


	bool Full() const {return Stream == null || Stream->Full();}
	void Put(const T& value) {Stream->Put(value);}
	void Put(T&& value) {Stream->Put(Move(value));}
	bool TryPut(const T& value) {return Stream != null && Stream->TryPut(value);}
	bool TryPut(T&& value) {return Stream != null && Stream->TryPut(Move(value));}

	index_t PutAllAdvance(CSpan<T>& dst)
	{
		if(Stream == null) return 0;
		return Stream->PutAllAdvance(dst);
	}

	index_t PutAll(CSpan<T> dst) {return PutAllAdvance(dst);}

	template<typename AR> Requires<
		CArrayRangeOfExactly<AR, T> &&
		!CConst<AR>,
	index_t> PutAllAdvance(AR& src)
	{
		CSpan<T> srcArr = CSpanOf(src);
		const auto result = PutAllAdvance(srcArr);
		PopFirstExactly(src, result);
		return result;
	}

	Unique<IOutputEx<T>> Stream;
};

using OutputStream = OutputRange<char>;
INTRA_END
