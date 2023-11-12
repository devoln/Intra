#pragma once

#include <Intra/Container/Array.h>

namespace Intra { INTRA_BEGIN

template<bool PoTCapacity, typename TSize = size_t> class FixedQueueLogic
{
	TSize mCapacityOrMask = 0;
	TSize mNumPushed = 0, mNumPopped = 0;
	[[nodiscard]] INTRA_FORCEINLINE index_t wrap(index_t index) const {return PoTCapacity? index & mCapacityOrMask: index % mCapacityOrMask;}
public:
	explicit FixedQueueLogic(index_t capacity): mCapacityOrMask(capacity - (PoTCapacity? 1: 0))
	{
		if constexpr(PoTCapacity) INTRA_PRECONDITION(IsPow2(capacity));
	}

	index_t Push(index_t n = 1)
	{
		INTRA_PRECONDITION(FreeSpace() >= n);
		return wrap((mNumPushed += n) - n);
	}

	INTRA_FORCEINLINE index_t PushAllowOverrun(index_t n = 1)
	{
		const index_t overrunCount = NumQueuedElements() + n - Capacity();
		if(overrunCount > 0) mNumPopped += overrunCount;
		return wrap((mNumPushed += n) - n);
	}

	index_t Pop(index_t n = 1)
	{
		INTRA_PRECONDITION(NumQueuedElements() >= n);
		return wrap((mNumPopped += n) - n);
	}

	index_t TryPush()
	{
		if(Full()) return -1;
		return wrap(mNumPushed++);
	}

	index_t TryPop()
	{
		if(Empty()) return -1;
		return wrap(mNumPopped++);
	}

	index_t TryPush(index_t& n)
	{
		const auto count = n = Min(n, FreeSpace());
		return wrap((mNumPushed += count) - count);
	}

	index_t TryPop(index_t& n)
	{
		const auto count = n = Min(n, NumQueuedElements());
		return wrap((mNumPopped += count) - count);
	}

	[[nodiscard]] INTRA_FORCEINLINE index_t NumQueuedElements() const {return index_t(mNumPushed - mNumPopped);}
	[[nodiscard]] INTRA_FORCEINLINE index_t Capacity() const {return index_t(mCapacityOrMask + (PoTCapacity? 1: 0));}
	[[nodiscard]] INTRA_FORCEINLINE index_t FreeSpace() const {return Capacity() - NumQueuedElements();}
	[[nodiscard]] INTRA_FORCEINLINE bool Empty() const {return NumQueuedElements() == 0;}
	[[nodiscard]] INTRA_FORCEINLINE bool Full() const {return NumQueuedElements() == Capacity();}
};

enum class QueueThreadSafety {
	SingleThreaded,
	SingleProducerSingleConsumer,
	MultipleProducersMultipleConsumers
};

template<class TIndexable, QueueThreadSafety ThreadSafety, bool WaitOnEmptyOrFull, bool PoTCapacity> class GenericQueue
{
	static_assert(!WaitOnEmptyOrFull || ThreadSafety != QueueThreadSafety::SingleThreaded);

	using T = TRemoveReference<decltype(Val<TIndexable>()[0])>;
	static constexpr bool CanUseMemcpy = CTriviallyCopyable<T> && (CConvertibleToSpan<TIndexable> || CPointer<TIndexable>);

	TIndexable mIndexable;
	FixedQueueLogic<PoTCapacity> mLogic;
	INTRA_NO_UNIQUE_ADDRESS mutable TSelect<Mutex, DummyLockable, (ThreadSafety >= QueueThreadSafety::SingleProducerSingleConsumer)> mLock;
	INTRA_NO_UNIQUE_ADDRESS TSelect<CondVar, DummyCondVar, WaitOnEmptyOrFull> mEmptyCond, mFullCond;
	bool mClosed = false;
public:
	GenericQueue() = default;

	template<typename... Args> explicit GenericQueue(Args&&... args) requires CList<TIndexable>:
		mIndexable(INTRA_FWD(args)...), mLogic(Length(mIndexable)) {}

	INTRA_FORCEINLINE explicit GenericQueue(TIndexable ptr, index_t capacity) requires CPointer<TIndexable>:
		mIndexable(ptr), mLogic(capacity) {}

	INTRA_FORCEINLINE explicit GenericQueue(Span<TRemovePointer<TIndexable>> span) requires CPointer<TIndexable>:
		mIndexable(span.Begin), mLogic(Length(span)) {}

	void Close()
	{
		mClosed = true;
		if constexpr(WaitOnEmptyOrFull)
		{
			this->EmptyCond.notify_all();
			this->FullCond.notify_all();
		}
	}

	bool IsClosed() const
	{
		auto l = Lock(mLock);
		return mClosed;
	}

	// @return true unless the queue has been closed or !WaitOnEmptyOrFull and Full()
	bool Push(T x)
	{
		INTRA_SYNCHRONIZED(mLock)
		{
			if constexpr(WaitOnEmptyOrFull)
				while(mLogic.Full() && !mClosed) mEmptyCond.wait(mLock); // block thread until the element is popped instead of failing
			if(mClosed) return false;
			if(mLogic.Full()) return false;
			mIndexable[mLogic.Push()] = std::move(x);
		}
		if constexpr(WaitOnEmptyOrFull)
			this->EmptyCond.notify_one();
		return true;
	}

	// @return The number of elements pushed to the queue until it's full. src is also advanced by this value.
	index_t TryPushArray(Span<const T>& src)
	{
		if(mClosed) return 0;
		index_t n;
		INTRA_SYNCHRONIZED(mLock)
			n = tryPushArrayNoLock(src);
		if constexpr(WaitOnEmptyOrFull)
			if(n) mEmptyCond.notify_all();
		src.Begin += n;
		return n;
	}

	// @return The number of elements pushed to the queue. May be less than Length(src).
	//  It may be 0 only if the queue is closed or if !WaitOnEmptyOrFull.
	index_t PartialPushArray(Span<const T> src)
	{
		if(mClosed) return 0;
		const auto srcStart = src.Begin;
		INTRA_SYNCHRONIZED(mLock)
		{
			if constexpr(WaitOnEmptyOrFull)
				while(mLogic.Full() && !mClosed) mFullCond.wait(mLock); // block thread until consumers free some space
			if(mClosed) return 0;
			src.Begin += pushArrayNoLock(src, !WaitOnEmptyOrFull);
		}
		return src.Begin - srcStart;
	}

	// @return The number of elements pushed to the queue or discarded (if !WaitOnEmptyOrFull). Matches Length(src) unless the queue has been closed.
	index_t PushArray(Span<const T> src)
	{
		if(mClosed) return 0;
		const auto srcStart = src.Begin;
		for(;;)
		{
			INTRA_SYNCHRONIZED(mLock)
			{
				if constexpr(WaitOnEmptyOrFull)
					while(mLogic.Full() && !mClosed) mFullCond.wait(mLock); // block thread until consumers free space instead of failing
				if(mClosed) break;
				src.Begin += pushArrayNoLock(src, !WaitOnEmptyOrFull);
			}
			if(!WaitOnEmptyOrFull || src.empty()) break;
		}
		return src.Begin - srcStart;
	}

	bool Pop(T& dst)
	{
		INTRA_SYNCHRONIZED(mLock)
		{
			if constexpr(WaitOnEmptyOrFull)
				while(mLogic.Empty() && !mClosed) mFullCond.wait(mLock); // block thread until the element is pushed instead of failing
			if(mClosed) return false;
			dst = std::move(mIndexable[mLogic.Pop()]);
		}
		if constexpr(WaitOnEmptyOrFull)
			mFullCond.notify_one();
		return true;
	}

	index_t TryPopArray(Span<T> dst)
	{
		index_t n;
		INTRA_SYNCHRONIZED(mLock)
			n = tryPopArrayNoLock(dst);
		if constexpr(WaitOnEmptyOrFull)
			if(n) mFullCond.notify_all();
		dst.Begin += n;
		return n;
	}

	index_t PartialPopArray(Span<T> dst)
	{
		const auto dstStart = dst.Begin;
		INTRA_SYNCHRONIZED(mLock)
		{
			if constexpr(WaitOnEmptyOrFull)
				while(mLogic.Empty() && !mClosed) mEmptyCond.wait(mLock); // block thread until producers push new elements or close the queue
			if(mClosed) return 0;
			dst.Begin += tryPopArrayNoLock(dst);
		}
		return dst.Begin - dstStart;
	}

	index_t PopArray(Span<T> dst)
	{
		const auto dstStart = dst.Begin;
		for(;;)
		{
			INTRA_SYNCHRONIZED(mLock)
			{
				if constexpr(WaitOnEmptyOrFull)
					while(mLogic.Empty() && !mClosed) mEmptyCond.wait(mLock); // block thread until producers push new elements or close the queue
				if(mClosed) break;
				dst.Begin += tryPopArrayNoLock(dst);
			}
			if(dst.empty()) break;
			if constexpr(!WaitOnEmptyOrFull)
			{
				// if we can't wait for new elements, then just default initialize the data
				Log().Warn("queue underrun {}/{}", Length(dst), dst.End - dstStart);
				for(auto& x: dst) x = {};
				dst.Begin = dst.End;
				break;
			}
		}
		return dst.Begin - dstStart;
	}

	bool TryPush(T x)
	{
		INTRA_SYNCHRONIZED(mLock)
		{
			if(mLogic.Full() || mClosed) return false;
			mIndexable[mLogic.Push()] = std::move(x);
		}
		return true;
	}

	bool TryPop(T& dst)
	{
		INTRA_SYNCHRONIZED(mLock)
		{
			if(mLogic.Empty() || mClosed) return false;
			dst = std::move(mIndexable[mLogic.Pop()]);
		}
		return true;
	}

	[[nodiscard]] bool Empty() const
	{
		auto l = Lock(mLock);
		return mLogic.Empty();
	}

	[[nodiscard]] bool Full() const
	{
		auto l = Lock(mLock);
		return mLogic.Full();
	}

	[[nodiscard]] INTRA_FORCEINLINE index_t Capacity() const {return mLogic.Capacity();}

	[[nodiscard]] index_t NumQueuedElements() const
	{
		auto l = Lock(mLock);
		return mLogic.NumQueuedElements();
	}

	[[nodiscard]] index_t FreeSpace() const
	{
		auto l = Lock(mLock);
		return mLogic.FreeSpace();
	}

private:
	index_t pushArrayNoLock(const Span<const T>& src, bool allowOverrun)
	{
		index_t n = Length(src);
		auto start = allowOverrun? mLogic.PushAllowOverrun(n): mLogic.TryPush(n); // may decrease n
		index_t m = Min(n, Capacity() - start);
		if constexpr(CanUseMemcpy)
		{
			memcpy(&mIndexable[start], src.Begin, m * sizeof(T));
			memcpy(&mIndexable[0], src.Begin + m, (n - m) * sizeof(T));
		}
		else
		{
			index_t i = 0;
			for(; i < m; i++) mIndexable[start + i] = src[i];
			start -= Capacity();
			for(; i < n; i++) mIndexable[start + i] = src[i];
		}
		return n;
	}

	index_t tryPopArrayNoLock(Span<T> dst)
	{
		index_t n = Length(dst);
		auto start = mLogic.TryPop(n);
		index_t m = Min(n, Capacity() - start);
		if constexpr(CanUseMemcpy)
		{
			memcpy(dst.Begin, &mIndexable[start], m * sizeof(T));
			memcpy(dst.Begin + m, &mIndexable[0], (n - m) * sizeof(T));
		}
		else
		{
			index_t i = 0;
			for(; i < m; i++) dst[i] = std::move(mIndexable[start + i]);
			start -= Capacity();
			for(; i < n; i++) dst[i] = std::move(mIndexable[start + i]);
		}
		return n;
	}
};

template<typename T, bool PoTCapacity = false> using Queue = GenericQueue<DynArray<T>, QueueThreadSafety::SingleThreaded, false, PoTCapacity>;
template<typename T, bool PoTCapacity = false> using ConcurrentQueue = GenericQueue<DynArray<T>, QueueThreadSafety::MultipleProducersMultipleConsumers, false, PoTCapacity>;
template<typename T, bool PoTCapacity = false> using ConcurrentWaitQueue = GenericQueue<DynArray<T>, QueueThreadSafety::MultipleProducersMultipleConsumers, true, PoTCapacity>;

} INTRA_END
