#pragma once

#include "Intra/Functional.h"
#include "Intra/Range.h"
#include "Intra/Range/Stream/OutputStreamMixin.h"

#include "IntraX/Utils/Unique.h"
#include "Intra/Range/Polymorphic/IOutput.h"


namespace Intra { INTRA_BEGIN
INTRA_IGNORE_WARN_COPY_MOVE_CONSTRUCT_IMPLICITLY_DELETED
INTRA_IGNORE_WARN_SIGN_CONVERSION

template<typename T, typename R> class OutputRangePolymorphicWrapper: public IOutput<T>
{
public:
	template<typename A> OutputRangePolymorphicWrapper(A&& range):
		OriginalRange(INTRA_FWD(range)) {}

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

	size_t PutAllAdvance(Span<const T>& src) final
	{return ReadWrite(src, OriginalRange);}

	R OriginalRange;
};

template<typename T, typename R> inline IOutput<T>* WrapOutputRange(R&& range)
{
	return new OutputRangePolymorphicWrapper<T, TRangeOf<R&&>>(RangeOf(INTRA_FWD(range)));
}

template<typename T> class OutputRange: public TSelect<
	OutputStreamMixin<OutputRange<T>, TRemoveConst<T>>,
	EmptyType, CChar<T>>
{
public:
	OutputRange() = default;

	constexpr OutputRange(Unique<IOutputEx<T>> stream): Stream(INTRA_MOVE(stream)) {}

	constexpr OutputRange(OutputRange&&) = default;
	constexpr OutputRange& operator=(OutputRange&&) = default;
	OutputRange(const OutputRange&) = delete;
	OutputRange& operator=(const OutputRange&) = delete;

	template<typename R> requires CAsOutputRangeOf<R, T> && !CDerived<R, OutputRange>
	OutputRange(R&& range): Stream(WrapOutputRange<T>(ForwardAsOutputRangeOf<R, T>(range))) {}

	template<typename R> requires CAsOutputRangeOf<R, T> && !CDerived<R, OutputRange>
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


	bool Full() const {return !Stream || Stream->Full();}
	void Put(const T& value) {Stream->Put(value);}
	void Put(T&& value) {Stream->Put(Move(value));}
	bool TryPut(const T& value) {return Stream != nullptr && Stream->TryPut(value);}
	bool TryPut(T&& value) {return Stream != nullptr && Stream->TryPut(Move(value));}

	index_t PutAllAdvance(Span<const T>& dst)
	{
		if(Stream == nullptr) return 0;
		return Stream->PutAllAdvance(dst);
	}

	index_t PutAll(Span<const T> dst) {return PutAllAdvance(dst);}

	template<typename AR> requires CArrayRangeOfExactly<AR, T> && (!CConst<AR>)
	index_t PutAllAdvance(AR& src)
	{
		Span<const T> srcArr = CSpanOf(src);
		const auto result = PutAllAdvance(srcArr);
		PopFirstExactly(src, result);
		return result;
	}

	Unique<IOutputEx<T>> Stream;
};

using OutputStream = OutputRange<char>;
} INTRA_END
