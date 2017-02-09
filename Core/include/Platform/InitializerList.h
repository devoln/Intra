#pragma once

#include "Platform/CppFeatures.h"
#include "Platform/FundamentalTypes.h"

#ifdef _MSC_VER

#ifndef _INITIALIZER_LIST_
#define _INITIALIZER_LIST_

namespace std {

template<class T> class initializer_list
{
public:
	typedef T value_type;
	typedef const T& reference;
	typedef const T& const_reference;
	typedef size_t size_type;

	typedef const T* iterator;
	typedef const T* const_iterator;

	constexpr initializer_list() throw(): _First(nullptr), _Last(nullptr) {}
	constexpr initializer_list(const T* first, const T* last) throw(): _First(first), _Last(last) {}

	constexpr const T* begin() const throw() {return _First;}
	constexpr const T* end() const throw() {return _Last;}
	constexpr size_t size() const throw() {return size_t(_Last-_First);}

private:
	const T* _First;
	const T* _Last;
};

template<class T> inline constexpr const T* begin(initializer_list<T> list) throw() {return list.begin();}
template<class T> inline constexpr const T* end(initializer_list<T> list) throw() {return list.end();}

}

#endif

#else

#include <initializer_list>

#endif

namespace Intra {

template<typename T> using InitializerList = std::initializer_list<T>;

}
