#pragma once

#include "IntraX/Container/ForwardDecls.h"
#include "IntraX/Container/Sequential/Array.h"

#include "Intra/Range/Search/Single.h"
#include "Intra/Range/Zip.h"

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
		INTRA_FORCEINLINE iterator(LinearMap<K,V>* mymap, size_t index):
			mMymap(mymap), mIndex(index) {}

		INTRA_FORCEINLINE Tuple<const K&, V&> operator*() const
		{
			INTRA_DEBUG_ASSERT(mIndex < mMymap->Count());
			return {mMymap->mKeys[mIndex], mMymap->mValues[mIndex]};
		}

		INTRA_FORCEINLINE iterator& operator++()
		{
			INTRA_DEBUG_ASSERT(mIndex < mMymap->Count());
			mIndex++;
			return *this;
		}

		INTRA_FORCEINLINE iterator operator++(int)
		{
			INTRA_DEBUG_ASSERT(mIndex < mMymap->Count());
			iterator result = *this;
			mIndex++;
			return result;
		}

		INTRA_FORCEINLINE iterator& operator--()
		{
			INTRA_DEBUG_ASSERT(mIndex > 0);
			mIndex--;
			return *this;
		}

		INTRA_FORCEINLINE iterator operator--(int)
		{
			INTRA_DEBUG_ASSERT(mIndex > 0);
			iterator result = *this;
			mIndex++;
			return result;
		}

		INTRA_FORCEINLINE bool operator==(const iterator& rhs) const {return mMymap==rhs.mMymap && mIndex==rhs.mIndex;}
		INTRA_FORCEINLINE bool operator!=(const iterator& rhs) const {return !operator==(rhs);}
		INTRA_FORCEINLINE bool operator==(decltype(null)) const {return mMymap==null;}
		INTRA_FORCEINLINE bool operator!=(decltype(null)) const {return !operator==(null);}

	private:
		LinearMap* mMymap;
		size_t mIndex;
	};

	struct const_iterator
	{
		INTRA_FORCEINLINE const Tuple<const K&, V&> operator*() const
		{
			INTRA_DEBUG_ASSERT(mIndex < mMymap->Count());
			return {mMymap->mKeys[mIndex], mMymap->mValues[mIndex]};
		}

		INTRA_FORCEINLINE const_iterator& operator++()
		{
			INTRA_DEBUG_ASSERT(mIndex < mMymap->Count());
			mIndex++;
			return *this;

		}
		INTRA_FORCEINLINE const_iterator operator++(int)
		{
			INTRA_DEBUG_ASSERT(mIndex < mMymap->Count());
			const_iterator result = *this;
			mIndex++;
			return result;
		}

		INTRA_FORCEINLINE const_iterator& operator--()
		{
			INTRA_DEBUG_ASSERT(mIndex > 0);
			mIndex--;
			return *this;
		}

		INTRA_FORCEINLINE const_iterator operator--(int)
		{
			INTRA_DEBUG_ASSERT(mIndex > 0);
			const_iterator result = *this;
			mIndex++;
			return result;
		}

		INTRA_FORCEINLINE bool operator==(const const_iterator& rhs) const {return mMymap==rhs.mMymap && mIndex==rhs.mIndex;}
		INTRA_FORCEINLINE bool operator!=(const const_iterator& rhs) const {return !operator==(rhs);}
		INTRA_FORCEINLINE bool operator==(decltype(null)) const {return mMymap==null;}
		INTRA_FORCEINLINE bool operator!=(decltype(null)) const {return !operator==(null);}
	private:
		const LinearMap* mMymap;
		size_t mIndex;
	};

	explicit LinearMap(size_t initialCapacity):
		mKeys(initialCapacity), mValues(initialCapacity) {}

	LinearMap(decltype(null)=null):
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

	INTRA_FORCEINLINE bool Empty() const {return mKeys.Empty();}
	INTRA_FORCEINLINE bool operator==(decltype(null)) const {return Empty();}
	INTRA_FORCEINLINE bool operator!=(decltype(null)) const {return !Empty();}

	///@{
	/// Linear time insertion O(n)
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
	///@}


	///@{
	/// New element insertion in constant time O(1). Element with p.Key must not exist!
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
	///@}

	/// Linear time O(n) lookup
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

	INTRA_FORCEINLINE size_t FindIndex(const K& key) const
	{
		size_t index = 0;
		Find(mKeys, key, &index);
		return index;
	}

	INTRA_FORCEINLINE iterator Find(const K& key) {return iterator{this, FindIndex(key)};}
	INTRA_FORCEINLINE const_iterator Find(const K& key) const {return const_iterator{this, FindIndex(key)};}

	void Rename(const K& key, const K& newKey)
	{
		if(key == newKey) return;
		INTRA_PRECONDITION(!Contains(mKeys, newKey));
		auto i = FindIndex(key);
		if(i==Count()) return;
		mKeys[i] = newKey;
	}

	INTRA_FORCEINLINE size_t Count() const {return mKeys.Count();}
	INTRA_FORCEINLINE size_t Capacity() const {return mKeys.Capacity();}
	INTRA_FORCEINLINE void Reserve(size_t newSize) {mKeys.Reserve(newSize); mValues.Reserve(newSize);}
	INTRA_FORCEINLINE void Clear() {mKeys.Clear(); mValues.Clear();}

	INTRA_FORCEINLINE bool KeyExists(const K& key) const {return Contains(mKeys, key);}

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
	
	INTRA_FORCEINLINE CPairRef GetPair(size_t index) const {return {mKeys[index], mValues[index]};}
	INTRA_FORCEINLINE PairRef GetPair(size_t index) {return {mKeys[index], mValues[index]};}

	CSpan<K> Keys() const {return mKeys;}
	CSpan<V> Values() const {return mValues;}
	Span<V> Values() {return mValues;}

	const K& Key(size_t index) const {return mKeys[index];}
	const V& Value(size_t index) const {return mValues[index];}
	K& Key(size_t index) {return mKeys[index];}
	V& Value(size_t index) {return mValues[index];}

	LinearMap& operator=(decltype(null))
	{
		mKeys.Clear();
		mValues.Clear();
		return *this;
	}

	INTRA_FORCEINLINE ElementRange AsRange()
	{return Zip(mKeys.AsConstRange(), mValues.AsRange());}

	INTRA_FORCEINLINE ElementConstRange AsConstRange() const
	{return Zip(mKeys.AsConstRange(), mValues.AsConstRange());}

	INTRA_FORCEINLINE ElementConstRange AsRange() const
	{return AsConstRange();}

	INTRA_FORCEINLINE iterator begin() {return iterator(this, 0);}
	INTRA_FORCEINLINE iterator end() {return iterator(this, Count());}
	INTRA_FORCEINLINE const_iterator begin() const {return const_iterator(this, 0);}
	INTRA_FORCEINLINE const_iterator end() const {return const_iterator(this, Count());}

#ifdef INTRA_CONTAINER_STL_FORWARD_COMPATIBILITY
	INTRA_FORCEINLINE iterator emplace(K&& key, V&& value) {return Insert(Move(key), Move(value));}
	INTRA_FORCEINLINE iterator insert(const Tuple<K,V>& pair) {return Insert(pair.first, pair.second);}
	INTRA_FORCEINLINE bool empty() const {return Empty();}
	INTRA_FORCEINLINE size_t size() const {return Count();}
	INTRA_FORCEINLINE void clear() {Clear();}
	INTRA_FORCEINLINE iterator find(const K& key) {return Find(key);}
	INTRA_FORCEINLINE const_iterator find(const K& key) const {return Find(key);}
#endif

private:
	Array<K> mKeys;
	Array<V> mValues;
};
INTRA_END
