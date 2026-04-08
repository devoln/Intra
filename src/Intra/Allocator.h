#pragma once

#include <Intra/Platform/Toolchain.h>
#include <Intra/Concepts.h>
#include <Intra/Numeric/Exponential.h>
#include <Intra/Range.h>
#include <Intra/Container/Ownership.h>
#include <Intra/LifeCycle.h>

namespace Intra { INTRA_BEGIN
// Non-relocatable type T can assign this implementation to FPostRelocateFix<T> if the code below fixes the object after relocation.
// Otherwise it either will have to provide its own fix implementation or array reallocation with this type will be less efficient.
template<class T> constexpr void DefaultPostRelocateFix(T* newBaseAddress, [[maybe_unused]] const T* oldFreedBaseAddress, size_t numElementsToPatch)
{
	T temp;
	for(size_t i = 0; i < numElementsToPatch; i++)
	{
		auto& x = newBaseAddress[i];
		temp = INTRA_MOVE(x);
		x = INTRA_MOVE(temp);
	}
}

INTRA_DEFINE_FUNCTOR(AlignUp)<typename T>(size_t alignmentInBytes, T value) requires CBasicIntegral<T> || CBasicPointer<T>
{
	INTRA_PRECONDITION(IsPowerOf<2>(alignmentInBytes));
	if constexpr(CBasicPointer<T>)
	{
		if(IsConstantEvaluated()) return value;
		else return reinterpret_cast<T>((TUnsignedIntOfSizeAtLeast<sizeof(T)>(reinterpret_cast<size_t>(value)) + alignmentInBytes - 1) & (~alignmentInBytes + 1));
	}
	else return T((TUnsignedIntOfSizeAtLeast<sizeof(T)>(value) + alignmentInBytes - 1) & (~alignmentInBytes + 1));
};

INTRA_DEFINE_FUNCTOR(AlignDown)<typename T>(size_t alignmentInBytes, T value) requires CBasicIntegral<T> || CBasicPointer<T>
{
	INTRA_PRECONDITION(IsPowerOf<2>(alignmentInBytes));
	if constexpr(CBasicPointer<T>)
	{
		if(IsConstantEvaluated()) return value;
		else return reinterpret_cast<T>(TUnsignedIntOfSizeAtLeast<sizeof(T)>(reinterpret_cast<size_t>(value)) & (~alignmentInBytes + 1));
	}
	else return T(TUnsignedIntOfSizeAtLeast<sizeof(T)>(value) & (~alignmentInBytes + 1));
};

INTRA_DEFINE_FUNCTOR(IsAligned)<typename T>(size_t alignment, T value) requires CBasicIntegral<T> || CBasicPointer<T> {
	return TUnsignedIntOfSizeAtLeast<sizeof(T)>(value) % alignment == 0;
};



// All allocators should be accessed through these external type-safe functions.
template<typename T> INTRA_FORCEINLINE constexpr Span<T> Allocate(CAllocator auto& rawAllocator, AllocParams<T> params)
{
	if constexpr(CTriviallyDestructible<T> && CMoveAssignable<T>)
		if(IsConstantEvaluated()) // constexpr evaluation ignores allocator and just uses new[]
		{
			auto res = params.NumElements? new T[params.NumElements]{}: nullptr;
			if(params.ExistingMemoryBlock)
			{
				auto prevAllocation = params.ExistingMemoryBlock.Begin;
				for(size_t i = 0; i < params.NumPrevElementsToKeep; i++)
					res[i] = INTRA_MOVE(prevAllocation[i]);
				delete[] prevAllocation;
			}
			return Span(Unsafe, res, res + params.NumElements);
		}
	
	constexpr auto customRelocateFunc = []() -> RawAllocParams::TCustomRelocateSignature* {
		if constexpr(CDefined<decltype(FPostRelocateFix<T>)>) return nullptr; // if T has a defined post-relocation fix or requires no fix, allow allocator to use memcpy
		return [](char* dst, char* src, size_t numBytes) {
			for(size_t i = 0, n = numBytes / sizeof(T); i < n; i++)
				static_cast<T*>(dst)[i] = INTRA_MOVE(static_cast<T*>(src)[i]);
		};
	}();

	const auto rawRes = rawAllocator({
		.ByteAllocParams = {
			.ExistingMemoryBlock = reinterpret_cast<const Span<char>&>(params.ExistingMemoryBlock),
			.NumElements = params.NumElements * sizeof(T),
			.ValueInitialize = CTriviallyConstructible<T> && params.ValueInitialize,
			.NumPrevElementsToKeep = params.NumPrevElementsToKeep * sizeof(T)
		},
		.CustomRelocateFunc = customRelocateFunc
	}).Allocation;

	const auto res = reinterpret_cast<const Span<T>&>(rawRes);
	if constexpr(CDefined<decltype(FPostRelocateFix<T>)> && FPostRelocateFix<T>) // apply a post-relocation fix if necessary
		if(res.Begin != params.ExistingMemoryBlock.Begin) FPostRelocateFix<T>(res.Begin, params.ExistingMemoryBlock.Begin, params.NumPrevElementsToKeep);

	if constexpr(!CTriviallyConstructible<T>) if(params.ValueInitialize)
	{
		for(size_t i = params.NumPrevElementsToKeep, n = params.NumElements; i < n; i++)
			LifeCycle::ConstructOne(res.Begin + i);
	}

	return res;
}

template<typename T> INTRA_FORCEINLINE constexpr void Free(CAllocator auto& allocator, T* ptr)
{
	Allocate<T>(allocator, {.ExistingMemoryBlock = Span(Unsafe, ptr, ptr)});
}

template<typename T> INTRA_FORCEINLINE constexpr size_t AllocationCapacity(CAllocator auto& allocator, T* ptr)
{
	const auto res = allocator(RawAllocParams::MakeCommand(RawAllocParams::Cmd::GetSize, ptr));
	return res.RawValue[0] / sizeof(T);
}


#if !defined(INTRA_OPTIMIZE_FOR_SEGMENT_HEAP) && defined(WINAPI_FAMILY) && WINAPI_FAMILY == 2 // assume Windows 10 2004+: segment heap is introduced
#define INTRA_OPTIMIZE_FOR_SEGMENT_HEAP
#endif

// Can be used to implement a linear allocator for any kind of intervals or memory
template<bool ThreadSafe> struct LinearAllocatorLogic
{
	TSelectAtomic<size_t, ThreadSafe> Head{};

	// assumes unlimited buffer size
	[[nodiscard]] constexpr size_t Allocate(size_t sizeInBytes) {return Head.GetAdd(sizeInBytes);}

	// assumes unlimited buffer size
	[[nodiscard]] constexpr size_t Allocate(size_t sizeInBytes, size_t alignment)
	{return tryReallocate<true, false, false>(sizeInBytes, 0, 0, 0, alignment).Or(0);}

	[[nodiscard]] constexpr Optional<size_t> TryAllocate(size_t sizeInBytes, size_t bufferSize)
	{return tryReallocate<false, false, true>(sizeInBytes, bufferSize, 0, 0, 0);}

	[[nodiscard]] constexpr Optional<size_t> TryAllocate(size_t sizeInBytes, size_t bufferSize, size_t alignment)
	{return tryReallocate<true, false, true>(sizeInBytes, bufferSize, 0, 0, alignment);}

	[[nodiscard]] constexpr Optional<size_t> TryReallocate(size_t sizeInBytes, size_t bufferSize, size_t prevBegin, size_t prevEnd, size_t alignment)
	{return tryReallocate<true, true, true>(sizeInBytes, bufferSize, prevBegin, prevEnd, alignment);}

	[[nodiscard]] constexpr Span<char> Allocate(size_t sizeInBytes, size_t alignment, Span<char> buffer)
	{return buffer|DropExactly(Allocate(sizeInBytes, Length(buffer), alignment))|TakeExactly(sizeInBytes);}

	[[nodiscard]] constexpr Span<char> Reallocate(Span<char> existing, size_t sizeInBytes, size_t alignment, Span<char> buffer)
	{
		const auto res = TryReallocate(sizeInBytes, Length(buffer),
			existing.Begin - buffer.Begin, existing.End - buffer.End, alignment);
		if(!res) return {};
		return buffer|DropExactly(res.Unwrap())|TakeExactly(sizeInBytes);
	}

private:
	template<bool Align, bool Realloc, bool Limit> [[nodiscard]] constexpr Optional<size_t> tryReallocate(
		size_t size, size_t bufferSize, size_t prevBegin, size_t prevEnd, size_t alignment)
	{
		size_t prevHead = Head.template Get<MemoryOrder::Relaxed>(), newHead = 0;
		do
		{
			if(Realloc && prevHead == prevEnd)
				newHead = prevBegin + size;
			else if constexpr(Align) newHead = AlignUp(alignment, prevHead) + size;
			else newHead = prevHead + size;
			if constexpr(Limit) if(newHead > bufferSize) return {};
			if constexpr(!ThreadSafe) Head.Set(newHead);
		} while(ThreadSafe && Head.template WeakCompareGetSet<MemoryOrder::Release, MemoryOrder::Relaxed>(prevHead, newHead) != prevHead);
		return prevHead;
	}
};

template<CConvertibleToSpan B, bool ThreadSafe = false> struct LinearAllocator
{
	constexpr LinearAllocator(B uninitializedBuffer): mBuffer(uninitializedBuffer) {}

	constexpr RawAllocResult operator()(RawAllocParams params)
	{
		return {.Allocation = Impl.Reallocate(params.ByteAllocParams.ExistingMemoryBlock,
			params.ByteAllocParams.NumElements, sizeof(void*) * 2, mBuffer)};
	}

	constexpr Size SpaceLeft() const {return Length(mBuffer) - Impl.Head.Get();}

	// Freeing elements can be achieved by moving back Impl.Head
	LinearAllocatorLogic<ThreadSafe> Impl;
private:
	B mBuffer;
};

template<CBasicIntegral IndexT, typename GenT> requires CBasicIntegral<GenT> || (!CDefined<GenT>)
struct BaseCheckedID
{
	bool operator==(const BaseCheckedID& rhs) const = default;
	bool operator!=(const BaseCheckedID& rhs) const = default;
	INTRA_FORCEINLINE explicit operator bool() const {return SlotIndex != MaxValueOf<IndexT>;}
protected:
	IndexT SlotIndex = MaxValueOf<IndexT>;
	INTRA_NO_UNIQUE_ADDRESS GenT Generation = []{if constexpr(CBasicIntegral<GenT>) return MaxValueOf<GenT>; else return GenT{};};
};

// Can be used to implement pool for Span or for any other CConvertibleToSpan: see GenericItemPool below
template<typename T, typename GenT = TUndefined> requires CBasicIntegral<GenT> || (!CDefined<GenT>)
struct ItemPoolImpl
{
	using IndexT = TUnsignedIntOfSizeAtMost<Min(sizeof(size_t), sizeof(T))>;
	struct ID: BaseCheckedID<IndexT, GenT> {friend ItemPoolImpl;};
	struct Item
	{
		union
		{
			T ValueIfAllocated;
			IndexT NextFreeIndex;
		};
		INTRA_NO_UNIQUE_ADDRESS GenT Generation;

		Item() {}
		~Item() {}
	};
	static_assert(CDefined<GenT> || sizeof(Item) == Max(sizeof(T), sizeof(IndexT)));

	ItemPoolImpl() = default;
	ItemPoolImpl(ItemPoolImpl&&) = default;
	ItemPoolImpl& operator=(ItemPoolImpl&&) = default;

	explicit ItemPoolImpl(Span<T> uninitializedBuffer): ItemPoolImpl(Span(
		reinterpret_cast<Item*>(uninitializedBuffer.Begin),
		reinterpret_cast<Item*>(uninitializedBuffer.End))) {}

	constexpr explicit ItemPoolImpl(Span<Item> unitializedBuffer) {RebindToNewBuffer(unitializedBuffer);}

	// Expected to be called after the caller grows the buffer to allow more allocations.
	// NOTE: Growing is only possible when the pool is empty.
	constexpr void RebindToNewBuffer(Span<Item> newBuffer, Size oldBufferSize)
	{
		INTRA_PRECONDITION(!CanAllocate(oldBufferSize));
		INTRA_PRECONDITION(newBuffer.Length() < MaxValueOf<IndexT>);
		for(size_t i = size_t(oldBufferSize), n = size_t(newBuffer.Length()); i < n; i++)
			newBuffer[i].NextFreeIndex = i + 1;
		mFirstFreeItem = IndexT(oldBufferSize);
	}

	[[nodiscard]] constexpr bool CanAllocate(Size bufferSize) const {return mFirstFreeItem < bufferSize;}

	constexpr Optional<IndexT> AllocateIndex(Span<Item> buffer)
	{
		auto res = mFirstFreeItem;
		mFirstFreeItem = buffer[res].NextFreeIndex;
		return res;
	}

	constexpr void FreeIndex(IndexT index, Span<Item> buffer)
	{
		buffer[index].NextFreeIndex = mFirstFreeItem;
		if constexpr(CDefined<GenT>) buffer[index].Generation++;
		mFirstFreeItem = index;
	}

	Owner<T*> AllocateOne(Span<Item> buffer, SourceInfo = {})
	{
		if(auto index = AllocateIndex(buffer))
			return &buffer[index.Unwrap()].ValueIfAllocated;
		return nullptr;
	}

	void FreeOne(NotNull<Owner<T*>> ptr, Span<Item> buffer) {FreeIndex(reinterpret_cast<Item*>(ptr) - buffer.Begin);}

	bool ReleaseOne(NotNull<Owner<T*>> ptr, Span<Item> buffer)
	{
		bool release = true;
		if constexpr(CRefCounted<T>) release = ptr->RefCount.DecRef();
		if(release) FreeOne(ptr, buffer);
		return release;
	}

	[[nodiscard]] INTRA_FORCEINLINE constexpr bool IsValidID(ID id, Span<Item> buffer) const requires CDefined<GenT>
	{
		return buffer[id.SlotIndex].Generation == id.Generation;
	}

private:
	IndexT mFirstFreeItem = MaxValueOf<IndexT>;
};

template<typename T, CConvertibleToSpan B, typename GenT = TUndefined>
struct GenericItemPool
{
	using IndexT = typename ItemPoolImpl<T, GenT>::IndexT;
	using ID = typename ItemPoolImpl<T, GenT>::ID;
	using Item = typename ItemPoolImpl<T, GenT>::Item;

	GenericItemPool() = default;
	GenericItemPool(GenericItemPool&&) = default;
	GenericItemPool& operator=(GenericItemPool&&) = default;

	explicit GenericItemPool(B uninitializedBuffer) requires(!CResizableArrayContainer<B>): GenericItemPool(Span(Unsafe,
		reinterpret_cast<Item*>(uninitializedBuffer.Begin),
		reinterpret_cast<Item*>(uninitializedBuffer.End))) {}

	constexpr explicit GenericItemPool(Span<Item> uninitializedBuffer) requires(!CResizableArrayContainer<B>):
		mImpl(uninitializedBuffer), mBuffer(uninitializedBuffer) {}

	// Use to grow the buffer to allow more allocations.
	// NOTE: Resize is only possible when there are no free elements in the pool.
	constexpr void Resize(Size newSize) requires CResizableArrayContainer<B>
	{
		const auto oldSize = mBuffer.size();
		mBuffer.Resize(newSize);
		mImpl.RebindToNewBuffer(mBuffer, oldSize);
	}

	// Expected to be called after the caller grows the buffer to allow more allocations.
	// NOTE: Growing is only possible when the pool is empty.
	constexpr void RebindToNewBuffer(B newBuffer) requires(!CResizableArrayContainer<B>)
	{
		mImpl.RebindToNewBuffer(newBuffer, mBuffer.size());
		mBuffer = newBuffer;
	}

	[[nodiscard]] constexpr bool CanAllocate() const {return CResizableArrayContainer<B> || CanAllocateWithoutGrowing();}
	[[nodiscard]] constexpr bool CanAllocateWithoutGrowing() const {return mImpl.CanAllocate(mBuffer.size());}
	constexpr IndexT AllocateIndex()
	{
		if constexpr(CResizableArrayContainer<B>) if(!CanAllocateWithoutGrowing()) grow();
		return mImpl.AllocateIndex(mBuffer).Unwrap();
	}
	constexpr void FreeIndex(IndexT index) {mImpl.FreeIndex(index, mBuffer);}

	constexpr T& Get(IndexT index) {return mBuffer[index].ValueIfAllocated;}
	constexpr const T& Get(IndexT index) const {return mBuffer[index].ValueIfAllocated;}

	Owner<T*> AllocateOne(SourceInfo = {})
	{
		if constexpr(CResizableArrayContainer<B>) if(!CanAllocateWithoutGrowing()) grow();
		return mImpl.AllocateOne(mBuffer);
	}
	void FreeOne(NotNull<Owner<T*>> ptr) {mImpl.FreeOne(ptr, mBuffer);}
	bool ReleaseOne(NotNull<Owner<T*>> ptr) {return mImpl.ReleaseOne(ptr, mBuffer);}

	[[nodiscard]] INTRA_FORCEINLINE constexpr bool IsValidID(ID id) const requires CDefined<GenT> {return mImpl.IsValidID(id, mBuffer);}

private:
	ItemPoolImpl<T, GenT> mImpl;
	B mBuffer;
	void grow() requires CResizableArrayContainer<B> {Resize(mBuffer.size() + mBuffer.size() / 2 + 1);}
};

template<typename T> INTRA_CLASS_ALIAS(ItemPool, GenericItemPool<T, Span<typename ItemPoolImpl<T>::Item>>);
//template<typename T> INTRA_CLASS_ALIAS(DynItemPool, GenericItemPool<T, ResizableArray<typename ItemPoolImpl<T>::Item>>);

template<typename T = void> struct FreeList
{
	using TT = TSelect<uint8, T, CVoid<T>>;
	union Node
	{
		TT ValueIfAllocated;
		Node* NextFree = nullptr;
		[[nodiscard]] constexpr Node* NextListNode() const {return NextFree;}
	};

	INTRA_NOINLINE static FreeList FromSpan(Span<uint8> buf, size_t elementSize = sizeof(Node), size_t alignment = alignof(Node))
	{
		INTRA_PRECONDITION(elementSize >= sizeof(Node));
		uint8* bufPtr = AlignUp(alignment, buf.Begin);
		elementSize = AlignUp(alignment, elementSize);

		const auto numElements = size_t(buf.End - bufPtr) / elementSize;
		FreeList res = {};
		if(!numElements) return res;

		res.FirstFreeItem = reinterpret_cast<Node*>(bufPtr);
		bufPtr += elementSize;

		Node* nextNodeToInit = res.FirstFreeItem;
		for(size_t i = 1; i < numElements; i++)
		{
			nextNodeToInit = nextNodeToInit->NextFree = reinterpret_cast<Node*>(bufPtr);
			bufPtr += elementSize;
		}
		nextNodeToInit->NextFree = nullptr;
		return res;
	}

	static FreeList FromSpan(Span<Node> buf)
	{
		return FromSpan(Span<uint8>(Unsafe,
			reinterpret_cast<uint8*>(buf.Begin),
			reinterpret_cast<uint8*>(buf.End)));
	}

	INTRA_FORCEINLINE constexpr bool CanAllocate() const {return FirstFreeItem != nullptr;}

	constexpr Owner<Node*> AllocateNode()
	{
		if(!CanAllocate()) return nullptr;
		const auto head = FirstFreeItem;
		FirstFreeItem = head->NextFree;
		return head;
	}

	constexpr void FreeNode(NotNull<Owner<Node*>> ptr)
	{
		ptr->NextFree = FirstFreeItem;
		FirstFreeItem = ptr;
	}

	Owner<T*> AllocateOne() {return &AllocateNode()->ValueIfAllocated;}
	void FreeOne(NotNull<Owner<T*>> ptr) {FreeNode(reinterpret_cast<Node*>(ptr));}
	bool ReleaseOne(NotNull<Owner<T*>> ptr)
	{
		if constexpr(!CRefCounted<T>) {FreeOne(ptr); return true;}
		else if(ptr->RefCount.DecRef()) {FreeOne(ptr); return true;}
		return false;
	}

	Node* FirstFreeItem = nullptr;
};

} INTRA_END
