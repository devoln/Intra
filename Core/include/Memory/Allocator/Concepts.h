#pragma once

#include "Platform/FundamentalTypes.h"
#include "Meta/Type.h"

namespace Intra { namespace Memory {

INTRA_DEFINE_EXPRESSION_CHECKER(HasGetAllocationSize,\
	static_cast<size_t>(Meta::Val<T>().GetAllocationSize(static_cast<void*>(null))));

}}
