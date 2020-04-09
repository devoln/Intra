#pragma once

#include "Intra/Type.h"
#include "Intra/Concepts.h"
#include "Intra/Range/Concepts.h"
#include "Intra/Range/Operations.h"

#include "Intra/CContainer.h"
#include "Intra/Range/Span.h"

#include "Intra/Range/Comparison.h"
#include "Intra/Range/Search/Single.h"
#include "Intra/Range/Mutation/Copy.h"
#include "Extra/Memory/Memory.h"
#include "Extra/Memory/Allocator/Global.h"
#include "Extra/Container/ForwardDecls.h"
#include "Extra/Container/Operations.hh"


INTRA_BEGIN
template<typename T> class Array
{
public:
	constexpr Array() = default;
	constexpr Array(decltype(null)) {}

	explicit INTRA_FORCEINLINE Array(index_t initialCount) {SetCount(initialCount);}

	INTRA_FORCEINLINE Array(InitializerList<T> values):
		Array(CSpan<T>(values)) {}
	
	Array(CSpan<T> values)
	{
		SetCountUninitialized(values.Length());
		CopyInit(range, values);
	}

	template<index_t N> INTRA_FORCEINLINE Array(const T(&values)[N]): Array(SpanOf(values)) {}

	template<typename R,
		typename AsR=TRangeOf<R>,
	typename = Requires<
		!CSameIgnoreCVRef<R, Array> &&
		CInputRange<AsR> &&
		(CCopyConstructible<AsR> ||
			CHasLength<AsR>)
	>> INTRA_FORCEINLINE Array(R&& values)
	{AddLastRange(ForwardAsRange<R>(values));}

	constexpr Array(Array&& rhs) noexcept: buffer(rhs.buffer), range(rhs.range)
	{rhs.buffer = null; rhs.range = null;}

	INTRA_FORCEINLINE Array(const Array& rhs): Array(rhs.AsConstRange()) {}
	
	INTRA_FORCEINLINE ~Array() {operator=(null);}


	/** Take ownership of rangeToOwn elements and memory.
	  Warning: ``rangeToOwn`` must be allocated with the same allocator as the template argument Allocator!
	*/
	[[nodiscard]] static Array CreateAsOwnerOfUnsafe(Span<T> rangeToOwn)
	{
		Array result;
		result.range = result.buffer = rangeToOwn;
		return result;
	}

	//! Create an Array with count default initialized elements.
	[[nodiscard]] static Array CreateWithCount(size_t count)
	{
		Array result;
		result.SetCount(count);
		return result;
	}

	//! Create an Array with count copies of initValue.
	[[nodiscard]] static Array CreateWithCount(size_t count, const T& initValue)
	{
		Array result;
		result.SetCount(count, initValue);
		return result;
	}

	/** Copy arrays.
	  Destruct all elements of this Array then copy construct all elements from rhs.
	*/
	Array& operator=(const Array& rhs)
	{
		if(this == &rhs) return *this;
		Assign(rhs.AsConstRange());
		return *this;
	}

	/** Move array.
	  Destruct all this Array's elements.
	  Takes ownerwhip of all rhs elements.
	  rhs becomes empty but takes ownership of this Array memory allocation.
	*/
	Array& operator=(Array&& rhs)
	{
		if(this == &rhs) return *this;
		Clear();
		range = rhs.range;
		rhs.range.End = rhs.range.Begin;
		Swap(buffer, rhs.buffer);
		return *this;
	}

	Array& operator=(CSpan<T> values)
	{
		INTRA_PRECONDITION(!buffer.Overlaps(values));
		Assign(values);
		return *this;
	}

	template<typename R> Requires<
		CAsAccessibleRange<R>,
	Array&> operator=(R&& values) {return operator=(Array(Forward<R>(values)));}

	//! Delete all elements and free memory.
	Array& operator=(decltype(null))
	{
		Clear();
		FreeRangeUninitialized(GlobalHeap, buffer);
		buffer = null;
		range = null;
		return *this;
	}

	template<typename U> void Assign(CSpan<U> rhs)
	{
		Clear();
		SetCountUninitialized(rhs.Length());
		CopyInit(range, rhs);
	}

	template<typename U> void Assign(Span<U> rhs) {Assign(rhs.AsConstRange());}

	//! Add a new element to the begining of the array by moving value.
	/*!
	  Unlike most other array implementations this operation has O(1) complexity.
	*/
	T& AddFirst(T&& value)
	{
		if(LeftSpace()) return *new(--range.Begin) T(Move(value));
		T temp = Move(value);
		CheckSpace(0, 1);
		return *new(Construct, --range.Begin) T(Move(temp));
	}

	/** Add \p value to the beginning of the Array.
	  Unlike most other array implementations this operation has amortized constant O(1) complexity.
	*/
	T& AddFirst(const T& value)
	{
		if(LeftSpace()) return *new(--range.Begin) T(value);
		T temp = value;
		CheckSpace(0, 1);
		return *new(Construct, --range.Begin) T(Move(temp));
	}

	/** Construct a new element at the beginning of the array with constructor parameters args.
	  Unlike most other array implementations this operation has amortized constant O(1) complexity.
	*/
	template<typename... Args> inline Requires<
		CConstructible<T, Args...>,
	T&> EmplaceFirst(Args&&... args)
	{
		if(LeftSpace()) return *new(--range.Begin) T(Forward<Args>(args)...);
		T temp(Forward<Args>(args)...); //if this array contains values referenced by args they will become invalid after reallocation
		CheckSpace(0, 1);
		return *new(Construct, --range.Begin) T(Move(T));
	}

	/** Add all range values to the beginning of the array.
	  This operation has linear O(values.Length) complexity and unlike most other array imlementations it doesn't depend on this array length.
	*/
	template<typename R> Requires<
		CAsConsumableRangeOf<R, T> &&
		!(CForwardRange<R> || CHasLength<R>)
	> AddFirstRange(R&& values)
	{
		addFirstRangeHelper(Forward<R>(values));
	}

	/** Add all range values to the beginning of the array.
	  This operation has linear O(values.Length) complexity and unlike most other array imlementations it doesn't depend on this array length.
	*/
	template<typename R,
		typename AsR = TRangeOf<R>> Requires<
		CConsumableRangeOf<AsR, T> &&
		(CForwardRange<AsR> || CHasLength<AsR>)
	> AddFirstRange(R&& values)
	{
		auto valueRange = ForwardAsRange<R>(values);
		const auto valuesCount = Intra::Count(values);
		if(LeftSpace() < valuesCount)
		{
			if(!Empty())
			{
				//use slower implementaation with a temporary copy
				//otherwise reallocation may affect valueRange
				addFirstRangeHelper(Move(valueRange));
				return;
			}
			Reserve(0, valuesCount);
		}
		range.Begin -= valuesCount;
		for(T* dst = range.Begin; !valueRange.Empty(); valueRange.PopFirst())
			new(Construct, dst++) T(valueRange.First());
	}

	/** Add a new element to the end of the array by moving \p value.
	  This operation has amortized constant O(1) complexity.
	*/
	inline T& AddLast(T&& value)
	{
		if(range.End != buffer.End) return *new(Construct, range.End++) T(Move(value));
		T temp = Move(value); //if this array contains value it will become invalid after reallocation, so store it here
		CheckSpace(1, 0);
		return *new(Construct, range.End++) T(Move(temp));
	}

	/** Add \p value to the end of the Array.
	  This operation has amortized constant O(1) complexity.
	*/
	inline T& AddLast(const T& value)
	{
		if(range.End != buffer.End) return *new(Construct, range.End++) T(value);
		T temp = value; //if this array contains value it will become invalid after reallocation, so store it here
		CheckSpace(1, 0);
		return *new(Construct, range.End++) T(Move(temp));
	}

	/** Construct a new element at the end of the array passing args to constructor.
	  This operation has amortized constant O(1) complexity.
	*/
	template<typename... Args> inline Requires<
		CConstructible<T, Args...>,
	T&> EmplaceLast(Args&&... args)
	{
		if(RightSpace()) return *new(Construct, range.End++) T(Forward<Args>(args)...);
		T temp(Forward<Args>(args)...); //if this array contains values referenced by args they will become invalid after reallocation, so construct it before it
		CheckSpace(1, 0);
		return *new(Construct, range.End++) T(Move(temp));
	}

	//! Add all range values to the end of the array.
	/*!
	  This operation has linear O(values.Length) complexity.
	*/
	template<typename R,
		typename AsR = TRangeOf<R>
	> Requires<
		CConsumableRangeOf<AsR, T> &&
		(CForwardRange<AsR> || CHasLength<AsR>)
	> AddLastRange(R&& values)
	{
		auto valueRange = ForwardAsRange<R>(values);
		const auto valuesCount = Intra::Count(valueRange);
		if(RightSpace() < valuesCount)
		{
			if(!Empty())
			{
				//use slower implementaation with a temporary copy
				//otherwise reallocation may affect valueRange
				addLastRangeHelper(Move(valueRange));
				return;
			}
			Reserve(valuesCount, 0);
		}
		for(; !valueRange.Empty(); valueRange.PopFirst())
			new(Construct, range.End++) T(valueRange.First());
	}

	/** Add all range values to the end of the array.
	  This operation has linear O(values.Length) complexity.
	*/
	template<typename R,
		typename AsR = TRangeOf<R>
	> Requires<
		CConsumableRangeOf<AsR, T> &&
		!(CForwardRange<AsR> || CHasLength<AsR>)
	> AddLastRange(R&& values)
	{
		addLastRangeHelper(ForwardAsRange<R>(values));
	}

	//! Set element at index ``pos`` to ``value``.
	//! If ``pos`` >= Count(), it adds ``pos`` - Count() default initialized elements and adds ``value``.
	template<typename U> T& Set(Index pos, U&& value)
	{
		if(pos >= Count())
		{
			Reserve(pos + 1);
			SetCount(pos);
			return AddLast(Forward<U>(value));
		}
		operator[](pos) = Forward<U>(value);
	}
	

	/** Insert ``values`` into ``pos``.
	  As a result for each i-th element its index will stay unchanged if i < ``pos`` and will become i + ``values.Length()`` otherwise.
	  The first inserted element will have index ``pos``.
	*/
	template<typename U> void Insert(Index pos, CSpan<U> values)
	{
		if(values.Empty()) return;
		INTRA_PRECONDITION(!range.Overlaps(values));
		const auto valuesCount = values.Length();

		//If there is not enough space available, reallocate and move
		if(Count() + valuesCount > Capacity())
		{
			size_t newCapacity = Count() + valuesCount + Capacity()/2;
			Span<T> newBuffer = AllocateRangeUninitialized<T>(
				GlobalHeap, newCapacity, INTRA_SOURCE_INFO);
			Span<T> newRange = newBuffer.Drop(LeftSpace()).Take(Count()+valuesCount);
			MoveInitDelete(newRange.Take(pos), range.Take(pos));
			MoveInitDelete(newRange.Drop(pos+valuesCount), range.Drop(pos));
			CopyInit(newRange.Drop(pos).Take(valuesCount), values);
			FreeRangeUninitialized(GlobalHeap, buffer);
			buffer = newBuffer;
			range = newRange;
			return;
		}

		if(pos >= (Count() + valuesCount)/2 || LeftSpace() < valuesCount) //move valuesCount positions forward
		{
			range.End += valuesCount;
			MoveInitDeleteBackwards<T>(range.Drop(pos+valuesCount), range.Drop(pos).DropLast(valuesCount));
		}
		else //move valuesCount positions backwards
		{
			range.Begin -= valuesCount;
			MoveInitDelete<T>(range.Take(pos), range.Drop(valuesCount).Take(pos));
		}
		CopyInit<T>(range.Drop(pos).Take(valuesCount), values);
	}

	template<typename U> void Insert(const T* it, CSpan<U> values)
	{
		INTRA_PRECONDITION(range.ContainsAddress(it));
		Insert(it - range.Begin, values);
	}

	INTRA_FORCEINLINE void Insert(Index pos, const T& value)
	{
		if(range.ContainsAddress(&value))
		{
			T temp = value;
			Insert(pos, {&temp, 1});
			return;
		}
		Insert(pos, {&value, 1});
	}

	void Insert(const T* it, const T& value)
	{
		INTRA_PRECONDITION(range.ContainsAddress(it));
		Insert(it - range.Begin, value);
	}


	//! Add new element to the end using stream syntax.
	Array& operator<<(const T& value) {AddLast(value); return *this;}
	Array& operator<<(T&& value) {AddLast(Move(value)); return *this;}

	//! Get and remove the last array element moving it to the right operand.
	Array& operator>>(T& value)
	{
		value = Move(Last());
		RemoveLast();
		return *this;
	}

	//! Get and remove the last array element.
	T PopLastElement()
	{
		T result = Move(Last());
		RemoveLast();
		return result;
	}


	//! Pop the first element from the end.
	T PopFirstElement()
	{
		T result = Move(First());
		RemoveFirst();
		return result;
	}

	/** Set new capacity of the array.
	  If rightPartSize < Count() Calls destructor for all elements with index >= rightPartSize.
	*/
	void Resize(Size rightPartSize, Size leftPartSize = 0)
	{
		if((size_t(rightPartSize)|size_t(leftPartSize)) == 0) {*this = null; return;}

		// Delete all elements out of the new bounds
		if(size_t(rightPartSize) <= size_t(Count())) Destruct(range.Drop(rightPartSize));

		size_t newCapacity = size_t(rightPartSize) + size_t(leftPartSize);
		Span<T> newBuffer = AllocateRangeUninitialized<T>(
			GlobalHeap, newCapacity, INTRA_SOURCE_INFO);
		Span<T> newRange = newBuffer.Drop(leftPartSize).Take(Count());

		if(!buffer.Empty())
		{
			//Move elements to the new buffer
			MoveInitDelete<T>(newRange, range);
			FreeRangeUninitialized(GlobalHeap, buffer);
		}
		buffer = newBuffer;
		range = newRange;
	}

	//! Makes sure that the array has enough capacity.
	/*/
	  If it already has enough space to add at least \p rightPart - Count() new elements to the end
	  to add at least \p leftSpace elements to the beginning without reallocation then it does nothing.
	  Otherwise reallocates space and moves elements to a new memory allocation using move costructor and destructor.
	*/
	void Reserve(Index rightPart, Index leftSpace = 0)
	{
		const auto currentRightPartSize = size_t(buffer.End - range.Begin);
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

	//! May be more comfortable alternative to Reserve.
	//! @see Reserve
	void CheckSpace(Index rightSpace, Index leftSpace = 0) {Reserve(size_t(Count()) + size_t(rightSpace), leftSpace);}

	//! Remove all array elements without freeing allocated memory.
	void Clear()
	{
		if(Empty()) return;
		Destruct<T>(range);
		range.End = range.Begin;
	}

	//! @returns true if Array is empty.
	constexpr bool Empty() const noexcept {return range.Empty();}

	//! @returns number of elements that can be inserted into the beginning of the array before reallocation is necessary.
	constexpr index_t LeftSpace() const noexcept {return range.Begin - buffer.Begin;}

	//! @returns number of elements that can be inserted into the end of the array before reallocation is necessary.
	constexpr index_t RightSpace() const noexcept {return buffer.End - range.End;}

	/*! @name Element order preserving remove operations
	  @warning: These operations invalidate all ranges, iterators and pointers referring to the elements of this Array.
	*/
	///@{
	//! Remove one element at \p index.
	void Remove(Index index)
	{
		INTRA_PRECONDITION(index < Count());
		range[index].~T();

		// The ratio 1/4 instead of 1/2 was selected, because moving of overlapping memory
		// forward is ~2 times slower then backwards
		if(index >= Count() / 4) //Move right part to the left
		{
			MoveInitDelete<T>({range.Begin+index, range.End-1}, {range.Begin+index+1, range.End});
			--range.End;
		}
		else //Moving the left part forward
		{
			MoveInitDeleteBackwards<T>({range.Begin+1, range.Begin+index+1}, {range.Begin, range.Begin+index});
			++range.Begin;
		}
	}

	//! Remove one element at \p ptr.
	void Remove(T* ptr)
	{
		INTRA_PRECONDITION(range.ContainsAddress(ptr));
		Remove(ptr - range.Begin);
	}

	//! Remove the all elements in index range ``[removeStart; removeEnd)``.
	void Remove(Index removeStart, Index removeEnd)
	{
		INTRA_PRECONDITION(size_t(removeStart) <= size_t(removeEnd));
		INTRA_PRECONDITION(size_t(removeEnd) <= size_t(Count()));
		const size_t elementsToRemove = size_t(removeEnd) - size_t(removeStart);
		if(elementsToRemove == 0) return;
		Destruct<T>(range.Drop(removeStart).Take(elementsToRemove));

		//Fast particular cases
		if(removeEnd == Count())
		{
			range.End -= elementsToRemove;
			return;
		}

		if(removeStart == 0)
		{
			range.Begin += elementsToRemove;
			return;
		}

		if(size_t(removeStart) + elementsToRemove / 2 >= size_t(Count()) / 4)
		{
			MoveInitDelete<T>(
				range.Drop(removeStart).DropLast(elementsToRemove),
				range.Drop(removeEnd));
			range.End -= elementsToRemove;
		}
		else
		{
			MoveInitDeleteBackwards<T>(
				range(elementsToRemove, removeEnd),
				range.Take(removeStart));
			range.Begin += elementsToRemove;
		}
	}

	//! Find the first element equal to ``value`` and remove it.
	void FindAndRemove(const T& value)
	{
		const auto found = range.CountUntil(value);
		if(found != Count()) Remove(found);
	}
	///@}

	//! Remove duplicated elements from the Array without keeping order of its elements.
	void RemoveDuplicatesUnordered()
	{
		for(size_t i = 0; i < size_t(Count()); i++)
			for(size_t j = i + 1; j < size_t(Count()); j++)
				if(operator[](i) == operator[](j))
					RemoveUnordered(j--);
	}

	//! Remove first Array element. Complexity ``O(1)``
	void RemoveFirst() {INTRA_PRECONDITION(!Empty()); (range.Begin++)->~T();}

	//! Remove last Array element. Complexity ``O(1)``
	void RemoveLast() {INTRA_PRECONDITION(!Empty()); (--range.End)->~T();}

	//! Fast ``O(1)`` remove by moving last element onto element being removed (no shift).
	void RemoveUnordered(Index index)
	{
		INTRA_PRECONDITION(size_t(index) < size_t(Count()));
		if(size_t(index) + 1 < size_t(Count())) range[index] = Move(*--range.End);
		else RemoveLast();
	}

	//! Find the first element equal to ``value`` and remove it by replacing it with the last element.
	void FindAndRemoveUnordered(const T& value)
	{
		const auto index = CountUntil(range, value);
		if(index != Count()) RemoveUnordered(index);
	}

	//! If the ratio of Capacity() / Count() > 125% do a reallocation to free all unused memory.
	void TrimExcessCapacity()
	{
		const auto threshold = size_t(Count()) + size_t(Count()) / 4;
		if(size_t(Capacity()) > threshold) Resize(size_t(Count()));
	}


	[[nodiscard]] T& operator[](Index index)
	{
		INTRA_PRECONDITION(size_t(index) < size_t(Count()));
		return range.Begin[size_t(index)];
	}

	[[nodiscard]] const T& operator[](Index index) const
	{
		INTRA_PRECONDITION(size_t(index) < size_t(Count()));
		return range.Begin[size_t(index)];
	}

	[[nodiscard]] T& Last()
	{
		INTRA_PRECONDITION(!Empty());
		return range.Last();
	}

	[[nodiscard]] const T& Last() const
	{
		INTRA_PRECONDITION(!Empty());
		return range.Last();
	}

	[[nodiscard]] T& First()
	{
		INTRA_PRECONDITION(!Empty());
		return range.First();
	}

	[[nodiscard]] const T& First() const
	{
		INTRA_PRECONDITION(!Empty());
		return range.First();
	}

	[[nodiscard]] T* Data() {return begin();}
	[[nodiscard]] const T* Data() const {return begin();}

	[[nodiscard]] T* End() {return end();}
	[[nodiscard]] const T* End() const {return end();}


	//! @return total size of Array contents in bytes.
	[[nodiscard]] size_t SizeInBytes() const noexcept {return size_t(Count())*sizeof(T);}

	///@{
	//! @return Number of stored elements.
	[[nodiscard]] index_t Count() const noexcept {return range.Length();}
	[[nodiscard]] index_t Length() const noexcept {return Count();}
	///@}

	/** Set number of stored elements.
	  If ``newCount`` > Count() removes ``newCount`` - Count() last elements.
	  Otherwise construct Count() - ``newCount`` elements at the end using default constructor.
	*/
	void SetCount(Size newCount)
	{
		const auto oldCount = setCountNotConstruct(newCount);
		Initialize<T>(range.Drop(oldCount));
	}

	/** Set number of stored elements.
	  If newCount > Count removes newCount - Count last elements.
	  Otherwise construct Count - newCount elements at the end passing \p args to its constructor.
	*/
	template<typename... Args> void SetCountEmplace(index_t newCount, Args&... args)
	{
		const index_t oldCount = setCountNotConstruct(newCount);
		for(T& obj: range.Drop(oldCount)) new(Construct, &obj) T(args...);
	}

	/** Set number of stored elements.
	  If newCount > Count removes newCount - Count last elements.
	  Otherwise adds Count - newCount copies of \p initValue at the end.
	*/
	void SetCount(index_t newCount, const T& initValue)
	{
		const index_t oldCount = setCountNotConstruct(newCount);
		for(T& dst: Drop(oldCount)) new(Construct, dst) T(initValue);
	}

	/** Set number of stored elements without calling destructors and constructors.

	  Warning: Do not use it with non-POD types.
	  Otherwise calling constructors and destructors is on the caller's responsibility.
	*/
	void SetCountUninitialized(Size newCount)
	{
		Reserve(newCount, 0);
		if(size_t(newCount) == 0) range.Begin = buffer.Begin;
		range.End = range.Begin + size_t(newCount);
	}

	/** Add \p newElements elements to the beginning of the Array without initialization.
	
	  Warning: Do not use it with non-POD types.
	  Otherwise calling constructors and destructors is on the caller's responsibility.
	*/
	void AddLeftUninitialized(Size newElements)
	{
		Reserve(0, newElements);
		range.Begin -= size_t(newElements);
	}

	//! Get current size of the buffer measured in elements it can store.
	[[nodiscard]] index_t Capacity() const { return buffer.Length(); }

	[[nodiscard]] bool IsFull() const {return Count() == Capacity();}

	/** @name View operations
	  @warning
	  Ranges returned by the following functions and all derivatives of these ranges are only valid until:
	  1) owner's Length() becomes less than endIndex
	  2) calling Insert method with 0 < position < Count()
	  3) calling Remove method with 0 < position < Count()
	  4) deallocation, for example:
		a) its owner grows its capacity explicitly or because of insertion
		b) its owner goes out of its scope
	  Moving this array to another array transfers the ownership to the latter.
	*/
	///@{
	[[nodiscard]] operator Span<T>() {return AsRange();}
	[[nodiscard]] operator CSpan<T>() const {return AsRange();}
	[[nodiscard]] Span<T> AsRange() {return range;}
	[[nodiscard]] CSpan<T> AsConstRange() const {return range;}
	[[nodiscard]] CSpan<T> AsRange() const {return AsConstRange();}

	/** Create a slice of this array's elements.
	  @returns Span containing elements with indices [\p firstIndex; \p endIndex)
	*/
	[[nodiscard]] Span<T> Slice(Index firstIndex, Index endIndex)
	{
		INTRA_PRECONDITION(firstIndex <= endIndex);
		INTRA_PRECONDITION(endIndex <= Count());
		return range(firstIndex, endIndex);
	}

	/** Create a slice of this array's elements.
	  @returns Span containing elements with indices [\p firstIndex; \p endIndex)
	*/
	[[nodiscard]] CSpan<T> Slice(Index firstIndex, Index endIndex) const
	{
		INTRA_PRECONDITION(firstIndex <= endIndex);
		INTRA_PRECONDITION(endIndex <= Count());
		return AsConstRange()(firstIndex, endIndex);
	}

	//! @returns at most \p count elements from the beginning of the array.
	[[nodiscard]] Span<T> Take(index_t count) { return range.Take(count); }

	//! @returns at most \p count elements from the beginning of the array.
	[[nodiscard]] CSpan<T> Take(index_t count) const { return AsConstRange().Take(count); }
	
	//! @returns Span containing all elements of Array after \p count first elements.
	[[nodiscard]] Span<T> Drop(index_t count) { return range.Drop(count); }

	//! @returns Span containing all elements of Array after \p count first elements.
	[[nodiscard]] CSpan<T> Drop(index_t count) const { return AsConstRange().Drop(count); }

	//! @returns at most \p count elements from the end of the array.
	[[nodiscard]] Span<T> Tail(index_t count) { return range.Tail(count); }

	//! @returns at most \p count elements from the end of the array.
	[[nodiscard]] CSpan<T> Tail(index_t count) const { return AsConstRange().Tail(count); }
	//!@}


	[[nodiscard]] T* begin() {return range.Begin;}
	[[nodiscard]] const T* begin() const {return range.Begin;}
	[[nodiscard]] T* end() {return range.End;}
	[[nodiscard]] const T* end() const {return range.End;}

#ifdef INTRA_CONTAINER_STL_FORWARD_COMPATIBILITY
	typedef T value_type;
	typedef T* iterator;
	typedef const T* const_iterator;
	void push_back(T&& value) {AddLast(Move(value));}
	void push_back(const T& value) {AddLast(value);}
	void push_front(T&& value) {AddFirst(Move(value));}
	void push_front(const T& value) {AddFirst(value);}
	void pop_back() {RemoveLast();}
	void pop_front() {RemoveFirst();}
	T& back() {return Last();}
	const T& back() const {return Last();}
	T& front() {return First();}
	const T& front() const {return First();}
	bool empty() const {return Empty();}
	T* data() {return Data();}
	const T* data() const {return Data();}
	T& at(size_t index) {return operator[](index_t(index));}
	const T& at(size_t index) const {return operator[](index_t(index));}
	const T* cbegin() const {return begin();}
	const T* cend() const {return end();}
	size_t size() const {return size_t(Count());}
	size_t capacity() const {return size_t(Capacity());}
	void shrink_to_fit() {TrimExcessCapacity();}
	void clear() {Clear();}

	iterator insert(const_iterator pos, const T& value) {Insert(size_t(pos-Data()), value);}
	iterator insert(const_iterator pos, T&& value) {Insert(size_t(pos-Data()), Move(value));}

	iterator insert(const_iterator pos, size_t count, const T& value);
	template<typename InputIt> iterator insert(const_iterator pos, InputIt first, InputIt last);

	iterator insert(const T* pos, std::initializer_list<T> ilist)
	{Insert(size_t(pos-Data()), CSpan<T>(ilist));}

	/*!
	  Invalidates all iterators. This behaviour is different from the one of std::vector<T>::erase
	  @see Remove
	*/
	T* erase(const_iterator pos)
	{
		Remove(size_t(pos-Data()));
		return Data()+(pos-Data());
	}

	/*!
	  Invalidates all iterators. This behaviour is different from the one of std::vector<T>::erase
	  @see Remove
	*/
	T* erase(const_iterator firstPtr, const_iterator endPtr)
	{
		Remove(firstPtr-Data(), endPtr-Data());
		return Data()+(firstPtr-Data());
	}

	void reserve(size_t newCapacity) { Reserve(index_t(newCapacity)); }
	void resize(size_t newCount) { SetCount(index_t(newCount)); }
	//void resize(size_t count, const T& value);

	void swap(Array& rhs)
	{
		Swap(range, rhs.range);
		Swap(buffer, rhs.buffer);
	}
#endif


private:
	size_t setCountNotConstruct(Index newCount)
	{
		const auto oldCount = size_t(Count());
		if(size_t(newCount) <= oldCount)
		{
			Destruct<T>(range.Drop(newCount));
			range.End = range.Begin + size_t(newCount);
			return oldCount;
		}
		SetCountUninitialized(newCount);
		return oldCount;
	}

	template<typename R> void addFirstRangeHelper(R&& values)
	{
		Array temp = ForwardAsRange<R>(values);
		CheckSpace(0, temp.Length());
		range.Begin -= temp.Length();
		MoveInit(Span<T>(range.Begin, temp.Length()), temp.AsRange());
	}

	template<typename R> void addLastRangeHelper(R&& values)
	{
		Array temp = ForwardAsRange<R>(values);
		CheckSpace(temp.Length(), 0);
		MoveInit(SpanOfPtr(range.End, temp.Length()), SpanOf(temp));
		range.End += temp.Length();
	}

	Span<T> buffer, range;
};

template<typename T> [[nodiscard]] inline T* begin(Array<T>& arr) {return arr.begin();}
template<typename T> [[nodiscard]] inline const T* begin(const Array<T>& arr) {return arr.begin();}
template<typename T> [[nodiscard]] inline T* end(Array<T>& arr) {return arr.end();}
template<typename T> [[nodiscard]] inline const T* end(const Array<T>& arr) {return arr.end();}


template<typename T> constexpr bool IsTriviallyRelocatable<Array<T>> = true;

#if INTRA_CONSTEXPR_TEST
static_assert(CHasDataOf<Array<int>>);
static_assert(CHasLengthOf<Array<int>>);
static_assert(CHasDataOf<Array<StringView>>);
static_assert(CHasDataOf<const Array<StringView>&>);
static_assert(CArrayClass<const Array<StringView>&>);
static_assert(CTriviallyRelocatable<Array<int>>);
#endif

INTRA_END
