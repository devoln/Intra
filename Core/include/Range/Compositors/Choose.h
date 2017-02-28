#pragma once

#include "Range/Concepts.h"
#include "Range/AsRange.h"

namespace Intra { namespace Range {

INTRA_WARNING_PUSH
INTRA_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED

template<typename R1, typename R2> struct RChoose
{
private:
	typedef Meta::CommonTypeRef<ReturnValueTypeOf<R1>, ReturnValueTypeOf<R2>> ReturnValueType;
public:
	enum: bool {RangeIsFinite = CommonRangeCategoryAllFinite<R1, R2>::Finite};

	RChoose(null_t=null): mRange1(null), mRange2(null) {}

	RChoose(R1&& ifFalseRange, R2&& ifTrueRange, bool condition):
		mRange1(condition? null: Meta::Move(ifFalseRange)),
		mRange2(condition? Meta::Move(ifTrueRange): null) {}

	RChoose(const R1& ifFalseRange, const R2& ifTrueRange, bool condition):
		mRange1(condition? null: ifFalseRange),
		mRange2(condition? ifTrueRange: null) {}

	forceinline bool Empty() const
	{return mRange1.Empty() && mRange2.Empty();}

	forceinline ReturnValueType First() const
	{return mRange1.Empty()? ReturnValueType(mRange2.First()): ReturnValueType(mRange1.First());}

	forceinline void PopFirst()
	{
		if(!mRange1.Empty())
		{
			mRange1.PopFirst();
			return;
		}
		INTRA_DEBUG_ASSERT(!Empty());
		mRange2.PopFirst();
	}

	forceinline ReturnValueType Last() const
	{return mRange1.Empty()? mRange2.Last(): mRange1.Last();}

	forceinline void PopLast()
	{
		if(!mRange1.Empty())
		{
			mRange1.PopLast();
			return;
		}
		INTRA_DEBUG_ASSERT(!Empty());
		mRange2.PopLast();
	}

	forceinline ReturnValueType operator[](size_t index) const
	{return mRange1.Empty()? mRange2[index]: mRange1[index];}

	forceinline RChoose operator()(size_t start, size_t end) const
	{
		return {mRange1.Empty()? null: mRange1(start, end),
			    mRange1.Empty()? null: mRange2(start, end)};
	}

	forceinline bool operator==(const RChoose& rhs) const
	{return mRange1==rhs.mRange1 && mRange2==rhs.mRange2;}

private:
	R1 mRange1;
	R2 mRange2;
};

INTRA_WARNING_POP

template<typename R1, typename R2> forceinline Meta::EnableIf<
	!Meta::TypeEqualsIgnoreCVRef<AsRangeResult<R1>, AsRangeResult<R2>>::_,
RChoose<AsRangeResultNoCRef<R1>, AsRangeResultNoCRef<R2>>>
	Choose(R1&& ifFalseRange, R2&& ifTrueRange, bool condition)
{return {Range::Forward<R1>(ifFalseRange), Range::Forward<R2>(ifTrueRange), condition};}

template<typename R1, typename R2> forceinline Meta::EnableIf<
	Meta::TypeEqualsIgnoreCVRef<AsRangeResult<R1>, AsRangeResult<R2>>::_,
AsRangeResult<R1&&>> Choose(R1&& ifFalseRange, R2&& ifTrueRange, bool condition)
{return condition? Range::Forward<R2>(ifTrueRange): Range::Forward<R1>(ifFalseRange);}

}}
