#pragma once

#include "IntraX/Container/Sequential/Array.h"

INTRA_BEGIN


template<typename T> class LinearSet
{
public:
	LinearSet() = default;
	LinearSet(CSpan<T> values) {Insert(values);}
	LinearSet(const LinearSet<T>& rhs) {Insert(rhs);}

	void Insert(T&& value) {if(!mData.Contains(value)) mData.AddLast(Move(value));}
	void Insert(const T& value) {if(!mData.Contains(value)) mData.AddLast(value);}
	void Insert(CSpan<T> values) {for(auto& v: values) Insert(v);}

	void Remove(const T& val)
	{
		auto i = mData.Find(val);
		if(i!=-1) mData.RemoveUnordered(i);
	}

	bool operator==(decltype(null)) const {return mData==null;}
	bool operator!=(decltype(null)) const {return !operator==(null);}

	bool Contains(const T& v) const {return mData.AsConstRange().Contains(v);}

	index_t Find(const T& v) const {return mData.Find(v);}

	T& operator[](size_t i) {return mData[i];}
	const T& operator[](size_t i) const {return mData[i];}

	Span<T> AsRange() {return mData.AsRange();}
	CSpan<T> AsRange() const {return mData.AsConstRange();}
	CSpan<T> AsConstRange() const {return mData.AsConstRange();}
	operator Span<T>() {return AsRange();}
	operator CSpan<T>() const {return AsConstRange();}

	typedef T value_type;

	T* begin() {return mData.begin();}
	const T* begin() const {return mData.begin();}
	T* end() {return mData.end();}
	const T* end() const {return mData.end();}

private:
	Array<T> mData;
};

template<typename T> using Set = LinearSet<T>;

}
