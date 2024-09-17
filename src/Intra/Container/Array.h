#pragma once

#include <Intra/Core.h>
#include <Intra/Concepts.h>
#include <Intra/Container.h>
#include <Intra/Range.h>
#include <Intra/LifeCycle.h>
#include <Intra/TypeErasure.h>

#include "Intra/Range/Comparison.h"


namespace Intra { INTRA_BEGIN

struct ArrayDesc
{
	enum class DynType: uint8 {
		// no push_back/push_front support, size = capacity
		Static,
		// supports push_back and pop_back: separate size and capacity
		Growing,
		// supports push_back, pop_back, push_front, pop_front: separate left free space, size and capacity
		GrowingQueue
	};
	uint32 InplaceCapacity = 0;
	bool IsCompact = false;
	DynType Type;

	[[nodiscard]] constexpr bool IsGrowing() const {return Type != DynType::Static;}
	[[nodiscard]] constexpr bool IsGrowingQueue() const {return Type == DynType::GrowingQueue;}
};

namespace z_D {
template<typename T> struct GenericArrayImpl
{
	Span<T> Buffer, Range;
	bool IsOwning = false;

	/// @brief Insert values read from reader into pos.
	/// As a result for each i-th element its index will stay unchanged if i < pos and will become i + values.Length() otherwise.
	/// The first inserted element will have index pos.
	/// @return true if reallocated
	INTRA_NOINLINE constexpr bool Insert(size_t pos, IGenericReader<T>& reader, index_t numValues, IAllocator* allocator) requires CConstructible<T>
	{
		const auto leftUnusedCapacity = Range.Begin - Buffer.Begin;
		const auto rightUnusedCapacity = Buffer.End - Range.End;

		// If there is not enough space available, reallocate and move
		if(numValues > rightUnusedCapacity)
		{
			if(!allocator) return true;
			const size_t newMinCapacity = size_t(Range.Length() + numValues) + size_t(Buffer.Length()) / 2;
			Span<T> newBuffer = Allocate<T>(*allocator, {.NumElements = newMinCapacity});
			Span<T> newRange = newBuffer|Drop(leftUnusedCapacity)|Take(Length() + numValues);
			auto dst = newRange|Drop(pos)|Take(numValues);
			INTRA_TRY {
				if constexpr(!CTriviallyConstructible<T> && !CTriviallyCopyable<T>) LifeCycle::ValueInitialize(dst);
				reader.ReadSome(dst); // reader is expected to be non-blocking, read exactly numValues values or throw
			} INTRA_CATCH_ALL {
				LifeCycle::Destruct(dst);
				Free(mAllocator, newBuffer.Begin);
				INTRA_RETHROW;
			}
			LifeCycle::MoveConstructDestruct(newRange.Take(pos), Range|Take(pos));
			LifeCycle::MoveConstructDestruct(newRange|Drop(pos + numValues), Range|Drop(pos));
			if(IsOwning) Free(*allocator, Buffer.Begin);
			Buffer = newBuffer;
			Range = newRange;
			return true;
		}

		if(pos >= (Length() + numValues) / 2 || leftUnusedCapacity < numValues)
		{ // move valuesLength positions forward
			Range.End += numValues;
			LifeCycle::MoveConstructDestructBackward(Range|Drop(pos + numValues), Range|Drop(pos)|DropLast(numValues));
		}
		else //move valuesCount positions backward
		{
			Range.Begin -= numValues;
			LifeCycle::MoveConstructDestructForward(Range|Take(pos), Range|Drop(numValues)|Take(pos));
		}
		auto dst = Range|Drop(pos)|Take(numValues);
		INTRA_TRY {
			if constexpr(!CTriviallyConstructible<T> && !CTriviallyCopyable<T>) LifeCycle::ValueInitialize(dst);
			reader.ReadSome(dst); // reader is expected to be non-blocking, read exactly valuesLength values or throw
		} INTRA_CATCH_ALL {
			LifeCycle::Destruct(dst);
			INTRA_RETHROW;
		}
		return false;
	}

	INTRA_NOINLINE void Resize(size_t rightPartSize, size_t leftPartSize, IAllocator* allocator, bool valueInitialize)
	{
		if(size_t(rightPartSize) < size_t(Range.Length()))
		{ // Destroy the elements out of the new buffer bounds
			LifeCycle::Destruct(Range|Drop(rightPartSize));
			Range.End = Range.Begin + rightPartSize;
		}

		size_t newCapacity = size_t(rightPartSize) + size_t(leftPartSize);
		Span<T> newBuffer, newRange;
		if(!leftPartSize)
		{
			newBuffer = Allocate(*allocator, AllocParams<T>{
				.ExistingMemoryBlock = Buffer,
				.ValueInitialize = valueInitialize,
				.NumElements = newCapacity,
				.NumPrevElementsToKeep = size_t(Range.Length())
			});
		}
		newRange = newBuffer|Drop(leftPartSize)|Take(Range.Length());
		if(!Buffer.Empty())
		{
			// Move elements to the new buffer
			LifeCycle::MoveInitDestruct(newRange, Range);
			if(IsAllocated()) Free(mAllocator, Span(Unsafe, mCb.BufferBegin, getCbBufferEnd()));
		}
		Buffer = newBuffer;
		Range = newRange;
	}
};
}

// Generic array that can be parametrized to store its elements inplace, 
//  allocate via allocator or choose at runtime depending on size (small buffer optimization).
template<CMoveConstructible T, ArrayDesc D, COptAllocator Allocator = DefaultAllocator> class GenericArray
{
public:
	GenericArray() = default;

	explicit INTRA_FORCEINLINE constexpr GenericArray(index_t initialLength, Allocator al = Allocator()):
		mAllocator(INTRA_MOVE(al)) {SetLength(initialLength);}

	template<CList L> requires(!CSameUnqualRef<L, GenericArray>)
	constexpr GenericArray(L&& values, Allocator al = Allocator()):
		mAllocator(INTRA_MOVE(al)) {AddLastRange(RangeOf(INTRA_FWD(values)));}

	constexpr GenericArray(GenericArray&& rhs) noexcept(!D.InplaceCapacity || CNothrowMoveConstructible<T>):
		mAllocator(INTRA_MOVE(rhs.mAllocator))
	{
		if(this == &rhs) return;
		if constexpr(!D.InplaceCapacity || CTriviallyRelocatable<T> && sizeof(m) <= 64) m = rhs.m;
		else
		{
			if(rhs.IsInplace()) LifeCycle::MoveInitDestruct(Unsafe, m.InplaceBuf, rhs.m.InplaceBuf, rhs.getShortLength());
			else m.Cb = rhs.m.Cb;
			m.StorageInfo = rhs.m.StorageInfo;
		}
		rhs.setFieldsToDefaultConstructedState();
	}

	constexpr GenericArray(const GenericArray& rhs): GenericArray(Span(rhs)) {}
	
	INTRA_FORCEINLINE constexpr ~GenericArray() {operator=(nullptr);}


	/** Take ownership of rangeToOwn elements and memory.
	  @warning `rangeToOwn` must be allocated with the same allocator as the template argument Allocator.
	*/
	[[nodiscard]] static constexpr GenericArray CreateAsOwnerOf(TUnsafe, Span<T> rangeToOwn)
	{
		GenericArray res;
		res.setCb(rangeToOwn.Begin, rangeToOwn.End);
		return res;
	}

	constexpr GenericArray& operator=(const GenericArray& rhs)
	{
		if(this == &rhs) return *this;
		return Assign(Span(rhs));
	}

	/** Move array.
	  Destruct all this GenericArray's elements.
	  Takes ownerwhip of all rhs elements.
	  rhs becomes empty but takes ownership of this GenericArray memory allocation.
	*/
	constexpr GenericArray& operator=(GenericArray&& rhs)
	{
		if(this == &rhs) return *this;
		Clear();
		if constexpr(D.InplaceCapacity) if(rhs.IsInplace())
		{
			if(IsAllocated()) Free(mAllocator, m.Cb.BufferBegin);
			LifeCycle::MoveInitDestruct(Unsafe, m.InplaceBuf, rhs.m.InplaceBuf, rhs.getShortLength());
			m.StorageInfo = rhs.m.StorageInfo;
			rhs.setFieldsToDefaultConstructedState();
			return;
		}
		m.Cb = rhs.mCb;
		m.StorageInfo = rhs.m.StorageInfo;
		rhs.setFieldsToDefaultConstructedState();
		return *this;
	}

	/// Delete all elements and free memory.
	void Reset()
	{
		Clear();
		if constexpr(CDefined<Allocator>) if(IsAllocated()) Free(mAllocator, m.Cb.BufferBegin);
		setFieldsToDefaultConstructedState();
	}

	template<typename U> void Assign(Span<const U> rhs)
	{
		Clear();
		SetLengthRaw(rhs.Length());
		LifeCycle::CopyConstruct(*this, rhs);
	}

	template<typename U> void Assign(Span<U> rhs) {Assign(ConstSpanOf(rhs));}

	/// Insert a new element to the beginning of the array.
	/// Unlike most other array implementations this operation has O(1) complexity.
	constexpr T& AddFirst(T value) requires(D.IsGrowingQueue())
	{
		if constexpr(!CDefined<Allocator>) INTRA_PRECONDITION(LeftSpace() >= 1);
		else if(!LeftSpace()) growLeft();
		const auto newCbBegin = getCbBegin() - 1;
		auto& res = *LifeCycle::ConstructOne(newCbBegin, INTRA_MOVE(value));
		setCbBegin(newCbBegin);
		return res;
	}

	/// Construct a new element at the beginning of the array with constructor parameters args.
	/// Unlike most other array implementations this operation has amortized constant O(1) complexity.
	template<typename... Args> requires CConstructible<T, Args...>
	constexpr T& EmplaceFirst(Args&&... args) requires(D.IsGrowingQueue())
	{
		if constexpr(!CDefined<Allocator>) INTRA_PRECONDITION(LeftSpace() >= 1);
		else if(!LeftSpace())
		{
			// If args reference any value from this array, they will become dangling references after reallocation.
			// To avoid this, construct a temporary object
			return AddFirst(T(INTRA_FWD(args)...));
		}
		const auto newCbBegin = getCbBegin() - 1;
		auto& res = *LifeCycle::ConstructOne(newCbBegin, INTRA_FWD(args)...);
		setCbBegin(newCbBegin);
		return res;
	}

	/// Add all range values to the beginning of the array.
	template<CConsumableListOf<T> L> constexpr void AddFirstRange(L&& values) requires(D.IsGrowingQueue())
	{
		if constexpr(CForwardList<L> || CHasLength<L>)
		{
			auto valueRange = RangeOf(INTRA_FWD(values));
			const auto valuesLength = values|Count;
			if(LeftSpace() < valuesLength)
			{
				if(!Empty())
				{
					// Use slower implementaation with a temporary copy.
					// Otherwise reallocation may affect valueRange
					addFirstRangeHelper(INTRA_MOVE(valueRange));
					return;
				}
				Reserve(0, valuesLength);
			}
			const auto newCbBegin = getCbBegin() - valuesLength;
			for(T* dst = newCbBegin; !valueRange.Empty();)
				LifeCycle::ConstructOne(dst++, valueRange|Next);
			setCbBegin(newCbBegin);
		}
		else addFirstRangeHelper(INTRA_FWD(values));
	}

	/// @brief Add value to the end of the array.
	/// This operation has amortized constant O(1) complexity.
	constexpr T& AddLast(T value) requires(D.IsGrowing())
	{
		if constexpr(!CDefined<Allocator>) INTRA_PRECONDITION(RightSpace() >= 1);
		else if(!RightSpace()) growRight();
		auto& res = *LifeCycle::ConstructOne(getEnd(), INTRA_MOVE(value));
		addToLengthField(1);
		return res;
	}

	/// Construct a new element at the end of the array passing args to constructor
	template<typename... Args> requires CConstructible<T, Args...>
	T& EmplaceLast(Args&&... args) requires(D.IsGrowing())
	{
		if constexpr(!CDefined<Allocator>) INTRA_PRECONDITION(RightSpace() >= 1);
		else if(!RightSpace())
		{
			// If args reference any value from this array, they will become dangling references after reallocation.
			// To avoid this, construct a temporary object.
			return AddLast(T(INTRA_FWD(args)...));
		}
		auto& res = *LifeCycle::ConstructOne(getEnd(), INTRA_FWD(args)...);
		addToLengthField(1);
		return res;
	}

	/// Add all list values to the end of the array.
	template<CConsumableListOf<T> L> INTRA_NOINLINE constexpr void AddLastRange(L&& values) requires(D.IsGrowing())
	{
		if constexpr((CForwardList<L> || CHasLength<L>))
		{
			auto valueRange = RangeOf(INTRA_FWD(values));
			const auto valuesLength = valueRange|Count;
			if(RightSpace() < valuesLength)
			{
				if(!Empty())
				{
					// use slower implementation with a temporary copy
					// otherwise reallocation may affect valueRange
					addLastRangeHelper(INTRA_MOVE(valueRange));
					return;
				}
				Reserve(valuesLength);
			}
			const auto prevEnd = getEnd();
			auto end = prevEnd;
			INTRA_TRY {
				for(auto n = size_t(valuesLength); n--;)
				{
					LifeCycle::ConstructOne(end, valueRange|Next);
					end++;
				}
				setEnd(end);
			} INTRA_CATCH_ALL {
				LifeCycle::Destruct(Unsafe, prevEnd, end - prevEnd);
				INTRA_RETHROW;
			}
		}
		else addLastRangeHelper(RangeOf(INTRA_FWD(values)));
	}

	/// @brief Set element at index pos to value.
	/// If pos >= Length(), add (pos - Length()) default initialized elements and add value.
	template<typename U> constexpr T& Set(Index pos, U&& value)
	{
		if(pos >= Length())
		{
			Reserve(pos + 1);
			SetLength(pos);
			return AddLast(INTRA_FWD(value));
		}
		operator[](pos) = INTRA_FWD(value);
	}

	/// @brief Insert values into pos.
	/// As a result for each i-th element its index will stay unchanged if i < pos and will become i + values.Length() otherwise.
	/// The first inserted element will have index pos.
	template<CList L> requires CSame<TListValue<L>, T> constexpr void Insert(Index pos, L&& values)
	{
		if(values.Empty()) return;
		z_D::GenericArrayImpl<T> arr{.Buffer = getBuffer(), .Range = Span(*this), .IsOwning = IsAllocated()};
		RangeToReader<false, TRangeOf<L>, IGenericReader<T>> reader(RangeOf(INTRA_FWD(values)));
		if constexpr(CDefined<Allocator>)
		{
			PolymorphicAllocator<TSelect<Allocator, Allocator&, CEmpty<Allocator>>> allocator(mAllocator);
			arr.Insert(size_t(pos), reader, Length(values), &allocator);
		}
		else arr.Insert(size_t(pos), reader, Length(values), nullptr);
		if constexpr(D.InplaceCapacity) if(arr.Buffer.Begin == m.InplaceBuf)
		{
			setShortLength(size_t(arr.Range.Length()));
			return;
		}
		setCb(arr.Buffer.Begin, arr.Range.Begin, arr.Range.End, arr.Buffer.End);
	}

	template<CList L> requires CSame<TListValue<L>, T> constexpr void Insert(const T* it, L&& values)
	{
		INTRA_PRECONDITION(Span(*this).ContainsAddress(it));
		Insert(it - Data(), INTRA_FWD(values));
	}

	INTRA_FORCEINLINE constexpr void Insert(Index pos, const T& value) {Insert(pos, Span<const T>(Unsafe, &value, 1));}

	constexpr void Insert(const T* it, const T& value)
	{
		INTRA_PRECONDITION(Span(*this).ContainsAddress(it));
		Insert(it - Data(), value);
	}

	/// Get and remove the last array element.
	constexpr T PopLastElement()
	{
		T res = INTRA_MOVE(Last());
		RemoveLast();
		return res;
	}


	/// Pop the first element from the end.
	constexpr T PopFirstElement()
	{
		T res = INTRA_MOVE(First());
		RemoveFirst();
		return res;
	}

	/// Set new capacity of the array (maybe rounded up by allocator, cannot be less than D.InplaceCapacity).
	/// If rightPartSize < Length() then calls destructor for all elements with index >= rightPartSize.
	void Resize(Size rightPartSize, Size leftPartSize) requires(D.IsGrowingQueue())
	{
		if((size_t(rightPartSize)|size_t(leftPartSize)) == 0) {Reset(); return;}

		// Delete all elements out of the new bounds
		if(size_t(rightPartSize) <= size_t(Length())) LifeCycle::Destruct(*this|Drop(rightPartSize));

		size_t newCapacity = size_t(rightPartSize) + size_t(leftPartSize);
		if(newCapacity > D.InplaceCapacity)
		{
			Span<T> newBuffer = Allocate<T>(mAllocator, newCapacity);
			Span<T> newRange = newBuffer | Drop(leftPartSize) | Take(Length());
		}
		if(!mBuffer.Empty())
		{
			//Move elements to the new mBuffer
			LifeCycle::MoveInitDestruct(newRange, Span(*this));
			if(IsAllocated()) Free(mAllocator, Span(Unsafe, mCb.BufferBegin, getCbBufferEnd()));
		}
		mBuffer = newBuffer;
		mRange = newRange;
	}

	/// Set new capacity of the array (maybe rounded up by allocator, cannot be less than D.InplaceCapacity).
	/// If rightPartSize < Length() then calls destructor for all elements with index >= rightPartSize.
	void Resize(Size rightPartSize) requires(D.IsGrowing())
	{
		if(!rightPartSize) { Reset(); return; }

		// Delete all elements out of the new bounds
		if(rightPartSize <= size_t(Length())) LifeCycle::Destruct(*this | Drop(rightPartSize));

		if(rightPartSize > D.InplaceCapacity)
		{
			Span<T> newBuffer = Allocate<T>(mAllocator, newCapacity);
			Span<T> newRange = newBuffer | Drop(leftPartSize) | Take(Length());

			if(!mBuffer.Empty())
			{
				//Move elements to the new mBuffer
				LifeCycle::MoveInitDestruct(newRange, Span(*this));
				if(IsAllocated()) Free(mAllocator, mCb.BufferBegin);
			}
			mBuffer = newBuffer;
			mRange = newRange;
		}
	}

	/// @brief Makes sure that the array has enough capacity.
	/// If it already has enough space to add at least rightPart - Length() new elements to the end
	/// to add at least `leftSpace` elements to the beginning without reallocation then it does nothing.
	/// Otherwise reallocates space and moves elements to a new memory allocation using move costructor and destructor.
	void Reserve(Index rightPart, Index leftSpace) requires(D.IsGrowingQueue())
	{
		const auto currentRightPartSize = size_t(getCbBufferEnd() - getCbBegin());
		const auto currentLeftSpace = size_t(LeftSpace());
		if(size_t(rightPart) <= currentRightPartSize && size_t(leftSpace) <= currentLeftSpace) return;

		const auto currentSize = size_t(Capacity());
		if(size_t(rightPart) > 0)
		{
			if(size_t(leftSpace) > 0) Resize(currentSize / 4 + size_t(rightPart), currentSize / 4 + size_t(leftSpace));
			else Resize(currentSize / 2 + size_t(rightPart), currentLeftSpace);
		}
		else Resize(currentRightPartSize, currentLeftSpace + currentSize/2 + size_t(leftSpace));
	}

	void Reserve(Index rightPart) requires(D.IsGrowing())
	{
		if(size_t(rightPart) > RightSpace())
			Resize(size_t(Capacity()) / 2 + size_t(rightPart));
	}

	/// A convenient alternative to Reserve to call before adding new elements.
	void CheckSpace(Index rightSpace) requires(D.IsGrowing()) {Reserve(size_t(Length()) + size_t(rightSpace));}
	void CheckSpace(Index rightSpace, Index leftSpace) requires(D.IsGrowingQueue()) {Reserve(size_t(Length()) + size_t(rightSpace), leftSpace);}

	/// Remove all array elements without freeing allocated memory.
	void Clear() requires(D.IsGrowing())
	{
		LifeCycle::Destruct(Span(*this));
		setLengthField(0);
	}

	/// @returns true if ArrayList is empty.
	constexpr bool Empty() const noexcept
	{
		if constexpr(D.InplaceCapacity)
		{
			if(m.StorageInfo == emptyInplaceFieldValue) return true;
			if(IsInplace()) return false;
		}
		return getCbBegin() == getCbEnd();
	}

	/// @returns number of elements that can be inserted into the beginning of the array before reallocation is necessary.
	constexpr index_t LeftSpace() const noexcept requires(D.IsGrowingQueue())
	{
		if constexpr(D.InplaceCapacity) if(IsInplace()) return 0;
		return getCbBegin() - m.Cb.BufferBegin;
	}

	/// @return Number of elements that can be inserted into the end of the array before reallocation is necessary.
	constexpr index_t RightSpace() const noexcept requires(D.IsGrowing())
	{
		if constexpr(D.InplaceCapacity) if(IsInplace()) return Intra::Length(m.InplaceBuf) - getShortLength();
		return getCbBufferEnd() - getCbEnd();
	}

	/// @name Element order preserving remove operations
	/// @warning: These operations invalidate all ranges, iterators and pointers referring to the elements of this GenericArray.
	/// Remove one element at index.
	constexpr void Remove(Index index) requires(D.IsGrowing())
	{
		operator[](index).~T();

		// The ratio 1/4 instead of 1/2 was selected, because moving of overlapping memory
		// forward is ~2 times slower then backwards
		if constexpr(D.IsGrowingQueue()) if(index < size_t(Length()) / 4) // Moving the left part forward
		{
			LifeCycle::MoveInitDestructBackward(*this|Drop(1)|Take(index), *this|Take(index));
			addToBeginField(1);
			return;
		}
		//Move right part to the left
		LifeCycle::MoveInitDestructForward(*this|Drop(index)|DropLast(1), *this|Drop(index + 1));
		addToLengthField(-1);
	}

	/// Remove one element at `ptr`.
	constexpr void Remove(TUnsafe, T* ptr) requires(D.IsGrowing())
	{
		INTRA_PRECONDITION(Span(*this).ContainsAddress(ptr));
		Remove(ptr - Data());
	}

	/// Remove the all elements in index mRange [`removeStart`; `removeEnd`).
	INTRA_NOINLINE constexpr void Remove(Index removeStart, Index removeEnd) requires(D.IsGrowing())
	{
		INTRA_PRECONDITION(removeEnd <= Length());
		const size_t elementsToRemove = size_t(removeEnd - removeStart);
		if(elementsToRemove == 0) return;
		LifeCycle::Destruct(*this|Drop(removeStart)|Take(elementsToRemove));

		//Fast particular cases
		if(removeEnd == Length())
		{
			addToLengthField(-index_t(size_t(elementsToRemove)));
			return;
		}
		if constexpr(D.IsGrowingQueue()) if(removeStart == 0)
		{
			addToBeginField(index_t(size_t(elementsToRemove)));
			return;
		}

		if constexpr(D.IsGrowingQueue())
		{
			bool canMoveBegin = true;
			if constexpr(D.InplaceCapacity) if(IsInplace()) canMoveBegin = false;
			if(canMoveBegin && size_t(removeStart) + size_t(elementsToRemove) / 2 < size_t(Length()) / 4)
			{
				LifeCycle::MoveInitDestructBackward(
					*this|Drop(elementsToRemove)|Take(removeEnd - elementsToRemove),
					*this|Take(removeStart));
				addToBeginField(index_t(elementsToRemove));
				return;
			}
		}
		LifeCycle::MoveInitDestructForward(
			*this|Drop(removeStart)|DropLast(elementsToRemove),
			*this|Drop(removeEnd));
		addToLengthField(-index_t(size_t(elementsToRemove)));
	}

	/// Find the first element equal to value and remove it.
	void FindAndRemove(const T& value)
	{
		const auto found = *this|TakeUntil(EqualsTo(value))|Count;
		if(found != Length()) Remove(found);
	}

	/// Remove first element. Complexity O(1)
	constexpr void RemoveFirst() requires(D.IsGrowingQueue())
	{
		INTRA_PRECONDITION(!Empty());
		if constexpr(D.InplaceCapacity) if(IsInplace())
		{
			LifeCycle::MoveInitDestructForward(Unsafe, m.InplaceBuf, m.InplaceBuf + 1, getShortLength() - 1);
			m.StorageInfo -= shortLenIncrement;
			return;
		}
		getCbBegin()->~T();
		setCbBegin(getCbBegin() + 1);
	}

	/// Remove last element. Complexity O(1)
	constexpr void RemoveLast() requires(D.IsGrowing())
	{
		INTRA_PRECONDITION(!Empty());
		const auto newLen = size_t(Length() - 1);
		setLengthField(newLen);
		Data()[newLen]->~T();
	}

	/// Fast O(1) remove by moving last element onto element being removed (no shift).
	constexpr void RemoveUnordered(Index index)
	{
		INTRA_PRECONDITION(index < Length());
		if(index + 1 < Length()) Data()[index] = INTRA_MOVE(Data()[Length() - 1]);
		RemoveLast();
	}

	/// Find the first element equal to `value` and remove it by replacing it with the last element.
	constexpr void FindAndRemoveUnordered(const T& value)
	{
		const auto index = *this|TakeUntil(EqualsTo(value))|Count;
		if(index != Length()) RemoveUnordered(index);
	}

	/// If the ratio of Capacity() / `Length()` > 125% do a reallocation to free all unused memory.
	constexpr void TrimExcessCapacity()
	{
		const auto len = size_t(Length());
		if(size_t(Capacity()) > len + len / 4) Resize(len);
	}


	[[nodiscard]] constexpr T& operator[](Index index)
	{
		INTRA_PRECONDITION(index < Length());
		return Data()[size_t(index)];
	}

	[[nodiscard]] constexpr const T& operator[](Index index) const
	{
		INTRA_PRECONDITION(index < Length());
		return Data()[size_t(index)];
	}

	[[nodiscard]] constexpr T& Last()
	{
		INTRA_PRECONDITION(!Empty());
		return Data()[Length() - 1];
	}

	[[nodiscard]] constexpr const T& Last() const
	{
		INTRA_PRECONDITION(!Empty());
		return Data()[Length() - 1];
	}

	[[nodiscard]] constexpr T& First()
	{
		INTRA_PRECONDITION(!Empty());
		return Data()[0];
	}

	[[nodiscard]] constexpr const T& First() const
	{
		INTRA_PRECONDITION(!Empty());
		return Data()[0];
	}

	[[nodiscard]] INTRA_FORCEINLINE constexpr T* Data() noexcept
	{
		if constexpr(D.InplaceCapacity) if(IsInplace()) return m.InplaceBuf;
		return getCbBegin();
	}
	[[nodiscard]] INTRA_FORCEINLINE constexpr const T* Data() const noexcept
	{
		if constexpr(D.InplaceCapacity) if(IsInplace()) return m.InplaceBuf;
		return getCbBegin();
	}

	[[nodiscard]] INTRA_FORCEINLINE constexpr index_t Length() const noexcept
	{
		if constexpr(D.InplaceCapacity) if(IsInplace()) return index_t(getShortLength());
		return getCbEnd() - getCbBegin();
	}

	[[nodiscard]] INTRA_FORCEINLINE constexpr bool IsInplace() const requires(D.InplaceCapacity != 0) {return (getStorageType() & 1) == 0;}
	[[nodiscard]] INTRA_FORCEINLINE constexpr bool IsAllocated() const requires CDefined<Allocator> {return (getStorageType() & 2) != 0;}

	/// @brief Set number of stored elements.
	/// If newLength > Length() removes newLength - Length() last elements.
	/// Otherwise construct Length() - newLength elements at the end passing args to its constructor.
	template<typename... Args> constexpr void SetLength(Size newLength, Args&... args)
	{
		const auto oldLength = setLengthNotConstruct(size_t(newLength));
		if constexpr(!sizeof...(args)) LifeCycle::ValueInitialize(*this|Drop(oldLength));
		else for(T& dst: *this|Drop(oldLength)) LifeCycle::ConstructOne(__builtin_addressof(dst), args...);
	}

	/// @brief Set number of stored elements without calling destructors or constructors.
	/// NOTE: Do not use it with non-POD types.
	///  Otherwise calling constructors and destructors is the caller's responsibility.
	constexpr void SetLengthRaw(TUnsafe, Size newLength) requires(D.IsGrowing())
	{
		Reserve(newLength);
		if constexpr(D.IsGrowingQueue())
		{
			bool resetLeftSpace = size_t(newLength) == 0;
			if constexpr(D.InplaceCapacity) resetLeftSpace = resetLeftSpace && IsInplace();
			if(resetLeftSpace) setCbBegin(m.Cb.BufferBegin);
		}
		if constexpr(D.IsGrowing()) setLengthField(size_t(newLength));
	}

	/// Add unitialized newElements to the beginning of the array without initialization.
	/// Calling constructors is on the caller's responsibility.
	constexpr void AddLeftUninitialized(TUnsafe, Size newElements) requires(D.IsGrowingQueue())
	{
		if(newElements == 0) return;
		Reserve(0, newElements); // guarantees that !IsInplace()
		setCbBegin(getCbBegin() - size_t(newElements));
	}

	/// Get current size of the buffer measured in elements it can store.
	[[nodiscard]] constexpr index_t Capacity() const
	{
		if constexpr(D.InplaceCapacity) if(IsInplace()) return index_t(D.InplaceCapacity);
		return index_t(getLongCapacity());
	}

	[[nodiscard]] constexpr bool IsFull() const {return Length() == Capacity();}

private:
	constexpr size_t setLengthNotConstruct(Index newLength) requires(D.IsGrowing())
	{
		const auto oldLength = size_t(Length());
		if(size_t(newLength) <= oldLength)
		{
			LifeCycle::Destruct(*this|Drop(newLength));
			setLengthField(size_t(newLength));
		}
		else SetLengthRaw(Unsafe, newLength);
		return oldLength;
	}

	constexpr void setLengthField(size_t newLength) requires(D.IsGrowing())
	{
		if constexpr(D.InplaceCapacity) if(IsInplace())
		{
			setShortLength(newLength);
			return;
		}
		setCbEnd(getCbBegin() + newLength);
	}

	template<CRange R> constexpr void addFirstRangeHelper(R&& values) requires(D.IsGrowingQueue())
	{
		GenericArray temp = RangeOf(INTRA_FWD(values));
		CheckSpace(0, temp.Length());
		mRange.Begin -= temp.Length();
		LifeCycle::MoveInit(Span<T>(Unsafe, Data(), temp.Length()), Span(temp));
	}

	template<CRange R> void addLastRangeHelper(R&& values) requires(D.IsGrowing())
	{
		GenericArray temp = RangeOf(INTRA_FWD(values));
		CheckSpace(temp.Length());
		LifeCycle::MoveInit(Span(Unsafe, Data() + Length(), temp.Length()), Span(temp));
		addToLengthField(temp.Length());
	}

	INTRA_NOINLINE void growRight()
	{
		if constexpr(!CDefined<Allocator>) INTRA_DEBUG_FATAL_ERROR("buffer overflow");
		else Reserve(size_t(Length()) + 1);
	}
	INTRA_NOINLINE void growLeft() requires(D.IsGrowingQueue())
	{
		if constexpr(!CDefined<Allocator>) INTRA_DEBUG_FATAL_ERROR("buffer overflow");
		else Reserve(Length(), 1);
	}

	using TLen = size_t;
	using TBegin = TConditionalField<TLen, D.IsGrowingQueue()>;
	using TEnd = TConditionalField<TLen, D.IsGrowing()>;
	static constexpr size_t CbSize = sizeof(T*) + sizeof(TBegin) + sizeof(TEnd) + sizeof(TLen);
	static constexpr bool PackedStorageType = D.InplaceCapacity * sizeof(T) < CbSize;

	using TInplaceControlField = TBasicUnsignedIntegerWithRange<D.InplaceCapacity * 4>;
	INTRA_IGNORE_WARN("pedantic")
	struct alignas(Max(sizeof(T*), sizeof(T)))
	{
#pragma pack(push, 1)
		union
		{
			struct
			{
				T* BufferBegin;

				// The following fields store either pointer values or offsets in elements from BufferBegin.
				// We prefer pointer values for growing arrays because they make push_back/push_front faster.
				// But in compile-time, we have to fallback to offsets anyway.
				INTRA_NO_UNIQUE_ADDRESS TBegin Begin;
				INTRA_NO_UNIQUE_ADDRESS TEnd End;
				TSelect<char[sizeof(TLen) - 1], TLen, PackedStorageType> BufferEnd;
			} Cb;
			INTRA_NO_UNIQUE_ADDRESS TConditionalField<T[Max(1u, D.InplaceCapacity)], D.InplaceCapacity != 0> InplaceBuf;
		};
		TInplaceControlField StorageInfo;
#pragma pack(pop)
	} m;
	INTRA_NO_UNIQUE_ADDRESS Allocator mAllocator;
	static_assert(!PackedStorageType || sizeof(TInplaceControlField) == 1);

	INTRA_FORCEINLINE constexpr int getStorageType() const
	{
		if constexpr(Config::TargetIsBigEndian) return int(m.StorageInfo & 3); // in 2 lower bits
		else return int(m.StorageInfo >> (SizeofInBits<TInplaceControlField> - 2)); // in 2 upper bits
	}

	INTRA_FORCEINLINE constexpr void setStorageType(int newType)
	{
		if constexpr(Config::TargetIsBigEndian) m.StorageInfo = (m.StorageInfo & ~TInplaceControlField(3)) | newType; // in 2 lower bits
		else m.StorageInfo = (m.StorageInfo << 2 >> 2) | (newType << (SizeofInBits<TInplaceControlField> - 2)); // in 2 upper bits
	}



	INTRA_FORCEINLINE constexpr T* getCbBegin() const
	{
		if constexpr(D.IsGrowingQueue()) return m.Cb.BufferBegin;
		else if constexpr(CSameSize<TLen, T*>)
		{
			if(IsConstantEvaluated()) // can't constant evaluate reinterpret cast, so need another implementation
				return m.Cb.BufferBegin + m.Cb.Begin;
			return reinterpret_cast<T*>(m.Cb.Begin);
		}
		else return m.Cb.BufferBegin + m.Cb.Begin;
	}

	INTRA_FORCEINLINE constexpr void setCbBegin(T* newBegin) requires (D.IsGrowingQueue())
	{
		if constexpr(CSameSize<TLen, T*>)
		{
			if(IsConstantEvaluated()) m.Cb.Begin = size_t(newBegin - m.Cb.BufferBegin);
			m.Cb.Begin = reinterpret_cast<TLen>(newBegin);
		}
		else m.Cb.Begin = size_t(newBegin - m.Cb.BufferBegin);
	}

	INTRA_FORCEINLINE constexpr T* getCbEnd() const
	{
		if constexpr(!D.IsGrowing()) return getCbBufferEnd(); // store size (= capacity) instead of a pointer because Static arrays have no AddLast
		else if constexpr(CSameSize<TLen, T*>)
		{
			if(IsConstantEvaluated()) return m.Cb.BufferBegin + m.Cb.End;
			return reinterpret_cast<T*>(m.Cb.End);
		}
		else return m.Cb.BufferBegin + m.Cb.End;
	}

	INTRA_FORCEINLINE constexpr void setCbEnd(T* newEnd) requires (D.IsGrowing())
	{
		if constexpr(CSameSize<TLen, T*>)
		{
			if(IsConstantEvaluated()) m.Cb.End = size_t(newEnd - m.Cb.BufferBegin);
			m.Cb.End = reinterpret_cast<TLen>(newEnd);
		}
		else m.Cb.End = size_t(newEnd - m.Cb.BufferBegin);
	}

	INTRA_FORCEINLINE constexpr T* getCbBufferEnd() const
	{
		if constexpr(PackedStorageType)
		{ // 2-bit storage type flags are encoded in last byte of val
			TLen val = 0;
			if(!IsConstantEvaluated()) val = *reinterpret_cast<const TLen*>(&m.Cb.BufferEnd);
			else
			{
				char data[sizeof(TLen)];
				MemoryCopy(Unsafe, data, m.Cb.BufferEnd, sizeof(m.Cb.BufferEnd));
				data[sizeof(m.Cb.BufferEnd)] = char(m.StorageInfo);
				val = BinaryDeserialize<TLen>(Unsafe, data);
			}

			if constexpr(Config::TargetIsBigEndian) // in 2 lower bits
			{
				if constexpr(D.IsGrowing() && CSameSize<TLen, T*>)
				{
					if(!IsConstantEvaluated()) // reinterpret_cast doesn't work at compile-time, so use pointers only at runtime
						return reinterpret_cast<T*>(val & ~size_t(3)); // allocations are expected to be multiple of 4
				}
				return m.Cb.BufferBegin + (val >> 2);
			}
			else // in 2 upper bits
			{
				if constexpr(D.IsGrowing() && CSameSize<TLen, T*>)
				{
					if(!IsConstantEvaluated()) // reinterpret_cast doesn't work at compile-time, so use pointers only at runtime
						return reinterpret_cast<T*>(val << 2); // allocations are expected to be multiple of 4, storage bits overflow
				}
				return m.Cb.BufferBegin + (val & (MaxValueOf<TLen> >> 2));
			}
		}
		else
		{
			if constexpr(D.IsGrowing() && CSameSize<TLen, T*>)
			{
				if(!IsConstantEvaluated()) // reinterpret_cast doesn't work at compile-time, so use pointers only at runtime
					return reinterpret_cast<T*>(m.Cb.BufferEnd);
			}
			return m.Cb.BufferBegin + m.Cb.BufferEnd;
		}
	}

	INTRA_FORCEINLINE constexpr void setCbBufferEnd(T* newEnd)
	{
		if constexpr(PackedStorageType)
		{
			TLen val = 0;
			if constexpr(Config::TargetIsBigEndian)
			{
				if constexpr(D.IsGrowing() && CSameSize<TLen, T*>)
					if(!IsConstantEvaluated())
					{
						INTRA_DEBUG_ASSERT(IsAligned(newEnd, 4));
						val = reinterpret_cast<TLen>(newEnd) | getStorageType();
						return;
					}
				val = (TLen(newEnd - m.Cb.BufferBegin) << 2) | getStorageType();
			}
			else
			{
				if constexpr(D.IsGrowing() && CSameSize<TLen, T*>)
					if(!IsConstantEvaluated())
					{
						INTRA_DEBUG_ASSERT(IsAligned(val, 4));
						val = (reinterpret_cast<TLen>(newEnd) >> 2) | (getStorageType() << (SizeofInBits<TLen> - 2));
						return;
					}
				val = TLen(newEnd - m.Cb.BufferBegin) | TLen(getStorageType() << (SizeofInBits<TLen> - 2));
			}
			if(!IsConstantEvaluated()) *reinterpret_cast<TLen*>(&m.Cb.BufferEnd) = val;
			else
			{
				char data[sizeof(TLen)];
				BinarySerialize<TLen>(Unsafe, data, val);
				MemoryCopy(Unsafe, m.Cb.BufferEnd, data, sizeof(m.Cb.BufferEnd));
				m.StorageInfo = TInplaceControlField(data[sizeof(m.Cb.BufferEnd)]);
			}
		}
		else
		{
			if constexpr(D.IsGrowing() && CSameSize<TLen, T*>)
				if(!IsConstantEvaluated())
				{
					m.Cb.BufferEnd = reinterpret_cast<TLen>(newEnd);
					return;
				}
			m.Cb.BufferEnd = TLen(newEnd - m.Cb.BufferBegin);
		}
	}

	INTRA_FORCEINLINE constexpr void setCb(T* newBufferBegin, T* newBegin, T* newEnd, T* newBufferEnd)
	{
		m.Cb.BufferBegin = newBufferBegin;
		if constexpr(D.IsGrowingQueue()) setCbBegin(newBegin);
		if constexpr(D.IsGrowing()) setCbEnd(newEnd);
		setCbBufferEnd(newBufferEnd);
	}

	INTRA_FORCEINLINE constexpr void setCb(T* newBufferBegin, T* newEnd, T* newBufferEnd)
	{setCb(newBufferBegin, newBufferBegin, newEnd, newBufferEnd);}

	INTRA_FORCEINLINE constexpr void setCb(T* newBegin, T* newEnd)
	{setCb(newBegin, newBegin, newEnd, newEnd);}

	static constexpr bool InverseInplaceLength = CChar<T> && D.InplaceCapacity * sizeof(T) == sizeof(m.Cb);

	INTRA_FORCEINLINE constexpr void setShortLength(size_t len) noexcept
	{
		if constexpr(InverseInplaceLength) // turn mInplaceControlField into '\0' when len == InplaceCapacity
			m.StorageInfo = TInplaceControlField(D.InplaceCapacity - len); // to allow implement c_str() for strings
		else m.StorageInfo = TInplaceControlField(len);
		if constexpr(Config::TargetIsBigEndian) m.StorageInfo <<= 2; // lower bits are flags (== 0 for string)
	}

	INTRA_FORCEINLINE constexpr size_t getShortLength() const noexcept
	{
		size_t res = 0;
		if constexpr(InverseInplaceLength) res = D.InplaceCapacity - m.StorageInfo;
		else res = m.StorageInfo;
		if constexpr(Config::TargetIsBigEndian) res >>= 2;
		return res;
	}

	INTRA_FORCEINLINE constexpr void setLongCapacity(size_t newCapacity) noexcept {setCbBufferEnd(m.Cb.BufferBegin + newCapacity);}
	INTRA_FORCEINLINE constexpr size_t getLongCapacity() const noexcept {return size_t(getCbBufferEnd() - m.Cb.BufferBegin);}

	INTRA_FORCEINLINE constexpr void setFieldsToDefaultConstructedState()
	{
		if constexpr(!D.InplaceCapacity) m.Cb = {};
		m.StorageInfo = emptyInplaceFieldValue;
	}

	INTRA_FORCEINLINE constexpr void addToLengthField(index_t increment) requires(D.IsGrowing()) {setLengthField(Length() + increment);}
	INTRA_FORCEINLINE constexpr void addToBeginField(index_t increment) requires(D.IsGrowingQueue()) {setCbBegin(getCbBegin() + increment);}
	INTRA_FORCEINLINE constexpr T* getEnd() {return Data() + Length();}
	
	INTRA_FORCEINLINE constexpr Span<T> getBuffer()
	{
		if constexpr(D.InplaceCapacity) if(IsInplace()) return Span(Unsafe, m.InplaceBuf, D.InplaceCapacity);
		return Span(Unsafe, m.Cb.BufferBegin, getCbBufferEnd());
	}

	static constexpr TInplaceControlField shortLenIncrement = (InverseInplaceLength? -1: 1) << (Config::TargetIsBigEndian? 2: 0);
	static constexpr auto emptyInplaceFieldValue = TInplaceControlField(InverseInplaceLength? D.InplaceCapacity: 0);
};

template<typename T, ArrayDesc D, COptAllocator Allocator>
constexpr bool IsTriviallyRelocatable<GenericArray<T, D, Allocator>> = CTriviallyRelocatable<Allocator>;

template<typename T> using ResizableArray = GenericArray<T, INTRA_NTTA(ArrayDesc{
	.InplaceCapacity = Max(1u, sizeof(void*) * 2 / sizeof(T)) - 1,
	.Type = ArrayDesc::DynType::Static
})>;

template<typename T> using DynArray = GenericArray<T, INTRA_NTTA(ArrayDesc{
	.InplaceCapacity = Max(1u, sizeof(void*) * 3 / sizeof(T)) - 1,
	.Type = ArrayDesc::DynType::Growing
})>;

template<typename T, int Capacity = 3072 / sizeof(T)> using LocalArray = GenericArray<T, INTRA_NTTA(ArrayDesc{
	.InplaceCapacity = Capacity,
	.Type = ArrayDesc::DynType::Static
})>;

#if INTRA_CONSTEXPR_TEST
static_assert(CHasData<DynArray<int>>);
static_assert(CHasLength<DynArray<int>>);
static_assert(CHasData<DynArray<StringView>>);
static_assert(CHasData<const DynArray<StringView>&>);
static_assert(CSpanConvertible<const DynArray<StringView>&>);
static_assert(CTriviallyRelocatable<DynArray<int>>);
#endif

} INTRA_END
