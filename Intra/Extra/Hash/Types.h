#pragma once

#include "Core/Core.h"

INTRA_BEGIN
namespace Hash {

struct hash128
{
	hash128() = default;
	constexpr hash128(uint64 _h1, uint64 _h2) : h1(_h1), h2(_h2) {}
	constexpr bool operator==(const hash128& rhs) const {return h1 == rhs.h1 && h2 == rhs.h2;}
	constexpr bool operator!=(const hash128& rhs) const {return !operator==(rhs);}

	uint64 h1, h2;
};

}
INTRA_END
