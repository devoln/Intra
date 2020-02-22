#pragma once

#include "Core/Range/Concepts.h"


INTRA_BEGIN
INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED

//TODO: reimplement with Variant
#if INTRA_DISABLED
template<typename R1, typename R2> struct RChoose
{
private:
	typedef TCommonRef<TReturnValueTypeOf<R1>, TReturnValueTypeOf<R2>> ReturnValueType;
public:
	enum: bool {RangeIsFinite = CommonRangeCategoryAllFinite<R1, R2>::Finite};

	RChoose(R1&& ifFalseRange, R2&& ifTrueRange, bool condition):
		mRange1(condition? null: Move(ifFalseRange)),
		mRange2(condition? Move(ifTrueRange): null) {}

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

private:
	R1 mRange1;
	R2 mRange2;
};
#endif

#if INTRA_DISABLED
template<typename R1, typename R2> forceinline Requires<
	!CSameIgnoreCVRef<TRangeOfType<R1>, TRangeOfType<R2>>,
RChoose<TRangeOfTypeNoCRef<R1>, TRangeOfTypeNoCRef<R2>>>
	Choose(R1&& ifFalseRange, R2&& ifTrueRange, bool condition)
{return {ForwardAsRange<R1>(ifFalseRange), ForwardAsRange<R2>(ifTrueRange), condition};}

template<typename R1, typename R2> forceinline Requires<
	CSameIgnoreCVRef<TRangeOfType<R1>, TRangeOfType<R2>>,
TRangeOfType<R1&&>> Choose(R1&& ifFalseRange, R2&& ifTrueRange, bool condition)
{return condition? ForwardAsRange<R2>(ifTrueRange): ForwardAsRange<R1>(ifFalseRange);}
#endif
INTRA_END
