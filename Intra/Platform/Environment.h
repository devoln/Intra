#pragma once

#include "Utils/Span.h"
#include "Utils/FixedArray.h"

#include "Meta/Pair.h"

namespace Intra {

struct TEnvironment
{
	const CSpan<StringView> Arguments;

	struct VarSet
	{
		CSpan<const KeyValuePair<StringView, StringView>> AsRange() const;
		size_t Length() const {return count;}
	private:
		FixedArray<char> data;
		size_t count;
	};

	TEnvironment();
	VarSet Variables();
} Environment;


extern const ;

}
