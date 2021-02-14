#pragma once

#include "IntraX/Container/Sequential/Array.h"

namespace Intra { INTRA_BEGIN


template<typename T> class LinearSet
{
public:
	LinearSet() = default;
	LinearSet(Span<const T> values) {Insert(values);}
	LinearSet(const LinearSet<T>& rhs) {Insert(rhs);}

	void Insert(T&& value) {if(!mData.Contains(value)) mData.AddLast(Move(value));}
	void Insert(const T& value) {if(!mData.Contains(value)) mData.AddLast(value);}
	void Insert(Span<const T> values) {for(auto& v: values) Insert(v);}

	void Remove(const T& val)
	{
		auto i = mData.Find(val);
		if(i!=-1) mData.RemoveUnordered(i);
	}

	bool operator==(decltype(nullptr)) const {return mData==nullptr;}
	bool operator!=(decltype(nullptr)) const {return !operator==(nullptr);}

	bool Contains(const T& v) const {return mData.AsConstRange().Contains(v);}

	index_t Find(const T& v) const {return mData.Find(v);}

	T& operator[](size_t i) {return mData[i];}
	const T& operator[](size_t i) const {return mData[i];}

	Span<T> AsRange() {return mData.AsRange();}
	Span<const T> AsRange() const {return mData.AsConstRange();}
	Span<const T> AsConstRange() const {return mData.AsConstRange();}
	operator Span<T>() {return AsRange();}
	operator Span<const T>() const {return AsConstRange();}

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
