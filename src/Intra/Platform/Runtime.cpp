#include <Intra/Core.h>
#include <Intra/Allocator.h>

#if INTRA_UNIFIED_MAIN // define this to be able to just define main function on any platform (instead of WinMain or android_main)
#ifdef __ANDROID__
struct android_app;
namespace Intra {
android_app* gGlobalAndroidApp = nullptr;
}
extern "C" int main(int argc, const char* argv[]);
void android_main(struct android_app* state)
{
	Intra::gGlobalAndroidApp = state;
	const char* argv[] = {"program"};
	main(1, argv);
}
#elif defined(_WIN32)
#pragma comment(linker, "/ENTRY:mainCRTStartup")
extern "C" { // the pragma above doesn't work with Crinkler, this is a workaround
	void mainCRTStartup();
	void WinMainCRTStartup() {mainCRTStartup();}
}
#endif
#endif

namespace Intra { INTRA_BEGIN

inline size_t mallocGoodSize(size_t n)
{
	constexpr size_t
#ifdef _WIN32
		// these values were measured on Windows 11 22H2 by minimizing minimum distance between allocated addresses
		// Works well for both segment heap
		minimumAllocationBytes = sizeof(void*) * 2, // for segment heap; for standard heap the value is 8 but 16 on x86 also works well
		minimumGranularity = sizeof(void*) * 2,
#ifdef INTRA_OPTIMIZE_FOR_SEGMENT_HEAP // best values for segment heap
		smallAllocationShift = 0,
		largeAllocationShift = 0,
#else // NT Heap
		smallAllocationShift = 8, // for NT heap; for segment heap this is very inefficient, the value must be 0, otherwise we waste 8 bytes on every allocation
		largeAllocationShift = 80, // also not bad for segment heap
#endif
		largeGranularity = 4096,
		largeAllocationThreshold = 1 << 17, //for segment heap; the value for NT heap is larger: 0xFE00 * minimumGranularity
#else // these values were measured on x86/x64 Linux (glibc) using malloc_usable_size
		minimumAllocationBytes = 3 * sizeof(void*),
		minimumGranularity = 16,
		smallAllocationShift = sizeof(void*),
		largeGranularity = 4096,
		largeAllocationShift = 24,
		largeAllocationThreshold = sizeof(void*) == 8? 3 << 16: 1 << 17,
#endif
		smallOffset = minimumGranularity - 1 + smallAllocationShift,
		smallMask = ~(minimumGranularity - 1),
		largeOffset = largeGranularity - 1 + largeAllocationShift,
		largeMask = ~(largeGranularity - 1);

	if constexpr(((1 + smallOffset) & smallMask) - smallAllocationShift < minimumAllocationBytes)
		if(n < minimumAllocationBytes) n = minimumAllocationBytes;

	return n < largeAllocationThreshold?
		((n + smallOffset) & smallMask) - smallAllocationShift:
		((n + largeOffset) & largeMask) - largeAllocationShift;
}
}

RawAllocResult PlatformTunedByteAllocator(RawAllocParams params)
{
	auto& p = params.ByteAllocParams;
	INTRA_PRECONDITION(p.NumElements < size_t() - 32);
	const auto actualNumBytes = mallocGoodSize(p.NumElements);
	numBytes = actualNumBytes;
	char* res = nullptr;
#ifdef _WIN32
	// In experiments allocations above this threshold were never expandable on Windows 11:
	//  neither with NT heap nor with segment heap (its threshold is even lower - 128 KB)
	if(p.ExistingMemoryBlock.Begin && numBytes < 0xFE00 * sizeof(void*) * 2)
	{
#ifdef INTRA_DEBUG_ABI
		const auto prevAllocSize = p.ValueInitialize? z_D::_msize(prevAllocation): 0;
		res = static_cast<char*>(z_D::_expand(prevAllocation, actualNumBytes));
		if(p.ValueInitialize) memset(res + prevAllocSize, 0, numBytes - prevAllocSize);
#else
		res = static_cast<char*>(p.ValueInitialize?
			HeapReAlloc(reinterpret_cast<void*>(z_D::_get_heap_handle()), 0x18, prevAllocation, actualNumBytes): // HEAP_ZERO_MEMORY|HEAP_REALLOC_IN_PLACE_ONLY
			z_D::_expand(prevAllocation, actualNumBytes));
#endif
	}
	if(res) return res;

	// Manual malloc+free allows to copy only necessary part of data.
	// Also malloc+free is faster than realloc on Windows
	const bool allocZero = p.ValueInitialize && actualNumBytes >= p.NumPrevElementsToKeep * 3;
	res = static_cast<char*>(allocZero? z_D::calloc(actualNumBytes, 1): z_D::malloc(actualNumBytes));
	if(!res) return nullptr;
	if(prevAllocation)
	{
		if(!params.CustomRelocateFunc) z_D::memcpy(res, prevAllocation, p.NumPrevElementsToKeep);
		else params.CustomRelocateFunc(res, prevAllocation, p.NumPrevElementsToKeep);
		z_D::free(prevAllocation);
	}
	if(p.ValueInitialize && !allocZero)
		z_D::memset(res + p.NumPrevElementsToKeep, 0, actualNumBytes - p.NumPrevElementsToKeep);
#else
	// Non-null value customRelocateFunc prevents using realloc on most platforms
#if !defined(__wasm__) && !defined(__EMSCRIPTEN__) // calloc is slower on this platform so realloc is probably a preferred method
	// Assume calloc performs the same as malloc and memset is twice as fast as memcpy.
	// Also take into account probability of growing with realloc.
	const bool useRealloc = !customRelocateFunc && (!p.ValueInitialize || actualNumBytes / 4 < p.NumPrevElementsToKeep);
#else
	const bool useRealloc = !customRelocateFunc; // TODO: Emscripten may have realloc_in_place which may be useful for non-null customRelocateFunc
#endif
	if(useRealloc)
	{
		res = static_cast<char*>(realloc(prevAllocation, actualNumBytes));
		if(p.ValueInitialize) z_D::memset(res + p.NumPrevElementsToKeep, 0, actualNumBytes - p.NumPrevElementsToKeep);
	}
	else
	{
		res = static_cast<char*>(p.ValueInitialize? z_D::calloc(actualNumBytes, 1): z_D::malloc(actualNumBytes));
		if(!customRelocateFunc) z_D::memcpy(res, prevAllocation, p.NumPrevElementsToKeep);
		else customRelocateFunc(res, prevAllocation, p.NumPrevElementsToKeep);
	}
#endif
	return res;
}

} INTRA_END
