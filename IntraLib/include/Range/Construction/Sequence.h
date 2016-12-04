#pragma once

#include "Range/Mixins/RangeMixins.h"
#include "Utils/Optional.h"

namespace Intra { namespace Range {

template<typename T, typename F> struct SequenceResult:
	Range::RangeMixin<SequenceResult<T, F>, T, Range::TypeEnum::RandomAccess, false>
{
	typedef T value_type;
	typedef T return_value_type;

	forceinline SequenceResult(null_t=null): Function(), Offset(0) {}
	forceinline SequenceResult(F function, size_t offset=0): Function(function), Offset(offset) {}

	forceinline T First() const {return Function()(Offset);}
	forceinline void PopFirst() {Offset++;}
	forceinline bool Empty() const {return false;}
	forceinline T operator[](size_t index) const {return Function()(Offset+index);}

	forceinline bool operator==(const SequenceResult<T, F>& rhs) const {return Offset==rhs.Offset;}

	forceinline Range::TakeResult<SequenceResult<T, F>> opSlice(size_t start, size_t end) const
	{
		auto result = *this;
		result.Offset += start;
		return result.Take(end-start);
	}

	Utils::Optional<F> Function;
	size_t Offset;
};

template<typename F> forceinline SequenceResult<Meta::ResultOf<F, size_t>, F> Sequence(F function)
{
	return SequenceResult<Meta::ResultOf<F, size_t>, F>(function);
}

}}
