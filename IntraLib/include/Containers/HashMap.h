#pragma once

#include "Memory/AllocatorInterface.h"
#include "Memory/Allocator.h"
#include "Algorithms/Hash.h"
#include "Algorithms/Sort.h"
#include "Meta/Type.h"
#include "Meta/Tuple.h"
#include "Containers/ForwardDeclarations.h"

namespace Intra {

template<typename T> struct HashTableRange;

namespace detail {

template<typename K> struct KeyWrapper {K Key;};

template<typename T> struct HashNode:
	private Meta::SelectType<
		Meta::EmptyType, Meta::WrapperStruct<uint>,
		Meta::IsFundamentalType<decltype(Meta::Val<T>().Key)>::_>
{
	template<typename T> friend struct Intra::HashTableRange;
	template<typename K, typename V, class Allocator> friend class Intra::HashMap;
private:
	template<typename... Args> HashNode(Args&&... args): element(core::forward<Args>(args)...) {}
	~HashNode() {}

	template<typename U=decltype(Meta::Val<T>().Key)> forceinline Meta::EnableIf<
		Meta::IsFundamentalType<U>::_,
	bool> compare_keys(const U& key, uint keyHash)
	{
		(void)keyHash;
		return element.Key==key;
	}

	template<typename U=decltype(Meta::Val<T>().Key)> forceinline Meta::EnableIf<
		!Meta::IsFundamentalType<U>::_,
	bool> compare_keys(const U& key, uint keyHash)
	{
		if(Meta::WrapperStruct<uint>::value==keyHash) return true;
		return element.Key==key;
	}
		
	template<typename U=decltype(Meta::Val<T>().Key)> forceinline Meta::EnableIf<
		Meta::IsFundamentalType<U>::_
	> init_key(uint keyHash) {(void)keyHash;}

	template<typename U=decltype(Meta::Val<T>().Key)> forceinline Meta::EnableIf<
		!Meta::IsFundamentalType<U>::_
	> init_key(uint keyHash) {Meta::WrapperStruct<uint>::value = keyHash;}


	HashNode* down;
	HashNode* prev;
	HashNode* next;
	T element;

private:
	HashNode& operator=(const HashNode&) = delete;
};

}

template<typename T> struct HashTableRange: Range::RangeMixin<HashTableRange<T>, T, Range::TypeEnum::Bidirectional, true>
{
	typedef T& return_value_type;

	HashTableRange(null_t=null): first_node(null), last_node(null) {}

	forceinline bool operator==(const HashTableRange& rhs) const {return Empty() && rhs.Empty() || first_node==rhs.first_node && last_node==rhs.last_node;}
	forceinline bool operator!=(const HashTableRange& rhs) const {return !operator==(rhs);}
	forceinline bool operator==(null_t) const {return Empty();}
	forceinline bool operator!=(null_t) const {return !Empty();}

	forceinline void PopFirst() {INTRA_ASSERT(!Empty()); first_node = first_node->next;}
	forceinline void PopLast() {INTRA_ASSERT(!Empty()); last_node = last_node->prev;}
	forceinline bool Empty() const {return first_node==null || first_node->prev==last_node;}
	forceinline T& First() const {INTRA_ASSERT(!Empty()); return first_node->element;}
	forceinline T& Last() const {INTRA_ASSERT(!Empty()); return last_node->element;}

	const HashTableRange<const T>& AsConstRange() const {return *reinterpret_cast<const HashTableRange<const T>>(this);}
	operator HashTableRange<const T>() const {return AsConstRange();}

private:
	forceinline HashTableRange(detail::HashNode<T>* startNode, detail::HashNode<T>* lastNode):
		first_node(startNode), last_node(lastNode) {}

	detail::HashNode<T>* first_node;
	detail::HashNode<T>* last_node;

	template<typename K, typename V, class Allocator> friend class HashMap;
};


template<typename K, typename V, typename Allocator>
class HashMap: Memory::AllocatorRef<Allocator>
{
	typedef Memory::AllocatorRef<Allocator> AllocatorRef;
	typedef detail::HashNode<KeyValuePair<const K, V>> Node;
public:
	typedef Allocator Allocator;

	typedef K key_type;
	typedef V mapped_type;
	typedef KeyValuePair<const K, V> value_type;
	typedef HashTableRange<value_type> Range;
	typedef HashTableRange<const value_type> ConstRange;


	//typedef ::Range::ForwardRangeIterator<Range> iterator;
	//typedef ::Range::ForwardRangeIterator<ConstRange> const_iterator;


	struct iterator
    {
        forceinline iterator(Node* node=null): node(node) {}
		forceinline iterator(const Range& r): node(r.first_node) {}
 
		forceinline bool operator==(const iterator& rhs) const {return node==rhs.node;}
        forceinline bool operator!=(const iterator& rhs) const {return node!=rhs.node;}
        forceinline void GotoNext() {INTRA_ASSERT(node!=null); node = node->next;}
        forceinline void GotoPrev() {INTRA_ASSERT(node!=null); node = node->prev;}
 
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
        forceinline const_iterator(Node* node=null): node(node) {}
		forceinline const_iterator(const Range& r): node(r.first_node) {}
		forceinline const_iterator(const ConstRange& r): node(r.first_node) {}
        forceinline const_iterator(const iterator& rhs): node(rhs.node) {}
 
        forceinline bool operator==(const const_iterator& rhs) const {return node==rhs.node;}
        forceinline bool operator!=(const const_iterator& rhs) const {return node!=rhs.node;}
        forceinline void GotoNext() {INTRA_ASSERT(node!=null); node = node->next;}
        forceinline void GotoPrev() {INTRA_ASSERT(node!=null); node = node->prev;}
 
        forceinline const_iterator& operator++() {GotoNext(); return *this;}
        forceinline const_iterator operator++(int) {const_iterator it = *this; GotoNext(); return it;}
        forceinline const_iterator& operator--() {GotoPrev(); return *this;}
        forceinline const_iterator operator--(int) {const_iterator it = *this; GotoPrev(); return it;}
 
        forceinline const value_type* operator->() const {return &node->element;}
        forceinline const value_type& operator*() const {return node->element;}
         
        Node* node;
    };
 


	HashMap(): bucket_heads(null), range(null) {}

	HashMap(const HashMap& rhs): bucket_heads(null), range(null) {operator=(rhs);}

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
		{
			K keyCopy = key;
			return insert_node(core::move(keyCopy), V(), false)->element.Value;
		}
		uint keyHash = ToHash(key);
		Node* node = find_node(key, keyHash);
		if(node!=null) return node->element.Value;
		K keyCopy = key;
		return insert_node(core::move(keyCopy), V(), false)->element.Value;
	}

	Range Insert(const value_type& pair)
	{
		return Range(insert_node(pair.Key, pair.Value), range.last_node);
	}

	Range Insert(K&& key, V&& value)
	{
		return Range(insert_node(core::move(key), core::move(value)), range.last_node);
	}

	Range Insert(const K& key, const V& value)
	{
		K keyCopy = key;
		V valueCopy = value;
		return Range(insert_node(core::move(keyCopy), core::move(valueCopy)), range.last_node);
	}

	void Insert(const HashMap& map)
	{
		Range rangeCopy = map.range;
		while(!rangeCopy.Empty())
		{
			K keyCopy = rangeCopy.First().Key;
			V valueCopy = rangeCopy.First().Value;
			insert_node(core::move(keyCopy), core::move(valueCopy));
			rangeCopy.PopFirst();
		}
	}

	//! Вставить пару только в случае если пары с указанным ключом не существует.
	//! Диапазон, содержащий найденный или вставленный элемент с указанным ключом и все элементы, идущие после него в контейнере.
	Range InsertNew(const K& key, const V& value)
	{
		uint keyHash = ToHash(key);
		if(bucket_heads!=null)
		{
			auto node = find_node(key, keyHash);
			if(node!=null) return Range(node, range.last_node);
		}
		K keyCopy = key;
		V valueCopy = value;
		return Range(insert_node(core::move(keyCopy), core::move(valueCopy), false), range.last_node);
	}

	iterator InsertNew(K&& key, V&& value)
	{
		uint keyHash = ToHash(key);
		if(bucket_heads!=null)
		{
			auto node = find_node(key, keyHash);
			if(node!=null) return Range(node, range.last_node);
		}
		return Range(insert_node(core::move(key), core::move(value), false), range.last_node);
	}

	//! Удалить элемент по ключу.
	//! \return Возвращает, существовал ли элемент с таким ключом.
	bool Remove(const K& key)
	{
		if(bucket_heads==null) return false;
		uint keyHash = ToHash(key);
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

		auto node = it.Range.first_node;
		auto next = node->next;

		uint keyHash = ToHash(node->element.Key);

		Node* previous = null;
		auto& bh = get_bucket_head(keyHash);
		auto current = bh;
		while(current!=null && current!=node)
		{
			previous = current;
			current = current->down;
		}

		INTRA_ASSERT(current==node);

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
				range.First().~value_type();
				auto node = range.first_node;
				range.PopFirst();
				free_node(node);
				if(range.first_node!=null) range.first_node->prev = null;
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
		auto ptr = range.first_node;

		for(size_t i=0; i<numKeys; i++)
		{
			ptrs.AddLast(ptr);
			ptr = ptr->next;
		}

		Algo::QuickSort(ptrs(), [&pred](Node* lhs, Node* rhs)
		{
			return pred(lhs->element, rhs->element);
		});

		range.first_node = ptrs[0];
		ptrs[0]->prev = null;
		for(uint i=1; i<numKeys; i++)
		{
			ptrs[i-1]->next = ptrs[i];
			ptrs[i]->prev = ptrs[i-1];
		}
		range.last_node = ptrs.Last();
		range.last_node->next = null;
	}

	//! Сортирует все элементы контейнера. После сортировки итерация по контейнеру выполняется в порядке, задаваемым pred, пока не будут добавлены новые элементы.
	//! \param pred Предикат сравнения ключей.
	template<typename P = Algo::Comparers::Function<K>> void SortByKey(P pred = Op::Less<K>)
	{
		SortByPair([&pred](const KeyValuePair<const K, V>& lhs, const KeyValuePair<const K, V>& rhs)
		{
			return pred(lhs.Key, rhs.Key);
		});
	}

	//! Сортирует все элементы контейнера. После сортировки итерация по контейнеру выполняется в порядке, задаваемым pred, пока не будут добавлены новые элементы.
	//! \param pred Предикат сравнения ключей.
	template<typename P = Algo::Comparers::Function<V>> void SortByValue(P pred = Op::Less<V>)
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

		INTRA_ASSERT(Math::IsPow2(numBuckets));

		allocate_buckets(Count(), numBuckets);
		rehash();
		return true;
	}

	//! Поиск по ключу.
	//! \return Диапазон, содержащий все элементы контейнера начиная с элемента с ключом key.
	Range Find(const K& key)
	{
		if(bucket_heads==null) return null;
		uint keyHash = ToHash(key);
		Node* node = find_node(key, keyHash);
		if(node==null) return null;
		return Range(node, range.last_node);
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
			static V empty;
			return empty;
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
		uint keyHash = ToHash(key);
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

	iterator rbegin() {return iterator(range.last_node);}
	const_iterator rbegin() const {return const_iterator(range.last_node);}
	iterator rend() {return iterator(null);}
	const_iterator rend() const {return const_iterator(null);}

#ifdef INTRA_STL_INTERFACE
	forceinline iterator emplace(K&& key, V&& value) {return iterator(Insert(core::move(key), core::move(value)));}
	forceinline iterator insert(const value_type& pair) {return iterator(Insert(pair.Key, pair.Value));}
	forceinline bool empty() const {return Empty();}
	forceinline size_t size() const {return Count();}
	forceinline void clear() {Clear();}
	forceinline iterator find(const K& key) {return iterator(Find(key));}
	forceinline const_iterator find(const K& key) const {return const_iterator(Find(key));}
#endif

    size_t Count() const {return bucket_heads!=null? (reinterpret_cast<size_t*>(bucket_heads))[0]: 0;}
    size_t BucketCount() const {return bucket_heads!=null? (reinterpret_cast<size_t*>(bucket_heads))[1]: 0;}

	void GetStats(size_t* oNumBuckets, size_t* oFreeBucketCount, double* oAverageBucketLoad, size_t* oMaxBucketLoad, size_t oBucketLoads[], size_t bucketLoadsCount)
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
		if(oAverageBucketLoad) *oAverageBucketLoad = double(loadSum)/num;
	}

private:
	void allocate_buckets(size_t size, size_t numBuckets)
	{
		delete[] bucket_heads;

		Node** ptrs = new Node*[numBuckets+2];
		size_t* data = reinterpret_cast<size_t*>(ptrs);
		data[0] = size;
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

    void set_count(size_t newCount) {if(bucket_heads!=null) reinterpret_cast<size_t*>(bucket_heads)[0] = newCount;}

	forceinline Node** get_bucket_heads() const {return bucket_heads!=null? bucket_heads+2: null;}
	forceinline Node*& get_bucket_head(size_t index) const {return get_bucket_heads()[index & (BucketCount()-1)];}

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

	Node* insert_node(K&& key, V&& value, bool findExisting=true)
	{
		if(bucket_heads==null)
		{
			allocate_buckets(Count(), 8);
			rehash();
		}

		uint keyHash = ToHash(key);

		if(findExisting)
		{
			Node* existing = find_node(key, keyHash);
			if(existing!=null)
			{
				existing->element.Value = value;
				return existing;
			}
		}

		Node* newNode = insert_node_after(range.last_node);
		new(&newNode->element) value_type(core::move(key), core::move(value));
		newNode->init_key(keyHash);
		auto& bh = get_bucket_head(keyHash);
		newNode->down = bh;
		bh = newNode;

		if(Count()>BucketCount())
		{
			allocate_buckets(Count(), BucketCount()*2);
			rehash();
		}

		return newNode;
	}

	Node* insert_node_after(Node* dest)
	{
		Node* newNode = new_node();
		newNode->next = dest==null? null: dest->next;
		newNode->prev = dest;
		if(newNode->next!=null) newNode->next->prev = newNode;
		if(dest!=null) dest->next = newNode;

		if(dest == range.last_node) range.last_node = newNode;
		if(range.first_node==null) range.first_node = newNode;

		set_count(Count()+1);

		return newNode;
	}

	Node* erase_node(Node* node)
	{
		if(node==null) return null;

		if(node->prev!=null) node->prev->next = node->next;
		if(node->next!=null) node->next->prev = node->prev;

		if(node == range.first_node) range.first_node = node->next;
		if(node == range.last_node) range.last_node = node->prev;
		Node* result = node->next;

		node->element.~value_type();
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
			Node* node = rangeCopy.first_node;
			uint keyHash = ToHash(node->element.Key);
			auto& bh = get_bucket_head(keyHash);
			node->down = bh;
			bh = node;
			rangeCopy.PopFirst();
		}
	}
};

}

