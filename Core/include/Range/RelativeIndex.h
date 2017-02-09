#pragma once

#include "Platform/FundamentalTypes.h"
#include "Platform/CppWarnings.h"
#include "Platform/CppFeatures.h"

namespace Intra { namespace Range {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

struct RelativeIndex
{
	enum: size_t {MaxMultiplyer = size_t(-1)};

	constexpr RelativeIndex(size_t plusValue, size_t sizeMultiplyer=0):
		multiplyer(sizeMultiplyer), plus_value(intptr(plusValue)) {}

	constexpr RelativeIndex(const RelativeIndex& rhs) = default;

	constexpr RelativeIndex operator/(intptr divisor) const
	{return RelativeIndex(size_t(plus_value/divisor), multiplyer/size_t(divisor));}

	constexpr RelativeIndex operator+(intptr value) const
	{return RelativeIndex(size_t(plus_value+value), multiplyer);}

	constexpr RelativeIndex operator+(RelativeIndex rhs) const
	{return RelativeIndex(size_t(plus_value+rhs.plus_value), multiplyer+rhs.multiplyer);}

	constexpr RelativeIndex operator-(intptr value) const
	{return RelativeIndex(size_t(plus_value-value), multiplyer);}

	constexpr RelativeIndex operator-(RelativeIndex rhs) const
	{return RelativeIndex(size_t(plus_value-rhs.plus_value), multiplyer-rhs.multiplyer);}

	friend RelativeIndex operator+(intptr value, RelativeIndex pos) {return pos+value;}
	friend RelativeIndex operator-(intptr value, RelativeIndex pos) {return RelativeIndex(0)-pos+value;}

	RelativeIndex& operator=(const RelativeIndex& rhs) = default;

	forceinline size_t GetRealIndex(size_t containerItemCount) const
	{
		if(multiplyer==0) return size_t(plus_value);
		if(multiplyer==MaxMultiplyer) return size_t(intptr(containerItemCount)+plus_value);
		return size_t(intptr(ulong64(multiplyer)*containerItemCount/MaxMultiplyer)+plus_value);
	}

private:
	size_t multiplyer;
	intptr plus_value;
};

struct RelativeIndexEnd: RelativeIndex
{
	constexpr RelativeIndexEnd():
		RelativeIndex(0, RelativeIndex::MaxMultiplyer) {}
};
constexpr const RelativeIndexEnd $;

INTRA_WARNING_POP

}

using Range::$;

}
