#pragma once

#include "Container/ForwardDecls.h"
#include "Container/Sequential/Array.h"
#include "Meta/Pair.h"
#include "Algo/Search/Single.h"
#include "Platform/CppFeatures.h"
#include "Platform/CppWarnings.h"
#include "Range/Compositors/ZipKV.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Container {

template<typename K, typename V> class LinearMap
{
public:
	typedef KeyValuePair<K, V> Pair;
	typedef KeyValuePair<const K, V> CPair;
	typedef KeyValuePair<K&, V&> PairRef;
	typedef KeyValuePair<const K&, V&> CPairRef;
	typedef Intra::Range::RZipKV<ArrayRange<const K>, ArrayRange<V>> Range;
	typedef Intra::Range::RZipKV<ArrayRange<const K>, ArrayRange<const V>> ConstRange;

	typedef K key_type;
	typedef V mapped_type;
	typedef CPairRef value_type;

	struct iterator
	{
		forceinline iterator(LinearMap<K,V>* mymap, size_t index):
			mMymap(mymap), mIndex(index) {}

		forceinline Meta::Pair<const K&, V&> operator*() const
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
		forceinline const Meta::Pair<const K&, V&> operator*() const
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

	LinearMap(ArrayRange<const Pair> pairs):
		mKeys(), mValues()
	{
		for(auto& p: pairs) Insert(p);
	}

	LinearMap(ArrayRange<const K> keyRange, ArrayRange<const V> valueRange):
		mKeys(), mValues()
	{
		INTRA_DEBUG_ASSERT(keyRange.Length()==valueRange.Length());
		Reserve(keyRange.Length());
		for(size_t i=0; i<keyRange.Length(); i++)
			Insert({keyRange[i], valueRange[i]});
	}

	LinearMap(ArrayRange<const K> keyRange, ArrayRange<const V* const> valuePtrRange):
		mKeys(), mValues()
	{
		INTRA_DEBUG_ASSERT(keyRange.Length()==valuePtrRange.Length());
		Reserve(keyRange.Length());
		for(size_t i=0; i<keyRange.Length(); i++)
			Insert(keyRange[i], *valuePtrRange[i]);
	}

	LinearMap(ArrayRange<const K> keyRange, ArrayRange<V* const> valuePtrRange):
		mKeys(), mValues()
	{
		INTRA_DEBUG_ASSERT(keyRange.Count()==valuePtrRange.Count());
		Reserve(keyRange.Count());
		for(size_t i=0; i<keyRange.Count(); i++)
			Insert(keyRange[i], *valuePtrRange[i]);
	}

	LinearMap(const LinearMap& rhs) = default;

	forceinline LinearMap(LinearMap&& rhs):
		mKeys(Meta::Move(rhs.mKeys)), mValues(Meta::Move(rhs.mValues)) {}

	forceinline bool Empty() const {return mKeys.Empty();}
	forceinline bool operator==(null_t) const {return Empty();}
	forceinline bool operator!=(null_t) const {return !Empty();}

	//!@{
	//! Вставка за линейное время O(n)
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
		if(i!=Count()) return mValues[i] = Meta::Move(value);
		mKeys.AddLast(key);
		mValues.AddLast(Meta::Move(value));
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
	//! Вставка нового элемента за постоянное время O(1). Элемент с таким ключом не должен существовать!
	void InsertNew(const Pair& p)
	{
		INTRA_DEBUG_ASSERT(!KeyExists(p.key));
		mKeys.AddLast(p.key);
		return mValues.AddLast(p.value);
	}

	V& InsertNew(const K& key, V&& value)
	{
		INTRA_DEBUG_ASSERT(!KeyExists(key));
		mKeys.AddLast(key);
		return mValues.AddLast(Meta::Move(value));
	}

	V& InsertNew(const K& key)
	{
		INTRA_DEBUG_ASSERT(!KeyExists(key));
		mKeys.AddLast(key);
		return mValues.AddLast();
	}

	V& InsertNew(const K& key, const V& value)
	{
		INTRA_DEBUG_ASSERT(!KeyExists(key));
		mKeys.AddLast(key);
		return mValues.AddLast(value);
	}

	void InsertNew(const LinearMap<K, V>& rhs)
	{
		for(size_t i=0, count=rhs.Count(); i<count; i++)
			InsertNew(rhs.mKeys[i], rhs.mValues[i]);
	}
	//!@}

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
		Algo::Find(mKeys, key, &index);
		return index;
	}

	forceinline iterator Find(const K& key) {return iterator{this, FindIndex(key)};}
	forceinline const_iterator Find(const K& key) const {return const_iterator{this, FindIndex(key)};}

	void Rename(const K& key, const K& newKey)
	{
		if(key==newKey) return;
		INTRA_DEBUG_ASSERT(!Algo::Contains(mKeys, newKey));
		auto i = FindIndex(key);
		if(i==Count()) return;
		mKeys[i] = newKey;
	}

	forceinline size_t Count() const {return mKeys.Count();}
	forceinline size_t Capacity() const {return mKeys.Capacity();}
	forceinline void Reserve(size_t newSize) {mKeys.Reserve(newSize); mValues.Reserve(newSize);}
	forceinline void Clear() {mKeys.Clear(); mValues.Clear();}

	forceinline bool KeyExists(const K& key) const {return Algo::Contains(mKeys, key);}

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
		if(i==Count()) return;
		mKeys.RemoveUnordered(i);
		mValues.RemoveUnordered(i);
	}
	
	forceinline CPairRef GetPair(size_t index) const {return {mKeys[index], mValues[index]};}
	forceinline PairRef GetPair(size_t index) {return {mKeys[index], mValues[index]};}

	ArrayRange<const K> Keys() const {return mKeys;}
	ArrayRange<const V> Values() const {return mValues;}
	ArrayRange<V> Values() {return mValues;}

	const K& Key(size_t index) const {return mKeys[index];}
	const V& Value(size_t index) const {return mValues[index];}
	K& Key(size_t index) {return mKeys[index];}
	V& Value(size_t index) {return mValues[index];}

	LinearMap& operator=(const LinearMap& rhs) = default;

	LinearMap& operator=(null_t)
	{
		mKeys.Clear();
		mValues.Clear();
		return *this;
	}

	LinearMap& operator=(LinearMap&& rhs)
	{
		mKeys = Meta::Move(rhs.mKeys);
		mValues = Meta::Move(rhs.mValues);
		return *this;
	}

	forceinline Range AsRange()
	{return ZipKV(mKeys.AsConstRange(), mValues.AsRange());}

	forceinline ConstRange AsConstRange() const
	{return ZipKV(mKeys.AsConstRange(), mValues.AsConstRange());}

	forceinline ConstRange AsRange() const
	{return AsConstRange();}

	forceinline iterator begin() {return iterator(this, 0);}
	forceinline iterator end() {return iterator(this, Count());}
	forceinline const_iterator begin() const {return const_iterator(this, 0);}
	forceinline const_iterator end() const {return const_iterator(this, Count());}

	//! @defgroup LinearMap_STL_Interface STL-подобный интерфейс для LinearMap
	//! Этот интерфейс предназначен для совместимости с обобщённым контейнеро-независимым кодом.
	//! Использовать напрямую этот интерфейс не рекомендуется.
	//!@{
	forceinline iterator emplace(K&& key, V&& value) {return Insert(Meta::Move(key), Meta::Move(value));}
	forceinline iterator insert(const Meta::Pair<K,V>& pair) {return Insert(pair.first, pair.second);}
	forceinline bool empty() const {return Empty();}
	forceinline size_t size() const {return Count();}
	forceinline void clear() {Clear();}
	forceinline iterator find(const K& key) {return Find(key);}
	forceinline const_iterator find(const K& key) const {return Find(key);}
	//!@}

private:
	Array<K> mKeys;
	Array<V> mValues;
};

INTRA_WARNING_POP

}}
