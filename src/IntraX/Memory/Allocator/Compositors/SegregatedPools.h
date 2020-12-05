#pragma once

#include "IntraX/Memory/Allocator/Basic/Pool.h"
#include "IntraX/Memory/Allocator/System.h"
#include "Intra/Range/Span.h"

INTRA_BEGIN
template<size_t N> struct LogSizes
{
	enum: size_t {NumBins = N};

    static size_t GetSizeClass(size_t size)
    {
		auto Log = Log2i(unsigned(size));
        return size_t(Log>5? Log-5u: 0u);
    }
 
    static size_t GetSizeClassMaxSize(size_t sizeClass)
    {return size_t(32u << sizeClass);}
};

template<typename FA, typename Traits = LogSizes<7>>
struct ASegregatedPools: FA
{
	ASegregatedPools(FA&& fallback, Span<APool> pools): FA(Move(fallback))
	{
		mAlignment = FA::GetAlignment();
		for(auto& allocator: mPools)
		{
			if(!pools.Empty())
			{
				allocator = pools.First();
				pools.PopFirst();
			}
			mAlignment = Min(mAlignment, allocator.GetAlignment());
		}
	}

	ASegregatedPools(ASegregatedPools&& rhs):
		FA(Move(rhs)),
		mPools(Move(rhs.mPools)),
		mAlignment(rhs.mAlignment) {}

	size_t GetSizeClass(size_t size)
	{
		if(size>Traits::GetSizeClassMaxSize(Traits::NumBins-1))
			return Traits::NumBins;
		return Traits::GetSizeClass(size);
	}


	size_t GetAlignment() const {return mAlignment;}

    AnyPtr Allocate(size_t& bytes, SourceInfo sourceInfo = SourceInfo())
    {
        size_t sizeClass = GetSizeClass(bytes);
 
        if(sizeClass >= Traits::NumBins)
            return FA::Allocate(bytes, sourceInfo);
        return mPools[sizeClass].Allocate(bytes, sourceInfo);
    }

	void Free(void* ptr, size_t size)
	{
		if(ptr==null) return;
		size_t elementSizeClass = GetSizeClass(size);
		if(elementSizeClass >= Traits::NumBins) FA::Free(ptr, size);
		else mPools[elementSizeClass].Free(ptr, size);
	}

private:
	APool mPools[Traits::NumBins];
	size_t mAlignment;
};
INTRA_END
