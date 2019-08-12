#pragma once

#include "Memory/Allocator/Global.h"
#include "ForwardDecls.h"

INTRA_BEGIN
namespace Container {

template<typename T, class Allocator = Memory::GlobalHeapType> class BList;
template<typename K, typename V, class Allocator = Memory::GlobalHeapType> class HashMap;

}

using Container::HashMap;
using Container::BList;

}
