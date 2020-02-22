#pragma once

#include "Core/Type.h"
#include "Core/CArray.h"
#include "Core/Range/Concepts.h"
#include "Core/Range/Operations.h"

#include "Core/CContainer.h"
#include "Core/Range/Span.h"

#include "Core/Range/Comparison.h"
#include "Core/Range/Search/Single.h"
#include "Core/Range/Mutation/Copy.h"
#include "Memory/Memory.h"
#include "Memory/Allocator/Global.h"
#include "Container/ForwardDecls.h"
#include "Container/Operations.hh"


INTRA_BEGIN
template<typename T> class Array
{
public:
	constexpr forceinline Array(null_t=null) {}

	explicit forceinline Array(size_t initialCount) {SetCount(initialCount);}

	forceinline Array(InitializerList<T> values):
		Array(CSpan<T>(values)) {}
	
	Array(CSpan<T> values)
	{
		SetCountUninitialized(values.Length());
		CopyInit(range, values);
	}

	template<size_t N> forceinline Array(const T(&values)[N]): Array(SpanOf(values)) {}

	template<typename R,
		typename AsR=TRangeOfTypeNoCRef<R>,
	typename = Requires<
		!CSameIgnoreCVRef<R, Array> &&
		CInputRange<AsR> &&
		(CCopyConstructible<AsR> ||
			CHasLength<AsR>)
	>> forceinline Array(R&& values)
	{AddLastRange(ForwardAsRange<R>(values));}

	constexpr forceinline Array(Array&& rhs) noexcept: buffer(rhs.buffer), range(rhs.range)
	{rhs.buffer = null; rhs.range = null;}

	forceinline Array(const Array& rhs): Array(rhs.AsConstRange()) {}
	
	forceinline ~Array() {operator=(null);}


	/** Take ownership of rangeToOwn elements and memory.
	  Warning: ``rangeToOwn`` must be allocated with the same allocator as the template argument Allocator!
	*/
	static INTRA_NODISCARD forceinline Array CreateAsOwnerOfUnsafe(Span<T> rangeToOwn)
	{
		Array result;
		result.range = result.buffer = rangeToOwn;
		return result;
	}

	//! Create an Array with count default initialized elements.
	static INTRA_NODISCARD forceinline Array CreateWithCount(size_t count)
	{
		Array result;
		result.SetCount(count);
		return result;
	}

	//! Create an Array with count copies of initValue.
	static INTRA_NODISCARD forceinline Array CreateWithCount(size_t count, const T& initValue)
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

	forceinline Array& operator=(CSpan<T> values)
	{
		INTRA_DEBUG_ASSERT(!buffer.Overlaps(rhs));
		Assign(values);
		return *this;
	}

	template<typename R> forceinline Requires<
		CAsAccessibleRange<R>,
	Array&> operator=(R&& values) {return operator=(Array(Forward<R>(values)));}

	//! Delete all elements and free memory.
	Array& operator=(null_t)
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

	template<typename U> forceinline void Assign(Span<U> rhs) {Assign(rhs.AsConstRange());}

	//! Add a new element to the begining of the array by moving value.
	/*!
	  Unlike most other array implementations this operation has O(1) complexity.
	*/
	inline T& AddFirst(T&& value)
	{
		if(LeftSpace()) return *new(--range.Begin) T(Move(value));
		T temp = Move(value);
		CheckSpace(0, 1);
		return *new(Construct, --range.Begin) T(Move(temp));
	}

	/** Add \p value to the beginning of the Array.
	  Unlike most other array implementations this operation has amortized constant O(1) complexity.
	*/
	inline T& AddFirst(const T& value)
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
	template<typename R> forceinline Requires<
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
		typename AsR = TRangeOfTypeNoCRef<R>> Requires<
		CConsumableRangeOf<AsR, T> &&
		(CForwardRange<AsR> || CHasLength<AsR>)
	> AddFirstRange(R&& values)
	{
		auto valueRange = ForwardAsRange<R>(values);
		const size_t valuesCount = Count(values);
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
		typename AsR = TRangeOfTypeNoCRef<R>
	> Requires<
		CConsumableRangeOf<AsR, T> &&
		(CForwardRange<AsR> || CHasLength<AsR>)
	> AddLastRange(R&& values)
	{
		auto valueRange = ForwardAsRange<R>(values);
		const size_t valuesCount = Intra::Count(valueRange);
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
		typename AsR = TRangeOfTypeNoCRef<R>
	> forceinline Requires<
		CConsumableRangeOf<AsR, T> &&
		!(CForwardRange<AsR> || CHasLength<AsR>)
	> AddLastRange(R&& values)
	{
		addLastRangeHelper(ForwardAsRange<R>(values));
	}

	//! Set element at index ``pos`` to ``value``.
	//! If ``pos`` >= Count(), it adds ``pos`` - Count() default initialized elements and adds ``value``.
	template<typename U> T& Set(size_t pos, U&& value)
	{
		if(pos >= Count())
		{
			Reserve(pos+1);
			SetCount(pos);
			return AddLast(Forward<U>(value));
		}
		operator[](pos) = Forward<U>(value);
	}
	

	/** Insert ``values`` into ``pos``.
	  As a result for each i-th element its index will stay unchanged if i < ``pos`` and will become i + ``values.Length()`` otherwise.
	  The first inserted element will have index ``pos``.
	*/
	template<typename U> void Insert(size_t pos, CSpan<U> values)
	{
		if(values.Empty()) return;
		INTRA_PRECONDITION(!range.Overlaps(values));
		const size_t valuesCount = values.Length();

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

	template<typename U> forceinline void Insert(const T* it, CSpan<U> values)
	{
		INTRA_PRECONDITION(range.ContainsAddress(it));
		Insert(it-range.Begin, values);
	}

	forceinline void Insert(size_t pos, const T& value)
	{
		if(range.ContainsAddress(&value))
		{
			T temp = value;
			Insert(pos, {&temp, 1});
			return;
		}
		Insert(pos, {&value, 1});
	}

	forceinline void Insert(const T* it, const T& value)
	{
		INTRA_PRECONDITION(range.ContainsAddress(it));
		Insert(size_t(it-range.Begin), value);
	}


	//! Add new element to the end using stream syntax.
	forceinline Array& operator<<(const T& value) {AddLast(value); return *this;}
	forceinline Array& operator<<(T&& value) {AddLast(Move(value)); return *this;}

	//! Get and remove the last array element moving it to the right operand.
	forceinline Array& operator>>(T& value)
	{
		value = Move(Last());
		RemoveLast();
		return *this;
	}

	//! Get and remove the last array element.
	forceinline T PopLastElement()
	{
		T result = Move(Last());
		RemoveLast();
		return result;
	}


	//! Pop the first element from the end.
	forceinline T PopFirstElement()
	{
		T result = Move(First());
		RemoveFirst();
		return result;
	}

	/** Set new capacity of the array.
	  If rightPartSize < Count() Calls destructor for all elements with index >= rightPartSize.
	*/
	void Resize(size_t rightPartSize, size_t leftPartSize = 0)
	{
		if(rightPartSize + leftPartSize == 0) {*this = null; return;}

		// Delete all elements out of the new bounds
		if(rightPartSize <= Count()) Destruct(range.Drop(rightPartSize));

		size_t newCapacity = rightPartSize+leftPartSize;
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
	void Reserve(size_t rightPart, size_t leftSpace = 0)
	{
		const size_t currentRightPartSize = size_t(buffer.End - range.Begin);
		const size_t currentLeftSpace = LeftSpace();
		if(rightPart <= currentRightPartSize && leftSpace <= currentLeftSpace) return;

		const size_t currentSize = Capacity();
		if(rightPart > 0)
		{
			if(leftSpace>0) Resize(currentSize/4+rightPart, currentSize/4+leftSpace);
			else Resize(currentSize/2+rightPart, currentLeftSpace);
		}
		else Resize(currentRightPartSize, currentLeftSpace+currentSize/2+leftSpace);
	}

	//! May be more comfortable alternative to Reserve.
	//! @see Reserve
	forceinline void CheckSpace(size_t rightSpace, size_t leftSpace=0) {Reserve(Count() + rightSpace, leftSpace);}

	//! Remove all array elements without freeing allocated memory.
	forceinline void Clear()
	{
		if(Empty()) return;
		Destruct<T>(range);
		range.End = range.Begin;
	}

	//! @returns true if Array is empty.
	constexpr forceinline bool Empty() const noexcept {return range.Empty();}

	//! @returns number of elements that can be inserted into the beginning of the array before reallocation is necessary.
	constexpr forceinline size_t LeftSpace() const noexcept {return size_t(range.Begin - buffer.Begin);}

	//! @returns number of elements that can be inserted into the end of the array before reallocation is necessary.
	constexpr forceinline size_t RightSpace() const noexcept {return size_t(buffer.End - range.End);}

	/*! @name Element order preserving remove operations
	  @warning: These operations invalidate all ranges, iterators and pointers referring to the elements of this Array.
	*/
	///@{
	//! Remove one element at \p index.
	void Remove(size_t index)
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
	forceinline void Remove(T* ptr)
	{
		INTRA_PRECONDITION(range.ContainsAddress(ptr));
		Remove(ptr - range.Begin);
	}

	//! Remove the all elements in index range ``[removeStart; removeEnd)``.
	void Remove(size_t removeStart, size_t removeEnd)
	{
		INTRA_PRECONDITION(removeStart <= removeEnd);
		INTRA_PRECONDITION(removeEnd <= Count());
		if(removeEnd == removeStart) return;
		const size_t elementsToRemove = removeEnd-removeStart;
		Destruct<T>(range(removeStart, removeEnd));

		//Быстрые частные случаи
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

		if(removeStart + elementsToRemove/2 >= Count()/4)
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
	forceinline void FindAndRemove(const T& value)
	{
		size_t found = range.CountUntil(value);
		if(found != Count()) Remove(found);
	}
	///@}

	//! Remove duplicated elements from the Array without keeping order of its elements.
	void RemoveDuplicatesUnordered()
	{
		for(size_t i=0; i<Count(); i++)
			for(size_t j=i+1; j<Count(); j++)
				if(operator[](i)==operator[](j))
					RemoveUnordered(j--);
	}

	//! Remove first Array element. Complexity ``O(1)``
	forceinline void RemoveFirst() {INTRA_PRECONDITION(!Empty()); (range.Begin++)->~T();}

	//! Remove last Array element. Complexity ``O(1)``
	forceinline void RemoveLast() {INTRA_PRECONDITION(!Empty()); (--range.End)->~T();}

	//! Fast ``O(1)`` remove by moving last element onto element being removed (no shift).
	forceinline void RemoveUnordered(size_t index)
	{
		INTRA_PRECONDITION(index < Count());
		if(index < Count() - 1) range[index] = Move(*--range.End);
		else RemoveLast();
	}

	//! Find the first element equal to ``value`` and remove it by replacing it with the last element.
	void FindAndRemoveUnordered(const T& value)
	{
		size_t index = CountUntil(range, value);
		if(index != Count()) RemoveUnordered(index);
	}

	//! If the ratio of Capacity() / Count() > 125% do a reallocation to free all unused memory.
	forceinline void TrimExcessCapacity() {if(Capacity() > size_t(uint64(Count()) * 5/4)) Resize(Count());}


	INTRA_NODISCARD forceinline T& operator[](size_t index) {INTRA_PRECONDITION(index < Count()); return range.Begin[index];}
	INTRA_NODISCARD forceinline const T& operator[](size_t index) const {INTRA_PRECONDITION(index < Count()); return range.Begin[index];}

	INTRA_NODISCARD forceinline T& Last() {INTRA_PRECONDITION(!Empty()); return range.Last();}
	INTRA_NODISCARD forceinline const T& Last() const {INTRA_PRECONDITION(!Empty()); return range.Last();}
	INTRA_NODISCARD forceinline T& First() {INTRA_PRECONDITION(!Empty()); return range.First();}
	INTRA_NODISCARD forceinline const T& First() const {INTRA_PRECONDITION(!Empty()); return range.First();}

	INTRA_NODISCARD forceinline T* Data() {return begin();}
	INTRA_NODISCARD forceinline const T* Data() const {return begin();}

	INTRA_NODISCARD forceinline T* End() {return end();}
	INTRA_NODISCARD forceinline const T* End() const {return end();}


	//! @return total size of Array contents in bytes.
	INTRA_NODISCARD forceinline size_t SizeInBytes() const noexcept {return Count()*sizeof(T);}

	///@{
	//! @return Number of stored elements.
	INTRA_NODISCARD forceinline size_t Count() const noexcept {return range.Length();}
	INTRA_NODISCARD forceinline index_t Length() const noexcept {return Count();}
	///@}

	/** Set number of stored elements.
	  If ``newCount`` > Count() removes ``newCount`` - Count() last elements.
	  Otherwise construct Count() - ``newCount`` elements at the end using default constructor.
	*/
	void SetCount(size_t newCount)
	{
		const size_t oldCount = setCountNotConstruct(newCount);
		Initialize<T>(range.Drop(oldCount));
	}

	/** Set number of stored elements.
	  If newCount > Count removes newCount - Count last elements.
	  Otherwise construct Count - newCount elements at the end passing \p args to its constructor.
	*/
	template<typename... Args> void SetCountEmplace(size_t newCount, Args&... args)
	{
		const size_t oldCount = setCountNotConstruct(newCount);
		for(T& obj: range.Drop(oldCount)) new(&obj) T(args...);
	}

	/** Set number of stored elements.
	  If newCount > Count removes newCount - Count last elements.
	  Otherwise adds Count - newCount copies of \p initValue at the end.
	*/
	void SetCount(size_t newCount, const T& initValue)
	{
		const size_t oldCount = setCountNotConstruct(newCount);
		for(T& dst: Drop(oldCount)) new(dst) T(initValue);
	}

	/** Set number of stored elements without calling destructors and constructors.

	  Warning: Do not use it with non-POD types.
	  Otherwise calling constructors and destructors is on the caller's responsibility.
	*/
	void SetCountUninitialized(size_t newCount)
	{
		Reserve(newCount, 0);
		if(newCount == 0) range.Begin = buffer.Begin;
		range.End = range.Begin + newCount;
	}

	/** Add \p newElements elements to the beginning of the Array without initialization.
	
	  Warning: Do not use it with non-POD types.
	  Otherwise calling constructors and destructors is on the caller's responsibility.
	*/
	void AddLeftUninitialized(size_t newElements)
	{
		Reserve(0, newElements);
		range.Begin -= newElements;
	}

	//! Get current size of the buffer measured in elements it can store.
	INTRA_NODISCARD forceinline size_t Capacity() const {return buffer.Length();}

	INTRA_NODISCARD forceinline bool IsFull() const {return Count() == Capacity();}

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
	INTRA_NODISCARD forceinline operator Span<T>() {return AsRange();}
	INTRA_NODISCARD forceinline operator CSpan<T>() const {return AsRange();}
	INTRA_NODISCARD forceinline Span<T> AsRange() {return range;}
	INTRA_NODISCARD forceinline CSpan<T> AsConstRange() const {return range.AsConstRange();}
	INTRA_NODISCARD forceinline CSpan<T> AsRange() const {return AsConstRange();}

	/** Create a slice of this array's elements.
	  @returns Span containing elements with indices [\p firstIndex; \p endIndex)
	*/
	INTRA_NODISCARD forceinline Span<T> operator()(size_t firstIndex, size_t endIndex)
	{
		INTRA_PRECONDITION(firstIndex <= endIndex);
		INTRA_PRECONDITION(endIndex <= Count());
		return range(firstIndex, endIndex);
	}

	/** Create a slice of this array's elements.
	  @returns Span containing elements with indices [\p firstIndex; \p endIndex)
	*/
	INTRA_NODISCARD forceinline CSpan<T> operator()(size_t firstIndex, size_t endIndex) const
	{
		INTRA_PRECONDITION(firstIndex <= endIndex);
		INTRA_PRECONDITION(endIndex <= Count());
		return AsConstRange()(firstIndex, endIndex);
	}

	//! @returns at most \p count elements from the beginning of the array.
	INTRA_NODISCARD Span<T> Take(size_t count) {return range.Take(count);}

	//! @returns at most \p count elements from the beginning of the array.
	INTRA_NODISCARD CSpan<T> Take(size_t count) const {return AsConstRange().Take(count);}
	
	//! @returns Span containing all elements of Array after \p count first elements.
	INTRA_NODISCARD Span<T> Drop(size_t count) {return range.Drop(count);}

	//! @returns Span containing all elements of Array after \p count first elements.
	INTRA_NODISCARD CSpan<T> Drop(size_t count) const {return AsConstRange().Drop(count);}

	//! @returns at most \p count elements from the end of the array.
	INTRA_NODISCARD Span<T> Tail(size_t count) {return range.Tail(count);}

	//! @returns at most \p count elements from the end of the array.
	INTRA_NODISCARD CSpan<T> Tail(size_t count) const {return AsConstRange().Tail(count);}
	//!@}


	INTRA_NODISCARD forceinline T* begin() {return range.Begin;}
	INTRA_NODISCARD forceinline const T* begin() const {return range.Begin;}
	INTRA_NODISCARD forceinline T* end() {return range.End;}
	INTRA_NODISCARD forceinline const T* end() const {return range.End;}

#ifdef INTRA_CONTAINER_STL_FORWARD_COMPATIBILITY
	typedef T value_type;
	typedef T* iterator;
	typedef const T* const_iterator;
	forceinline void push_back(T&& value) {AddLast(Move(value));}
	forceinline void push_back(const T& value) {AddLast(value);}
	forceinline void push_front(T&& value) {AddFirst(Move(value));}
	forceinline void push_front(const T& value) {AddFirst(value);}
	forceinline void pop_back() {RemoveLast();}
	forceinline void pop_front() {RemoveFirst();}
	forceinline T& back() {return Last();}
	forceinline const T& back() const {return Last();}
	forceinline T& front() {return First();}
	forceinline const T& front() const {return First();}
	forceinline bool empty() const {return Empty();}
	forceinline T* data() {return Data();}
	forceinline const T* data() const {return Data();}
	forceinline T& at(size_t index) {return operator[](index);}
	forceinline const T& at(size_t index) const {return operator[](index);}
	forceinline const T* cbegin() const {return begin();}
	forceinline const T* cend() const {return end();}
	forceinline size_t size() const {return Count();}
	forceinline size_t capacity() const {return Capacity();}
	forceinline void shrink_to_fit() {TrimExcessCapacity();}
	forceinline void clear() {Clear();}

	forceinline iterator insert(const_iterator pos, const T& value) {Insert(size_t(pos-Data()), value);}
	forceinline iterator insert(const_iterator pos, T&& value) {Insert(size_t(pos-Data()), Move(value));}

	forceinline iterator insert(const_iterator pos, size_t count, const T& value);
	template<typename InputIt> forceinline iterator insert(const_iterator pos, InputIt first, InputIt last);

	forceinline iterator insert(const T* pos, std::initializer_list<T> ilist)
	{Insert(size_t(pos-Data()), CSpan<T>(ilist));}

	/*!
	  Invalidates all iterators. This behaviour is different from the one of std::vector<T>::erase
	  @see Remove
	*/
	forceinline T* erase(const_iterator pos)
	{
		Remove(size_t(pos-Data()));
		return Data()+(pos-Data());
	}

	/*!
	  Invalidates all iterators. This behaviour is different from the one of std::vector<T>::erase
	  @see Remove
	*/
	forceinline T* erase(const_iterator firstPtr, const_iterator endPtr)
	{
		Remove(size_t(firstPtr-Data()), size_t(endPtr-Data()));
		return Data()+(firstPtr-Data());
	}

	forceinline void reserve(size_t newCapacity) {Reserve(newCapacity);}
	forceinline void resize(size_t newCount) {SetCount(newCount);}
	//void resize(size_t count, const T& value);

	forceinline void swap(Array& rhs)
	{
		Swap(range, rhs.range);
		Swap(buffer, rhs.buffer);
	}
#endif


private:
	size_t setCountNotConstruct(size_t newCount)
	{
		const size_t oldCount = Count();
		if(newCount <= oldCount)
		{
			Destruct<T>(range.Drop(newCount));
			range.End = range.Begin + newCount;
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
		MoveInit(Span<T>(range.End, temp.Length()), temp.AsRange());
		range.End += temp.Length();
	}

	Span<T> buffer, range;
};

template<typename T> INTRA_NODISCARD forceinline T* begin(Array<T>& arr) {return arr.begin();}
template<typename T> INTRA_NODISCARD forceinline const T* begin(const Array<T>& arr) {return arr.begin();}
template<typename T> INTRA_NODISCARD forceinline T* end(Array<T>& arr) {return arr.end();}
template<typename T> INTRA_NODISCARD forceinline const T* end(const Array<T>& arr) {return arr.end();}


template<typename T> constexpr bool IsTriviallyRelocatable<Array<T>> = true;

#if INTRA_CONSTEXPR_TEST
static_assert(CHasDataOf<Array<int>>, "DataOf must work!");
static_assert(CHasLengthOf<Array<int>>, "LengthOf must work!");
static_assert(CHasDataOf<Array<StringView>>, "DataOf must work!");
static_assert(CHasDataOf<const Array<StringView>&>, "DataOf must work!");
static_assert(CArrayClass<const Array<StringView>&>, "Array must be an array class!");
static_assert(CTriviallyRelocatable<Array<int>>, "Array must be trivially relocatable!");
#endif

INTRA_END
