#pragma once

#include "Container/Sequential/Array.h"

INTRA_BEGIN


template<typename T> class IntervalAllocator
{
public:
	struct Interval
	{
		T Begin, End;
		decltype(T()-T()) Length() const {return End-Begin;}
	};


	IntervalAllocator(): used_length(0), has_holes(false) {}

	const Interval& Add(T length)
	{
		ranges.AddLast({used_length, used_length+length});
		used_length += length;
		compact_length += length;
		return ranges.Last();
	}

	void Remove(size_t i)
	{
		auto len = ranges[i].Length();
		if(ranges[i].End==used_length)
			used_length -= len;
		compact_length -= len;
		ranges.Remove(i);
	}

	void RemoveUnordered(size_t i)
	{
		auto len = ranges[i].Length();
		if(ranges[i].End==used_length)
			used_length -= len;
		compact_length -= len;
		ranges.RemoveUnordered(i);
	}

	const Interval& Get(size_t i) const {return ranges[i];}
	size_t Count() const {return ranges.Count();}
	T UsedLength() const {return used_length;}
	T CompactLength() const {return compact_length;}
	CSpan<Interval> GetRanges() const {return ranges;}

	Array<Interval> Compactify()
	{
		Array<Interval> oldIntervals = Move(ranges);
		ranges.Reserve(oldIntervals.Count());
		T len=0;
		for(auto&& oldInterval: oldIntervals)
		{
			ranges.AddLast({len, used_length+oldInterval.length});
			len += oldInterval.length;
		}
		INTRA_DEBUG_ASSERT(len<=used_length);
		has_holes=false;
		return oldIntervals;
	}

private:
	Array<Interval> ranges;
	T compact_length, used_length;
	bool has_holes;
};

}
