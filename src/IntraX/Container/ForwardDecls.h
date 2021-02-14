#pragma once

#include "Intra/Core.h"

// TODO: It is temporary until Core become required feature by this library
#define INTRA_CONTAINER_STL_FORWARD_COMPATIBILITY

namespace Intra { INTRA_BEGIN
template<typename T> class ArrayList;
template<typename K, typename V> class LinearMap;
template<typename Char> class GenericString;
typedef GenericString<char> String;
} INTRA_END

