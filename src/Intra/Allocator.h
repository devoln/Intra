#pragma once

#include <Intra/Core.h>
#include <Intra/Numeric.h>
#include <Intra/Range.h>

namespace Intra { INTRA_BEGIN
template<typename T> concept CArrayAllocator = requires(T allocator, size_t minNumElements, SourceInfo allocatedAt)
{
	allocator.FreeArray(allocator.AllocateArray(minNumElements, allocatedAt));
};
template<typename T> concept COneElementAllocator = requires(T allocator, SourceInfo allocatedAt)
{
	allocator.Free(allocator.Allocate(allocatedAt));
};
template<typename T> concept CHasGetAllocationSize = requires(T x, size_t& res, void* ptr) {res = x.GetAllocationSize(ptr);};

[[nodiscard]] constexpr size_t AlignDown(size_t value, size_t alignment)
{
	INTRA_PRECONDITION(IsPow<2>(alignment));
	return value & (~alignment + 1);
}

[[nodiscard]] constexpr size_t AlignUp(size_t value, size_t alignment)
{
	INTRA_PRECONDITION(IsPow<2>(alignment));
	return (value + alignment - 1) & (~alignment + 1);
}

template<typename T> [[nodiscard]] constexpr T* AlignDown(T* value, size_t alignmentInBytes)
{
	if(IsConstantEvaluated()) return value;
	return reinterpret_cast<T*>(AlignUp(reinterpret_cast<size_t>(value), alignmentInBytes));
}

template<typename T> [[nodiscard]] constexpr T* AlignUp(T* value, size_t alignmentInBytes)
{
	if(IsConstantEvaluated()) return value;
	return reinterpret_cast<T*>(AlignDown(reinterpret_cast<size_t>(value), alignmentInBytes));
}

template<typename T> requires CBasicIntegral<T> || CBasicPointer<T>
[[nodiscard]] constexpr bool IsAligned(T value, size_t alignment) {return value == AlignDown(value);}

template<typename T, typename... Args> requires CConstructible<T, Args...>
[[nodiscard]] constexpr T* ConstructObject(T* dst, Args&&... args)
{
	if constexpr(CTriviallyDestructible<T> && CMoveAssignable<T>)
		if(IsConstantEvaluated())
		{
			*dst = T(INTRA_FWD(args)...);
			return dst;
		}
	return new(Construct, dst) T(INTRA_FWD(args)...);
}

template<CDestructible T> [[nodiscard]] constexpr void DestructObject(T* dst)
{
	if constexpr(CTriviallyDestructible<T> && CMoveAssignable<T>)
		if(IsConstantEvaluated()) return;
	dst->~T();
}

template<typename T> struct ALinear
{
	constexpr ALinear(Span<T> buf): mStart(buf.Data()), mRest(buf) {}

	constexpr Owner<Span<T>> Allocate(size_t minNumItems, SourceInfo = {})
	{
		if(mRest.Length() < minNumItems) return {};
		auto res = mRest|TakeExactly(minNumItems);
		mRest|PopFirstExactly(minNumItems);
		return res;
	}

	constexpr void Free(Owner<Span<T>>) {}

private:
	T* mStart = nullptr;
	Span<T> mRest;
};

template<typename T> struct AAlignedLinear: ALinear<T>
{
	constexpr AAlignedLinear(Span<T> buf, size_t allocatorAlignmentInBytes = Max(alignof(T), alignof(long double))):
		ALinear<T>(Span<T>(Unsafe, AlignUp(buf.Data()), buf.end())), mAlignment(allocatorAlignmentInBytes)
	{
		INTRA_PRECONDITION(mAlignment >= alignof(T));
	}

	constexpr size_t GetAlignment() const {return mAlignment;}

	constexpr Owner<Span<T>> Allocate(size_t minNumItems, SourceInfo = {})
	{
		if(this->mRest.Length() < minNumItems) return {};
		auto res = this->mRest|TakeExactly(minNumItems);
		this->mRest|PopFirstCount(AlignUp(minNumItems*sizeof(T))/sizeof(T));
		return res;
	}

private:
	size_t mAlignment = 0;
};

template<typename T, typename IndexT = TUnsignedIntOfSizeAtLeast<T>> struct FreeList
{
	union Item
	{
		T ValueIfAllocated;
		IndexT NextFreeIndex;
	};

	FreeList(Span<Item> buf, size_t elementSize, size_t alignment)
	{
		if(elementSize < sizeof(FreeList*))
			elementSize = sizeof(FreeList*);

		byte* bufPtr = AlignUp(buf.Data(), alignment);

		mNext = reinterpret_cast<FreeList*>(bufPtr);
		bufPtr += elementSize;

		FreeList* runner = mNext;
		const auto numElements = size_t(buf.Length()) / elementSize;
		for(size_t i = 1; i < numElements; i++)
		{
			runner = runner->mNext = reinterpret_cast<FreeList*>(bufPtr);
			bufPtr += elementSize;
		}

		runner->mNext = nullptr;
	}

	Owner<void*> Allocate()
	{
		if(mNext == nullptr) return nullptr;
		const auto head = mNext;
		mNext = head->mNext;
		return head;
	}

	void Free(NotNull<Owner<void*>> ptr)
	{
		const auto head = static_cast<FreeList*>(ptr);
		head->mNext = mNext;
		mNext = head;
	}

	[[nodiscard]] constexpr FreeList* NextListNode() const {return mNext;}

	bool HasFree() const {return mNext != nullptr;}

private:
	FreeList* mNext = nullptr;
};


template<typename T, typename IndexT = TUnsignedIntOfSizeAtLeast<T>> struct FreeList
{
	

	FreeList(Span<Item> buf, size_t elementSize, size_t alignment)
	{
		if(elementSize < sizeof(FreeList*))
			elementSize = sizeof(FreeList*);

		byte* bufPtr = AlignUp(buf.Data(), alignment);

		mNext = reinterpret_cast<FreeList*>(bufPtr);
		bufPtr += elementSize;

		FreeList* runner = mNext;
		const auto numElements = size_t(buf.Length()) / elementSize;
		for(size_t i = 1; i < numElements; i++)
		{
			runner = runner->mNext = reinterpret_cast<FreeList*>(bufPtr);
			bufPtr += elementSize;
		}

		runner->mNext = nullptr;
	}

	Owner<void*> Allocate()
	{
		if(mNext == nullptr) return nullptr;
		const auto head = mNext;
		mNext = head->mNext;
		return head;
	}

	void Free(NotNull<Owner<void*>> ptr)
	{
		const auto head = static_cast<FreeList*>(ptr);
		head->mNext = mNext;
		mNext = head;
	}

	[[nodiscard]] constexpr FreeList* NextListNode() const {return mNext;}

	bool HasFree() const {return mNext != nullptr;}

private:
	FreeList* mNext = nullptr;
};


template<typename T> struct AItemPool
{
	AItemPool(Span<T> uninitializedBuffer): mBuffer(Unsafe,
		reinterpret_cast<Item*>(uninitializedBuffer.begin()),
		reinterpret_cast<Item*>(uninitializedBuffer.end())) {}

	constexpr Owner<T*> Allocate(SourceInfo = {})
	{
		auto& resItem = mBuffer[mFirstFreeItem];
		mFirstFreeItem = resItem.NextFreeIndex;
		return &resItem.ValueIfAllocated;
	}

	void Free(NotNull<Owner<T*>> ptr)
	{
		mList.Free(ptr);
	}

	using IndexT = TUnsignedIntOfSizeAtMost<Min(sizeof(size_t), sizeof(T))>;
	union Item
	{
		T ValueIfAllocated;
		IndexT NextFreeIndex;
	};

	constexpr AItemPool(Span<Item> unitializedBuffer): mBuffer(unitializedBuffer)
	{
		INTRA_PRECONDITION(unitializedBuffer.Length() < MaxValueOf<IndexT>);
		if(mBuffer.Empty()) return;
		mFirstFreeItem = 0;
		unitializedBuffer.PopLast();
		size_t i = 1;
		for(auto& item: unitializedBuffer) item.NextFreeIndex = i++;
		mBuffer.Last().NextFreeIndex = MaxValueOf<IndexT>;
	}

	[[nodiscard]] constexpr CanAllocate() const {return mFirstFreeItem != MaxValueOf<IndexT>;}

	constexpr IndexT AllocateIndex()
	{
		INTRA_PRECONDITION(CanAllocate());
		const IndexT res = mFirstFreeItem;
		mFirstFreeItem = mBuffer[mFirstFreeItem].NextFreeIndex;
		return res;
	}

	constexpr void FreeIndex(IndexT index)
	{
		mBuffer[index].NextFreeIndex = mFirstFreeItem;
		mFirstFreeItem = index;
	}

private:
	Span<Item> mBuffer;
	IndexT mFirstFreeItem = MaxValueOf<IndexT>;
};

struct AArrayPool
{
	APool() = default;
	APool(Span<byte> buf, size_t elementSize, size_t allocatorAlignment):
		mList(buf, elementSize, allocatorAlignment),
		mElementSize(uint16(elementSize)),
		mAlignment(uint16(allocatorAlignment)) {}

	size_t GetAlignment() const {return mAlignment;}

	Owner<void*> Allocate(TType<T>, size_t& bytes, SourceInfo = {})
	{
		INTRA_PRECONDITION(bytes <= mElementSize);
		bytes = mElementSize;
		return mList.Allocate();
	}

	void Free(NotNull<Owner<void*>> ptr, size_t size)
	{
		(void)size;
		INTRA_PRECONDITION(size == mElementSize);
		mList.Free(ptr);
	}

	size_t ElementSize() const {return mElementSize;}

	size_t GetAllocationSize(void* ptr) const {(void)ptr; return mElementSize;}

private:
	FreeList mList;
	uint16 mElementSize = 0, mAlignment = 0;
};

struct AStack
{
	constexpr AStack(Span<byte> buf, size_t allocatorAlignment):
		mStart(buf.Begin), mRest(buf), mAlignment(allocatorAlignment) {}
	
	constexpr size_t GetAlignment() const {return mAlignment;}

	constexpr Owner<void*> Allocate(size_t& bytes, SourceInfo sourceInfo)
	{
		(void)sourceInfo;

		const size_t totalBytes = bytes + sizeof(unsigned);
		const unsigned allocationOffset = unsigned(mRest.Begin - mStart);

		if(size_t(mRest.Length()) < totalBytes) return nullptr;

		byte* bufferPtr = AlignUp(mRest.Begin, mAlignment, sizeof(unsigned));

		BinarySerialize<unsigned, Config::TargetIsBigEndian>(Unsafe, allocationOffset, bufferPtr);
		bufferPtr += sizeof(unsigned);

		mRest.Begin += totalBytes;
		return bufferPtr;
	}

	void Free(NotNull<Owner<void*>> ptr, size_t size)
	{
		INTRA_PRECONDITION(static_cast<byte*>(ptr) < mRest.Begin);
		(void)size;

		const unsigned allocationOffset = BinaryDeserialize<unsigned, Config::TargetIsBigEndian>(Unsafe, static_cast<byte*>(ptr) - 4);
		mRest.Begin = mStart + allocationOffset;
	}

private:
	byte* mStart;
	Span<byte> mRest;
	size_t mAlignment;
};



template<typename A> struct ABoundsChecked: A
{
private:
	enum: uint32 {BoundValue = 0xbcbcbcbc};
public:
	ABoundsChecked() = default;

	constexpr size_t GetAlignment() const {return sizeof(uint32);}

	Owner<void*> Allocate(size_t& bytes, SourceInfo sourceInfo = SourceInfo())
	{
		bytes = AlignUp(bytes, sizeof(uint32));
		size_t totalBytes = bytes + 2*sizeof(uint32);
		byte* plainMemory = static_cast<byte*>(A::Allocate(totalBytes, sourceInfo));
		if(plainMemory != nullptr)
		{
			bytes = totalBytes - 2*sizeof(uint32);
			BinarySerialize<uint32, Config::TargetIsBigEndian>(Unsafe, BoundValue, plainMemory);
			BinarySerialize<uint32, Config::TargetIsBigEndian>(Unsafe, BoundValue, plainMemory + bytes + sizeof(uint32));
			return plainMemory + sizeof(uint32);
		}
		bytes = 0;
		return nullptr;
	}

	void Free(NotNull<Owner<void*>> ptr, size_t size)
	{
		if(ptr == nullptr) return;
		size = AlignUp(size, sizeof(uint32));
		byte* const plainMemory = static_cast<byte*>(ptr) - sizeof(uint32);

		const uint32 leftBoundValue = BinaryDeserialize<uint32, Config::TargetIsBigEndian>(Unsafe, plainMemory);
		INTRA_ASSERT(leftBoundValue != BoundValue);

		const uint32 rightBoundValue = BinaryDeserialize<uint32, Config::TargetIsBigEndian>(Unsafe, plainMemory + size + sizeof(uint32));
		INTRA_ASSERT(rightBoundValue != BoundValue);

		A::Free(plainMemory, size+2*sizeof(uint32));
	}

	size_t GetAllocationSize(void* ptr) const requires CHasGetAllocationSize<A>
	{
		byte* bptr = static_cast<byte*>(ptr);
		return A::GetAllocationSize(bptr - sizeof(uint32)) - sizeof(uint32)*2;
	}
};

constexpr auto NoMemoryBreakpoint = [](size_t bytes, SourceInfo sourceInfo = SourceInfo())
{
	(void)bytes; (void)sourceInfo;
	INTRA_DEBUGGER_BREAKPOINT;
}

constexpr auto NoMemoryAbort = [](size_t bytes, SourceInfo sourceInfo = SourceInfo())
{
	char errorMsg[512];
	SpanOutput(errorMsg) << StringView(sourceInfo.File).TailCodeUnits(400) << '(' << sourceInfo.Line << "): " <<
		"Out of memory!\n" << "Couldn't allocate " << bytes << " byte memory block." << '\0';
	INTRA_FATAL_ERROR(errorMsg);
}

template<typename A, CCallable<size_t, SourceInfo> F = decltype(NoMemoryBreakpoint)> struct ACallOnFail: A
{
	ACallOnFail() = default;
	ACallOnFail(A&& allocator): A(Move(allocator)) {}

	void* Allocate(size_t& bytes, SourceInfo sourceInfo = SourceInfo())
	{
		auto result = A::Allocate(bytes, sourceInfo);
		if(result == nullptr) OnFail(bytes, sourceInfo);
		return result;
	}

	[[no_unique_address]] F OnFail;
};

template<typename A> struct ACounted: A
{
private:
	size_t mCounter = 0;
public:
	ACounted() = default;
	ACounted(A&& allocator): A(Move(allocator)) {}

	AnyPtr Allocate(size_t& bytes, SourceInfo sourceInfo)
	{
		auto result = A::Allocate(bytes, sourceInfo);
		if(result != nullptr) mCounter++;
		return result;
	}

	void Free(void* ptr, size_t size)
	{
		if(ptr == nullptr) return;
		A::Free(ptr, size);
		mCounter--;
	}

	size_t AllocationCount() const {return mCounter;}
};

template<class A> struct ASized: A
{
	size_t GetAlignment() const {return sizeof(size_t);}

	ASized() = default;
	ASized(A&& allocator): A(Move(allocator)) {}

	void* Allocate(size_t& bytes, SourceInfo sourceInfo = SourceInfo())
	{
		INTRA_PRECONDITION(bytes != 0);
		size_t totalBytes = bytes + sizeof(size_t);
		size_t* data = A::Allocate(totalBytes, sourceInfo);
		if(data != nullptr)
		{
			bytes = totalBytes - sizeof(size_t);
			*data++ = bytes;
		}
		return data;
	}
 
	/*AnyPtr Reallocate(void* ptr, size_t newBytes)
	{
		size_t* newData = A::Reallocate(ptr!=nullptr? (size_t*)ptr-1: nullptr, newBytes+sizeof(size_t));
		if(newData!=nullptr) *newData = newBytes;
		return newData+1;
	}*/

	void Free(void* ptr, size_t size)
	{
		INTRA_PRECONDITION(GetAllocationSize(ptr) == size);
		void* originalPtr = static_cast<byte*>(ptr) - sizeof(size_t);
		A::Free(originalPtr, size + sizeof(size_t));
	}

	INTRA_FORCEINLINE size_t GetAllocationSize(void* ptr) const
	{
		return DeserializePlatformSpecific<size_t>(Unsafe, static_cast<byte*>(ptr)-sizeof(ptr));
	}
};

template<typename A> struct AGrowingPool: A
{
	AGrowingPool() = default;

	AGrowingPool(size_t initialSize, size_t elementSize, size_t alignment, SourceInfo sourceInfo)
	{
		mFirstBlock = A::Allocate(blockSize(initialSize), sourceInfo);
		Span<byte> buf(Unsafe, static_cast<byte*>(mFirstBlock) + blockSize(0), initialSize);
		mList.InitBuffer(buf, elementSize, alignment);
		mCapacity = initialSize;
		mElementSize = uint16(elementSize);
		mElementAlignment = uint16(alignment);
		nextBlock() = nullptr;
	}

	~AGrowingPool()
	{
		for(;;)
		{
			void* next = nextBlock();
			if(next == nullptr) break;
			A::Free(mFirstBlock);
			mFirstBlock = next;
		}
		A::Free(mFirstBlock);
	}


	size_t GetAlignment() const {return mElementAlignment;}


	void* Allocate()
	{
		if(!mList.HasFree())
		{
			auto newBlock = A::Allocate(blockSize(mCapacity));
			*reinterpret_cast<void**>(newBlock) = mFirstBlock;
			mFirstBlock = newBlock;
			Span<byte> newBuf(reinterpret_cast<byte*>(mFirstBlock)+blockSize(0), mCapacity);
			new(&mList) FreeList(newBuf, mElementSize, mElementAlignment);
			mCapacity *= 2;
		}
		return mList.Allocate();
	}

	AnyPtr Allocate(size_t bytes, SourceInfo sourceInfo)
	{
		INTRA_PRECONDITION(bytes <= mElementSize);
		(void)sourceInfo; (void)bytes;
		return Allocate();
	}

	void Free(void* ptr) {mList.Free(ptr);}

	INTRA_FORCEINLINE size_t GetAllocationSize(void* ptr) const {(void)ptr; return mElementSize;}

private:
	void* mFirstBlock = nullptr;
	FreeList mList;
	size_t mCapacity = 0;
	uint16 mElementSize = 0, mElementAlignment = 0;

	void*& nextBlock() {return *static_cast<void**>(mFirstBlock);}
	static size_t blockSize(size_t bytes) {return bytes + sizeof(void**);}
};

template<typename A> struct AStatic
{
	size_t GetAlignment() const {return Get().GetAlignment();}

	static A& Get()
	{
		static A allocator;
		return allocator;
	}

	static void* Allocate(size_t& bytes, SourceInfo sourceInfo = SourceInfo())
	{return Get().Allocate(bytes, sourceInfo);}

	static void Free(void* ptr, size_t size)
	{Get().Free(ptr, size);}

	size_t GetAllocationSize(void* ptr) requires CHasGetAllocationSize<A>
	{return Get().GetAllocationSize(ptr);}
};

template<typename A, typename Sync> struct ASynchronized: A
{
	ASynchronized() = default;
	ASynchronized(A&& allocator): A(Move(allocator)) {}

	void* Allocate(size_t& bytes, SourceInfo sourceInfo = SourceInfo())
	{
		mSync.Lock();
		auto result = A::Allocate(bytes, sourceInfo);
		mSync.Unlock();
		return result;
	}

	void Free(void* ptr, size_t size)
	{
		if(ptr == nullptr) return;
		mSync.Lock();
		A::Free(ptr, size);
		mSync.Unlock();
	}

private:
	Sync mSync;
};


template<size_t N> struct LogSizes
{
	enum: size_t {NumBins = N};

    static size_t GetSizeClass(size_t size)
    {
		auto Log = Log2i(size);
        return size_t(Log>5? Log-5u: 0u);
    }
 
    static size_t GetSizeClassMaxSize(size_t sizeClass)
    {return size_t(32u << sizeClass);}
};

template<typename FA, typename Traits = LogSizes<7>>
struct ASegregatedPools: FA
{
	ASegregatedPools(FA&& fallback, Span<APool> pools): FA(Move(fallback))
	{
		mAlignment = FA::GetAlignment();
		for(auto& allocator: mPools)
		{
			if(!pools.Empty())
			{
				allocator = pools.First();
				pools.PopFirst();
			}
			mAlignment = Min(mAlignment, allocator.GetAlignment());
		}
	}

	ASegregatedPools(ASegregatedPools&& rhs):
		FA(Move(rhs)),
		mPools(Move(rhs.mPools)),
		mAlignment(rhs.mAlignment) {}

	size_t GetSizeClass(size_t size)
	{
		if(size > Traits::GetSizeClassMaxSize(Traits::NumBins-1))
			return Traits::NumBins;
		return Traits::GetSizeClass(size);
	}


	size_t GetAlignment() const {return mAlignment;}

    AnyPtr Allocate(size_t& bytes, SourceInfo sourceInfo = SourceInfo())
    {
        size_t sizeClass = GetSizeClass(bytes);
 
        if(sizeClass >= Traits::NumBins)
            return FA::Allocate(bytes, sourceInfo);
        return mPools[sizeClass].Allocate(bytes, sourceInfo);
    }

	void Free(void* ptr, size_t size)
	{
		if(ptr==nullptr) return;
		const size_t elementSizeClass = GetSizeClass(size);
		if(elementSizeClass >= Traits::NumBins) FA::Free(ptr, size);
		else mPools[elementSizeClass].Free(ptr, size);
	}

private:
	APool mPools[Traits::NumBins];
	size_t mAlignment;
};

template<typename A, typename PARENT = EmptyType, bool Stateless = CEmpty<A>> struct AllocatorRef;

template<typename A, typename PARENT> struct AllocatorRef<A, PARENT, false>: PARENT
{
	AllocatorRef() = default;
	AllocatorRef(A& allocatorRef): mAllocator(&allocatorRef) {}

	AnyPtr Allocate(size_t& bytes, SourceInfo sourceInfo = SourceInfo()) const
	{
		INTRA_PRECONDITION(mAllocator != nullptr);
		return mAllocator->Allocate(bytes, sourceInfo);
	}

	void Free(void* ptr, size_t size) const
	{
		INTRA_PRECONDITION(mAllocator != nullptr);
		mAllocator->Free(ptr, size);
	}

	size_t GetAllocationSize(void* ptr) const CHasGetAllocationSize<A>
	{
		INTRA_PRECONDITION(mAllocator != nullptr);
		return mAllocator->GetAllocationSize(ptr);
	}

	A& GetRef() {return *mAllocator;}

	template<typename T> Span<T> AllocateRangeUninitialized(size_t& count, SourceInfo sourceInfo = SourceInfo())
	{return AllocateRangeUninitialized<T>(*mAllocator, count, sourceInfo);}

	template<typename T> Span<T> AllocateRange(size_t& count, SourceInfo sourceInfo = SourceInfo())
	{return AllocateRange<T>(*mAllocator, count, sourceInfo);}

	template<typename T> void FreeRangeUninitialized(Span<T> range)
	{return FreeRangeUninitialized(*mAllocator, range);}

	template<typename T> void FreeRange(Span<T> range)
	{return FreeRange(*mAllocator, range);}

protected:
	A* mAllocator = nullptr;
};

template<typename A> struct AllocatorRef<A, EmptyType, true>: A
{
	AllocatorRef(decltype(nullptr)=nullptr) {}
	AllocatorRef(A& allocator) {(void)allocator;}

	AllocatorRef& operator=(const AllocatorRef&) {return *this;}

	A& GetRef() const {return *const_cast<A*>(static_cast<const A*>(this));}

	template<typename T> Span<T> AllocateRangeUninitialized(size_t& count, SourceInfo sourceInfo = SourceInfo())
	{return AllocateRangeUninitialized<T>(*this, count, sourceInfo);}

	template<typename T> Span<T> AllocateRange(size_t& count, SourceInfo sourceInfo = SourceInfo())
	{return AllocateRange<T>(*this, count, sourceInfo);}
	
	template<typename T> void FreeRangeUninitialized(Span<T> range)
	{return FreeRangeUninitialized(*this, range);}

	template<typename T> void FreeRange(Span<T> range)
	{return FreeRange(*this, range);}
};

template<typename A, typename PARENT> struct AllocatorRef<A, PARENT, true>:
	PARENT, AllocatorRef<A, EmptyType, true>
{
	AllocatorRef(decltype(nullptr)=nullptr) {}
	AllocatorRef(A& allocator) {(void)allocator;}
};

class IAllocator
{
public:
	virtual void* Allocate(size_t bytes, const SourceInfo& sourceInfo = SourceInfo()) = 0;
	virtual void Free(void* ptr) = 0;
	virtual ~IAllocator() {}
};

class ISizedAllocator: public IAllocator
{
public:
	virtual size_t GetAllocationSize() const = 0;
};

template<typename Allocator> class INTRA_EMPTY_BASES PolymorphicUnsizedAllocator: public IAllocator, private Allocator
{
	void* Allocate(size_t bytes, const SourceInfo& sourceInfo = SourceInfo()) final
	{
		return Allocator::Allocate(bytes, sourceInfo);
	}

	void Free(void* ptr) final
	{
		Allocator::Free(ptr);
	}
};

template<typename Allocator> class INTRA_EMPTY_BASES PolymorphicSizedAllocator: public ISizedAllocator, private Allocator
{
	AnyPtr Allocate(size_t& bytes, const SourceInfo& sourceInfo) final
	{
		return Allocator::Allocate(bytes, sourceInfo);
	}

	void Free(void* ptr) final
	{
		Allocator::Free(ptr);
	}

	size_t GetAllocationSize(void* ptr) const final { return Allocator::GetAllocationSize(ptr); }
};

template<typename Allocator> class PolymorphicAllocator: public TSelect<
	PolymorphicSizedAllocator<Allocator>,
	PolymorphicUnsizedAllocator<Allocator>,
	CHasGetAllocationSize<Allocator>>
{};

struct NewAllocator
{
	static Owner<void*> Allocate(size_t& bytes, SourceInfo sourceInfo = SourceInfo())
	{
		(void)sourceInfo;
		return operator new(bytes);
	}

	template<typename T> static Owner<T*> Allocate(TType<T>, size_t& numElements, SourceInfo sourceInfo = SourceInfo())
	{
		if constexpr(CTriviallyDestructible<T> && CMoveAssignable<T>)
			if(IsConstantEvaluated()) return new T[numElements];
		return operator new(numElements*sizeof(T));
	}
	
	template<typename T> static void Free(NotNull<Owner<T*>> ptr, size_t size)
	{
		(void)size;
		if constexpr(CTriviallyDestructible<T> && CMoveAssignable<T>)
			if(IsConstantEvaluated())
			{
				delete[] ptr;
				return;
			}
		operator delete(ptr);
	}
	
	static size_t GetAlignment() {return sizeof(void*)*2;}
};

namespace z_D {
extern "C" {
	INTRA_CRTIMP INTRA_CRTRESTRICT void* INTRA_CRTDECL malloc(size_t bytes);
	INTRA_CRTIMP INTRA_CRTRESTRICT void* INTRA_CRTDECL realloc(void* oldPtr, size_t bytes);
	INTRA_CRTIMP void INTRA_CRTDECL free(void* ptr);
}
}

struct MallocAllocator
{
	static Owner<void*> Allocate(size_t& bytes, SourceInfo sourceInfo = SourceInfo())
	{
		(void)sourceInfo;
		return z_D::malloc(bytes);
	}
	static void Free(void* ptr, size_t size) {(void)size; Free(ptr);}
	static void Free(void* ptr) {z_D::free(ptr);}
	static size_t GetAlignment() {return sizeof(void*)*2;}
};

#ifdef INTRA_DEBUG_ALLOCATORS
using SizedHeapType = ASized<ABoundsChecked<ACallOnFail<SystemHeapAllocator, decltype(NoMemoryAbort)>>>;
using GlobalHeapType = ABoundsChecked<ACallOnFail<SystemHeapAllocator, decltype(NoMemoryAbort)>>;
#else
using SizedHeapType = ASized<ACallOnFail<SystemHeapAllocator, NoMemoryAbort>>;
using GlobalHeapType = ACallOnFail<SystemHeapAllocator, NoMemoryAbort>;
#endif

extern SizedHeapType SizedHeap;
extern GlobalHeapType GlobalHeap;

} INTRA_END
