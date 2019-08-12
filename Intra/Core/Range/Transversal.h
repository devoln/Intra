#pragma once




INTRA_CORE_RANGE_BEGIN
INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED
template<typename Rs> struct RFirstTransversal
{
	enum: bool {
		RangeIsFinite = CFiniteRange<Rs>
	};

	forceinline RFirstTransversal() = default;

	INTRA_CONSTEXPR2 forceinline RFirstTransversal(Rs rangeOfRanges):
		mRanges(Move(rangeOfRanges)) {skip_empty();}

	INTRA_NODISCARD constexpr forceinline bool Empty() const {return mRanges.Empty();}

	INTRA_NODISCARD constexpr forceinline decltype(auto) First() const {return mRanges.First().First();}
	INTRA_NODISCARD constexpr forceinline decltype(auto) FirstRange() const {return mRanges.First();}

	INTRA_CONSTEXPR2 forceinline void PopFirst()
	{
		mRanges.PopFirst();
		skip_empty();
	}

	INTRA_NODISCARD INTRA_CONSTEXPR2 auto Last() const
	{
		while(mRanges.Last().Empty()) mRanges.PopLast();
		return mRanges.Last().First();
	}

	INTRA_CONSTEXPR2 void PopLast()
	{
		INTRA_DEBUG_ASSERT(!Empty());
		while(mRanges.Last().Empty()) mRanges.PopLast();
		mRanges.PopLast();
	}

private:
	mutable Rs mRanges;

	INTRA_CONSTEXPR2 void skip_empty() {while(!Empty() && mRanges.First().Empty()) mRanges.PopFirst();}
};

template<typename R,
	typename AsR = TRangeOfType<R&&>
> INTRA_NODISCARD constexpr forceinline Requires<
	CAccessibleRange<AsR> &&
	CAsInputRange<TValueTypeOf<AsR>>,
RFirstTransversal<TRemoveConstRef<AsR>>> FirstTransversal(R&& range)
{return ForwardAsRange<R>(range);}
INTRA_CORE_RANGE_END
