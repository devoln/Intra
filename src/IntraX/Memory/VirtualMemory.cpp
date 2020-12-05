#include "IntraX/Memory/VirtualMemory.h"
#include "Intra/Type.h"
#include "Intra/Assert.h"

#ifdef _WIN32

INTRA_BEGIN
namespace z_D { extern "C" {
__declspec(dllimport) void* __stdcall VirtualAlloc(void* lpAddress, size_t dwSize, uint32 flAllocationType, uint32 flProtect);
__declspec(dllimport) int __stdcall VirtualFree(void* lpAddress, size_t dwSize, uint32 dwFreeType);
struct SYSTEM_INFO
{
	uint16 wProcessorArchitecture, wReserved;
	uint32 dwPageSize;
	void* lpMinimumApplicationAddress;
	void* lpMaximumApplicationAddress;
	size_t dwActiveProcessorMask;
	uint32 dwNumberOfProcessors, dwProcessorType, dwAllocationGranularity;
	uint16 wProcessorLevel, wProcessorRevision;
}
__declspec(dllimport) void __stdcall GetSystemInfo(SYSTEM_INFO* lpSystemInfo);
}

enum: uint8 {WPAGE_NOACCESS = 1, WPAGE_READONLY = 2, WPAGE_READWRITE = 4, WPAGE_WRITECOPY = 8,
	WPAGE_EXECUTE = 0x10, WPAGE_EXECUTE_READ = 0x20, WPAGE_EXECUTE_READWRITE = 0x40, WPAGE_EXECUTE_WRITECOPY = 0x80}
enum: uint16 {WMEM_COMMIT = 0x1000, WMEM_RESERVE = 0x2000, WMEM_DECOMMIT = 0x4000, WMEM_RELEASE = 0x8000};
constexpr uint8 accessTable[] =
{
	z_D::WPAGE_NOACCESS, z_D::WPAGE_READONLY, z_D::WPAGE_WRITECOPY, z_D::WPAGE_READWRITE,
	z_D::WPAGE_EXECUTE, z_D::WPAGE_EXECUTE_READ, z_D::WPAGE_EXECUTE_WRITECOPY, z_D::WPAGE_EXECUTE_READWRITE
};
static_assert(LengthOf(accessTable) == index_t(Access::End));

}

AnyPtr VirtualAlloc(size_t bytes, Access access)
{
	return z_D::VirtualAlloc(null, bytes, uint32(access == Access::None? z_D::WMEM_RESERVE: z_D::WMEM_COMMIT), z_D::accessTable[uint8(access)];
}

void VirtualFree(void* ptr, size_t size)
{
	(void)size;
	z_D::VirtualFree(ptr, 0, z_D::WMEM_RELEASE);
}

void VirtualCommit(void* ptr, size_t bytes, Access access)
{
	INTRA_PRECONDITION(ptr != null);
	if(access != Access::None) z_D::VirtualAlloc(ptr, bytes, z_D::WMEM_COMMIT, z_D::accessTable[uint8(access)]);
	else z_D::VirtualFree(ptr, bytes, z_D::WMEM_DECOMMIT);
}

size_t VirtualMemoryPageSize()
{
	static const uint32 pageSize = []() {
		z_D::SYSTEM_INFO si;
		z_D::GetSystemInfo(&si);
		return si.dwPageSize;
	}();
	return pageSize;
}

INTRA_END

#elif defined(__unix__)
#include <unistd.h>
#include <sys/mman.h>

INTRA_BEGIN
namespace z_D {
extern "C"
{
	//TODO: declare functions
}
constexpr int accessTable[] =
{
	PROT_NONE, PROT_READ, PROT_WRITE, PROT_READ|PROT_WRITE,
	PROT_EXEC, PROT_EXEC|PROT_READ, PROT_EXEC|PROT_WRITE, PROT_EXEC|PROT_READ|PROT_WRITE
};
static_assert(LengthOf(accessTable) == index_t(Access::End));
}

AnyPtr VirtualAlloc(size_t bytes, Access access)
{
    void* ptr = mmap(null, bytes, z_D::accessTable[uint8(access)], MAP_ANON|(access == Access::None? MAP_PRIVATE: MAP_SHARED), -1, 0);
    msync(ptr, bytes, MS_SYNC|MS_INVALIDATE);
    return ptr;
}

void VirtualCommit(void* ptr, size_t bytes, Access access)
{
    mmap(ptr, bytes, z_D::accessTable[uint8(access)], MAP_FIXED|MAP_ANON|(access == Access::None? MAP_PRIVATE: MAP_SHARED), -1, 0);
    msync(ptr, bytes, MS_SYNC|MS_INVALIDATE);
}

void VirtualFree(void* ptr, size_t size)
{
    msync(ptr, size, MS_SYNC);
    munmap(ptr, size);
}

size_t VirtualMemoryPageSize()
{
	static const long pageSize = sysconf(_SC_PAGESIZE);
	return size_t(pageSize);
}
INTRA_END
#else
#include <stdlib.h>

INTRA_BEGIN
//TODO: align allocated memory
AnyPtr VirtualAlloc(size_t bytes, Access access)
{(void)bytes; (void)commit; (void)access; return z_D::malloc(bytes);}

void VirtualFree(void* ptr, size_t size) {z_D::free(ptr);}

void VirtualCommit(void* ptr, size_t bytes, Access access)
{(void)ptr; (void)bytes; (void)access;}

size_t VirtualMemoryPageSize() {return sizeof(void*)*2;}
INTRA_END

#endif
