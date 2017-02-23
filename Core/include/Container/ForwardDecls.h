#pragma once

#include "Memory/Allocator.hh"

namespace Intra { namespace Container {

template<typename T> class Array;
template<typename T, class Allocator = Memory::GlobalHeapType> class BList;
template<typename K, typename V> class LinearMap;
template<typename K, typename V, class Allocator = Memory::GlobalHeapType> class HashMap;

template<typename Char> class GenericString;
typedef GenericString<char> String;
typedef GenericString<wchar> WString;
typedef GenericString<dchar> DString;

}

using Container::Array;
using Container::HashMap;
using Container::LinearMap;
using Container::BList;

using Container::GenericString;
using Container::String;
using Container::WString;
using Container::DString;

}
