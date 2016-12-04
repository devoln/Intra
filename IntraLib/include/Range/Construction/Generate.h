#pragma once

#include "Range/Mixins/RangeMixins.h"

namespace Intra { namespace Range {

template<typename F> struct GenerateResult:
	RangeMixin<GenerateResult<F>, Meta::RemoveConstRef<Meta::ResultOf<F>>, TypeEnum::Input, false>
{
	typedef Meta::RemoveConstRef<Meta::ResultOf<F>> value_type;
	typedef const value_type& return_value_type;

	GenerateResult(null_t=null): func(null) {}
	GenerateResult(F function): func(function) {front = func()();}

	forceinline bool Empty() const {return func==null;}
	forceinline return_value_type First() const {return front;}

	forceinline void PopFirst() {front = func()();}

	forceinline bool operator==(const GenerateResult& rhs) const
	{
		return func==null && rhs.func==null;
	}

private:
	Utils::Optional<F> func;
	value_type front;
};

template<typename F> GenerateResult<F> Generate(F&& func)
{
	return GenerateResult<F>(core::forward<F>(func));
}

}}
