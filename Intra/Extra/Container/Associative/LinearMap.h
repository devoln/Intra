#pragma once

#include "Container/ForwardDecls.h"
#include "Container/Sequential/Array.h"

#include "Core/Range/Search/Single.h"
#include "Core/Range/Zip.h"

INTRA_BEGIN
template<typename K, typename V> class LinearMap
{
public:
	typedef Tuple<K, V> Pair;
	typedef Tuple<const K, V> CPair;
	typedef Tuple<K&, V&> PairRef;
	typedef Tuple<const K&, V&> CPairRef;
	typedef RZip<CSpan<K>, Span<V>> ElementRange;
	typedef RZip<CSpan<K>, CSpan<V>> ElementConstRange;

	typedef K key_type;
	typedef V mapped_type;
	typedef CPairRef value_type;

	struct iterator
	{
		forceinline iterator(LinearMap<K,V>* mymap, size_t index):
			mMymap(mymap), mIndex(index) {}

		forceinline Tuple<const K&, V&> operator*() const
		{
			INTRA_DEBUG_ASSERT(mIndex < mMymap->Count());
			return {mMymap->mKeys[mIndex], mMymap->mValues[mIndex]};
		}

		forceinline iterator& operator++()
		{
			INTRA_DEBUG_ASSERT(mIndex < mMymap->Count());
			mIndex++;
			return *this;
		}

		forceinline iterator operator++(int)
		{
			INTRA_DEBUG_ASSERT(mIndex < mMymap->Count());
			iterator result = *this;
			mIndex++;
			return result;
		}

		forceinline iterator& operator--()
		{
			INTRA_DEBUG_ASSERT(mIndex > 0);
			mIndex--;
			return *this;
		}

		forceinline iterator operator--(int)
		{
			INTRA_DEBUG_ASSERT(mIndex > 0);
			iterator result = *this;
			mIndex++;
			return result;
		}

		forceinline bool operator==(const iterator& rhs) const {return mMymap==rhs.mMymap && mIndex==rhs.mIndex;}
		forceinline bool operator!=(const iterator& rhs) const {return !operator==(rhs);}
		forceinline bool operator==(null_t) const {return mMymap==null;}
		forceinline bool operator!=(null_t) const {return !operator==(null);}

	private:
		LinearMap* mMymap;
		size_t mIndex;
	};

	struct const_iterator
	{
		forceinline const Tuple<const K&, V&> operator*() const
		{
			INTRA_DEBUG_ASSERT(mIndex < mMymap->Count());
			return {mMymap->mKeys[mIndex], mMymap->mValues[mIndex]};
		}

		forceinline const_iterator& operator++()
		{
			INTRA_DEBUG_ASSERT(mIndex < mMymap->Count());
			mIndex++;
			return *this;

		}
		forceinline const_iterator operator++(int)
		{
			INTRA_DEBUG_ASSERT(mIndex < mMymap->Count());
			const_iterator result = *this;
			mIndex++;
			return result;
		}

		forceinline const_iterator& operator--()
		{
			INTRA_DEBUG_ASSERT(mIndex > 0);
			mIndex--;
			return *this;
		}

		forceinline const_iterator operator--(int)
		{
			INTRA_DEBUG_ASSERT(mIndex > 0);
			const_iterator result = *this;
			mIndex++;
			return result;
		}

		forceinline bool operator==(const const_iterator& rhs) const {return mMymap==rhs.mMymap && mIndex==rhs.mIndex;}
		forceinline bool operator!=(const const_iterator& rhs) const {return !operator==(rhs);}
		forceinline bool operator==(null_t) const {return mMymap==null;}
		forceinline bool operator!=(null_t) const {return !operator==(null);}
	private:
		const LinearMap* mMymap;
		size_t mIndex;
	};

	explicit LinearMap(size_t initialCapacity):
		mKeys(initialCapacity), mValues(initialCapacity) {}

	LinearMap(null_t=null):
		mKeys(), mValues() {}

	LinearMap(CSpan<Pair> pairs):
		mKeys(), mValues()
	{
		for(auto& p: pairs) Insert(p);
	}

	LinearMap(CSpan<K> keyRange, CSpan<V> valueRange):
		mKeys(), mValues()
	{
		INTRA_PRECONDITION(keyRange.Length() == valueRange.Length());
		Reserve(keyRange.Length());
		for(size_t i=0; i<keyRange.Length(); i++)
			Insert({keyRange[i], valueRange[i]});
	}

	LinearMap(CSpan<K> keyRange, CSpan<const V*> valuePtrRange):
		mKeys(), mValues()
	{
		INTRA_PRECONDITION(keyRange.Length() == valuePtrRange.Length());
		Reserve(keyRange.Length());
		for(size_t i=0; i<keyRange.Length(); i++)
			Insert(keyRange[i], *valuePtrRange[i]);
	}

	LinearMap(CSpan<K> keyRange, CSpan<V*> valuePtrRange):
		mKeys(), mValues()
	{
		INTRA_DEBUG_ASSERT(keyRange.Length() == valuePtrRange.Length());
		Reserve(keyRange.Length());
		for(size_t i=0; i<keyRange.Length(); i++)
			Insert(keyRange[i], *valuePtrRange[i]);
	}

	forceinline bool Empty() const {return mKeys.Empty();}
	forceinline bool operator==(null_t) const {return Empty();}
	forceinline bool operator!=(null_t) const {return !Empty();}

	//!@{
	//! Linear time insertion O(n)
	iterator Insert(const Pair& p)
	{
		size_t i = mKeys.FindIndex(p.first);
		if(i!=Count())
		{
			mValues[i] = p.second;
			return iterator(this, i);
		}
		mKeys.AddLast(p.first);
		mValues.AddLast(p.second);
		return iterator{this, Count()-1};
	}

	iterator Insert(const K& key, V&& value)
	{
		size_t i = FindIndex(key);
		if(i!=Count()) return mValues[i] = Move(value);
		mKeys.AddLast(key);
		mValues.AddLast(Move(value));
		return iterator{this, Count()-1};
	}

	iterator Insert(const K& key, const V& value)
	{
		size_t i = FindIndex(key);
		if(i!=Count())
		{
			mValues[i] = value;
			return iterator{this, i};
		}
		mKeys.AddLast(key);
		mValues.AddLast(value);
		return iterator{this, Count()-1};
	}

	void Insert(const LinearMap<K, V>& rhs)
	{
		for(size_t i=0, count=rhs.Count(); i<count; i++)
			Insert(rhs.mKeys[i], rhs.mValues[i]);
	}
	//!@}


	//!@{
	//! New element insertion in constant time O(1). Element with p.Key must not exist!
	void InsertNew(const Pair& p)
	{
		INTRA_PRECONDITION(!KeyExists(p.key));
		mKeys.AddLast(p.key);
		return mValues.AddLast(p.value);
	}

	V& InsertNew(const K& key, V&& value)
	{
		INTRA_PRECONDITION(!KeyExists(key));
		mKeys.AddLast(key);
		return mValues.AddLast(Move(value));
	}

	V& InsertNew(const K& key)
	{
		INTRA_PRECONDITION(!KeyExists(key));
		mKeys.AddLast(key);
		return mValues.AddLast();
	}

	V& InsertNew(const K& key, const V& value)
	{
		INTRA_PRECONDITION(!KeyExists(key));
		mKeys.AddLast(key);
		return mValues.AddLast(value);
	}

	void InsertNew(const LinearMap<K, V>& rhs)
	{
		for(size_t i=0, count=rhs.Count(); i<count; i++)
			InsertNew(rhs.mKeys[i], rhs.mValues[i]);
	}
	//!@}

	//! Linear time O(n) lookup
	V& operator[](const K& key)
	{
		auto i = FindIndex(key);
		if(i!=Count()) return mValues[i];
		mKeys.AddLast(key);
		return mValues.EmplaceLast();
	}

	V& Get(const K& key, V& defaultValue, bool& oExists)
	{
		auto i = FindIndex(key);
		oExists = i!=Count();
		return oExists? mValues[i]: defaultValue;
	}

	const V& Get(const K& key, const V& defaultValue, bool& oExists) const
	{
		auto i = FindIndex(key);
		oExists = i!=Count();
		return oExists? mValues[i]: defaultValue;
	}

	V& Get(const K& key, V& defaultValue)
	{
		bool exists;
		return Get(key, defaultValue, exists);
	}

	const V& Get(const K& key, const V& defaultValue) const
	{
		bool exists;
		return Get(key, defaultValue, exists);
	}

	const K& FindByValue(const V& second) const;  //Найти ключ по значению

	forceinline size_t FindIndex(const K& key) const
	{
		size_t index = 0;
		Find(mKeys, key, &index);
		return index;
	}

	forceinline iterator Find(const K& key) {return iterator{this, FindIndex(key)};}
	forceinline const_iterator Find(const K& key) const {return const_iterator{this, FindIndex(key)};}

	void Rename(const K& key, const K& newKey)
	{
		if(key == newKey) return;
		INTRA_PRECONDITION(!Contains(mKeys, newKey));
		auto i = FindIndex(key);
		if(i==Count()) return;
		mKeys[i] = newKey;
	}

	forceinline size_t Count() const {return mKeys.Count();}
	forceinline size_t Capacity() const {return mKeys.Capacity();}
	forceinline void Reserve(size_t newSize) {mKeys.Reserve(newSize); mValues.Reserve(newSize);}
	forceinline void Clear() {mKeys.Clear(); mValues.Clear();}

	forceinline bool KeyExists(const K& key) const {return Contains(mKeys, key);}

	void Remove(const K& key)
	{
		size_t index = 0;
		if(mKeys().Find(key, &index).Empty()) return;
		mKeys.Remove(index);
		mValues.Remove(index);
	}

	void RemoveUnordered(const K& key)
	{
		auto i = mKeys.FindIndex(key);
		if(i == Count()) return;
		mKeys.RemoveUnordered(i);
		mValues.RemoveUnordered(i);
	}
	
	forceinline CPairRef GetPair(size_t index) const {return {mKeys[index], mValues[index]};}
	forceinline PairRef GetPair(size_t index) {return {mKeys[index], mValues[index]};}

	CSpan<K> Keys() const {return mKeys;}
	CSpan<V> Values() const {return mValues;}
	Span<V> Values() {return mValues;}

	const K& Key(size_t index) const {return mKeys[index];}
	const V& Value(size_t index) const {return mValues[index];}
	K& Key(size_t index) {return mKeys[index];}
	V& Value(size_t index) {return mValues[index];}

	LinearMap& operator=(null_t)
	{
		mKeys.Clear();
		mValues.Clear();
		return *this;
	}

	forceinline ElementRange AsRange()
	{return Zip(mKeys.AsConstRange(), mValues.AsRange());}

	forceinline ElementConstRange AsConstRange() const
	{return Zip(mKeys.AsConstRange(), mValues.AsConstRange());}

	forceinline ElementConstRange AsRange() const
	{return AsConstRange();}

	forceinline iterator begin() {return iterator(this, 0);}
	forceinline iterator end() {return iterator(this, Count());}
	forceinline const_iterator begin() const {return const_iterator(this, 0);}
	forceinline const_iterator end() const {return const_iterator(this, Count());}

#ifdef INTRA_CONTAINER_STL_FORWARD_COMPATIBILITY
	forceinline iterator emplace(K&& key, V&& value) {return Insert(Move(key), Move(value));}
	forceinline iterator insert(const Tuple<K,V>& pair) {return Insert(pair.first, pair.second);}
	forceinline bool empty() const {return Empty();}
	forceinline size_t size() const {return Count();}
	forceinline void clear() {Clear();}
	forceinline iterator find(const K& key) {return Find(key);}
	forceinline const_iterator find(const K& key) const {return Find(key);}
#endif

private:
	Array<K> mKeys;
	Array<V> mValues;
};
INTRA_END
