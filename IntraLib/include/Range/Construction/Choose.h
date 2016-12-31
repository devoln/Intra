#pragma once

#include "Range/Concepts.h"

namespace Intra { namespace Range {

INTRA_WARNING_PUSH_DISABLE_COPY_MOVE_IMPLICITLY_DELETED

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
	{return mRange1.Empty()? mRange2.First(): mRange1.First();}

	forceinline void PopFirst()
	{
		if(!mRange1.Empty())
		{
			mRange1.PopFirst();
			return;
		}
		INTRA_ASSERT(!Empty());
		mRange2.PopFirst();
	}

	template<typename U=R1> forceinline Meta::EnableIf<
		HasLast<U>::_ && HasLast<R2>::_,
	ReturnValueType> Last() const
	{return mRange1.Empty()? mRange2.Last(): mRange1.Last();}

	template<typename U=R1> forceinline Meta::EnableIf<
		HasPopLast<U>::_ && HasPopLast<R2>::_
	> PopLast()
	{
		if(!mRange1.Empty())
		{
			mRange1.PopLast();
			return;
		}
		INTRA_ASSERT(!Empty());
		mRange2.PopLast();
	}

	template<typename U=R1> forceinline Meta::EnableIf<
		HasIndex<U>::_ && HasIndex<R2>::_,
	ReturnValueType> operator[](size_t index) const
	{return mRange1.Empty()? mRange2[index]: mRange1[index];}

	template<typename U=R1> forceinline Meta::EnableIf<
		HasSlicing<U>::_ && HasSlicing<R2>::_,
	RChoose> operator()(size_t start, size_t end) const
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

template<typename R1, typename R2> forceinline
RChoose<Meta::RemoveConstRef<R1>, Meta::RemoveConstRef<R2>> Choose(
	R1&& ifFalseRange, R2&& ifTrueRange, bool condition)
{return {Meta::Forward<R1>(ifFalseRange), Meta::Forward<R2>(ifTrueRange), condition};}

template<typename R1, typename T2, size_t N2> forceinline
RChoose<Meta::RemoveConstRef<R1>, AsRangeResult<T2(&)[N2]>> Choose(
	R1&& ifFalseRange, T2(&ifTrueRange)[N2], bool condition)
{return Choose(Meta::Forward<R1>(ifFalseRange), AsRange(ifTrueRange), condition);}

template<typename T1, size_t N1, typename R2> forceinline
RChoose<AsRangeResult<T1(&)[N1]>, Meta::RemoveConstRef<R2>> Choose(
	T1(&ifFalseRange)[N1], R2&& ifTrueRange, bool condition)
{return Choose(AsRange(ifFalseRange), Meta::Forward<R2>(ifTrueRange), condition);}

template<typename T1, size_t N1, typename T2, size_t N2> forceinline
RChoose<AsRangeResult<T1(&)[N1]>, AsRangeResult<T2(&)[N2]>> Choose(
	T1(&ifFalseRange)[N1], T2(&ifTrueRange)[N2], bool condition)
{return Choose(AsRange(ifFalseRange), AsRange(ifTrueRange), condition);}

}}
