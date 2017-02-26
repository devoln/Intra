#pragma once

#include "Memory/Allocator/AllocatorRef.h"
#include "Algo/Op.h"
#include "Algo/Hash/ToHash.h"
#include "Algo/Sort/Quick.h"
#include "Meta/Type.h"
#include "Meta/Tuple.h"
#include "Container/AllForwardDecls.h"
#include "Platform/CppWarnings.h"
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
	template<typename... Args> HashNode(Args&&... args): element(Meta::Forward<Args>(args)...) {}
	~HashNode() {}

	template<typename U=decltype(Meta::Val<T>().Key)> forceinline Meta::EnableIf<
		Meta::IsScalarType<U>::_,
	bool> compare_keys(const U& key, uint keyHash)
	{
		(void)keyHash;
		return element.Key==key;
	}

	template<typename U=decltype(Meta::Val<T>().Key)> forceinline Meta::EnableIf<
		!Meta::IsScalarType<U>::_,
	bool> compare_keys(const U& key, uint keyHash)
	{
		if(Meta::WrapperStruct<uint>::value==keyHash) return true;
		return element.Key==key;
	}
		
	template<typename U=decltype(Meta::Val<T>().Key)> forceinline Meta::EnableIf<
		Meta::IsScalarType<U>::_
	> init_key(uint keyHash) {(void)keyHash;}

	template<typename U=decltype(Meta::Val<T>().Key)> forceinline Meta::EnableIf<
		!Meta::IsScalarType<U>::_
	> init_key(uint keyHash) {Meta::WrapperStruct<uint>::value = keyHash;}


	HashNode* down;
	HashNode* prev;
	HashNode* next;
	T element;

private:
	HashNode& operator=(const HashNode&) = delete;
};

}


template<typename T> struct HashTableRange
{
	HashTableRange(null_t=null): mFirstNode(null), mLastNode(null) {}

	forceinline bool operator==(const HashTableRange& rhs) const
	{
		return (Empty() && rhs.Empty()) ||
			(mFirstNode==rhs.mFirstNode && mLastNode==rhs.mLastNode);
	}
	forceinline bool operator!=(const HashTableRange& rhs) const {return !operator==(rhs);}
	forceinline bool operator==(null_t) const {return Empty();}
	forceinline bool operator!=(null_t) const {return !Empty();}

	forceinline void PopFirst() {INTRA_DEBUG_ASSERT(!Empty()); mFirstNode = mFirstNode->next;}
	forceinline void PopLast() {INTRA_DEBUG_ASSERT(!Empty()); mLastNode = mLastNode->prev;}
	forceinline bool Empty() const {return mFirstNode==null || mFirstNode->prev==mLastNode;}
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
	typedef HashTableRange<value_type> Range;
	typedef HashTableRange<const value_type> ConstRange;


	//typedef ::Range::ForwardRangeIterator<Range> iterator;
	//typedef ::Range::ForwardRangeIterator<ConstRange> const_iterator;


	struct iterator
    {
        forceinline iterator(Node* nodePtr=null): node(nodePtr) {}
		forceinline iterator(const Range& r): node(r.mFirstNode) {}
 
		forceinline bool operator==(const iterator& rhs) const {return node==rhs.node;}
        forceinline bool operator!=(const iterator& rhs) const {return node!=rhs.node;}
        forceinline void GotoNext() {INTRA_DEBUG_ASSERT(node!=null); node = node->next;}
        forceinline void GotoPrev() {INTRA_DEBUG_ASSERT(node!=null); node = node->prev;}
 
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
		forceinline const_iterator(const Range& r): node(r.mFirstNode) {}
		forceinline const_iterator(const ConstRange& r): node(r.mFirstNode) {}
        forceinline const_iterator(const iterator& rhs): node(rhs.node) {}
 
        forceinline bool operator==(const const_iterator& rhs) const {return node==rhs.node;}
        forceinline bool operator!=(const const_iterator& rhs) const {return node!=rhs.node;}
        forceinline void GotoNext() {INTRA_DEBUG_ASSERT(node!=null); node = node->next;}
        forceinline void GotoPrev() {INTRA_DEBUG_ASSERT(node!=null); node = node->prev;}
 
        forceinline const_iterator& operator++() {GotoNext(); return *this;}
        forceinline const_iterator operator++(int) {const_iterator it = *this; GotoNext(); return it;}
        forceinline const_iterator& operator--() {GotoPrev(); return *this;}
        forceinline const_iterator operator--(int) {const_iterator it = *this; GotoPrev(); return it;}
 
        forceinline const value_type* operator->() const {return &node->element;}
        forceinline const value_type& operator*() const {return node->element;}
         
        Node* node;
    };
 


	HashMap(): range(null), bucket_heads(null) {}

	HashMap(const HashMap& rhs): AllocatorRef(rhs),
		range(null), bucket_heads(null) {operator=(rhs);}

	~HashMap() {Clear();}

	HashMap& operator=(const HashMap& rhs)
	{
		Clear();
		Insert(rhs);
		return *this;
	}

	bool operator==(const HashMap& rhs) const
	{
		if(rhs.Count()!=Count()) return false;

		Range rangeCopy = range;
		while(!rangeCopy.Empty())
		{
			Range found = rhs.Find(rangeCopy.First().Key);
			if(found.Empty() || found.First().Value!=rangeCopy.First().Value) return false;
			rangeCopy.PopFirst();
		}
		return true;
	}

	bool operator!=(const HashMap& rhs) const {return !operator==(rhs);}

	bool Empty() const {return Count()==0;}
	bool operator==(null_t) const {return Empty();}
	bool operator!=(null_t) const {return !Empty();}

	V& operator[](const K& key)
	{
		if(bucket_heads==null)
			return insert_node(key, V(), false)->element.Value;
		uint keyHash = Algo::ToHash(key);
		Node* node = find_node(key, keyHash);
		if(node!=null) return node->element.Value;
		return insert_node(key, V(), false)->element.Value;
	}

	Range Insert(const value_type& pair)
	{return Range(insert_node(pair.Key, pair.Value), range.mLastNode);}

	Range Insert(K&& key, V&& value)
	{return Range(insert_node(Meta::Move(key), Meta::Move(value)), range.mLastNode);}

	Range Insert(const K& key, const V& value)
	{return Range(insert_node(key, value), range.mLastNode);}

	void Insert(const HashMap& map)
	{
		Range rangeCopy = map.range;
		while(!rangeCopy.Empty())
		{
			insert_node(rangeCopy.First().Key, rangeCopy.First().Value);
			rangeCopy.PopFirst();
		}
	}

	//! Вставить пару только в случае если пары с указанным ключом не существует.
	//! Диапазон, содержащий найденный или вставленный элемент с указанным ключом и все элементы, идущие после него в контейнере.
	Range InsertNew(const K& key, const V& value)
	{
		uint keyHash = Algo::ToHash(key);
		if(bucket_heads!=null)
		{
			auto node = find_node(key, keyHash);
			if(node!=null) return Range(node, range.mLastNode);
		}
		return Range(insert_node(key, value, false), range.mLastNode);
	}

	iterator InsertNew(K&& key, V&& value)
	{
		uint keyHash = Algo::ToHash(key);
		if(bucket_heads!=null)
		{
			auto node = find_node(key, keyHash);
			if(node!=null) return Range(node, range.mLastNode);
		}
		return Range(insert_node(Meta::Move(key), Meta::Move(value), false), range.mLastNode);
	}

	//! Удалить элемент по ключу.
	//! \return Возвращает, существовал ли элемент с таким ключом.
	bool Remove(const K& key)
	{
		if(bucket_heads==null) return false;
		uint keyHash = Algo::ToHash(key);
		Node* previous;
		auto node = find_node(key, keyHash, previous);
		if(node==null) return false;

		if(previous!=null) previous->down = node->down;
		else get_bucket_head(keyHash) = node->down;

		erase_node(node);
		return true;
	}

	//! Удалить элемент из контейнера, на который указывает итератор it.
	//! \return Возвращает диапазон, содержащий все элементы, идущие после удаляемого элемента.
	Range Remove(const_iterator it)
	{
		if(bucket_heads==null || it==null) return end();

		auto node = it.Range.mFirstNode;
		auto next = node->next;

		uint keyHash = Algo::ToHash(node->element.Key);

		Node* previous = null;
		auto& bh = get_bucket_head(keyHash);
		auto current = bh;
		while(current!=null && current!=node)
		{
			previous = current;
			current = current->down;
		}

		INTRA_DEBUG_ASSERT(current==node);

		if(previous!=null) previous->down = node->down;
		else bh = node->down;

		erase_node(node);
		return iterator(next);
	}

	void Clear()
	{
		if(!Empty())
		{
			while(!range.Empty())
			{
				Memory::DestructObj(range.First());
				auto node = range.mFirstNode;
				range.PopFirst();
				free_node(node);
				if(range.mFirstNode!=null) range.mFirstNode->prev = null;
			}
			set_count(0);
		}
		reset_bucket_heads();
	}

	//! Сортирует все элементы контейнера. После сортировки итерация по контейнеру выполняется в порядке, задаваемым pred, пока не будут добавлены новые элементы.
	//! \param pred Предикат сравнения пар типа KeyValuePair<const K, V>
	template<typename P> void SortByPair(P pred)
	{
		size_t numKeys = Count();
		if(numKeys<=1) return;

		Array<Node*> ptrs(numKeys);
		auto ptr = range.mFirstNode;

		for(size_t i=0; i<numKeys; i++)
		{
			ptrs.AddLast(ptr);
			ptr = ptr->next;
		}

		Algo::QuickSort(ptrs, [&pred](Node* lhs, Node* rhs)
		{return pred(lhs->element, rhs->element);});

		range.mFirstNode = ptrs[0];
		ptrs[0]->prev = null;
		for(uint i=1; i<numKeys; i++)
		{
			ptrs[i-1]->next = ptrs[i];
			ptrs[i]->prev = ptrs[i-1];
		}
		range.mLastNode = ptrs.Last();
		range.mLastNode->next = null;
	}

	//! Сортирует все элементы контейнера. После сортировки итерация по контейнеру выполняется в порядке, задаваемым pred, пока не будут добавлены новые элементы.
	//! \param pred Предикат сравнения ключей.
	template<typename P = Comparers::Function<K>> void SortByKey(P pred = Op::Less<K>)
	{
		SortByPair([&pred](const KeyValuePair<const K, V>& lhs, const KeyValuePair<const K, V>& rhs)
		{
			return pred(lhs.Key, rhs.Key);
		});
	}

	//! Сортирует все элементы контейнера. После сортировки итерация по контейнеру выполняется в порядке, задаваемым pred, пока не будут добавлены новые элементы.
	//! \param pred Предикат сравнения ключей.
	template<typename P = Comparers::Function<V>> void SortByValue(P pred = Op::Less<V>)
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
		if(numBuckets==0 || numBuckets < Count()) return false;

		INTRA_DEBUG_ASSERT(Math::IsPow2(numBuckets));

		allocate_buckets(Count(), numBuckets);
		rehash();
		return true;
	}

	//! Поиск по ключу.
	//! \return Диапазон, содержащий все элементы контейнера начиная с элемента с ключом key.
	Range Find(const K& key)
	{
		if(bucket_heads==null) return null;
		uint keyHash = Algo::ToHash(key);
		Node* node = find_node(key, keyHash);
		if(node==null) return null;
		return Range(node, range.mLastNode);
	}

	ConstRange Find(const K& key) const
	{
		return ConstRange(const_cast<HashMap*>(this)->Find(key));
	}

	V& Get(const K& key, bool* oExists=null)
	{
		Range found = Find(key);
		if(oExists!=null) *oExists = !found.Empty();
		if(found.Empty())
		{
			static V defaultValue;
			return defaultValue;
		}
		return found.First().Value;
	}

	const V& Get(const K& key, bool* oExists=null) const
	{
		return const_cast<HashMap*>(this)->Get(key, oExists);
	}

	bool Contains(const K& key) const
	{
		if(bucket_heads==null) return false;
		uint keyHash = Algo::ToHash(key);
		return find_node(key, keyHash)!=null;
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

	const Range& AsRange() {return range;}
	const ConstRange& AsRange() const {return range.AsConstRange();}
	const ConstRange& AsConstRange() const {return range.AsConstRange();}

	const Range& operator()() {return range;}

	iterator begin() {return iterator(range);}
	const_iterator begin() const {return const_iterator(range);}
	iterator end() {return iterator(null);}
	const_iterator end() const {return const_iterator(null);}

	iterator rbegin() {return iterator(range.mLastNode);}
	const_iterator rbegin() const {return const_iterator(range.mLastNode);}
	iterator rend() {return iterator(null);}
	const_iterator rend() const {return const_iterator(null);}

#ifdef INTRA_STL_INTERFACE
	forceinline iterator emplace(K&& key, V&& value) {return iterator(Insert(Meta::Move(key), Meta::Move(value)));}
	forceinline iterator insert(const value_type& pair) {return iterator(Insert(pair.Key, pair.Value));}
	forceinline bool empty() const {return Empty();}
	forceinline size_t size() const {return Count();}
	forceinline void clear() {Clear();}
	forceinline iterator find(const K& key) {return iterator(Find(key));}
	forceinline const_iterator find(const K& key) const {return const_iterator(Find(key));}
#endif

    size_t Count() const {return bucket_heads!=null? (reinterpret_cast<size_t*>(bucket_heads))[0]: 0;}
    size_t BucketCount() const {return bucket_heads!=null? (reinterpret_cast<size_t*>(bucket_heads))[1]: 0;}

	void GetStats(size_t* oNumBuckets, size_t* oFreeBucketCount,
		double* oAverageBucketLoad, size_t* oMaxBucketLoad, size_t oBucketLoads[], size_t bucketLoadsCount)
	{
		if(oNumBuckets) *oNumBuckets = BucketCount();
		if(oFreeBucketCount) *oFreeBucketCount=0;
		if(oMaxBucketLoad) *oMaxBucketLoad=0;
		size_t loadSum=0, num=0;
		for(size_t i=0; i<BucketCount(); i++)
		{
			Node* node = get_bucket_head(i);
			if(node==null)
			{
				if(oFreeBucketCount) ++*oFreeBucketCount;
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
			if(bucketLoad<bucketLoadsCount) oBucketLoads[bucketLoad]++;
			if(oMaxBucketLoad && *oMaxBucketLoad<bucketLoad) *oMaxBucketLoad = bucketLoad;
		}
		if(oAverageBucketLoad) *oAverageBucketLoad = double(loadSum)/double(num);
	}

private:
	void allocate_buckets(size_t elementCount, size_t numBuckets)
	{
		delete[] bucket_heads;

		Node** ptrs = new Node*[numBuckets+2];
		size_t* data = reinterpret_cast<size_t*>(ptrs);
		data[0] = elementCount;
		data[1] = numBuckets;
		bucket_heads = ptrs;

		reset_bucket_heads();
	}

	void reset_bucket_heads()
	{
		if(bucket_heads==null) return;
		size_t numBuckets = BucketCount();
		Node** ptrs = get_bucket_heads();
		for(uint i=0; i<numBuckets; i++) ptrs[i]=null;
	}

    void set_count(size_t newCount)
	{if(bucket_heads!=null) reinterpret_cast<size_t*>(bucket_heads)[0] = newCount;}

	forceinline Node** get_bucket_heads() const
	{return bucket_heads!=null? bucket_heads+2: null;}

	forceinline Node*& get_bucket_head(size_t index) const
	{return get_bucket_heads()[index & (BucketCount()-1)];}

	Range range;
	Node** bucket_heads;

	Node* find_node(const K& key, uint keyHash) const
	{
		Node* node = get_bucket_head(keyHash);
		while(node!=null)
		{
			if(node->compare_keys(key, keyHash)) return node;
			node = node->down;
		}
		return null;
	}

	Node* find_node(const K& key, uint keyHash, Node*& previous) const
	{
		previous = null;

		Node* node = get_bucket_head(keyHash);
		while(node!=null)
		{
			if(node->compare_keys(key, keyHash)) return node;
			previous = node;
			node = node->Down();
		}
		return null;
	}

	Node* insert_node_no_construct_or_assign(const K& key, bool* oExisting=null)
	{
		if(bucket_heads==null)
		{
			allocate_buckets(Count(), 8);
			rehash();
		}

		uint keyHash = Algo::ToHash(key);

		if(oExisting!=null)
		{
			Node* existing = find_node(key, keyHash);
			*oExisting = (existing!=null);
			if(existing!=null) return existing;
		}

		Node* newNode = insert_node_after(range.mLastNode);
		newNode->init_key(keyHash);
		auto& bh = get_bucket_head(keyHash);
		newNode->down = bh;
		bh = newNode;

		return newNode;
	}

	Node* insert_node(K&& key, V&& value, bool findExisting=true)
	{
		bool existed = false;
		Node* newNode = insert_node_no_construct_or_assign(key, findExisting? &existed: null);
		if(!existed) new(&newNode->element) value_type(Meta::Move(key), Meta::Move(value));
		else newNode->element.Value = Meta::Move(value);
		if(Count()>BucketCount())
		{
			allocate_buckets(Count(), BucketCount()*2);
			rehash();
		}
		return newNode;
	}

	Node* insert_node(const K& key, const V& value, bool findExisting=true)
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

	Node* insert_node_after(Node* dest)
	{
		Node* newNode = new_node();
		newNode->next = dest==null? null: dest->next;
		newNode->prev = dest;
		if(newNode->next!=null) newNode->next->prev = newNode;
		if(dest!=null) dest->next = newNode;

		if(dest == range.mLastNode) range.mLastNode = newNode;
		if(range.mFirstNode==null) range.mFirstNode = newNode;

		set_count(Count()+1);

		return newNode;
	}

	Node* erase_node(Node* node)
	{
		if(node==null) return null;

		if(node->prev!=null) node->prev->next = node->next;
		if(node->next!=null) node->next->prev = node->prev;

		if(node == range.mFirstNode) range.mFirstNode = node->next;
		if(node == range.mLastNode) range.mLastNode = node->prev;
		Node* result = node->next;

		Memory::DestructObj(node->element);
		free_node(node, sizeof(Node));
		set_count(Count()-1);

		return result;
	}

	Node* new_node()
	{
		size_t bytesToAllocate = sizeof(Node);
		return AllocatorRef::Allocate(bytesToAllocate, INTRA_SOURCE_INFO);
	}

	void free_node(Node* node)
	{
		AllocatorRef::Free(node, sizeof(Node));
	}

	void rehash()
	{
		Range rangeCopy = range;
		while(!rangeCopy.Empty())
		{
			Node* node = rangeCopy.mFirstNode;
			uint keyHash = Algo::ToHash(node->element.Key);
			auto& bh = get_bucket_head(keyHash);
			node->down = bh;
			bh = node;
			rangeCopy.PopFirst();
		}
	}
};

}}

INTRA_WARNING_POP

