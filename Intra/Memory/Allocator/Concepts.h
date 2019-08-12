#pragma once

#include "Core/Type.h"

INTRA_BEGIN
inline namespace Memory {

INTRA_DEFINE_CONCEPT_REQUIRES(CHasGetAllocationSize,\
	static_cast<size_t>(Val<T>().GetAllocationSize(static_cast<void*>(null))));

}
INTRA_END
