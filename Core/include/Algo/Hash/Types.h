#pragma once

#include "Platform/CppFeatures.h"
#include "Platform/CppWarnings.h"
#include "Platform/FundamentalTypes.h"

namespace Intra { namespace Algo {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

struct hash128
{
	hash128() = default;
	constexpr hash128(ulong64 _h1, ulong64 _h2) : h1(_h1), h2(_h2) {}
	constexpr bool operator==(const hash128& rhs) const {return h1==rhs.h1 && h2==rhs.h2;}
	constexpr bool operator!=(const hash128& rhs) const {return !operator==(rhs);}

	ulong64 h1, h2;
};

INTRA_WARNING_POP

}}
