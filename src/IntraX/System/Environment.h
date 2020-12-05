#pragma once

#include "Intra/Range/Span.h"
#include "Intra/Container/Tuple.h"
#include "Intra/Container/Optional.h"
#include "IntraX/Utils/FixedArray.h"
#include "IntraX/Container/Sequential/String.h"

INTRA_BEGIN
struct TEnvironment
{
	TEnvironment(const TEnvironment&) = delete;
	TEnvironment(TEnvironment&&) = delete;
	TEnvironment& operator=(const TEnvironment&) = delete;
	TEnvironment& operator=(TEnvironment&&) = delete;

	CSpan<StringView> CommandLine;

	struct VarSet
	{
		VarSet(FixedArray<char>&& data, Index count):
			mData(Move(data)), mCount(count) {}

		VarSet(const VarSet&) = delete;
		VarSet(VarSet&& rhs) = default;

		VarSet& operator=(const VarSet&) = delete;

		VarSet& operator=(VarSet&& rhs) noexcept
		{
			mData = Move(rhs.mData);
			mCount = rhs.mCount;
			return *this;
		}
		
		CSpan<Tuple<StringView, StringView>> AsRange() const;
		index_t Length() const {return mCount;}
	private:
		FixedArray<char> mData;
		index_t mCount;
	};

	TEnvironment();
	VarSet Variables() const;
	Optional<String> operator[](StringView var) const;
};

extern const TEnvironment Environment;
INTRA_END
