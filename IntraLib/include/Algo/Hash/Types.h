#pragma once

#include "Core/Core.h"

namespace Intra { namespace Algo {

struct hash128
{
	hash128() = default;
	constexpr hash128(ulong64 _h1, ulong64 _h2) : h1(_h1), h2(_h2) {}
	constexpr bool operator==(const hash128& rhs) const {return h1==rhs.h1 && h2==rhs.h2;}
	constexpr bool operator!=(const hash128& rhs) const {return !operator==(rhs);}

	ulong64 h1, h2;
};

}}
