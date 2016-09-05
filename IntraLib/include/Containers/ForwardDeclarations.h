#pragma once

#include "Memory/Allocator.h"

namespace Intra {

template<typename T, class Allocator = Memory::GlobalHeapType> class Array;
template<typename T, class Allocator = Memory::GlobalHeapType> class BList;
template<typename K, typename V, class Allocator = Memory::GlobalHeapType> class LinearMap;
template<typename K, typename V, class Allocator = Memory::GlobalHeapType> class HashMap;

template<typename Char, class Allocator = Memory::SizedHeapType> class GenericString;
typedef GenericString<char> String;
typedef GenericString<wchar> WString;
typedef GenericString<dchar> DString;

template<typename Char> struct GenericStringView;
typedef GenericStringView<char> StringView;
typedef GenericStringView<wchar> WStringView;
typedef GenericStringView<dchar> DStringView;

}
