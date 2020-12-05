#pragma once

#include "Intra/Range/Comparison.h"
#include "Intra/Range/Retro.h"
#include "Intra/Range/ListRange.h"

#include "IntraX/Memory/Allocator/AllocatorRef.h"
#include "IntraX/Container/AllForwardDecls.h"

INTRA_BEGIN
INTRA_IGNORE_WARN_SIGN_CONVERSION
template<typename T, class AllocatorType> class BList:
	public AllocatorRef<AllocatorType>
{
	typedef AllocatorRef<AllocatorType> AllocatorRef;
public:
	typedef T value_type;
	//typedef RangeForwardIterator<RRetro<BListRange<T>>> reverse_iterator;
	//typedef RangeForwardIterator<RRetro<BListRange<const T>>> const_reverse_iterator;
	typedef AllocatorType Allocator;
	typedef typename BListRange<T>::Node Node;
	typedef typename BListRange<const T>::Node ConstNode;

	struct iterator
	{
		iterator(decltype(null)=null): mNode(null) {}
		T& operator*() const {return mNode->Value;}
		T* operator->() const {return &mNode->Value;}
		iterator& operator++() {INTRA_PRECONDITION(mNode != null); mNode = mNode->Next;}
		iterator& operator--() {INTRA_PRECONDITION(mNode != null); mNode = mNode->Prev;}
	private:
		iterator(Node* node): mNode(node) {}
		Node* mNode;
	};

	struct const_iterator
	{
		const_iterator(decltype(null)=null): mNode(null) {}
		const_iterator(iterator it): mNode(reinterpret_cast<ConstNode*>(it.mNode)) {}
		const T& operator*() const {return mNode->Value;}
		const T* operator->() const {return &mNode->Value;}
		const_iterator& operator++() {INTRA_PRECONDITION(mNode != null); mNode = mNode->Next;}
		const_iterator& operator--() {INTRA_PRECONDITION(mNode != null); mNode = mNode->Prev;}
	private:
		const_iterator(ConstNode* node): mNode(node) {}
		ConstNode* mNode;
	};


	INTRA_FORCEINLINE BList(decltype(null)=null): mRange(null), mCount(0) {}

	INTRA_FORCEINLINE BList(Allocator& allocator):
		AllocatorRef(allocator), mRange(null), mCount(0) {}

	INTRA_FORCEINLINE BList(BList&& rhs):
		AllocatorRef(rhs), mRange(rhs.mRange), mCount(rhs.mCount)
	{rhs.mRange = null; rhs.mCount = 0;}

	template<typename R, typename = Requires<
		CConsumableRangeOf<R, T> &&
		!CSameIgnoreCVRef<R, BList>
	>> INTRA_FORCEINLINE BList(R&& values):
		mRange(null), mCount(0)
	{AddLastRange(ForwardAsRange<R>(values));}

	template<size_t N> BList(T(&arr)[N]) {operator=(RangeOf(arr));}

	BList(const BList& rhs):
		AllocatorRef(rhs), mRange(null), mCount(0)
	{operator=(rhs.AsRange());}
	
	~BList() {Clear();}

	/// Constructs a new element at the end of the list passing \p args to its constructor.
	/*!
	  @returns the reference to the added element.
	*/
	template<typename... Args> INTRA_FORCEINLINE T& EmplaceLast(Args&&... args)
	{return *new(Construct, &add_last_node()->Value) T(Forward<Args>(args)...);}

	/// Add \p value to the end of the list.
	/*!
	  @returns the reference to the added element.
	*/
	INTRA_FORCEINLINE T& AddLast(const T& value)
	{return *new(Construct, &add_last_node()->Value) T(value);}

	/** Move \p value to the end of the list.
	  @returns the reference to the added element.
	*/
	INTRA_FORCEINLINE T& AddLast(T&& value)
	{return *new(Construct, &add_last_node()->Value) T(Move(value));}

	/** Constructs a new element at the beginning of the list passing ``args`` to its constructor.
	  @returns the reference to the added element.
	*/
	template<typename... Args> INTRA_FORCEINLINE T& EmplaceFirst(Args&&... args)
	{return *new(Construct, &add_first_node()->Value) T(Forward<Args>(args)...);}

	/** Add ``value`` to the beginning of the list.
	  @returns the reference to the added element.
	*/
	INTRA_FORCEINLINE T& AddFirst(const T& value) {return EmplaceFirst(value);}
	
	/** Move ``value`` to the beginning of the list.
	  @returns the reference to the added element.
	*/
	INTRA_FORCEINLINE T& AddFirst(T&& value) {return EmplaceFirst(Move(value));}


	/** Construct a new element at the list immediately before the element pointed by ``pos`` passing ``args`` to its constructor.
	  @returns an iterator pointing to the inserted element.
	*/
	template<typename... Args> iterator Emplace(const_iterator pos, Args&&... args)
	{
		if(pos.mNode == null)
		{
			AddLast(Forward<Args>(args)...);
			return mRange;
		}
		Node* const node = insert_node(pos.mNode);
		new(Construct, &node->Value) T(Forward<Args>(args)...);
		return {node};
	}

	/** Insert a ``value`` to the list immediately before the element pointed by ``pos``.
	  @returns an iterator pointing to the inserted element.
	*/
	BListRange<T> Insert(const_iterator pos, const T& value) {return Emplace(pos, value);}

	/// Move ``value`` to the list immediately before the element pointed by ``pos``.
	iterator Insert(const_iterator pos, T&& value) {return Emplace(pos, Move(value));}

	/** Remove the element at position pointed by iterator ``pos``.
	  @returns the part of range after the removed element.
	*/
	iterator Remove(const_iterator pos)
	{
		INTRA_DEBUG_ASSERT(pos.mNode != null);
		pos.mNode->Value.~T();
		Node* const next = pos.mNode->Next;
		remove_node(pos.mNode);
		return {next};
	}

	/// Remove the first element from the list.
	void RemoveFirst()
	{
		mRange.FirstNode->Value.~T();
		auto deletedNode = mRange.FirstNode;
		mRange.FirstNode = mRange.FirstNode->Next;
		if(mRange.FirstNode!=null) mRange.FirstNode->Prev = null;
		else mRange.LastNode=null;
		AllocatorRef::Free(deletedNode, sizeof(Node));
	}

	/// Remove the last element from the list.
	void RemoveLast()
	{
		mRange.LastNode->Value.~T();
		auto deletedNode = mRange.LastNode;
		mRange.LastNode = mRange.LastNode->Prev;
		if(mRange.LastNode!=null) mRange.LastNode->Next = null;
		else mRange.FirstNode = null;
		AllocatorRef::Free(deletedNode, sizeof(Node));
	}

	/// Remove all elements from the list.
	void Clear()
	{
		Node* current = mRange.FirstNode;
		while(current!=null)
		{
			current->Value.~T();
			Node* next = current->Next;
			AllocatorRef::Free(current, sizeof(Node));
			current = next;
		}
		mRange = null;
		mCount = 0;
	}

	/** Set the element count for the list to ``newCount``.
	  If Count() > ``newCount`` delete the last Count() - ``newCount`` elements,
	  otherwise default construct new ``newCount`` - Count() elements at the end of the list.
	*/
	void SetCount(size_t newCount)
	{
		while(mCount > newCount)
		{
			RemoveLast();
			mCount--;
		}
		while(mCount < newCount)
		{
			EmplaceLast();
			mCount++;
		}
	}

	/** Set the element count for the list to ``newCount``.
	  If Count() > ``newCount` delete the last Count() - ``newCount`` elements,
	  otherwise insert ``newCount`` - Count() copies of ``value`` into the end of the list.
	*/
	void SetCount(size_t newCount, const T& value)
	{
		while(mCount > newCount)
		{
			RemoveLast();
			mCount--;
		}
		while(mCount < newCount)
		{
			AddLast(value);
			mCount++;
		}
	}


	template<typename R> Requires<
		CConsumableListOf<R, T> &&
		!CSameIgnoreCVRef<R, BList>
	> AddLastRange(R&& values)
	{
		auto valuesRange = ForwardAsRange<R>(values);
		for(; !valuesRange.Empty(); valuesRange.PopFirst())
			AddLast(valuesRange.First());
	}

	template<typename R> Requires<
		CConsumableListOf<R, T> &&
		!CSameIgnoreCVRef<R, BList>,
	BList&> operator=(R&& values)
	{
		Clear();
		AddLastRange(ForwardAsRange<R>(values));
		return *this;
	}

	template<size_t N> BList& operator=(T(&arr)[N])
	{return operator=(RangeOf(arr));}

	BList& operator=(const BList& rhs)
	{
		if(this == &rhs) return *this;
		return operator=(rhs.AsRange());
	}

	BList& operator=(BList&& rhs)
	{
		if(this == rhs) return *this;
		Clear();
		mRange = rhs.mRange;
		mCount = rhs.mCount;
		rhs.mRange = null;
		rhs.mCount = 0;
		return *this;
	}

	INTRA_FORCEINLINE BList& operator=(decltype(null)) {Clear(); return *this;}

	INTRA_FORCEINLINE T& First() {return mRange.First();}
	INTRA_FORCEINLINE T& Last() {return mRange.Last();}

	INTRA_FORCEINLINE const T& First() const {return mRange.First();}
	INTRA_FORCEINLINE const T& Last() const {return mRange.Last();}

	INTRA_FORCEINLINE iterator LastIterator() {return {mRange.LastNode};}
	INTRA_FORCEINLINE const_iterator LastConstIterator() const {return {mRange.AsConstRange().LastNode};}
	INTRA_FORCEINLINE const_iterator LastIterator() const {return LastConstIterator();}

	INTRA_FORCEINLINE bool Empty() const {return mRange.Empty();}
	INTRA_FORCEINLINE bool operator==(decltype(null)) const {return mRange.Empty();}
	INTRA_FORCEINLINE bool operator!=(decltype(null)) const {return !mRange.Empty();}
	INTRA_FORCEINLINE bool operator==(const BList& rhs) const {return mCount==rhs.mCount && Equals(mRange, rhs.mRange);}
	INTRA_FORCEINLINE bool operator!=(const BList& rhs) const {return !operator==(rhs);}

	INTRA_FORCEINLINE const BListRange<T>& AsRange() {return mRange;}
	INTRA_FORCEINLINE const BListRange<const T>& AsRange() const {return mRange.AsConstRange();}
	INTRA_FORCEINLINE const BListRange<T>& operator()() {return mRange;}
	INTRA_FORCEINLINE const BListRange<const T>& operator()() const {return mRange.AsConstRange();}



	INTRA_FORCEINLINE iterator begin() {return {mRange.FirstNode};}
	INTRA_FORCEINLINE iterator end() {return null;}
	INTRA_FORCEINLINE const_iterator begin() const {return {mRange.AsConstRange().FirstNode};}
	INTRA_FORCEINLINE const_iterator end() const {return null;}
	INTRA_FORCEINLINE const_iterator cbegin() const {return begin();}
	INTRA_FORCEINLINE const_iterator cend() const {return null;}

#ifdef INTRA_CONTAINER_STL_FORWARD_COMPATIBILITY
	/*! @name STL compatible interface
	  These methods are available only if INTRA_CONTAINER_STL_FORWARD_COMPATIBILITY is defined.
	*/
	///@{
	//INTRA_FORCEINLINE reverse_iterator rbegin() {return {Retro(mRange)};}
	//INTRA_FORCEINLINE reverse_iterator rend() {return null;}
	//INTRA_FORCEINLINE const_reverse_iterator rbegin() const {return {Retro(mRange.AsConstRange())};}
	//INTRA_FORCEINLINE const_reverse_iterator rend() const {return null;}
	//INTRA_FORCEINLINE const_reverse_iterator crbegin() const {return rbegin();}
	//INTRA_FORCEINLINE const_reverse_iterator crend() const {return null;}

	INTRA_FORCEINLINE iterator insert(const_iterator pos, const T& value) {return {Insert(pos, value)};}
	INTRA_FORCEINLINE iterator insert(const_iterator pos, T&& value) {return {Insert(pos, Move(value))};}
	template<typename... Args> INTRA_FORCEINLINE iterator emplace(const_iterator pos, Args&&... args) {return {Emplace(pos, Forward<Args>(args)...)};}

	INTRA_FORCEINLINE void push_back(const T& value) {AddLast(value);}
	INTRA_FORCEINLINE void push_back(T&& value) {AddLast(Move(value));}
	template<typename... Args> INTRA_FORCEINLINE void emplace_back(Args&&... args) {EmplaceLast(Forward<Args>(args)...);}

	INTRA_FORCEINLINE void push_front(const T& value) {AddFirst(value);}
	INTRA_FORCEINLINE void push_front(T&& value) {AddFirst(Move(value));}
	template<typename... Args> INTRA_FORCEINLINE void emplace_front(Args&&... args) {EmplaceFirst(Forward<Args>(args)...);}

	INTRA_FORCEINLINE iterator erase(const_iterator pos) {Remove(pos);}

	void pop_front() {RemoveFirst();}
	void pop_back() {RemoveLast();}

	T& front() {return First();}
	const T& front() const {return First();}
	T& back() {return Last();}
	const T& back() const {return Last();}

	void resize(size_t newCount) {SetCount(newCount);}
	void resize(size_t newCount, const T& value) {SetCount(newCount, value);}
	///@}
#endif

private:
	Node* add_first_node()
	{
		size_t nodeSize = sizeof(Node);
		Node* node = AllocatorRef::Allocate(nodeSize, INTRA_SOURCE_INFO);
		INTRA_DEBUG_ASSERT(nodeSize == sizeof(Node));
		node->Prev = null;
		node->Next = mRange.FirstNode;
		if(mRange.FirstNode!=null) mRange.FirstNode->Prev = node;
		mRange.FirstNode = node;
		if(mRange.LastNode==null) mRange.LastNode = node;
		mCount++;
		return node;
	}

	Node* add_last_node()
	{
		size_t nodeSize = sizeof(Node);
		Node* node = AllocatorRef::Allocate(nodeSize, INTRA_SOURCE_INFO);
		node->Prev = mRange.LastNode;
		node->Next = null;
		if(mRange.LastNode != null) mRange.LastNode->Next = node;
		mRange.LastNode = node;
		if(mRange.FirstNode==null) mRange.FirstNode = node;
		mCount++;
		return node;
	}

	Node* insert_node(Node* itnode)
	{
		auto prev = itnode->Prev;
		size_t nodeSize = sizeof(Node);
		Node* node = AllocatorRef::Allocate(nodeSize);
		INTRA_DEBUG_ASSERT(nodeSize == sizeof(Node));
		node->Prev = prev;
		node->Next = itnode;
		prev->Next = node;
		itnode->Prev = node;
		mCount++;
		return node;
	}

	void remove_node(Node* itnode)
	{
		if(itnode->Prev != null) itnode->Prev->Next = itnode->Next;
		if(itnode->Next != null) itnode->Next->Prev = itnode->Prev;
		if(mRange.FirstNode==itnode) mRange.FirstNode = mRange.FirstNode->Next;
		if(mRange.LastNode==itnode) mRange.LastNode = mRange.LastNode->Prev;
		AllocatorRef::Free(itnode, sizeof(Node));
		mCount--;
	}

	BListRange<T> mRange;
	size_t mCount;
};
INTRA_END
