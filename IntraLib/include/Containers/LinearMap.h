#pragma once

#include "Containers/ForwardDeclarations.h"
#include "Containers/Array.h"
#include "Memory/AllocatorInterface.h"
#include "Meta/Tuple.h"

namespace Intra {

template<typename K, typename V, class Allocator> class LinearMap
{
public:
	typedef KeyValuePair<K, V> Pair;
	typedef KeyValuePair<const K, V> CPair;
	typedef KeyValuePair<K&, V&> PairRef;
	typedef KeyValuePair<const K&, V&> CPairRef;
	typedef Intra::Range::ZipResult<ArrayRange<const K>, ArrayRange<V>> Range;
	typedef Intra::Range::ZipResult<ArrayRange<const K>, ArrayRange<const V>> ConstRange;

	typedef K key_type;
	typedef V mapped_type;
	typedef CPairRef value_type;

	struct iterator
	{
		forceinline iterator(LinearMap<K,V>* mymap_, size_t index_): mymap(mymap_), index(index_) {}

		forceinline core::pair<const K&, V&> operator*() const {INTRA_ASSERT(index<mymap->Count()); return {mymap->keys[index], mymap->values[index]};}
		forceinline iterator& operator++() {INTRA_ASSERT(index<mymap->Count()); index++; return *this;}
		forceinline iterator operator++(int) {INTRA_ASSERT(index<mymap->Count()); iterator result = *this; index++; return result;}
		forceinline iterator& operator--() {INTRA_ASSERT(index>0); index--; return *this;}
		forceinline iterator operator--(int) {INTRA_ASSERT(index>0); iterator result = *this; index++; return result;}

		forceinline bool operator==(const iterator& rhs) const {return mymap==rhs.mymap && index==rhs.index;}
		forceinline bool operator!=(const iterator& rhs) const {return !operator==(rhs);}
		forceinline bool operator==(null_t) const {return mymap==null;}
		forceinline bool operator!=(null_t) const {return !operator==(null);}

	private:
		LinearMap* mymap;
		size_t index;
	};

	struct const_iterator
	{
		forceinline const core::pair<const K&, V&> operator*() const {INTRA_ASSERT(index<mymap->Count()); return {mymap->keys[index], mymap->values[index]};}
		forceinline const_iterator& operator++() {INTRA_ASSERT(index<mymap->Count()); index++; return *this;}
		forceinline const_iterator operator++(int) {INTRA_ASSERT(index<mymap->Count()); const_iterator result = *this; index++; return result;}
		forceinline const_iterator& operator--() {INTRA_ASSERT(index>0); index--; return *this;}
		forceinline const_iterator operator--(int) {INTRA_ASSERT(index>0); const_iterator result = *this; index++; return result;}

		forceinline bool operator==(const const_iterator& rhs) const {return mymap==rhs.mymap && index==rhs.index;}
		forceinline bool operator!=(const const_iterator& rhs) const {return !operator==(rhs);}
		forceinline bool operator==(null_t) const {return mymap==null;}
		forceinline bool operator!=(null_t) const {return !operator==(null);}
	private:
		const LinearMap* mymap;
		size_t index;
	};

	explicit LinearMap(size_t size): keys(size), values(size) {}
	LinearMap(null_t=null) {}

	LinearMap(ArrayRange<const Pair> pairs)
	{
		for(auto& p: pairs) Insert(p);
	}

	LinearMap(ArrayRange<const K> keys, ArrayRange<const V> values)
	{
		INTRA_ASSERT(keys.Count()==values.Count());
		Reserve(keys.Count());
		for(size_t i=0; i<keys.Count(); i++)
			Insert({keys[i], values[i]});
	}

	LinearMap(ArrayRange<const K> keys, ArrayRange<const V* const> values)
	{
		INTRA_ASSERT(keys.Count()==values.Count());
		Reserve(keys.Count());
		for(size_t i=0; i<keys.Count(); i++)
			Insert(keys[i], *values[i]);
	}

	LinearMap(ArrayRange<const K> keys, ArrayRange<V* const> values)
	{
		INTRA_ASSERT(keys.Count()==values.Count());
		Reserve(keys.Count());
		for(size_t i=0; i<keys.Count(); i++)
			Insert(keys[i], *values[i]);
	}

	LinearMap(const LinearMap& rhs) = default;
	forceinline LinearMap(LinearMap&& rhs): keys(core::move(rhs.keys)), values(core::move(rhs.values)) {}

	forceinline bool Empty() const {return keys.Empty();}
	forceinline bool operator==(null_t) const {return Empty();}
	forceinline bool operator!=(null_t) const {return !Empty();}

	//!@{
	//! Вставка за линейное время O(n)
	iterator Insert(const Pair& p)
	{
		auto i = keys.FindIndex(p.first);
		if(i!=Count())
		{
			values[i] = p.second;
			return;
		}
		keys.AddLast(p.first);
		values.AddLast(p.second);
		return iterator{this, Count()-1};
	}

	iterator Insert(const K& key, V&& value)
	{
		auto i = FindIndex(key);
		if(i!=Count()) return values[i] = core::move(value);
		keys.AddLast(key);
		values.AddLast(core::move(value));
		return iterator{this, Count()-1};
	}

	iterator Insert(const K& key, const V& value)
	{
		size_t i = FindIndex(key);
		if(i!=Count())
		{
			values[i] = value;
			return iterator{this, i};
		}
		keys.AddLast(key);
		values.AddLast(value);
		return iterator{this, Count()-1};
	}

	void Insert(const LinearMap<K, V>& rhs)
	{
		for(size_t i=0, count=rhs.Count(); i<count; i++)
			Insert(rhs.keys[i], rhs.values[i]);
	}
	//!@}


	//!@{
	//! Вставка нового элемента за постоянное время O(1). Элемент с таким ключом не должен существовать!
	void InsertNew(const Pair& p)
	{
		INTRA_ASSERT(!KeyExists(p.key));
		keys.AddLast(p.key);
		return values.AddLast(p.value);
	}

	V& InsertNew(const K& key, V&& value)
	{
		INTRA_ASSERT(!KeyExists(key));
		keys.AddLast(key);
		return values.AddLast(core::move(value));
	}

	V& InsertNew(const K& key)
	{
		INTRA_ASSERT(!KeyExists(key));
		keys.AddLast(key);
		return values.AddLast();
	}

	V& InsertNew(const K& key, const V& value)
	{
		INTRA_ASSERT(!KeyExists(key));
		keys.AddLast(key);
		return values.AddLast(value);
	}

	void InsertNew(const LinearMap<K, V>& rhs)
	{
		for(size_t i=0, count=rhs.Count(); i<count; i++)
			InsertNew(rhs.keys[i], rhs.values[i]);
	}
	//!@}

	V& operator[](const K& key)
	{
		auto i = FindIndex(key);
		if(i!=Count()) return values[i];
		keys.AddLast(key);
		return values.EmplaceLast();
	}

	//V operator[](const K& key) const {return Get(key);}

	const V& Get(const K& key, bool* exists=null) const
	{
		auto i = FindIndex(key);
		if(exists!=null) *exists = (i!=Count());
		if(i==Count())
		{
			static V empty;
			return empty;
		}
		return values[i];
	}

	const K& FindByValue(const V& second) const;  //Найти ключ по значению

	forceinline size_t FindIndex(const K& key) const {size_t index = 0; keys().Find(key, &index); return index;}
	forceinline iterator Find(const K& key) {return iterator{this, FindIndex(key)};}
	forceinline const_iterator Find(const K& key) const {return const_iterator{this, FindIndex(key)};}

	void Rename(const K& key, const K& newKey)
	{
		if(key==newKey) return;
		INTRA_ASSERT(!keys.Contains(newKey));
		auto i = FindIndex(key);
		if(i==Count()) return;
		keys[i] = newKey;
	}

	forceinline size_t Count() const {return keys.Count();}
	forceinline size_t Capacity() const {return keys.Capacity();}
	forceinline void Reserve(size_t newSize) {keys.Reserve(newSize); values.Reserve(newSize);}
	forceinline void Clear() {keys.Clear(); values.Clear();}

	forceinline bool KeyExists(const K& key) const {return keys().Contains(key);}

	void Remove(const K& key)
	{
		size_t index = 0;
		if(keys().Find(key, &index).Empty()) return;
		keys.Remove(index);
		values.Remove(index);
	}

	void RemoveUnordered(const K& key)
	{
		auto i = keys.FindIndex(key);
		if(i==Count()) return;
		keys.RemoveUnordered(i);
		values.RemoveUnordered(i);
	}
	
	forceinline CPairRef GetPair(size_t index) const {return {keys[index], values[index]};}
	forceinline PairRef GetPair(size_t index) {return {keys[index], values[index]};}

	ArrayRange<const K> Keys() const {return keys;}
	ArrayRange<const V> Values() const {return values;}
	ArrayRange<V> Values() {return values;}

	const K& Key(size_t index) const {return keys[index];}
	const V& Value(size_t index) const {return values[index];}
	K& Key(size_t index) {return keys[index];}
	V& Value(size_t index) {return values[index];}

	LinearMap& operator=(const LinearMap& rhs) = default;

	LinearMap& operator=(null_t)
	{
		keys.Clear();
		values.Clear();
		return *this;
	}

	LinearMap& operator=(LinearMap&& rhs)
	{
		keys = core::move(rhs.keys);
		values = core::move(rhs.values);
		return *this;
	}

	forceinline Range AsRange() {return Intra::Range::Zip(keys.AsConstRange(), values.AsRange());}
	forceinline ConstRange AsConstRange() {return Intra::Range::Zip(keys.AsConstRange(), values.AsConstRange());}

	forceinline iterator begin() {return iterator(this, 0);}
	forceinline iterator end() {return iterator(this, Count());}
	forceinline const_iterator begin() const {return const_iterator(this, 0);}
	forceinline const_iterator end() const {return const_iterator(this, Count());}

#ifdef INTRA_STL_INTERFACE
	forceinline iterator emplace(K&& key, V&& value) {return Insert(core::move(key), core::move(value));}
	forceinline iterator insert(const core::pair<K,V>& pair) {return Insert(pair.first, pair.second);}
	forceinline bool empty() const {return Empty();}
	forceinline size_t size() const {return Count();}
	forceinline void clear() {Clear();}
	forceinline iterator find(const K& key) {return Find(key);}
	forceinline const_iterator find(const K& key) const {return Find(key);}
#endif

private:
	Array<K, Allocator> keys;
	Array<V, Allocator> values;
};

}
