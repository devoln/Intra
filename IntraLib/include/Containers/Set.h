#pragma once

#include "Array.h"
#include "Algorithms/Algorithms.h"

namespace Intra {

template<typename T> class LinearSet
{
public:
	typedef T value_type;

	LinearSet() = default;
	LinearSet(ArrayRange<const T> values) {Insert(values);}
	LinearSet(const LinearSet<T>& rhs) {Insert(rhs);}

	void Insert(T&& value) {if(!data.Contains(value)) data.AddLast(core::move(value));}
	void Insert(const T& value) {if(!data.Contains(value)) data.AddLast(value);}
	void Insert(ArrayRange<const T> values) {for(auto& v: values) Insert(v);}

	void Remove(const T& val)
	{
		auto i = data.Find(val);
		if(i!=-1) data.RemoveUnordered(i);
	}

	bool operator==(null_t) const {return data==null;}
	bool operator!=(null_t) const {return !operator==(null);}

	bool Contains(const T& v) const {return data.Find(v)!=-1;}

	intptr Find(const T& v) const {return data.Find(v);}

	T* begin() {return data.begin();}
	const T* begin() const {return data.begin();}
	T* end() {return data.end();}
	const T* end() const {return data.end();}

	T& operator[](size_t i) {return data[i];}
	const T& operator[](size_t i) const {return data[i];}

	ArrayRange<T> AsRange() {return data.AsRange();}
	ArrayRange<const T> AsRange() const {return data.AsConstRange();}
	ArrayRange<const T> AsConstRange() const {return data.AsConstRange();}
	operator ArrayRange<T>() {return AsRange();}
	operator ArrayRange<const T>() const {return AsConstRange();}

private:
	Array<T> data;
};

template<typename T> using Set = LinearSet<T>;

#if INTRA_DISABLED
#include <unordered_set>

template<typename T, typename H=HasherObject<T>> class HashSet
{
	typedef std::unordered_set<T, H> D;
public:
	explicit HashSet(size_t size=0) { if(size!=0) Reserve(size); }
	HashSet(ArrayRange<const T> values) { Insert(values); }
	HashSet(const Set<T>& rhs) { Insert(rhs); }

	void Insert(const T& value) { data.insert(value); }
	void Insert(ArrayRange<const T> values) { data.insert(values.begin(), values.end()); }
	void Insert(const Set<T>& set) { data.insert(set.begin(), set.end()); }

	void Remove(const T& val)
	{
		auto it=Find(val);
		if(it!=end()) data.erase(it);
	}

	bool operator==(null_t) const { return data.empty(); }
	bool operator!=(null_t) const { return !operator==(null); }

	bool Contains(const T& v) const { return Find(v)!=end(); }

	void Reserve(size_t newSize) { data.reserve(newSize); }

	typename D::iterator Find(const T& v) { return data.find(v); }
	typename D::const_iterator Find(const T& v) const { return data.find(v); }

	typename D::iterator begin() { return data.begin(); }
	typename D::const_iterator begin() const { return data.begin(); }
	typename D::iterator end() { return data.end(); }
	typename D::const_iterator end() const { return data.end(); }

private:
	D data;
};
#endif

}