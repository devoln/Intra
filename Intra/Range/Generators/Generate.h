#pragma once

#include "Concepts/Range.h"

INTRA_WARNING_PUSH
INTRA_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED

namespace Intra { namespace Range {

template<typename F> struct RGenerate
{
private:
	typedef Meta::RemoveConstRef<Meta::ResultOf<F>> value_type;
	typedef const value_type& return_value_type;
public:
	enum: bool {RangeIsInfinite = true};

	forceinline RGenerate(null_t=null): mFunc(null) {}
	forceinline RGenerate(F function): mFunc(function), mFront(mFunc()()) {}

	RGenerate(const RGenerate&) = delete;
	RGenerate& operator=(const RGenerate&) = delete;
	RGenerate(RGenerate&& rhs) = default;
	RGenerate& operator=(RGenerate&& rhs) = default;

	forceinline bool Empty() const {return mFunc==null;}
	forceinline return_value_type First() const {return mFront;}

	forceinline void PopFirst() {mFront = mFunc()();}

private:
	Utils::Optional<F> mFunc;
	value_type mFront;
};

template<typename F> forceinline RGenerate<F> Generate(F func)
{return Cpp::Move(func);}

}}

INTRA_WARNING_POP
