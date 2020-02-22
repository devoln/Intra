#pragma once

#include "Core/Type.h"
#include "Core/Tuple.h"
#include "Core/Functional.h"
#include "Utils/FixedArray.h"

#include "Hash/ToHash.h"

#include "Core/Range/Sort/Quick.h"
#include "Core/Range/Mutation/Fill.h"

#include "Container/AllForwardDecls.h"

#include "Memory/Allocator/AllocatorRef.h"
#include "Memory/Memory.h"

INTRA_BEGIN
template<typename T> struct HashTableRange;

namespace D {

template<typename K> struct KeyWrapper {K Key;};

template<typename T> struct HashNode
{
	template<typename T1> friend struct Intra::HashTableRange;
	template<typename K, typename V, class Allocator> friend class Intra::HashMap;
private:
	template<typename... Args> HashNode(Args&&... args): element(Forward<Args>(args)...) {}
	~HashNode() {}

	HashNode* down;
	HashNode* prev;
	HashNode* next;
	T element;

private:
	HashNode(const HashNode&) = delete;
	HashNode& operator=(const HashNode&) = delete;
};

}


template<typename T> struct HashTableRange
{
	typedef D::HashNode<TRemoveConst<T>> NodeType;
	forceinline HashTableRange() = default;
	forceinline HashTableRange(null_t) {}

	forceinline bool operator==(const HashTableRange& rhs) const
	{
		return (Empty() && rhs.Empty()) ||
			(mFirstNode == rhs.mFirstNode && mLastNode == rhs.mLastNode);
	}
	forceinline bool operator!=(const HashTableRange& rhs) const {return !operator==(rhs);}
	forceinline bool operator==(null_t) const {return Empty();}
	forceinline bool operator!=(null_t) const {return !Empty();}

	forceinline void PopFirst() {INTRA_PRECONDITION(!Empty()); mFirstNode = mFirstNode->next;}
	forceinline void PopLast() {INTRA_PRECONDITION(!Empty()); mLastNode = mLastNode->prev;}
	forceinline bool Empty() const {return mFirstNode == null || mFirstNode->prev == mLastNode;}
	forceinline T& First() const {INTRA_PRECONDITION(!Empty()); return mFirstNode->element;}
	forceinline T& Last() const {INTRA_PRECONDITION(!Empty()); return mLastNode->element;}


	forceinline const HashTableRange<const T> AsConstRange() const
	{return {mFirstNode, mLastNode};}
	
	forceinline operator HashTableRange<const T>() const {return AsConstRange();}

private:
	friend struct HashTableRange<TRemoveConst<T>>;
	forceinline HashTableRange(NodeType* startNode, NodeType* lastNode):
		mFirstNode(startNode), mLastNode(lastNode) {}

	NodeType* mFirstNode = null;
	NodeType* mLastNode = null;

	template<typename K, typename V, class Allocator> friend class HashMap;
};

struct HashMapStatistics
{
	size_t NumBuckets;
	size_t FreeBucketCount;
	size_t MaxBucketLoad;
	double AverageBucketLoad;
	Span<size_t> BucketLoads;
};

template<typename K, typename V, typename AllocatorType>
class HashMap: AllocatorRef<AllocatorType>
{
	typedef AllocatorRef<AllocatorType> AllocatorRef;
	typedef D::HashNode<Tuple<const K, V>> Node;
public:
	typedef AllocatorType Allocator;

	typedef K key_type;
	typedef V mapped_type;
	typedef Tuple<const K, V> value_type;
	typedef HashTableRange<value_type> ElementRange;
	typedef HashTableRange<const value_type> ElementConstRange;


	struct iterator
    {
        forceinline iterator(Node* nodePtr=null): node(nodePtr) {}
		forceinline iterator(const ElementRange& r): node(r.mFirstNode) {}
 
		forceinline bool operator==(const iterator& rhs) const {return node==rhs.node;}
        forceinline bool operator!=(const iterator& rhs) const {return node!=rhs.node;}
        forceinline void GotoNext()
		{
			INTRA_DEBUG_ASSERT(node != null);
			node = node->next;
		}
        forceinline void GotoPrev() {INTRA_PRECONDITION(node != null); node = node->prev;}
 
        forceinline iterator& operator++() {GotoNext(); return *this;}
        forceinline iterator operator++(int) {iterator it = *this; GotoNext(); return it;}
        forceinline iterator& operator--() {GotoPrev(); return *this;}
        forceinline iterator operator--(int) {iterator it = *this; GotoPrev(); return it;}
 
        forceinline value_type* operator->() const {return &node->element;}
        forceinline value_type& operator*() const {return node->element;}
         
        Node* node;
    };
 
    struct const_iterator
    {
        forceinline const_iterator(Node* nodePtr=null): node(nodePtr) {}
		forceinline const_iterator(const ElementRange& r): node(r.mFirstNode) {}
		forceinline const_iterator(const ElementConstRange& r): node(r.mFirstNode) {}
        forceinline const_iterator(const iterator& rhs): node(rhs.node) {}
 
        forceinline bool operator==(const const_iterator& rhs) const {return node==rhs.node;}
        forceinline bool operator!=(const const_iterator& rhs) const {return node!=rhs.node;}
        forceinline void GotoNext() {INTRA_PRECONDITION(node != null); node = node->next;}
        forceinline void GotoPrev() {INTRA_PRECONDITION(node != null); node = node->prev;}
 
        forceinline const_iterator& operator++() {GotoNext(); return *this;}
        forceinline const_iterator operator++(int) {const_iterator it = *this; GotoNext(); return it;}
        forceinline const_iterator& operator--() {GotoPrev(); return *this;}
        forceinline const_iterator operator--(int) {const_iterator it = *this; GotoPrev(); return it;}
 
        forceinline const value_type* operator->() const {return &node->element;}
        forceinline const value_type& operator*() const {return node->element;}
         
        Node* node;
    };
 

	HashMap() = default;
	HashMap(null_t) {}

	HashMap(const HashMap& rhs): AllocatorRef(rhs) {operator=(rhs);}

	~HashMap() {Clear();}

	HashMap& operator=(const HashMap& rhs)
	{
		Clear();
		Insert(rhs);
		return *this;
	}

	HashMap& operator=(null_t)
	{
		Clear();
		return *this;
	}

	bool operator==(const HashMap& rhs) const
	{
		if(rhs.Count() != Count()) return false;

		ElementRange rangeCopy = mRange;
		while(!rangeCopy.Empty())
		{
			ElementRange found = rhs.Find(get<0>(rangeCopy.First()));
			if(found.Empty() || get<1>(found.First()) != get<1>(rangeCopy.First())) return false;
			rangeCopy.PopFirst();
		}
		return true;
	}

	bool operator!=(const HashMap& rhs) const {return !operator==(rhs);}

	INTRA_NODISCARD forceinline bool Empty() const {return Count() == 0;}
	INTRA_NODISCARD forceinline bool operator==(null_t) const {return Empty();}
	INTRA_NODISCARD forceinline bool operator!=(null_t) const {return !Empty();}

	V& operator[](const K& key)
	{
		if(mBucketHeads == null)
			return get<1>(insertNode(key, V(), false)->element);
		uint keyHash = ToHash(key);
		Node* node = findNode(key, keyHash);
		if(node!=null) return get<1>(node->element);
		return get<1>(insertNode(key, V(), false)->element);
	}

	V& operator[](K&& key)
	{
		if(mBucketHeads == null)
			return get<1>(insertNode(Move(key), V(), false)->element);
		uint keyHash = ToHash(key);
		Node* node = findNode(key, keyHash);
		if(node != null) return get<1>(node->element);
		return get<1>(insertNode(Move(key), V(), false)->element);
	}

	ElementRange Insert(const value_type& pair)
	{return ElementRange(insertNode(get<0>(pair), get<1>(pair)), mRange.mLastNode);}

	ElementRange Insert(K&& key, V&& value)
	{return ElementRange(insertNode(Move(key), Move(value)), mRange.mLastNode);}

	ElementRange Insert(const K& key, const V& value)
	{return ElementRange(insertNode(key, value), mRange.mLastNode);}

	void Insert(const HashMap& map)
	{
		ElementRange rangeCopy = map.mRange;
		while(!rangeCopy.Empty())
		{
			insertNode(get<0>(rangeCopy.First()), get<1>(rangeCopy.First()));
			rangeCopy.PopFirst();
		}
	}

	//! Insert the key-value pair only if provided key doesn't exist in the map.
	//! @returns range containing existing or inserted element with corresponding key and all the elements after it.
	ElementRange InsertNew(const K& key, const V& value)
	{
		const uint keyHash = ToHash(key);
		if(mBucketHeads != null)
		{
			auto node = findNode(key, keyHash);
			if(node != null) return ElementRange(node, mRange.mLastNode);
		}
		return ElementRange(insertNode(key, value, false), mRange.mLastNode);
	}

	//! Insert the key-value pair by moving key and value only if provided key doesn't exist in the map.
	//! @returns range containing existing or inserted element with corresponding key and all the elements after it.
	iterator InsertNew(K&& key, V&& value)
	{
		const uint keyHash = ToHash(key);
		if(mBucketHeads != null)
		{
			auto node = findNode(key, keyHash);
			if(node!=null) return ElementRange(node, mRange.mLastNode);
		}
		return ElementRange(insertNode(Move(key), Move(value), false), mRange.mLastNode);
	}

	//! Remove element by its key.
	//! @returns true if key existed.
	bool Remove(const K& key)
	{
		if(mBucketHeads == null) return false;
		const uint keyHash = ToHash(key);
		Node* previous;
		auto node = findNode(key, keyHash, previous);
		if(node == null) return false;

		if(previous != null) previous->down = node->down;
		else get_bucket_head(keyHash) = node->down;

		eraseNode(node);
		return true;
	}

	//! Удалить элемент из контейнера, на который указывает итератор it.
	//! @return Возвращает диапазон, содержащий все элементы, идущие после удаляемого элемента.
	ElementRange Remove(const_iterator it)
	{
		if(mBucketHeads == null || it == null) return null;

		auto node = it.node;
		auto next = node->next;

		uint keyHash = ToHash(get<0>(node->element));

		Node* previous = null;
		auto& bh = get_bucket_head(keyHash);
		auto current = bh;
		while(current != null && current != node)
		{
			previous = current;
			current = current->down;
		}

		INTRA_DEBUG_ASSERT(current == node);

		if(previous != null) previous->down = node->down;
		else bh = node->down;

		eraseNode(node);
		return ElementRange(next, mRange.mLastNode);
	}

	void Clear()
	{
		if(!Empty())
		{
			while(!mRange.Empty())
			{
				DestructObj(mRange.First());
				auto node = mRange.mFirstNode;
				mRange.PopFirst();
				freeNode(node);
				if(mRange.mFirstNode != null) mRange.mFirstNode->prev = null;
			}
			mRange.mLastNode = null;
			set_count(0);
		}
		reset_bucket_heads();
	}

	//! Сортирует все элементы контейнера. После сортировки итерация по контейнеру выполняется в порядке, задаваемым pred, пока не будут добавлены новые элементы.
	//! \param pred Предикат сравнения пар типа KeyValuePair<const K, V>
	template<typename P> void SortByPair(const P& pred)
	{
		size_t numKeys = Count();
		if(numKeys <= 1) return;

		FixedArray<Node*> ptrs(numKeys);
		auto ptr = mRange.mFirstNode;

		for(Node*& p: ptrs)
		{
			p = ptr;
			ptr = ptr->next;
		}

		QuickSort(ptrs, [&pred](Node* lhs, Node* rhs)
		{
			return pred(lhs->element, rhs->element);
		});

		mRange.mFirstNode = ptrs[0];
		ptrs[0]->prev = null;
		for(uint i=1; i<numKeys; i++)
		{
			ptrs[i-1]->next = ptrs[i];
			ptrs[i]->prev = ptrs[i-1];
		}
		mRange.mLastNode = ptrs.Last();
		mRange.mLastNode->next = null;
	}

	//! Сортирует все элементы контейнера. После сортировки итерация по контейнеру выполняется в порядке, задаваемым pred, пока не будут добавлены новые элементы.
	//! \param pred Предикат сравнения ключей.
	template<typename P = TFLess> void SortByKey(P pred = FLess)
	{
		SortByPair([&pred](const value_type& lhs, const value_type& rhs)
		{
			return pred(get<0>(lhs), get<0>(rhs));
		});
	}

	//! Сортирует все элементы контейнера. После сортировки итерация по контейнеру выполняется в порядке, задаваемым pred, пока не будут добавлены новые элементы.
	//! \param pred Предикат сравнения ключей.
	template<typename P = TFLess> void SortByValue(P pred = FLess)
	{
		SortByPair([&pred](const value_type& lhs, const value_type& rhs)
		{
			return pred(get<1>(lhs), get<1>(rhs));
		});
	}

	//! Rehash to a specific bucket count, which must be a power of two. Return true if successful.
	bool Rehash(size_t numBuckets)
	{
		if(numBuckets == BucketCount()) return true;
		if(numBuckets == 0 || numBuckets < Count()) return false;

		INTRA_DEBUG_ASSERT(IsPow2(numBuckets));

		allocate_buckets(Count(), numBuckets);
		rehash();
		return true;
	}

	//! Поиск по ключу.
	//! \return Диапазон, содержащий все элементы контейнера начиная с элемента с ключом key.
	ElementRange Find(const K& key)
	{
		if(mBucketHeads == null) return {};
		uint keyHash = ToHash(key);
		Node* node = findNode(key, keyHash);
		if(node == null) return {};
		return ElementRange(node, mRange.mLastNode);
	}

	ElementConstRange Find(const K& key) const
	{
		return ElementConstRange(const_cast<HashMap*>(this)->Find(key));
	}

	V& Get(const K& key, V& defaultValue, bool& oExists)
	{
		ElementRange found = Find(key);
		oExists = !found.Empty();
		if(found.Empty()) return defaultValue;
		return get<1>(found.First());
	}

	V& Get(const K& key, V& defaultValue)
	{
		bool exists;
		return Get(key, defaultValue, exists);
	}

	const V& Get(const K& key, const V& defaultValue, bool& oExists) const
	{
		ElementConstRange found = Find(key);
		oExists = !found.Empty();
		if(found.Empty()) return defaultValue;
		return get<1>(found.First());
	}

	const V& Get(const K& key, const V& defaultValue) const
	{
		bool exists;
		return Get(key, defaultValue, exists);
	}

	bool Contains(const K& key) const
	{
		if(mBucketHeads == null) return false;
		uint keyHash = ToHash(key);
		return findNode(key, keyHash) != null;
	}

	/*Array<K> Keys() const
	{
		Array<K> result;
		result.Reserve(Count());
		for(const_iterator it=begin(); it!=end(); ++it)
			result.AddLast(get<0>(*it));
		return result;
	}

	Array<V> Values() const
	{
		Array<V> result;
		result.Reserve(Count());
		for(const_iterator it = begin(); it != end(); ++it)
			result.AddLast(get<1>(*it));
		return result;
	}*/

	const ElementRange& AsRange() {return mRange;}
	const ElementConstRange& AsRange() const {return mRange.AsConstRange();}
	const ElementConstRange& AsConstRange() const {return mRange.AsConstRange();}

	const ElementRange& operator()() {return mRange;}

	iterator begin() {return iterator(mRange);}
	const_iterator begin() const {return const_iterator(mRange);}
	iterator end() {return iterator(null);}
	const_iterator end() const {return const_iterator(null);}

	iterator rbegin() {return iterator(mRange.mLastNode);}
	const_iterator rbegin() const {return const_iterator(mRange.mLastNode);}
	iterator rend() {return iterator(null);}
	const_iterator rend() const {return const_iterator(null);}

#ifdef INTRA_CONTAINER_STL_FORWARD_COMPATIBILITY
	forceinline iterator emplace(K&& key, V&& value) {return iterator(Insert(Move(key), Move(value)));}
	forceinline iterator insert(const value_type& pair) {return iterator(Insert(get<0>(pair), get<1>(pair)));}
	forceinline bool empty() const {return Empty();}
	forceinline size_t size() const {return Count();}
	forceinline void clear() {Clear();}
	forceinline iterator find(const K& key) {return iterator(Find(key));}
	forceinline const_iterator find(const K& key) const {return const_iterator(Find(key));}
#endif

    size_t Count() const {return mBucketHeads!=null? (reinterpret_cast<size_t*>(mBucketHeads))[0]: 0;}
    size_t BucketCount() const {return mBucketHeads!=null? (reinterpret_cast<size_t*>(mBucketHeads))[1]: 0;}

	HashMapStatistics GetStats(Span<size_t> oBucketLoads)
	{
		FillZeros(oBucketLoads);
		HashMapStatistics result;
		result.NumBuckets = BucketCount();
		result.FreeBucketCount = 0;
		result.MaxBucketLoad = 0;
		size_t loadSum = 0, num = 0;
		for(size_t i=0; i<BucketCount(); i++)
		{
			Node* node = get_bucket_head(i);
			if(node==null)
			{
				++result.FreeBucketCount;
				continue;
			}
			num++;
			size_t bucketLoad=0;
			do
			{
				bucketLoad++;
				node = node->down;
			}
			while(node != null);
			loadSum += bucketLoad;
			if(bucketLoad<oBucketLoads.Length()) oBucketLoads[bucketLoad]++;
			if(result.MaxBucketLoad<bucketLoad) result.MaxBucketLoad = bucketLoad;
		}
		result.AverageBucketLoad = double(loadSum)/double(num);
		return result;
	}

private:
	void allocate_buckets(size_t elementCount, size_t numBuckets)
	{
		delete[] mBucketHeads;

		Node** ptrs = new Node*[numBuckets+2];
		size_t* data = reinterpret_cast<size_t*>(ptrs);
		data[0] = elementCount;
		data[1] = numBuckets;
		mBucketHeads = ptrs;

		reset_bucket_heads();
	}

	void reset_bucket_heads()
	{
		if(mBucketHeads == null) return;
		size_t numBuckets = BucketCount();
		Node** ptrs = get_bucket_heads();
		for(uint i = 0; i < numBuckets; i++) ptrs[i] = null;
	}

    void set_count(size_t newCount)
	{if(mBucketHeads!=null) reinterpret_cast<size_t*>(mBucketHeads)[0] = newCount;}

	forceinline Node** get_bucket_heads() const
	{return mBucketHeads!=null? mBucketHeads+2: null;}

	forceinline Node*& get_bucket_head(size_t index) const
	{return get_bucket_heads()[index & (BucketCount()-1)];}

	ElementRange mRange;
	Node** mBucketHeads = null;

	Node* findNode(const K& key, uint keyHash) const
	{
		Node* node = get_bucket_head(keyHash);
		while(node != null)
		{
			if(get<0>(node->element) == key) return node;
			node = node->down;
		}
		return null;
	}

	Node* findNode(const K& key, uint keyHash, Node*& previous) const
	{
		previous = null;

		Node* node = get_bucket_head(keyHash);
		while(node != null)
		{
			if(get<0>(node->element) == key) return node;
			previous = node;
			node = node->down;
		}
		return null;
	}

	Node* insert_node_no_construct_or_assign(const K& key, bool* oExisting = null)
	{
		if(mBucketHeads == null)
		{
			allocate_buckets(Count(), 8);
			rehash();
		}

		uint keyHash = ToHash(key);

		if(oExisting != null)
		{
			Node* existing = findNode(key, keyHash);
			*oExisting = existing != null;
			if(existing != null) return existing;
		}

		Node* newNode = insertNodeAfter(mRange.mLastNode);
		auto& bh = get_bucket_head(keyHash);
		newNode->down = bh;
		bh = newNode;

		return newNode;
	}

	Node* insertNode(K&& key, V&& value, bool findExisting=true)
	{
		bool existed = false;
		Node* newNode = insert_node_no_construct_or_assign(key, findExisting? &existed: null);
		if(!existed) new(Construct, &newNode->element) value_type(Move(key), Move(value));
		else get<1>(newNode->element) = Move(value);
		if(Count() > BucketCount())
		{
			allocate_buckets(Count(), BucketCount()*2);
			rehash();
		}
		return newNode;
	}

	Node* insertNode(const K& key, const V& value, bool findExisting=true)
	{
		bool existed = false;
		Node* node = insert_node_no_construct_or_assign(key, findExisting? &existed: null);
		if(!existed) new(Construct, &node->element) value_type(key, value);
		else get<1>(node->element) = value;
		if(Count() > BucketCount())
		{
			allocate_buckets(Count(), BucketCount()*2);
			rehash();
		}
		return node;
	}

	Node* insertNodeAfter(Node* dest)
	{
		Node* newNode = createNewNode();
		newNode->next = dest == null? null: dest->next;
		newNode->prev = dest;
		if(newNode->next != null) newNode->next->prev = newNode;
		if(dest != null) dest->next = newNode;

		if(dest == mRange.mLastNode) mRange.mLastNode = newNode;
		if(mRange.mFirstNode == null) mRange.mFirstNode = newNode;

		set_count(Count()+1);

		return newNode;
	}

	Node* eraseNode(Node* node)
	{
		if(node == null) return null;

		if(node->prev != null) node->prev->next = node->next;
		if(node->next != null) node->next->prev = node->prev;

		if(node == mRange.mFirstNode) mRange.mFirstNode = node->next;
		if(node == mRange.mLastNode) mRange.mLastNode = node->prev;
		Node* result = node->next;

		DestructObj(node->element);
		freeNode(node);
		set_count(Count()-1);

		return result;
	}

	Node* createNewNode()
	{
		size_t bytesToAllocate = sizeof(Node);
		return AllocatorRef::Allocate(bytesToAllocate, INTRA_SOURCE_INFO);
	}

	void freeNode(Node* node)
	{
		AllocatorRef::Free(node, sizeof(Node));
	}

	void rehash()
	{
		ElementRange rangeCopy = mRange;
		while(!rangeCopy.Empty())
		{
			Node* const node = rangeCopy.mFirstNode;
			uint keyHash = ToHash(get<0>(node->element));
			auto& bh = get_bucket_head(keyHash);
			node->down = bh;
			bh = node;
			rangeCopy.PopFirst();
		}
	}
};
INTRA_END
