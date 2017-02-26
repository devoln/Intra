#pragma once

#include "Range/Concepts.h"
#include "Range/Decorators/Take.h"
#include "Utils/Optional.h"

namespace Intra { namespace Range {

INTRA_WARNING_PUSH
INTRA_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED

template<typename T, typename F> struct RSequence
{
	enum: bool {RangeIsInfinite = true};

	forceinline RSequence(null_t=null): Function(), Offset(0) {}
	forceinline RSequence(F function, size_t offset=0): Function(function), Offset(offset) {}

	forceinline T First() const {return Function()(Offset);}
	forceinline void PopFirst() {Offset++;}
	forceinline bool Empty() const {return false;}
	forceinline T operator[](size_t index) const {return Function()(Offset+index);}

	forceinline bool operator==(const RSequence<T, F>& rhs) const {return Offset==rhs.Offset;}

	forceinline RTake<RSequence<T, F>> operator()(size_t start, size_t end) const
	{
		INTRA_DEBUG_ASSERT(start<=end);
		auto result = *this;
		result.Offset += start;
		return RTake<RSequence<T, F>>(Meta::Move(result), end-start);
	}

	Utils::Optional<F> Function;
	size_t Offset;
};

INTRA_WARNING_POP

template<typename F> forceinline
RSequence<Meta::ResultOf<Meta::RemoveConstRef<F>, size_t>, Meta::RemoveConstRef<F>> Sequence(F&& function)
{return {Meta::Forward<F>(function)};}

}}
