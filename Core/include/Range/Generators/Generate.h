#pragma once

#include "Range/Concepts.h"

namespace Intra { namespace Range {

INTRA_WARNING_PUSH
INTRA_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED

template<typename F> struct RGenerate
{
private:
	typedef Meta::RemoveConstRef<Meta::ResultOf<F>> value_type;
	typedef const value_type& return_value_type;
public:
	enum: bool {RangeIsInfinite = true};

	RGenerate(null_t=null): mFunc(null) {}
	RGenerate(F function): mFunc(function), mFront(mFunc()()) {}

	RGenerate(const RGenerate&) = delete;
	RGenerate& operator=(const RGenerate&) = delete;

	RGenerate(RGenerate&& rhs):
		mFunc(Meta::Move(rhs.mFunc)), mFront(Meta::Move(rhs.mFront)) {}

	RGenerate& operator=(RGenerate&& rhs)
	{
		mFunc = Meta::Move(rhs.mFunc);
		mFront = Meta::Move(rhs.mFront);
		return *this;
	}

	forceinline bool Empty() const {return mFunc==null;}
	forceinline return_value_type First() const {return mFront;}

	forceinline void PopFirst() {mFront = mFunc()();}

	forceinline bool operator==(const RGenerate& rhs) const
	{return mFunc==null && rhs.mFunc==null;}

private:
	Utils::Optional<F> mFunc;
	value_type mFront;
};

INTRA_WARNING_POP

template<typename F> RGenerate<F> Generate(F func)
{return Meta::Move(func);}

}}
