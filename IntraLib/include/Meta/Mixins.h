#pragma once

#include "Core/Core.h"

namespace Intra { namespace Meta {

template<typename T, typename PARENT=Meta::EmptyType> struct EqualComparableMixin: PARENT
{
private:
	forceinline const T& me() const {return *static_cast<const T*>(this);}
public:
	template<typename U> forceinline bool operator!=(const U& rhs) const {return !(me()==rhs);}
};

template<typename T, typename PARENT=Meta::EmptyType> struct SameComparableMixin: EqualComparableMixin<T, PARENT>
{
private:
	forceinline const T& me() const {return *static_cast<const T*>(this);}
public:
	template<typename U> forceinline bool operator>=(const U& rhs) const {return !(me()<rhs);}
	template<typename U> forceinline bool operator>(const U& rhs) const {return rhs<me();}
	template<typename U> forceinline bool operator<=(const U& rhs) const {return !(rhs<me());}
};

template<typename T, typename PARENT=Meta::EmptyType> struct ComparableMixin: EqualComparableMixin<T, PARENT>
{
private:
	forceinline const T& me() const {return *static_cast<const T*>(this);}
public:
	forceinline bool operator>(const T& rhs) const {return rhs<me();}
	template<typename U> forceinline bool operator>=(const U& rhs) const {return !(me()<rhs);}
	template<typename U> forceinline bool operator<=(const U& rhs) const {return !(me()>rhs);}
};

template<typename U, typename T, typename PARENT> forceinline Meta::EnableIf<
	!TypeEquals<U, T>::_,
bool> operator==(const U& lhs, const EqualComparableMixin<T, PARENT>& rhs)
{
    return static_cast<const T&>(rhs)==lhs;
}

template<typename U, typename T, typename PARENT> forceinline Meta::EnableIf<
	!TypeEquals<U, T>::_,
bool> operator!=(const U& lhs, const EqualComparableMixin<T, PARENT>& rhs)
{
    return !(static_cast<const T&>(rhs)==lhs);
}

template<typename U, typename T, typename PARENT> forceinline Meta::EnableIf<
	!TypeEquals<U, T>::_,
bool> operator<(const U& lhs, const ComparableMixin<T, PARENT>& rhs)
{
    return static_cast<const T&>(rhs)>lhs;
}

template<typename U, typename T, typename PARENT> forceinline Meta::EnableIf<
	!TypeEquals<U, T>::_,
bool> operator>(const U& lhs, const ComparableMixin<T, PARENT>& rhs)
{
    return static_cast<const T&>(rhs)<lhs;
}

template<typename U, typename T, typename PARENT> forceinline Meta::EnableIf<
	!TypeEquals<U, T>::_,
bool> operator<=(const U& lhs, const ComparableMixin<T, PARENT>& rhs)
{
    return static_cast<const T&>(rhs)>=lhs;
}

template<typename U, typename T, typename PARENT> forceinline Meta::EnableIf<
	!TypeEquals<U, T>::_,
bool> operator>=(const U& lhs, const ComparableMixin<T, PARENT>& rhs)
{
    return static_cast<const T&>(rhs)<=lhs;
}


}}
