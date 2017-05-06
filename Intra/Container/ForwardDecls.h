#pragma once

#include "Cpp/Fundamental.h"

namespace Intra { namespace Container {

template<typename T> class Array;
template<typename K, typename V> class LinearMap;
template<typename Char> class GenericString;
typedef GenericString<char> String;
typedef GenericString<wchar> WString;
typedef GenericString<dchar> DString;

}

using Container::Array;
using Container::LinearMap;
using Container::GenericString;
using Container::String;
using Container::WString;
using Container::DString;

}
