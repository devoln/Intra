#pragma once

#include "Range/ForwardDecls.h"
#include "Concepts/RangeOf.h"

namespace Intra { namespace Range {

INTRA_WARNING_PUSH
INTRA_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED

template<typename Rs> struct RFirstTransversal
{
	enum: bool {
		RangeIsFinite = Concepts::IsFiniteRange<Rs>::_
	};
	typedef Concepts::ReturnValueTypeOf<Concepts::ValueTypeOf<Rs>> return_value_type;

	forceinline RFirstTransversal(null_t=null) {}

	forceinline RFirstTransversal(Rs&& rangeOfRanges):
		mRanges(Cpp::Move(rangeOfRanges)) {skip_empty();}

	forceinline RFirstTransversal(const Rs& rangeOfRanges):
		mRanges(rangeOfRanges) {skip_empty();}

	forceinline bool Empty() const {return mRanges.Empty();}

	forceinline return_value_type First() const {return mRanges.First().First();}
	forceinline Concepts::ReturnValueTypeOf<Rs> FirstRange() const {return mRanges.First();}

	forceinline void PopFirst()
	{
		mRanges.PopFirst();
		skip_empty();
	}

	return_value_type Last() const
	{
		while(mRanges.Last().Empty()) mRanges.PopLast();
		return mRanges.Last().First();
	}

	void PopLast()
	{
		INTRA_DEBUG_ASSERT(!Empty());
		while(mRanges.Last().Empty()) mRanges.PopLast();
		mRanges.PopLast();
	}

	forceinline bool operator==(const RFirstTransversal& rhs) const {return mRanges==rhs.mRanges;}

private:
	Rs mRanges;

	void skip_empty() {while(!Empty() && mRanges.First().Empty()) mRanges.PopFirst();}
};

INTRA_WARNING_POP

template<typename R,
	typename AsR = Concepts::RangeOfType<R&&>
> forceinline Meta::EnableIf<
	Concepts::IsAccessibleRange<AsR>::_ &&
	Concepts::IsAsInputRange<Concepts::ValueTypeOf<AsR>>::_,
RFirstTransversal<Meta::RemoveConstRef<AsR>>> FirstTransversal(R&& range)
{return Range::Forward<R>(range);}

}}
