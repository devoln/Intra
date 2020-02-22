#pragma once

#include "Core/Range/Span.h"
#include "Utils/FixedArray.h"

#include "Core/Tuple.h"

#include "Container/Sequential/String.h"

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
		VarSet(FixedArray<char>&& data, size_t count):
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
		size_t mCount;
	};

	TEnvironment();
	VarSet Variables() const;
	String Get(StringView var, bool* oExists) const;
	
	String operator[](StringView var) const {return Get(var, null);}

	String Get(StringView var, StringView fallbackValue) const
	{
		bool exists;
		String result = Get(var, &exists);
		if(exists) return result;
		return fallbackValue;
	}
};

extern const TEnvironment Environment;
INTRA_END
