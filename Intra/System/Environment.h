#pragma once



#include "Core/Range/Span.h"
#include "Utils/FixedArray.h"

#include "Core/Pair.h"

#include "Container/Sequential/String.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

INTRA_BEGIN
namespace System {

struct TEnvironment
{
	CSpan<StringView> CommandLine;

	struct VarSet
	{
		VarSet(FixedArray<char>&& data, size_t count):
			mData(Move(data)), mCount(count) {}

		VarSet(const VarSet&) = delete;

		VarSet(VarSet&& rhs):
			mData(Move(rhs.mData)), mCount(rhs.mCount) {}

		VarSet& operator=(const VarSet&) = delete;

		VarSet& operator=(VarSet&& rhs)
		{
			mData = Move(rhs.mData);
			mCount = rhs.mCount;
			return *this;
		}
		
		CSpan<KeyValuePair<StringView, StringView>> AsRange() const;
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

}
using System::Environment;

}

INTRA_WARNING_POP
