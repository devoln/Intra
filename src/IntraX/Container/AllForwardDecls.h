#pragma once

#include "IntraX/Memory/Allocator/Global.h"
#include "ForwardDecls.h"

namespace Intra { INTRA_BEGIN
template<typename T, class Allocator = GlobalHeapType> class BList;
template<typename K, typename V, class Allocator = GlobalHeapType> class HashMap;
} INTRA_END
