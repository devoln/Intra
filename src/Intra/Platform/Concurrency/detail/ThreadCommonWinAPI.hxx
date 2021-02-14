#pragma once

#include "IntraX/Container/Sequential/String.h"

#define WIN32_LEAN_AND_MEAN
INTRA_WARNING_PUSH
INTRA_IGNORE_WARNS_MSVC(4668)
#include <Windows.h>
#undef Yield
INTRA_WARNING_POP

#ifdef _MSC_VER
namespace Intra { INTRA_BEGIN
namespace detail {



}
} INTRA_END
#endif
