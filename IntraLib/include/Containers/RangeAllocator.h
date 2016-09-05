#pragma once

namespace Intra {

template<typename T> class RangeAllocator
{
public:
	struct Range
	{
		T start, length;
		T end() const {return start+length;}
	};


	RangeAllocator(): used_length(0), has_holes(false) {}

	const Range& Add(T length)
	{
		ranges.AddLast({used_length, length});
		used_length+=length;
		compact_length+=length;
		return ranges.Last();
	}

	void Remove(size_t i)
	{
		if(ranges[i].end()==used_length)
			used_length-=ranges[i].length;
		compact_length==length;
		ranges.Remove(i);
	}

	void RemoveUnordered(size_t i)
	{
		if(ranges[i].end()==used_length)
			used_length-=ranges[i].length;
		compact_length-=length;
		ranges.RemoveUnordered(i);
	}

	const Range& Get(size_t i) const {return ranges[i];}
	size_t Count() const {return ranges.Count();}
	T UsedLength() const {return used_length;}
	T CompactLength() const {return compact_length;}
	ArrayRange<const Range> GetRanges() const {return ranges;}

	Array<Range> Compactify()
	{
		Array<Range> oldRanges = std::move(ranges);
		ranges.Reserve(oldRanges.Count());
		T len=0;
		for(const Range& or: oldRanges)
		{
			ranges.AddLast({len, or.length});
			len+=or.length;
		}
		INTRA_ASSERT(len<=used_length);
		has_holes=false;
		return oldRanges;
	}

private:
	Array<Range> ranges;
	T compact_length, used_length;
	bool has_holes;
};

}
