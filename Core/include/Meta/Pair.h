#pragma once

#include "Platform/CppFeatures.h"
#include "Platform/CppWarnings.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_COPY_MOVE_CONSTRUCT_IMPLICITLY_DELETED

#ifdef _MSC_VER
#pragma warning(disable: 4512 4626)

#if _MSC_VER>=1900
#pragma warning(disable: 5026 5027)
#endif

#endif

namespace Intra { namespace Meta {

INTRA_DEFINE_EXPRESSION_CHECKER(Has_first_second, (Val<T>().first, Val<T>().second));
INTRA_DEFINE_EXPRESSION_CHECKER(HasKeyValue, (Val<T>().Key, Val<T>().Value));

template<typename T1, typename T2> struct Pair
{
	T1 first;
	T2 second;
};

template<typename K, typename V> struct KeyValuePair
{
	K Key;
	V Value;

	KeyValuePair(): Key(), Value() {};
	template<typename K1, typename V1> KeyValuePair(K1&& key, V1&& value):
		Key(Meta::Forward<K1>(key)), Value(Meta::Forward<V1>(value)) {}

	operator KeyValuePair<const K, V>() const
	{return *reinterpret_cast<KeyValuePair<const K, V>*>(this);}

	operator Meta::Pair<K,V>&() {return *reinterpret_cast<Meta::Pair<K,V>*>(this);}
	operator const Meta::Pair<K,V>&() const {return *reinterpret_cast<Meta::Pair<K,V>*>(this);}

};



template<typename T1, typename T2> Meta::Pair<T1, T2> PairL(T1&& first, T2&& second)
{return {Meta::Forward<T1>(first), Meta::Forward<T2>(second)};}

template<typename K, typename V> KeyValuePair<K, V> KVPairL(K&& key, V&& value)
{return {Meta::Forward<K>(key), Meta::Forward<V>(value)};}

}

using Meta::Pair;
using Meta::KeyValuePair;

}

INTRA_WARNING_POP

