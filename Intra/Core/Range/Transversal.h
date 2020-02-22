#pragma once




INTRA_BEGIN
INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED
template<typename Rs> struct RFirstTransversal
{
	enum: bool {
		RangeIsFinite = CFiniteRange<Rs>
	};

	forceinline RFirstTransversal() = default;

	constexpr forceinline RFirstTransversal(Rs rangeOfRanges):
		mRanges(Move(rangeOfRanges)) {skip_empty();}

	INTRA_NODISCARD constexpr forceinline bool Empty() const {return mRanges.Empty();}

	INTRA_NODISCARD constexpr forceinline decltype(auto) First() const {return mRanges.First().First();}
	INTRA_NODISCARD constexpr forceinline decltype(auto) FirstRange() const {return mRanges.First();}

	constexpr forceinline void PopFirst()
	{
		mRanges.PopFirst();
		skip_empty();
	}

	INTRA_NODISCARD constexpr auto Last() const
	{
		while(mRanges.Last().Empty()) mRanges.PopLast();
		return mRanges.Last().First();
	}

	constexpr void PopLast()
	{
		INTRA_DEBUG_ASSERT(!Empty());
		while(mRanges.Last().Empty()) mRanges.PopLast();
		mRanges.PopLast();
	}

private:
	mutable Rs mRanges;

	constexpr void skip_empty() {while(!Empty() && mRanges.First().Empty()) mRanges.PopFirst();}
};

template<typename R,
	typename AsR = TRangeOfType<R&&>
> INTRA_NODISCARD constexpr forceinline Requires<
	CAccessibleRange<AsR> &&
	CAsInputRange<TValueTypeOf<AsR>>,
RFirstTransversal<TRemoveConstRef<AsR>>> FirstTransversal(R&& range)
{return ForwardAsRange<R>(range);}
INTRA_END
