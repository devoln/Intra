#pragma once

#include "Intra/Assert.h"
#include "Intra/Range/Concepts.h"
#include "Intra/Range/Operations.h"
#include "Intra/TypeSafe.h"

INTRA_BEGIN
INTRA_IGNORE_WARN_COPY_MOVE_CONSTRUCT_IMPLICITLY_DELETED

template<class T> concept CHasTakeMethod = requires(T x) {x.Take(index_t());};

template<CRange R, bool Exactly> struct RTake: CopyableIf<!CReference<R>>
{
	using TagAnyInstanceFinite = TTag<>;

	constexpr RTake() = default;

	template<CList L> constexpr RTake(L&& list, ClampedSize count):
		mOriginalRange(ForwardAsRange<L>(list)), mLen(size_t(count))
	{
		if constexpr(CHasLength<R>)
		{
			if constexpr(Exactly) INTRA_PRECONDITION(count <= mOriginalRange.Length());
			else mLen = Min(mLen, size_t(mOriginalRange.Length()));
		}
	}

	/*RTake(RTake&&) = default;
	RTake(const RTake&) = default;
	RTake& operator=(RTake&&) = default;
	RTake& operator=(const RTake&) = default;*/

	[[nodiscard]] constexpr bool Empty() const
	{
		if constexpr(Exactly || CHasLength<R> || CInfiniteRange<R>)
			return mLen == 0;
		else return mLen == 0 || mOriginalRange.Empty();
	}


	[[nodiscard]] constexpr TRangeValueRef<R> First() const
	{
		INTRA_PRECONDITION(!Empty());
		return mOriginalRange.First();
	}

	constexpr void PopFirst()
	{
		INTRA_PRECONDITION(!Empty());
		mOriginalRange.PopFirst();
		mLen--;
	}
	
	[[nodiscard]] constexpr TRangeValueRef<R> Last() const requires CHasIndex<R>
	{
		INTRA_PRECONDITION(!Empty());
		return mOriginalRange[mLen-1];
	}

	constexpr void PopLast() requires CHasIndex<R>
	{
		INTRA_PRECONDITION(!Empty());
		mLen--;
	}

	[[nodiscard]] constexpr TRangeValueRef<R> operator[](Index index) const requires CHasIndex<R>
	{
		INTRA_PRECONDITION(size_t(index) < mLen);
		return mOriginalRange[index];
	}

	constexpr index_t Length() const noexcept requires CHasLength<R> || CInfiniteRange<R> {return index_t(mLen);}

	[[nodiscard]] constexpr index_t LengthLimit() const noexcept {return index_t(mLen);}
	
	[[nodiscard]] constexpr RTake Take(Size count) const
	{
		return RTake(mOriginalRange, Min(size_t(count), mLen));
	}

private:
	R mOriginalRange;
	size_t mLen = 0;
};

constexpr auto Take = [](ClampedSize maxCount) noexcept {
	return [maxCount]<typename R>(R&& r) requires(CList<R> || CAdvance<R>) {
		if constexpr(CAdvance<R>)
		{
			if constexpr(CHasTakeMethod<decltype(r.RangeRef)> && CHasPopFirstCount<decltype(r.RangeRef)>)
			{
				auto res = r.RangeRef.Take(maxCount);
				r.RangeRef.PopFirstCount(maxCount);
				return res;
			}
			else return RTake<decltype(r.RangeRef), false>(r.RangeRef, maxCount);
		}
		else if constexpr(CHasTakeMethod<TRangeOfRef<R>>) return ForwardAsRange<R>(r).Take(maxCount);
		else return RTake<TRangeOf<R>, false>(ForwardAsRange<R>(r), maxCount);
	};
};

constexpr auto TakeExactly = [](Size numElementsToTake) noexcept {
	return [numElementsToTake]<typename R>(R&& r) requires(CList<R> || CAdvance<R>) {
		if constexpr(CAdvance<R>)
		{
			if constexpr(CHasTakeMethod<decltype(r.RangeRef)> && CHasPopFirstCount<decltype(r.RangeRef)>)
			{
				auto res = r.RangeRef.Take(numElementsToTake);
				auto numElementsTaken = r.RangeRef.PopFirstCount(numElementsToTake);
				INTRA_DEBUG_ASSERT(numElementsTaken == numElementsToTake);
				return res;
			}
			else return RTake<decltype(r.RangeRef), false>(r.RangeRef, numElementsToTake);
		}
		else if constexpr(CHasTakeMethod<TRangeOfRef<R>>)
		{
			if constexpr(CHasLength<TRangeOf<R>>) INTRA_PRECONDITION(numElementsToTake <= RangeOf(r).Length());
			return ForwardAsRange<R>(r).Take(numElementsToTake);
		}
		else return RTake<TRangeOf<R>, true>(ForwardAsRange<R>(r), numElementsToTake);
	};
};

INTRA_DEFINE_SAFE_DECLTYPE(TTakeResult, Take(0)(Val<T>()));
INTRA_DEFINE_SAFE_DECLTYPE(TTakeExactlyResult, TakeExactly(0)(Val<T>()));


template<class R, class P, bool TestSubranges> class RTakeUntil
{
	R mRange;
	[[no_unique_address]] P mPred;
	bool mEmpty = false;
public:
	constexpr RTakeUntil(R range, P pred): mPred(Move(pred)), mRange(Move(range)) {}

	[[nodiscard]] constexpr decltype(auto) First() const
	{
		INTRA_PRECONDITION(!Empty());
		return mRange.First();
	}

	constexpr void PopFirst()
	{
		mRange.PopFirst();
		mEmpty = mRange.Empty();
		if(mEmpty) return;
		if constexpr(TestSubranges) mEmpty = mPred(mRange);
		else mEmpty = mPred(mRange.First());
	}

	[[nodiscard]] constexpr bool Empty() const noexcept {return mEmpty;}

	[[nodiscard]] constexpr decltype(auto) Next()
	{
		INTRA_PRECONDITION(!Empty());
		decltype(auto) res = mRange|Intra::Next;
		if constexpr(CCallable<P, TRangeValue<R>>)
			mEmpty = mRange.Empty() || mPred(res);
		return res;
	}
};

constexpr auto TakeUntil = [](auto&& pred) {
	return [pred = ForwardAsFunc<decltype(pred)>(pred)]<CList R>(R&& r) {
		return RTakeUntil(ForwardAsRange<R>(r), pred);
	};
};

constexpr auto TakeUntilEagerly = [](auto&& pred) {
	return [pred = ForwardAsFunc<decltype(pred)>(pred)]<CForwardList R>(R&& r) {
		return r|Take(r|TakeUntil(pred)|Count);
	};
};

INTRA_IGNORE_WARN_ASSIGN_IMPLICITLY_DELETED
template<typename R> struct RCycle
{
	static_assert(CForwardRange<R> && !CInfiniteRange<R>);
	using TagAnyInstanceInfinite = TTag<>;

	constexpr RCycle(R range) requires !CRandomAccessRange<R>:
		mOriginalRange(Move(range)), mOffsetRange(mOriginalRange) {INTRA_PRECONDITION(!mOriginalRange.Empty());}

	constexpr RCycle(R range) requires CRandomAccessRange<R>:
		mOriginalRange(Move(range)) {INTRA_PRECONDITION(!mOriginalRange.Empty());}

	[[nodiscard]] constexpr bool Empty() const {return false;}

	[[nodiscard]] constexpr const decltype(auto) First() const
	{
		if constexpr(CRandomAccessRange<R>) return mOriginalRange[mOffsetRange.Counter];
		else return mOffsetRange.First();
	}

	constexpr void PopFirst()
	{
		mOffsetRange.PopFirst();
		if constexpr(CRandomAccessRange<R>)
		{
			if constexpr(CHasLength<R>)
				if(mOffsetRange.Counter == size_t(mOriginalRange.Length())) mOffsetRange.Counter = 0;
		}
		else if(mOffsetRange.Empty()) mOffsetRange = mOriginalRange;
	}

	constexpr index_t PopFirstCount(ClampedSize elementsToPop) const requires CRandomAccessRange<R> || CHasPopFirstCount<R> || CHasLength<R>
	{
		if constexpr(CRandomAccessRange<R>)
		{
			mOffsetRange.Counter += size_t(elementsToPop);
			mOffsetRange.Counter %= size_t(mOriginalRange.Length());
		}
		else
		{
			auto leftToPop = size_t(elementsToPop);
			if constexpr(CHasLength<R>)
			{
				leftToPop %= mOriginalRange.Length();
				const auto offLen = size_t(mOffsetRange.Length());
				if(offLen < leftToPop)
				{
					mOffsetRange = mOriginalRange;
					leftToPop -= offLen;
				}
				mOffsetRange|PopFirstExactly(leftToPop);
			}
			else while(leftToPop)
			{
				leftToPop -= mOffsetRange|Intra::PopFirstCount(leftToPop);
				if(mOffsetRange.Empty()) mOffsetRange = mOriginalRange;
			}
		}
		return index_t(elementsToPop);
	}

	[[nodiscard]] constexpr decltype(auto) operator[](Index index) const requires CRandomAccessRange<R>
	{
		size_t i = mOffsetRange.Counter + size_t(index);
		i %= size_t(mOriginalRange.Length());
		return mOriginalRange[i];
	}

private:
	R mOriginalRange;
	TSelect<RCounter<int, size_t>, R, CRandomAccessRange<R>> mOffsetRange;
};

constexpr auto Cycle = []<CList R>(R&& r) requires(CForwardList<R> || CInfiniteList<R>) {
	if constexpr(CInfiniteList<R>) return ForwardAsRange<R>(r);
	else return RCycle(ForwardAsRange<R>(r));
};

template<typename R, typename F> struct RMap
{
	using TagAnyInstanceFinite = TTag<CFiniteRange<R>>;
	using TagAnyInstanceInfinite = TTag<CInfiniteRange<R>>;

	template<CList R1, CAsCallable<TListValueRef<R1>> F1>
	constexpr RMap(R1&& r, F1&& f): Func(ForwardAsFunc<F1>(f)), OriginalRange(ForwardAsRange<R1>(r)) {}

	[[nodiscard]] constexpr decltype(auto) First() const {return Func(OriginalRange.First());}
	constexpr void PopFirst() {OriginalRange.PopFirst();}
	[[nodiscard]] constexpr bool Empty() const {return OriginalRange.Empty();}

	constexpr decltype(auto) Last() const requires CHasLast<R> {return Func(OriginalRange.Last());}
	constexpr void PopLast() requires CHaslPopLast<R> {OriginalRange.PopLast();}
	constexpr decltype(auto) operator[](size_t index) const {return Func(OriginalRange[index]);}
	[[nodiscard]] constexpr index_t Length() const requires CHasLength<R> {return OriginalRange.Length();}

	[[nodiscard]] constexpr auto PopFirstCount(ClampedSize numElementsToPop) requires CHasPopFirstCount<R>
	{
		return OriginalRange.PopFirstCount(numElementsToPop);
	}
	[[nodiscard]] constexpr auto PopLastCount(ClampedSize numElementsToPop) requires CHasPopLastCount<R>
	{
		return OriginalRange.PopLastCount(numElementsToPop);
	}

	[[no_unique_address]] F Func;
	R OriginalRange;
};

constexpr auto Map = []<typename F>(F&& f) {
	return [func = ForwardAsFunc<F>(f)]<CAccessibleList L>(L&& list) {
		return RMap<TRangeOf<L>, TFunctorOf<F>>(ForwardAsRange<L>(list), func);
	};
};

INTRA_IGNORE_WARN_COPY_MOVE_IMPLICITLY_DELETED
template<typename R, class P> class RFilter
{
	using TagAnyInstanceFinite = TTag<CFiniteRange<R>>;
	using TagAnyInstanceInfinite = TTag<CInfiniteRange<R>>;

	template<CConsumableList L> constexpr RFilter(L&& list, P&& filterPredicate):
		mPred(ForwardAsFunc<P>(filterPredicate)), mOriginalRange(ForwardAsRange<L>(list))
	{skipFalsesFront(mOriginalRange, filterPredicate);}


	[[nodiscard]] constexpr decltype(auto) First() const
	{
		INTRA_PRECONDITION(!Empty());
		return mOriginalRange.First();
	}

	constexpr void PopFirst()
	{
		mOriginalRange.PopFirst();
		mOriginalRange|PopFirstUntil(mPred);
	}

	[[nodiscard]] constexpr decltype(auto) Last() const requires CBidirectionalRange<R>
	{
		INTRA_PRECONDITION(!Empty());
		auto&& b = mOriginalRange.Last();
		if(mPred(b)) return b;

		auto copy = mOriginalRange;
		copy.PopLast();
		copy|PopLastUntil(mPred);
		return copy.Last();
	}

	constexpr void PopLast() requires CBidirectionalRange<R>
	{
		mOriginalRange|PopLastUntil(mPred);
		mOriginalRange.PopLast();
	}

	[[nodiscard]] constexpr bool Empty() const {return mOriginalRange.Empty();}

private:
	R mOriginalRange;
	[[no_unique_address]] mPred;
};

constexpr auto Filter = []<typename P>(P&& pred) {
	return [pred = ForwardAsFunc<P>(pred)]<CConsumableList L>(L&& list) {
		return RFilter<TRangeOf<L>, P>(INTRA_FWD(list), pred);
	};
};

template<typename R> struct RRetro
{
	R OriginalRange;

	[[nodiscard]] constexpr bool Empty() const {return OriginalRange.Empty();}
	[[nodiscard]] constexpr decltype(auto) First() const {return OriginalRange.Last();}
	constexpr void PopFirst() {OriginalRange.PopLast();}
	[[nodiscard]] constexpr decltype(auto) Last() const {return OriginalRange.First();}
	constexpr void PopLast() {OriginalRange.PopFirst();}
	
	[[nodiscard]] constexpr decltype(auto) operator[](Index index)
		requires CHasIndex<R> && CHasLength<R>
	{
		return OriginalRange[Length()-1-index];
	}

	[[nodiscard]] constexpr decltype(auto) operator[](Index index) const
		requires CHasIndex<R> && CHasLength<R>
	{
		return OriginalRange[Length()-1-index];
	}

	[[nodiscard]] constexpr auto Length() const requires CHasLength<R> {return OriginalRange.Length();}

	[[nodiscard]] constexpr index_t PopFirstCount(ClampedSize maxElementsToPop) requires CHasPopLastCount<R>
	{
		return OriginalRange.PopLastCount(maxElementsToPop);
	}

	[[nodiscard]] constexpr index_t PopLastCount(ClampedSize maxElementsToPop) requires CHasPopFirstCount<R>
	{
		return OriginalRange.PopFirstCount(maxElementsToPop);
	}
};

constexpr auto Retro = []<CBidirectionalList R>(R&& r) {
	if constexpr(CInstanceOfTemplate<TRemoveConstRef<R>, RRetro>) return r.OriginalRange;
	else return RRetro{ForwardAsRange<R>(r)};
};

template<typename R> struct RChunks
{
	using TagAnyInstanceFinite = TTag<CFiniteRange<R>>;
	using TagAnyInstanceInfinite = TTag<CInfiniteRange<R>>;

	template<typename R2> constexpr RChunks(R2&& range, ClampedSize chunkLen):
		mOriginalRange(ForwardAsRange<R2>(range)), mChunkLen(chunkLen)
	{
		INTRA_PRECONDITION(chunkLen >= 1);
	}

	[[nodiscard]] auto First() const {return mOriginalRange|Take(mChunkLen);}
	void PopFirst() {mOriginalRange|PopFirstCount(mChunkLen);}
	[[nodiscard]] bool Empty() const {return mChunkLen == 0 || mOriginalRange.Empty();}

	[[nodiscard]] constexpr index_t Length() const requires CHasLength<R>
	{
		return (mOriginalRange.Length() + mChunkLen - 1) / mChunkLen;
	}

	[[nodiscard]] constexpr auto operator[](Index index) const requires CHasPopFirstCount<R> && CHasLength<R>
	{
		INTRA_PRECONDITION(index < Length());
		return mOriginalRange|Drop(index*mChunkLen)|Take(mChunkLen);
	}

	[[nodiscard]] constexpr auto Last() const requires CHasPopFirstCount<R> && CHasLength<R>
	{
		INTRA_PRECONDITION(!Empty());
		size_t numToTake = size_t(mOriginalRange.Length()) % mChunkLen;
		if(numToTake == 0) numToTake = mChunkLen;
		return mOriginalRange|Tail(numToTake);
	}

	constexpr void PopLast() requires CHasPopLast<R> && CHasLength<R>
	{
		INTRA_PRECONDITION(!Empty());
		size_t numToPop = size_t(mOriginalRange.Length()) % mChunkLen;
		if(numToPop == 0) numToPop = mChunkLen;
		mOriginalRange|PopLastExactly(numToPop);
	}

private:
	R mOriginalRange;
	size_t mChunkLen;
};

constexpr auto Chunks = [](ClampedSize chunkLen) {
	return [chunkLen]<CForwardList R>(R&& r) {
		return RChunks<TRangeOf<R>>>(ForwardAsRange<R>(r), chunkLen);
	};
};

INTRA_IGNORE_WARN_ASSIGN_IMPLICITLY_DELETED
template<typename R> struct RStride
{
	using TagAnyInstanceFinite = TTag<CFiniteRange<R>>;
	using TagAnyInstanceInfinite = TTag<CInfiniteRange<R>>;

	RStride() = default;

	constexpr RStride(R range, Size strideStep):
		mOriginalRange(Move(range)), mStep(strideStep)
	{
		INTRA_PRECONDITION(strideStep > 0);
		if constexpr(CHasPopLast<R> && CHasLength<R>)
		{
			const size_t len = size_t(mOriginalRange.Length());
			if(len == 0) return;
			mOriginalRange|PopLastExactly((len-1) % mStep);
		}
	}

	[[nodiscard]] constexpr decltype(auto) First() const {return mOriginalRange.First();}
	constexpr void PopFirst() {mOriginalRange|Intra::PopFirstCount(mStep);}
	[[nodiscard]] constexpr bool Empty() const {return mOriginalRange.Empty();}

	[[nodiscard]] constexpr decltype(auto) Last() const requires CHasLast<R> {return mOriginalRange.Last();}
	constexpr void PopLast() requires CHasPopLast<R> {mOriginalRange|Intra::PopLastCount(mStep);}
	[[nodiscard]] constexpr decltype(auto) operator[](Index index) const requires CHasIndex<R> {return mOriginalRange[index*mStep];}

	[[nodiscard]] constexpr index_t PopFirstCount(ClampedSize maxElementsToPop) requires CHasPopFirstCount<R>
	{
		const auto originalElementsPopped = mOriginalRange.PopFirstCount(mStep*size_t(maxElementsToPop));
		return index_t((size_t(originalElementsPopped) + mStep - 1) / mStep);
	}

	[[nodiscard]] constexpr index_t PopLastCount(ClampedSize maxElementsToPop) requires CHasPopLastCount<R>
	{
		const auto originalElementsPopped = mOriginalRange.PopLastCount(mStep*size_t(maxElementsToPop));
		return index_t((size_t(originalElementsPopped) + mStep - 1) / mStep);
	}

	[[nodiscard]] constexpr index_t Length() const requires CHasLength<R>
	{
		return index_t((size_t(mOriginalRange.Length()) + mStep - 1) / mStep);
	}

	[[nodiscard]] constexpr auto Stride(Size step) const {return RStride{mOriginalRange, mStep*size_t(step)};}

private:
	R mOriginalRange;
	size_t mStep = 0;
};

constexpr auto Stride = [](Size step) {
	return [step]<CAccessibleList R>(R&& r) {
		if constexpr(CInstanceOfTemplate<TRemoveConstRef<R>, RStride>) return r.Stride(step);
		else return RStride<TRangeOf<R>>(ForwardAsRange<R>(r), step);
	};
};

template<class RangeOfLists> class RJoin
{
	using R = TRangeOf<TRangeValueRef<RangeOfLists>>;

	RangeOfLists mOriginalRanges;
	R mCurrentRange;
	[[no_unique_address]] TSelect<bool, EmptyType, CInfiniteRange<R>> mCreatedEmpty;
public:
	using TagAnyInstanceFinite = TTag<CFiniteRange<RangeOfRanges> && CFiniteRange<R>>;
	using TagAnyInstanceInfinite = TTag<CInfiniteRange<RangeOfRanges>>;

	template<CAccessibleList L> requires CAccessibleRange<TListValue<RoR>>
	constexpr RJoin(L&& listOfLists):
		mOriginalRanges(ForwardAsRange<L>(listOfLists)),
		mCreatedEmpty(mOriginalRanges.Empty())
	{
		goToNearestNonEmptyElement();
	}

	constexpr decltype(auto) First() const
	{
		INTRA_PRECONDITION(!Empty());
		return mCurrentRange.First();
	}
	constexpr void PopFirst()
	{
		INTRA_PRECONDITION(!Empty());
		mCurrentRange.PopFirst();
		goToNearestNonEmptyElement();
	}
	[[nodiscard]] constexpr bool Empty() const
	{
		if constexpr(CInfiniteRange<R>) return mCreatedEmpty;
		else return mCurrentRange.Empty();
	}

	[[nodiscard]] constexpr decltype(auto) operator[](Index index) const
		requires CHasIndex<R> && (CInfiniteRange<R> || CHasLength<R> && CCopyConstructible<RangeOfLists>)
	{
		INTRA_PRECONDITION(!Empty());
		if constexpr(CInfiniteRange<R>) return return mCurrentRange[index];
		else
		{
			INTRA_PRECONDITION(index < Length());
			size_t curIndex = size_t(index);
			size_t curLen = size_t(mCurrentRange.Length());
			if(curIndex < curLen) return mCurrentRange[curIndex];
			curIndex -= curLen;
			auto rr = mOriginalRanges;
			for(;;)
			{
				auto range = RangeOf(rr.First());
				curLen = range.Length();
				if(curIndex < curLen) return range[curIndex];
				curIndex -= curLen;
				rr.PopFirst();
			}
		}
	}

	[[nodiscard]] constexpr index_t Length() const
		requires CHasLength<R> && CCopyConstructible<RangeOfLists>
	{
		auto result = mCurrentRange.Length();
		for(auto&& range: mOriginalRanges) result += range|Intra::Length;
		return result;
	}

private:
	constexpr void goToNearestNonEmptyElement()
	{
		while(mCurrentRange.Empty() && !mOriginalRanges.Empty())
		{
			mCurrentRange = RangeOf(mOriginalRanges.First());
			mOriginalRanges.PopFirst();
		}
	}
};
constexpr auto Join = []<CList ListOfLists>(ListOfLists&& lists) {
	if constexpr(CInfiniteList<ListsOfLists> && CInfiniteList<TListValue<ListOfLists>>)
	return RJoin<TRangeOf<ListOfLists>>(INTRA_FWD(lists));
};

INTRA_END
