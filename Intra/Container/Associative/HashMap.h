#pragma once

#include "Cpp/Warnings.h"

#include "Meta/Type.h"
#include "Meta/Tuple.h"

#include "Funal/Op.h"
#include "Utils/FixedArray.h"

#include "Hash/ToHash.h"

#include "Range/Sort/Quick.h"
#include "Range/Mutation/Fill.h"

#include "Container/AllForwardDecls.h"

#include "Memory/Allocator/AllocatorRef.h"
#include "Memory/Memory.h"


INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Container {

template<typename T> struct HashTableRange;

namespace D {

template<typename K> struct KeyWrapper {K Key;};

template<typename T> struct HashNode:
	private Meta::SelectType<
		Meta::EmptyType, Meta::WrapperStruct<uint>,
		Meta::IsScalarType<decltype(Meta::Val<T>().Key)>::_>
{
	template<typename T1> friend struct Intra::Container::HashTableRange;
	template<typename K, typename V, class Allocator> friend class Intra::Container::HashMap;
private:
	template<typename... Args> HashNode(Args&&... args): element(Cpp::Forward<Args>(args)...) {}
	~HashNode() {}

	template<typename U=decltype(Meta::Val<T>().Key)> forceinline Meta::EnableIf<
		Meta::IsScalarType<U>::_,
	bool> compareKeys(const U& key, uint keyHash)
	{
		(void)keyHash;
		return element.Key == key;
	}

	template<typename U=decltype(Meta::Val<T>().Key)> forceinline Meta::EnableIf<
		!Meta::IsScalarType<U>::_,
	bool> compareKeys(const U& key, uint keyHash)
	{
		if(Meta::WrapperStruct<uint>::value == keyHash) return true;
		return element.Key == key;
	}
		
	template<typename U=decltype(Meta::Val<T>().Key)> forceinline Meta::EnableIf<
		Meta::IsScalarType<U>::_
	> initKey(uint keyHash) {(void)keyHash;}

	template<typename U=decltype(Meta::Val<T>().Key)> forceinline Meta::EnableIf<
		!Meta::IsScalarType<U>::_
	> initKey(uint keyHash) {Meta::WrapperStruct<uint>::value = keyHash;}


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
	HashTableRange(null_t=null): mFirstNode(null), mLastNode(null) {}

	forceinline bool operator==(const HashTableRange& rhs) const
	{
		return (Empty() && rhs.Empty()) ||
			(mFirstNode == rhs.mFirstNode && mLastNode == rhs.mLastNode);
	}
	forceinline bool operator!=(const HashTableRange& rhs) const {return !operator==(rhs);}
	forceinline bool operator==(null_t) const {return Empty();}
	forceinline bool operator!=(null_t) const {return !Empty();}

	forceinline void PopFirst() {INTRA_DEBUG_ASSERT(!Empty()); mFirstNode = mFirstNode->next;}
	forceinline void PopLast() {INTRA_DEBUG_ASSERT(!Empty()); mLastNode = mLastNode->prev;}
	forceinline bool Empty() const {return mFirstNode == null || mFirstNode->prev == mLastNode;}
	forceinline T& First() const {INTRA_DEBUG_ASSERT(!Empty()); return mFirstNode->element;}
	forceinline T& Last() const {INTRA_DEBUG_ASSERT(!Empty()); return mLastNode->element;}


	const HashTableRange<const T>& AsConstRange() const
	{return *reinterpret_cast<const HashTableRange<const T>*>(this);}
	
	operator HashTableRange<const T>() const {return AsConstRange();}

private:
	forceinline HashTableRange(Container::D::HashNode<T>* startNode, Container::D::HashNode<T>* lastNode):
		mFirstNode(startNode), mLastNode(lastNode) {}

	Container::D::HashNode<T>* mFirstNode;
	Container::D::HashNode<T>* mLastNode;

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
class HashMap: Memory::AllocatorRef<AllocatorType>
{
	typedef Memory::AllocatorRef<AllocatorType> AllocatorRef;
	typedef Container::D::HashNode<Meta::KeyValuePair<const K, V>> Node;
public:
	typedef AllocatorType Allocator;

	typedef K key_type;
	typedef V mapped_type;
	typedef KeyValuePair<const K, V> value_type;
	typedef HashTableRange<value_type> ElementRange;
	typedef HashTableRange<const value_type> ElementConstRange;


	struct iterator
    {
        forceinline iterator(Node* nodePtr=null): node(nodePtr) {}
		forceinline iterator(const ElementRange& r): node(r.mFirstNode) {}
 
		forceinline bool operator==(const iterator& rhs) const {return node==rhs.node;}
        forceinline bool operator!=(const iterator& rhs) const {return node!=rhs.node;}
        forceinline void GotoNext() {
			INTRA_DEBUG_ASSERT(node != null);
			node = node->next;
		}
        forceinline void GotoPrev() {INTRA_DEBUG_ASSERT(node != null); node = node->prev;}
 
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
        forceinline void GotoNext() {INTRA_DEBUG_ASSERT(node != null); node = node->next;}
        forceinline void GotoPrev() {INTRA_DEBUG_ASSERT(node != null); node = node->prev;}
 
        forceinline const_iterator& operator++() {GotoNext(); return *this;}
        forceinline const_iterator operator++(int) {const_iterator it = *this; GotoNext(); return it;}
        forceinline const_iterator& operator--() {GotoPrev(); return *this;}
        forceinline const_iterator operator--(int) {const_iterator it = *this; GotoPrev(); return it;}
 
        forceinline const value_type* operator->() const {return &node->element;}
        forceinline const value_type& operator*() const {return node->element;}
         
        Node* node;
    };
 


	HashMap(null_t=null): mRange(null), mBucketHeads(null) {}

	HashMap(const HashMap& rhs): AllocatorRef(rhs),
		mRange(null), mBucketHeads(null) {operator=(rhs);}

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
		if(rhs.Count()!=Count()) return false;

		ElementRange rangeCopy = mRange;
		while(!rangeCopy.Empty())
		{
			ElementRange found = rhs.Find(rangeCopy.First().Key);
			if(found.Empty() || found.First().Value!=rangeCopy.First().Value) return false;
			rangeCopy.PopFirst();
		}
		return true;
	}

	bool operator!=(const HashMap& rhs) const {return !operator==(rhs);}

	bool Empty() const {return Count() == 0;}
	bool operator==(null_t) const {return Empty();}
	bool operator!=(null_t) const {return !Empty();}

	V& operator[](const K& key)
	{
		if(mBucketHeads == null)
			return insertNode(key, V(), false)->element.Value;
		uint keyHash = ToHash(key);
		Node* node = findNode(key, keyHash);
		if(node!=null) return node->element.Value;
		return insertNode(key, V(), false)->element.Value;
	}

	V& operator[](K&& key)
	{
		if(mBucketHeads == null)
			return insertNode(Cpp::Move(key), V(), false)->element.Value;
		uint keyHash = ToHash(key);
		Node* node = findNode(key, keyHash);
		if(node != null) return node->element.Value;
		return insertNode(Cpp::Move(key), V(), false)->element.Value;
	}

	ElementRange Insert(const value_type& pair)
	{return ElementRange(insertNode(pair.Key, pair.Value), mRange.mLastNode);}

	ElementRange Insert(K&& key, V&& value)
	{return ElementRange(insertNode(Cpp::Move(key), Cpp::Move(value)), mRange.mLastNode);}

	ElementRange Insert(const K& key, const V& value)
	{return ElementRange(insertNode(key, value), mRange.mLastNode);}

	void Insert(const HashMap& map)
	{
		ElementRange rangeCopy = map.mRange;
		while(!rangeCopy.Empty())
		{
			insertNode(rangeCopy.First().Key, rangeCopy.First().Value);
			rangeCopy.PopFirst();
		}
	}

	//! Вставить пару только в случае если пары с указанным ключом не существует.
	//! Диапазон, содержащий найденный или вставленный элемент с указанным ключом и все элементы, идущие после него в контейнере.
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

	iterator InsertNew(K&& key, V&& value)
	{
		const uint keyHash = ToHash(key);
		if(mBucketHeads != null)
		{
			auto node = findNode(key, keyHash);
			if(node!=null) return ElementRange(node, mRange.mLastNode);
		}
		return ElementRange(insertNode(Cpp::Move(key), Cpp::Move(value), false), mRange.mLastNode);
	}

	//! Удалить элемент по ключу.
	//! @return Возвращает, существовал ли элемент с таким ключом.
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

		uint keyHash = ToHash(node->element.Key);

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
				Memory::DestructObj(mRange.First());
				auto node = mRange.mFirstNode;
				mRange.PopFirst();
				freeNode(node);
				if(mRange.mFirstNode!=null) mRange.mFirstNode->prev = null;
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

		Range::QuickSort(ptrs, [&pred](Node* lhs, Node* rhs)
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
	template<typename P = Funal::TLess> void SortByKey(P pred = Funal::Less)
	{
		SortByPair([&pred](const KeyValuePair<const K, V>& lhs, const KeyValuePair<const K, V>& rhs)
		{
			return pred(lhs.Key, rhs.Key);
		});
	}

	//! Сортирует все элементы контейнера. После сортировки итерация по контейнеру выполняется в порядке, задаваемым pred, пока не будут добавлены новые элементы.
	//! \param pred Предикат сравнения ключей.
	template<typename P = Funal::TLess> void SortByValue(P pred = Funal::Less)
	{
		SortByPair([&pred](const KeyValuePair<const K, V>& lhs, const KeyValuePair<const K, V>& rhs)
		{
			return pred(lhs.Value, rhs.Value);
		});
	}

	//! Rehash to a specific bucket count, which must be a power of two. Return true if successful.
	bool Rehash(size_t numBuckets)
	{
		if(numBuckets == BucketCount()) return true;
		if(numBuckets == 0 || numBuckets < Count()) return false;

		INTRA_DEBUG_ASSERT(Math::IsPow2(numBuckets));

		allocate_buckets(Count(), numBuckets);
		rehash();
		return true;
	}

	//! Поиск по ключу.
	//! \return Диапазон, содержащий все элементы контейнера начиная с элемента с ключом key.
	ElementRange Find(const K& key)
	{
		if(mBucketHeads==null) return null;
		uint keyHash = ToHash(key);
		Node* node = findNode(key, keyHash);
		if(node==null) return null;
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
		return found.First().Value;
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
		return found.First().Value;
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
		return findNode(key, keyHash)!=null;
	}

	/*Array<K> Keys() const
	{
		Array<K> result;
		result.Reserve(Count());
		for(const_iterator it=begin(); it!=end(); ++it)
			result.AddLast(it->Key);
		return result;
	}

	Array<V> Values() const
	{
		Array<V> result;
		result.Reserve(Count());
		for(const_iterator it=begin(); it!=end(); ++it)
			result.AddLast(it->Value);
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

	forceinline iterator emplace(K&& key, V&& value) {return iterator(Insert(Cpp::Move(key), Cpp::Move(value)));}
	forceinline iterator insert(const value_type& pair) {return iterator(Insert(pair.Key, pair.Value));}
	forceinline bool empty() const {return Empty();}
	forceinline size_t size() const {return Count();}
	forceinline void clear() {Clear();}
	forceinline iterator find(const K& key) {return iterator(Find(key));}
	forceinline const_iterator find(const K& key) const {return const_iterator(Find(key));}

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
			while(node!=null);
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
	Node** mBucketHeads;

	Node* findNode(const K& key, uint keyHash) const
	{
		Node* node = get_bucket_head(keyHash);
		while(node!=null)
		{
			if(node->compareKeys(key, keyHash)) return node;
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
			if(node->compareKeys(key, keyHash)) return node;
			previous = node;
			node = node->Down();
		}
		return null;
	}

	Node* insert_node_no_construct_or_assign(const K& key, bool* oExisting=null)
	{
		if(mBucketHeads==null)
		{
			allocate_buckets(Count(), 8);
			rehash();
		}

		uint keyHash = ToHash(key);

		if(oExisting!=null)
		{
			Node* existing = findNode(key, keyHash);
			*oExisting = (existing!=null);
			if(existing!=null) return existing;
		}

		Node* newNode = insertNodeAfter(mRange.mLastNode);
		newNode->initKey(keyHash);
		auto& bh = get_bucket_head(keyHash);
		newNode->down = bh;
		bh = newNode;

		return newNode;
	}

	Node* insertNode(K&& key, V&& value, bool findExisting=true)
	{
		bool existed = false;
		Node* newNode = insert_node_no_construct_or_assign(key, findExisting? &existed: null);
		if(!existed) new(&newNode->element) value_type(Cpp::Move(key), Cpp::Move(value));
		else newNode->element.Value = Cpp::Move(value);
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
		if(!existed) new(&node->element) value_type(key, value);
		else node->element.Value = value;
		if(Count()>BucketCount())
		{
			allocate_buckets(Count(), BucketCount()*2);
			rehash();
		}
		return node;
	}

	Node* insertNodeAfter(Node* dest)
	{
		Node* newNode = createNewNode();
		newNode->next = dest==null? null: dest->next;
		newNode->prev = dest;
		if(newNode->next!=null) newNode->next->prev = newNode;
		if(dest!=null) dest->next = newNode;

		if(dest == mRange.mLastNode) mRange.mLastNode = newNode;
		if(mRange.mFirstNode==null) mRange.mFirstNode = newNode;

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

		Memory::DestructObj(node->element);
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
			uint keyHash = ToHash(node->element.Key);
			auto& bh = get_bucket_head(keyHash);
			node->down = bh;
			bh = node;
			rangeCopy.PopFirst();
		}
	}
};

}
using Container::HashMap;

}

INTRA_WARNING_POP

