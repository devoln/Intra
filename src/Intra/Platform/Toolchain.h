#pragma once

#include <Intra/Core.h>

#define INTRA_VALIDATE_TOOLCHAIN

// This header includes most functions from C standard library and OS APIs that Intra may access.
// We avoid including any standard or OS headers on most popular platforms because:
//  1. We want to reduce compile time. Headers are bloated which increases compile time. Even smallest header may have a lot of indirect dependencies.
//  2. Crosscompile with a single Clang without requiring a separate toolchain for each target platform.
// To achieve this Intra prefers the following:
//  1. Using C/C++ standard and POSIX functions that present on most OS and runtimes. Emulate them if not present
//  2. Include necessary structs like stat, dirent and pthread primitives for the most popular OS and CPU arch combinations.
//   These are the biggest issues because for unsupported combinations we have to fallback to #include depending on toolchain.
//  3. Implement va_list for vsnprintf-like functions that Intra can optionally use in some configurations to reduce executable size.
//   This is a bit challnging on MSVC, other compilers have builtins for this that work on every platform.

// The corresponding source file (not implemented yet) will define stubs to generate import libraries that would allow crosscompilation with minimal toolchain.

#if defined(_MSC_VER) && defined(_DLL)
#define INTRA_CRTIMP INTRA_DLL_IMPORT
#else
#define INTRA_CRTIMP
#endif

#ifndef INTRA_PTHREADIMP
#define INTRA_PTHREADIMP // INTRA_DLL_IMPORT // uncomment for dynamic pthreads library on Windows
#endif

#if defined(_WIN32) && defined(__i386__)
#define INTRA_CRTDECL __cdecl
#define INTRA_WINAPI __stdcall
#else
#define INTRA_CRTDECL
#define INTRA_WINAPI
#endif

#define INTRA_CRTFUNC(...) INTRA_CRTIMP __VA_ARGS__ INTRA_CRTDECL
#define INTRA_WINFUNC(...) INTRA_DLL_IMPORT __VA_ARGS__ INTRA_WINAPI

#ifdef _WIN32
union _LARGE_INTEGER;
struct _FILETIME;
struct _TIME_ZONE_INFORMATION;
struct _RTL_CRITICAL_SECTION;
struct _RTL_SRWLOCK;
struct _RTL_CONDITION_VARIABLE;
struct _SYSTEMTIME;
struct _OVERLAPPED;
struct _GUID;
struct _SECURITY_ATTRIBUTES;
struct _WSANETWORKEVENTS;
struct WSAData;
struct _OVERLAPPED_ENTRY;
using HANDLE = void*;
#ifdef STRICT
using HINSTANCE = struct HINSTANCE__*;
#else
using HINSTANCE = void*;
#endif
using HMODULE = HINSTANCE;
#elif defined(__linux__)
struct itimerspec;
union epoll_data;
struct epoll_event;
#endif

struct timeval;
struct timespec;
struct timezone;
struct tm;

#ifdef _WIN32
using time_t = long long;
struct __crt_locale_pointers;
#else
using time_t = long;
using pid_t = int; // at least on Linux
struct iovec;
struct dirent;
struct DIR;
struct regex_t;
struct regmatch_t;
#ifdef __FreeBSD__
struct sf_hdtr;
#endif
#endif
struct sockaddr;
struct pollfd;
struct fd_set; // depends on the OS
struct in_addr;
struct addrinfo;

#ifndef INTRAZ_D_TOOLCHAIN_DECLARE_STRUCTS
#if defined(__linux__) && (defined(__amd64__) || defined(__i386__) || defined(__aarch64__) || defined(__arm__)) || defined(__APPLE__) && !defined(__i386__)
#define INTRAZ_D_TOOLCHAIN_DECLARE_STRUCTS 1
#else
#define INTRAZ_D_TOOLCHAIN_DECLARE_STRUCTS 0
#endif
#endif

#if !defined(_WIN32) && !INTRAZ_D_TOOLCHAIN_DECLARE_STRUCTS
// we can't define structs stat and dirent for every possible platform, so we do this only for most popular platforms
#include <sys/stat.h>
#include <dirent.h>
#endif

#if defined(_WIN32) || defined(__linux__) && (defined(__amd64__) || defined(__i386__) || defined(__aarch64__) || defined(__arm__) || defined(__mips__))
#define INTRAZ_D_TOOLCHAIN_DECLARE_PTHREAD

#ifdef _WIN32
using pthread_t = size_t;
using pthread_mutex_t = Intra::index_t;
using pthread_cond_t = Intra::index_t;
using pthread_rwlock_t = Intra::index_t;
using pthread_mutexattr_t = unsigned;
using pthread_condattr_t = int;
struct pthread_attr_t;
struct pthread_rwlockattr_t;
#elif defined(__ANDROID__)
using pthread_t = long;
struct pthread_mutex_t;
struct pthread_cond_t;
struct pthread_rwlock_t;
struct pthread_attr_t;
using pthread_mutexattr_t = long;
using pthread_condattr_t = long;
using pthread_rwlockattr_t = long;
#else // GLIBC
using pthread_t = unsigned long;
// TODO: to avoid conflicts when building with musl headers, we should detect them and use struct instead of union
union pthread_mutex_t;
union pthread_cond_t;
union pthread_rwlock_t;
union pthread_attr_t;
union pthread_mutexattr_t;
union pthread_condattr_t;
union pthread_rwlockattr_t;
#endif
#else
#include <pthread.h>
#endif


namespace Intra::z_D { extern "C" {
#if defined(_MSC_VER) && !defined(__clang__)
using va_list = char*;
void __cdecl __va_start(va_list*, ...);
#define INTRAZ_D_SLOTSIZEOF(t) ((sizeof(t) + sizeof(void*) - 1) & ~(sizeof(void*) - 1))
#define INTRAZ_D_APALIGN(t, ap) ((va_list(0) - (ap)) & (alignof(t) - 1))

#ifdef _M_X64
#define INTRAZ_D_VA_START(ap, v) __va_start(&ap, v)
#define INTRAZ_D_VA_ARG(ap, t) ((sizeof(t) > 8 || (sizeof(t) & (sizeof(t) - 1)))? **(t**)((ap += 8) - 8):  *(t*)((ap += 8) - 8))
#elif defined(_M_ARM64)
#define INTRAZ_D_VA_START(ap, v) __va_start(&ap, __builtin_addressof(v), INTRAZ_D_SLOTSIZEOF(v), alignof(v), __builtin_addressof(v))
#define INTRAZ_D_VA_ARG(ap, t) (sizeof(t) > 16? **(t**)((ap += 8) - 8):  *(t*)((ap += INTRAZ_D_SLOTSIZEOF(t) + INTRAZ_D_APALIGN(t, ap)) - INTRAZ_D_SLOTSIZEOF(t)))
#elif defined(_M_ARM)
#define INTRAZ_D_VA_START(ap, v) __va_start(&ap, __builtin_addressof(v), INTRAZ_D_SLOTSIZEOF(v), __builtin_addressof(v))
#define INTRAZ_D_VA_ARG(ap, t) (*(t*)((ap += INTRAZ_D_SLOTSIZEOF(t) + INTRAZ_D_APALIGN(t, ap)) - INTRAZ_D_SLOTSIZEOF(t)))
#elif defined(_M_IX86)
#define INTRAZ_D_VA_START(ap, v) ap = va_list(__builtin_addressof(v)) + INTRAZ_D_SLOTSIZEOF(v)
#define INTRAZ_D_VA_ARG(ap, t)     (*(t*)((ap += INTRAZ_D_SLOTSIZEOF(t)) - INTRAZ_D_SLOTSIZEOF(t)))
#endif
#define INTRAZ_D_VA_END(ap) ap = nullptr
#else
using va_list = __builtin_va_list;
#define INTRAZ_D_VA_START __builtin_va_start
#define INTRAZ_D_VA_ARG __builtin_va_arg
#define INTRAZ_D_VA_END __builtin_va_end
#endif

INTRA_CRTRESTRICT INTRA_CRTFUNC(void*) malloc(size_t bytes);
INTRA_CRTRESTRICT INTRA_CRTFUNC(void*) calloc(size_t count, size_t size);
INTRA_CRTRESTRICT INTRA_CRTFUNC(void*) realloc(void* oldPtr, size_t bytes);
INTRA_CRTFUNC(void) free(void* ptr);
INTRA_CRTFUNC(char*) strncpy(char* dst, const char* src, size_t count); // use it instead of strcpy_s because it is available everywhere, even in msvcrt.dll in Windows XP
INTRA_CRTFUNC(void) qsort(void* base, size_t numElements, size_t bytesPerElement,
    int(INTRA_CRTDECL* comparer)(void* context, const void* a, const void* b));

INTRA_CRTFUNC(char*) setlocale(int category, const char* locale);
INTRA_CRTFUNC(double) strtod(const char* string, char** endPtr);
#if !defined(_WIN32) || !INTRA_SUPPORT_MSVCRT
INTRA_CRTFUNC(int64) strtoll(const char* string, char** endPtr, int radix);
INTRA_CRTFUNC(uint64) strtoull(const char* string, char** endPtr, int radix);
#else
INTRA_CRTFUNC(int64) _strtoi64(const char* string, char** endPtr, int radix);
INTRA_CRTFUNC(uint64) _strtoui64(const char* string, char** endPtr, int radix);
INTRA_FORCEINLINE int64 strtoll(const char* string, char** endPtr, int radix) {return _strtoi64(string, endPtr, radix);}
INTRA_FORCEINLINE uint64 strtoull(const char* string, char** endPtr, int radix) {return _strtoui64(string, endPtr, radix);}
#endif

INTRA_CRTFUNC(int) isxdigit(int c);
INTRA_CRTFUNC(int) iswupper(int wc);
INTRA_CRTFUNC(int) iswspace(int wc);
INTRA_CRTFUNC(int) iswpunct(int wc);
INTRA_CRTFUNC(int) iswprint(int wc);
INTRA_CRTFUNC(int) iswlower(int wc);
INTRA_CRTFUNC(int) iswgraph(int wc);
INTRA_CRTFUNC(int) iswcntrl(int wc);
INTRA_CRTFUNC(int) iswalpha(int wc);
INTRA_CRTFUNC(int) iswalnum(int wc);
INTRA_CRTFUNC(int) iswctype(int wc, uint16 desc);

#ifdef _WIN32
using jmp_buf = int[16];
#elif defined(__ANDROID__)
#if defined(__aarch64__)
using jmp_buf = long[32];
#elif defined(__arm__)
using jmp_buf = long[64];
#elif defined(__i386__)
using jmp_buf = long[10];
#elif defined(__amd64__)
using jmp_buf = long[11];
#endif
using sigjmp_buf = long[sizeof(jmp_buf) / sizeof(long) + 1];
#elif defined(__linux__)
#if defined(__aarch64__)
struct __jmp_buf_tag {long[39] b;};
#elif defined(__arm__)
struct __jmp_buf_tag {long[98] b;};
#elif defined(__i386__)
struct __jmp_buf_tag {long[39] b;};
#elif defined(__amd64__)
struct __jmp_buf_tag {long[25] b;};
#else
#error Not supported platform
#endif
using jmp_buf = __jmp_buf_tag*;
using sigjmp_buf = __jmp_buf_tag*;
#elif defined(__APPLE__)
#if defined(__amd64__)
using jmp_buf = int[37];
#elif defined(__i386__)
using jmp_buf = int[18];
#elif defined(__arm64__) || defined(__ARM_ARCH_7K__)
using jmp_buf = int[48];
#elif defined(__arm__)
using jmp_buf = int[28];
#endif
using sigjmp_buf = int[sizeof(jmp_buf) / sizeof(int) + 1];
#else
#error Not supported platform
#endif
INTRA_CRTFUNC(int) setjmp(jmp_buf env);
[[noreturn]] INTRA_CRTFUNC(void) longjmp(jmp_buf env, int value);
#ifndef _WIN32
[[noreturn]] void siglongjmp(sigjmp_buf env, int val);
#endif

#if !defined(_WIN32) && (!defined(__ANDROID__) || __ANDROID_API__ >= 33)
int backtrace(void** array, int size);
char** backtrace_symbols(void* const* array, int size);
#endif


// define socket function common for all supported platforms including winsock
#ifdef _WIN32
using SOCKET = size_t;
using socklen_t = int;
using sockbuflen_t = int;
using sockretlen_t = int;
using sockbuf_t = char;
using in_addr_t = unsigned long;
#else
using SOCKET = int;
#ifndef __LP64__
using socklen_t = int32;
#else
using socklen_t = uint32;
#endif
using sockbuflen_t = size_t;
using sockretlen_t = index_t;
using sockbuf_t = void;
using in_addr_t = int;
#endif

struct timeval {long tv_sec, tv_usec;};
struct timespec {time_t tv_sec; long tv_nsec;};
struct timezone {int tz_minuteswest, tz_dsttime;};
struct tm {int tm_sec, tm_min, tm_hour, tm_mday, tm_mon, tm_year, tm_wday, tm_yday, tm_isdst;};

struct pollfd {SOCKET fd; short events, revents;};
INTRA_WINFUNC(SOCKET) socket(int af, int type, int protocol);
INTRA_WINFUNC(SOCKET) accept(SOCKET s, sockaddr* addr, socklen_t* addrlen);
INTRA_WINFUNC(int) bind(SOCKET s, const sockaddr* name, socklen_t namelen);
INTRA_WINFUNC(int) connect(SOCKET s, const sockaddr* name, socklen_t namelen);
INTRA_WINFUNC(sockretlen_t) recv(SOCKET s, sockbuf_t* buf, sockbuflen_t len, int flags);
INTRA_WINFUNC(sockretlen_t) recvfrom(SOCKET s, sockbuf_t* buf, sockbuflen_t len, int flags, sockaddr* from, socklen_t* fromlen);
INTRA_WINFUNC(int) select(int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, const ::timeval* timeout);
INTRA_WINFUNC(sockretlen_t) send(SOCKET s, const sockbuf_t* buf, sockbuflen_t len, int flags);
INTRA_WINFUNC(sockretlen_t) sendto(SOCKET s, const sockbuf_t* buf, sockbuflen_t len, int flags, const sockaddr* to, socklen_t tolen);
INTRA_WINFUNC(int) getsockopt(SOCKET s, int level, int optname, sockbuf_t* optval, socklen_t* optlen);
INTRA_WINFUNC(int) setsockopt(SOCKET s, int level, int optname, const sockbuf_t* optval, socklen_t optlen);
INTRA_WINFUNC(int) shutdown(SOCKET s, int how);
INTRA_WINFUNC(int) listen(SOCKET s, int backlog);
INTRA_WINFUNC(int) gethostname(char* name, sockbuflen_t namelen);
INTRA_WINFUNC(int) getaddrinfo(const char* nodeName, const char* serviceName, const addrinfo* hints, addrinfo** outResult);
INTRA_WINFUNC(void) freeaddrinfo(::addrinfo* pAddrInfo);
INTRA_WINFUNC(in_addr_t) inet_addr(const char* s);
INTRA_WINFUNC(char*) inet_ntoa(::in_addr in);
INTRA_WINFUNC(int) inet_pton(int family, const char* addrString, void* outAddrBuf); // Vista+

#ifdef INTRAZ_D_TOOLCHAIN_DECLARE_PTHREAD
constexpr int archValues(int win64, int win32, int android64, int android32, int linuxArm64, int linuxX64Mips64, int linuxX86Mips32Arm32)
{
    return Config::TargetOS == Config::OperatingSystem::Windows? sizeof(void*) == 8? win64: win32:
        Config::TargetOS == Config::OperatingSystem::Android? sizeof(void*) == 8? android64: android32:
        Config::TargetOS == Config::OperatingSystem::Linux?
#if defined(__aarch64__)
        linuxArm64:
#elif defined(__amd64__) || defined(__mips64__)
        linuxX64Mips64:
#elif defined(__arm__) || defined(__mips__)
        linuxX86Mips32Arm32:
#elif !defined(__linux__)
        0:
#endif
        0;
}
#define INTRAZ_D_DEFINE_STRUCT(T, ...) struct alignas(size_t) T {int32 priv[archValues(__VA_ARGS__) / 4];}

INTRAZ_D_DEFINE_STRUCT(pthread_attr_t, 32, 16, 56, 24, 64, 56, 36);
INTRAZ_D_DEFINE_STRUCT(pthread_mutex_t, 8, 4, 40, 4, 48, 40, 24);
INTRAZ_D_DEFINE_STRUCT(pthread_mutexattr_t, 4, 4, 8, 4, 8, 4, 4);
INTRAZ_D_DEFINE_STRUCT(pthread_cond_t, 8, 4, 48, 4, 48, 48, 48);
INTRAZ_D_DEFINE_STRUCT(pthread_condattr_t, 4, 4, 8, 4, 8, 4, 4);
INTRAZ_D_DEFINE_STRUCT(pthread_rwlock_t, 8, 4, 56, 40, 56, 56, 32);

#if defined(__ANDROID__) && (!defined(__LP64__) || __ANDROID_API__ >= 28)
#define INTRAZ_D_HAS_pthread_cond_timedwait_monotonic_np
#elif defined(__APPLE__) || defined(_WIN32)
#define INTRAZ_D_HAS_pthread_cond_timedwait_relative_np
#endif

#ifdef _WIN32
#define INTRA_PTHREAD_MUTEX_INITIALIZER {{-1}}
#define INTRA_PTHREAD_RECURSIVE_MUTEX_INITIALIZER {{-3}}
#define INTRA_PTHREAD_ERRORCHECK_MUTEX_INITIALIZER {{-2}}
#define INTRA_PTHREAD_COND_INITIALIZER {{-1}}
#define INTRA_PTHREAD_RWLOCK_INITIALIZER {{-1}}
#elif defined(__ANDROID__)
#define INTRA_PTHREAD_MUTEX_INITIALIZER {}
#define INTRA_PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP {{1 << 14}}
#define INTRA_PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP {{2 << 14}}
#define INTRA_PTHREAD_COND_INITIALIZER {}
#define INTRA_PTHREAD_RWLOCK_INITIALIZER {}
#else // GLIBC
#define INTRA_PTHREAD_MUTEX_INITIALIZER {}
#define INTRA_PTHREAD_COND_INITIALIZER {}
#define INTRA_PTHREAD_RWLOCK_INITIALIZER {}
#ifdef __amd64__
#define INTRA_PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP {{0,0,0,0,1}}
#define INTRA_PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP {{0,0,0,0,2}}
#elif defined __i386__
#define INTRA_PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP {{0,0,0,0,1}}
#define INTRA_PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP {{0,0,0,2}}
#else
// TODO: other platforms
#endif
#endif

int INTRA_PTHREADIMP pthread_attr_init(::pthread_attr_t* attr);
int INTRA_PTHREADIMP pthread_attr_setstacksize(::pthread_attr_t* attr, size_t stacksize);

int INTRA_PTHREADIMP pthread_create(::pthread_t *th, const ::pthread_attr_t *attr, void*(*func)(void*), void* arg);
int INTRA_PTHREADIMP pthread_join(::pthread_t thread, void** retval);

int INTRA_PTHREADIMP pthread_mutexattr_init(::pthread_mutexattr_t* a);
int INTRA_PTHREADIMP pthread_mutexattr_settype(::pthread_mutexattr_t* a, int type);

int INTRA_PTHREADIMP pthread_mutex_lock(::pthread_mutex_t* m);
int INTRA_PTHREADIMP pthread_mutex_timedlock(::pthread_mutex_t* m, const ::timespec* ts);
int INTRA_PTHREADIMP pthread_mutex_unlock(::pthread_mutex_t* m);
int INTRA_PTHREADIMP pthread_mutex_trylock(::pthread_mutex_t* m);
int INTRA_PTHREADIMP pthread_mutex_init(::pthread_mutex_t* m, const ::pthread_mutexattr_t* a);
int INTRA_PTHREADIMP pthread_mutex_destroy(::pthread_mutex_t* m);

int INTRA_PTHREADIMP pthread_rwlock_init(::pthread_rwlock_t* rwlock, const ::pthread_rwlockattr_t* attr);
int INTRA_PTHREADIMP pthread_rwlock_wrlock(::pthread_rwlock_t* l);
int INTRA_PTHREADIMP pthread_rwlock_timedwrlock(::pthread_rwlock_t* rwlock, const ::timespec* ts);
int INTRA_PTHREADIMP pthread_rwlock_rdlock(::pthread_rwlock_t* l);
int INTRA_PTHREADIMP pthread_rwlock_timedrdlock(::pthread_rwlock_t* l, const ::timespec* ts);
int INTRA_PTHREADIMP pthread_rwlock_unlock(::pthread_rwlock_t* l);
int INTRA_PTHREADIMP pthread_rwlock_tryrdlock(::pthread_rwlock_t* l);
int INTRA_PTHREADIMP pthread_rwlock_trywrlock(::pthread_rwlock_t* l);
int INTRA_PTHREADIMP pthread_rwlock_destroy(::pthread_rwlock_t* l);

int INTRA_PTHREADIMP pthread_cond_init(::pthread_cond_t* cv, const ::pthread_condattr_t* a);
int INTRA_PTHREADIMP pthread_cond_destroy(::pthread_cond_t* cv);
int INTRA_PTHREADIMP pthread_cond_signal(::pthread_cond_t* cv);
int INTRA_PTHREADIMP pthread_cond_broadcast(::pthread_cond_t* cv);
int INTRA_PTHREADIMP pthread_cond_wait(::pthread_cond_t* cv, ::pthread_mutex_t* mutex);
int INTRA_PTHREADIMP pthread_cond_timedwait(::pthread_cond_t* cv, ::pthread_mutex_t* mutex, const ::timespec* t);

#ifdef INTRAZ_D_HAS_pthread_cond_timedwait_monotonic_np
int pthread_cond_timedwait_monotonic_np(::pthread_cond_t* cv, ::pthread_mutex_t* mutex, const ::timespec* t);
#endif

#ifdef INTRAZ_D_HAS_pthread_cond_timedwait_relative_np
int INTRA_PTHREADIMP pthread_cond_timedwait_relative_np(::pthread_cond_t* cv, ::pthread_mutex_t* mutex, const ::timespec* t);
#endif

#if defined(__linux__) || defined(_WIN32)
int INTRA_PTHREADIMP pthread_setname_np(pthread_t thread, const char* name);
#elif defined(__APPLE__)
int pthread_setname_np(const char* name);
#elif defined(__NetBSD__)
int pthread_setname_np(pthread_t thread, const char* name, void* arg); // printf-like
inline int pthread_set_name_np(pthread_t thread, const char* name) {return pthread_setname_np(thread, "%s", name);}
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__DragonFly__)
int pthread_set_name_np(pthread_t thread, const char* name);
#endif
#else
using ::pthread_attr_init;
using ::pthread_attr_setstacksize;
using ::pthread_join;
using ::pthread_create;
#endif

#ifdef _WIN32
INTRA_CRTFUNC(index_t) _get_heap_handle();
INTRA_CRTFUNC(void*) _expand(void* ptr, size_t newSize);
INTRA_CRTFUNC(size_t) _msize(void* ptr);
INTRA_CRTFUNC(::tm*) _gmtime64(const time_t* time);
INTRA_CRTFUNC(::tm*) _localtime64(const time_t* time);
INTRA_FORCEINLINE ::tm* gmtime(const time_t* time) {return _gmtime64(time);}
INTRA_FORCEINLINE ::tm* localtime(const time_t* time) {return _localtime64(time);}
INTRA_CRTFUNC(char*) _gcvt(double number, int precision, char* buf);
INTRA_FORCEINLINE char* gcvt(double number, int precision, char* buf) {return _gcvt(number, precision, buf);}

#if INTRA_SUPPORT_MSVCRT
INTRA_CRTFUNC(int) _vsnprintf(char* dstBuf, size_t bufSize, const char* format, va_list argList);
inline int vsnprintf(char* dstBuf, size_t bufSize, const char* format, va_list argList)
{
    auto res = _vsnprintf(dstBuf, bufSize, format, argList);
    if(res == -1) dstBuf[bufSize - 1] = '\0';
    return res; // may return -1 instead of required size which is not a correct emulation of vsnprintf
}
#else
INTRA_CRTFUNC(int) __stdio_common_vsprintf(uint64 options, char* dstBuf, size_t bufSize,
    const char* format, __crt_locale_pointers* locale, va_list arglist);
inline int vsnprintf(char* dstBuf, size_t bufSize, const char* format, va_list argList)
{return Max(-1, __stdio_common_vsprintf(2, dstBuf, bufSize, format, nullptr, argList));}
#endif

#if !INTRA_BUILD_FOR_WINDOWS_XP || !INTRA_SUPPORT_MSVCRT
#define INTRA_TOOLCHAIN_HAS_QSORT_WITH_CONTEXT
// Non-standard. C11 version has a different order of arguments
INTRA_CRTFUNC(void) qsort_s(void* base, size_t numElements, size_t bytesPerElement,
    int(INTRA_CRTDECL* comparer)(void* context, const void* a, const void* b), void* context);
#endif

// On Windows returns exactly requested number of bytes.
inline size_t malloc_usable_size(void* ptr) {return ptr? _msize(ptr): 0;}

INTRA_WINFUNC(unsigned long) GetLastError();
INTRA_WINFUNC(int) MultiByteToWideChar(uint32 codePage, unsigned long flags, const char* src, int srcLength, wchar_t* dst, int dstLength); // we only use codePage = CP_UTF8 = 65001
INTRA_WINFUNC(HMODULE) LoadLibraryW(const wchar_t* libFileName);
INTRA_WINFUNC(HMODULE) LoadPackagedLibrary(const wchar_t* libFileName, unsigned long reserved); // Windows 8+, use only for UWP apps

INTRA_WINFUNC(int) QueryPerformanceFrequency(_LARGE_INTEGER* frequency);
INTRA_WINFUNC(int) QueryPerformanceCounter(_LARGE_INTEGER* counter);

using ULONG_PTR = TSelect<uint64, unsigned long, sizeof(void*) == 8>;

struct SYSTEMTIME
{
    uint16 wYear, wMonth, wDayOfWeek, wDay, wHour, wMinute, wSecond, wMilliseconds;
};

struct TIME_ZONE_INFORMATION
{
    long Bias;
    wchar_t StandardName[32];
    SYSTEMTIME StandardDate;
    long StandardBias;
    wchar_t DaylightName[32];
    SYSTEMTIME DaylightDate;
    long DaylightBias;
};

struct CRITICAL_SECTION
{
    void* DebugInfo;
    long LockCount;
    long RecursionCount;
    void* OwningThread;
    void* LockSemaphore;
    size_t SpinCount;
};
using FARPROC = index_t(INTRA_WINAPI*)();

struct SECURITY_ATTRIBUTES
{
    uint32 nLength;
    void* lpSecurityDescriptor;
    int bInheritHandle;
};

struct OVERLAPPED
{
    size_t Internal, InternalHigh;
    uint64 Offset;
    HANDLE hEvent;
};

struct OVERLAPPED_ENTRY
{
    ULONG_PTR CompletionKey;
    OVERLAPPED* Overlapped;
    size_t Internal;
    unsigned long NumBytesTransferred;
};

using LPOVERLAPPED_COMPLETION_ROUTINE = void(INTRA_WINAPI*)(unsigned long errorCode,
    unsigned long numBytesTransfered, _OVERLAPPED* overlapped);

using LPWSAOVERLAPPED_COMPLETION_ROUTINE = void(INTRA_WINAPI*)(unsigned long errorCode,
    unsigned long numBytesTransferred, _OVERLAPPED* overlapped, unsigned long flags);

struct WSAData
{
    uint16 wVersion, wHighVersion;
#ifndef _WIN64
    char szDescription[257], szSystemStatus[129];
#endif
    uint16 iMaxSockets, iMaxUdpDg;
    char* lpVendorInfo;
#ifdef _WIN64
    char szDescription[257], szSystemStatus[129];
#endif
};

struct WSANETWORKEVENTS
{
    long lNetworkEvents;
    int iErrorCode[10];
};

INTRA_WINFUNC(FARPROC) GetProcAddress(HMODULE hModule, const char* procName);
INTRA_WINFUNC(void) GetLocalTime(_SYSTEMTIME* lpSystemTime);
INTRA_WINFUNC(int) FileTimeToSystemTime(const _FILETIME* fileTime, _SYSTEMTIME* systemTime);
INTRA_WINFUNC(void) GetSystemTimeAsFileTime(_FILETIME* systemTimeAsFileTime);
INTRA_WINFUNC(unsigned long) GetTimeZoneInformation(_TIME_ZONE_INFORMATION* lpTimeZoneInformation);

INTRA_CRTFUNC(size_t) _beginthreadex(void* security, uint32 stackSize,
	uint32(INTRA_WINAPI* startAddress)(void*), void* argList, uint32 initFlag, uint32* thrdAddr);
INTRA_CRTFUNC(void) _endthreadex(uint32 returnCode);

INTRA_WINFUNC(HANDLE) GetCurrentThread();
INTRA_WINFUNC(int) CloseHandle(HANDLE);
INTRA_WINFUNC(unsigned long) GetThreadId(HANDLE);

INTRA_WINFUNC(void) RaiseException(unsigned long exceptionCode, unsigned long exceptionFlags, unsigned long numberOfArguments, const ULONG_PTR* arguments);

#if defined(_MSC_VER) && (defined(__i386__) || defined(__amd64__))
void _mm_pause();
#pragma intrinsic(_mm_pause)
#endif

INTRA_WINFUNC(unsigned long) WaitForSingleObject(HANDLE, unsigned long milliseconds);
INTRA_WINFUNC(unsigned long) WaitForMultipleObjectsEx(unsigned long count, const HANDLE* handles, int waitAll, unsigned long milliseconds, int alertable); // count <= 64

INTRA_WINFUNC(HANDLE) CreateEventW(_SECURITY_ATTRIBUTES* eventAttributes, int manualReset, int initialState, const wchar_t* name);
INTRA_WINFUNC(int) SetEvent(HANDLE event);

using PTIMERAPCROUTINE = void(INTRA_WINAPI*)(void* argToCompletionRoutine, unsigned long timerLowValue, unsigned long timerHighValue);
INTRA_WINFUNC(HANDLE) CreateWaitableTimerW(_SECURITY_ATTRIBUTES* timerAttributes, int manualReset, const wchar_t* timerName);
INTRA_WINFUNC(int) SetWaitableTimer(HANDLE timer, const _LARGE_INTEGER* dueTime, long lPeriod,
    PTIMERAPCROUTINE completionRoutine, void* argToCompletionRoutine, int resume);

using PAPCFUNC = void(INTRA_WINAPI*)(ULONG_PTR param);
INTRA_WINFUNC(unsigned long) QueueUserAPC(PAPCFUNC procedure, HANDLE thread, ULONG_PTR data);

INTRA_WINFUNC(HANDLE) CreateIoCompletionPort(HANDLE fileHandle,
    HANDLE optExistingCompletionPort, ULONG_PTR completionKey, unsigned long numConcurrentThreads);
INTRA_WINFUNC(int) PostQueuedCompletionStatus(HANDLE completionPort,
    unsigned long numBytesTransferred, ULONG_PTR completionKey, OVERLAPPED* overlapped);
INTRA_WINFUNC(int) GetQueuedCompletionStatus(HANDLE completionPort, unsigned long* numBytesTransferred,
    ULONG_PTR* outCompletionKey, OVERLAPPED** outOverlapped, unsigned long milliseconds);
INTRA_WINFUNC(int) GetQueuedCompletionStatusEx(HANDLE completionPort, _OVERLAPPED_ENTRY* outCompletionPortEntries,
    unsigned long count, unsigned long* outNumEntriesRemoved, unsigned long illiseconds, int alertable); // Vista+

INTRA_WINFUNC(int) InitializeCriticalSection(_RTL_CRITICAL_SECTION*);
INTRA_WINFUNC(int) InitializeCriticalSectionAndSpinCount(_RTL_CRITICAL_SECTION*, unsigned long spinCount);
INTRA_WINFUNC(int) TryEnterCriticalSection(_RTL_CRITICAL_SECTION*);
INTRA_WINFUNC(void) EnterCriticalSection(_RTL_CRITICAL_SECTION*);
INTRA_WINFUNC(void) LeaveCriticalSection(_RTL_CRITICAL_SECTION*);
INTRA_WINFUNC(void) DeleteCriticalSection(_RTL_CRITICAL_SECTION*);

INTRA_WINFUNC(int) WSAStartup(short versionRequired, ::WSAData* wsaData);
INTRA_WINFUNC(int) WSACleanup();
INTRA_WINFUNC(int) closesocket(size_t sock);
INTRA_WINFUNC(int) ioctlsocket(SOCKET s, long cmd, unsigned long* argp);

INTRA_WINFUNC(int) WSAIoctl(SOCKET s, unsigned long ioControlCode, void* inBuffer, unsigned long inBufferSize,
    void* outBuffer, unsigned long outBufferSize, unsigned long* bytesReturned,
    _OVERLAPPED* lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);

using LPFN_CONNECTEX = int(__stdcall*)(SOCKET s, const sockaddr* name, int namelen,
    void* sendBuffer, unsigned long sendDataLength, unsigned long* bytesSent, _OVERLAPPED* overlapped);

INTRA_WINFUNC(int) WSAEventSelect(SOCKET s, HANDLE eventObject, long networkEvents);
INTRA_WINFUNC(int) WSAEnumNetworkEvents(SOCKET s, HANDLE eventObject, _WSANETWORKEVENTS* lpNetworkEvents);

INTRA_WINFUNC(unsigned long) FormatMessageA(unsigned long flags, void* source,
    unsigned long messageId, unsigned long languageId, char* buffer, unsigned long nSize, va_list* args);

INTRA_WINFUNC(int) IsDebuggerPresent();

#if INTRA_BUILD_FOR_WINDOWS_XP
struct _RTL_RWLOCK
{
    _RTL_CRITICAL_SECTION rtlCS;

    HANDLE hSharedReleaseSemaphore;
    uint32 uSharedWaiters;

    HANDLE hExclusiveReleaseSemaphore;
    uint32 uExclusiveWaiters;

    int iNumberActive;
    HANDLE hOwningThreadId;
    unsigned long dwTimeoutBoost;
    void* pDebugInfo;
};

INTRA_WINFUNC(void) RtlInitializeResource(_RTL_RWLOCK*);
INTRA_WINFUNC(void) RtlDeleteResource(_RTL_RWLOCK*);
INTRA_WINFUNC(void) RtlReleaseResource(_RTL_RWLOCK*);
INTRA_WINFUNC(uint8) RtlAcquireResourceExclusive(_RTL_RWLOCK*, uint8);
INTRA_WINFUNC(uint8) RtlAcquireResourceShared(_RTL_RWLOCK*, uint8);
#else
// Vista+
INTRA_WINFUNC(void) ReleaseSRWLockExclusive(_RTL_SRWLOCK*);
INTRA_WINFUNC(void) ReleaseSRWLockShared(_RTL_SRWLOCK*);
INTRA_WINFUNC(void) AcquireSRWLockExclusive(_RTL_SRWLOCK*);
INTRA_WINFUNC(void) AcquireSRWLockShared(_RTL_SRWLOCK*);
INTRA_WINFUNC(uint8) TryAcquireSRWLockExclusive(_RTL_SRWLOCK*);
INTRA_WINFUNC(uint8) TryAcquireSRWLockShared(_RTL_SRWLOCK*);
INTRA_WINFUNC(void) WakeConditionVariable(_RTL_CONDITION_VARIABLE*);
INTRA_WINFUNC(void) WakeAllConditionVariable(_RTL_CONDITION_VARIABLE*);
INTRA_WINFUNC(int) SleepConditionVariableCS(_RTL_CONDITION_VARIABLE*, _RTL_CRITICAL_SECTION*, unsigned long ms);
INTRA_WINFUNC(int) SleepConditionVariableSRW(_RTL_CONDITION_VARIABLE*, _RTL_SRWLOCK*, unsigned long ms, unsigned long flags);
INTRA_WINFUNC(int) WSAPoll(pollfd fds[], unsigned long nfds, int timeoutMs);
INTRA_FORCEINLINE int poll(pollfd fds[], unsigned nfds, int timeoutMs) {return WSAPoll(fds, nfds, timeoutMs);}
#endif
#else
tm* gmtime(const time_t* time);
tm* localtime(const time_t* time);
int gettimeofday(timeval* tv, timezone* tz);
int settimeofday(const timeval* tv, const timezone* tz);
int clock_gettime(int clockid, timespec* tp);
int nanosleep(const ::timespec* req, ::timespec* rem);
int vsnprintf(char* dstBuf, size_t bufSize, const char* format, va_list argList);
void* memmem(const void* haystack, size_t haystackLen, const void* needle, size_t needleLen);
void* memrchr(const void* s, int c, size_t n);
int pipe(int pipefd[2]);
int ioctl(int fd, unsigned long request, ...);
INTRA_FORCEINLINE int ioctlsocket(int s, long cmd, unsigned long* argp) {return ioctl(s, int(cmd), argp);}

struct Dl_info
{
    const char* dli_fname;
    void* dli_fbase;
    const char* dli_sname;
    void* dli_saddr;
};
void* dlopen(const char* filename, int flags);
int dlclose(void* handle);
int dladdr(const void* addr, ::Dl_info* info);
int dlinfo(void* handle, int request, void* info);
void* dlsym(void* handle, const char* symbol);
void* dlvsym(void* handle, const char* symbol, const char* version);
char* dlerror();

const char* gai_strerror(int errorCode);
int poll(pollfd fds[], unsigned nfds, int timeoutMs);

#if INTRAZ_D_TOOLCHAIN_DECLARE_STRUCTS
#if defined(__APPLE__) || defined(__FreeBSD__) || INTRA_TARGET_IS_BSD
using off_t = int64;
#else
using off_t = long;
#endif
using off64_t = long long;
using mode_t = unsigned;
using uid_t = unsigned;
using gid_t = unsigned;
#endif
struct iovec {void* iov_base; size_t iov_len;};
int open(const char* pathname, int flags, mode_t mode);
int close(int fd);
index_t read(int fd, void* buf, size_t n);
index_t write(int fd, const void* buf, size_t count);
index_t readv(int fd, const ::iovec* iov, int iovcnt);
index_t writev(int fd, const ::iovec *iov, int iovcnt);
index_t pread(int fd, void* buf, size_t n, off_t offset);
index_t pwrite(int fd, void* buf, size_t n, off_t offset);
off_t lseek(int fd, off_t offset, int whence);
int eventfd(unsigned val, int flags); // had been Linux-only until recently; FreeBSD 13+, NetBSD 10+

int symlink(const char* target, const char* linkpath); // relative linkpath is relative to CWD
index_t readlink(const char* pathname, char* buf, size_t bufsiz);
int chown(const char* pathname, uid_t owner, gid_t group);
int lchown(const char* pathname, uid_t owner, gid_t group); // like chown but ignores symlinks
int fchown(int fd, uid_t owner, gid_t group);
int chmod(const char* pathname, mode_t mode);
int fchmod(int fd, mode_t mode);
int mkdir(const char* pathname, mode_t mode);
int chdir(const char* path);
int fchdir(int fd);
char* getcwd(char* buf, size_t size);

#if INTRAZ_D_TOOLCHAIN_DECLARE_STRUCTS
struct dirent
{
#ifdef __linux__
#ifdef __ANDROID__
    uint64 d_ino;
    off64_t d_off;
#else
    unsigned long d_ino;
    off_t d_off;
#endif
    uint16 d_reclen;
    uint8 d_type;
    char d_name[256];
#elif defined(__FreeBSD__) || INTRA_TARGET_IS_BSD
    uint64 d_fileno;
    off_t d_off;
    uint16 d_reclen;
    uint8 d_type, d_namlen;
    uint32 d_pad0;
    char d_name[256];
#elif defined(__APPLE__) && !defined(__i386__) && !defined(__arm__) // support only 64-bit Mac OS apps and Apple A7+ devices (iPhone 5S+)
    uint64 d_fileno;
    uint16 d_seekoff, d_reclen, d_namlen;
    uint8 d_type;
    char d_name[1024];
#else
#error Not implemented on this target platform!
#endif
};
struct stat {
#ifdef __linux__
#if defined(__aarch64__)
    uint64 st_dev;
    uint64 st_ino;
    mode_t st_mode;
    uint64 st_nlink;
    uid_t st_uid;
    gid_t st_gid;
    uint64 st_rdev;
    uint64: 64;
    off_t st_size;
    int32 st_blksize;
    int32: 32;
    int64 st_blocks;
    struct timespec st_atim, st_mtim, st_ctim;
    uint32: 32, : 32;
#elif defined(__amd64__)
    uint64 st_dev;
    uint64 st_ino;
    uint64 st_nlink;
    mode_t st_mode;
    uid_t st_uid;
    gid_t st_gid;
    uint32: 32;
    uint64 st_rdev;
    off_t st_size;
    int64 st_blksize, st_blocks;
    timespec st_atim, st_mtim, st_ctim;
    int64: 64, : 64, : 64;
#else // __arm__ || __i386__
    uint64 st_dev;
    uint32: 4;
    uint64 __st_ino;
    uint32 st_mode;
    uint32 st_nlink;
    uid_t st_uid;
    gid_t st_gid;
    uint64 st_rdev;
    uint32: 32;
    int64 st_size;
    uint64 st_blksize;
    uint64 st_blocks;
    timespec st_atim, st_mtim, st_ctim;
    uint64 st_ino;
#endif
#elif defined(__APPLE__) && !defined(__i386__) && !defined(__arm__)
    int32 st_dev;
    mode_t st_mode;
    uint16 st_nlink;
    uint64 st_ino;
    uid_t st_uid;
    gid_t st_gid;
    int32 st_rdev;
    timespec st_atimespec, st_mtimespec, st_ctimespec, st_birthtimespec;
    off_t st_size;
    int64 st_blocks;
    int32 st_blksize;
    uint32 st_flags;
    uint32 st_gen;
    int32 st_lspare;
    int64 st_qspare[2];
#elif defined(__FreeBSD__)
    uint64 st_dev;
    uint64 st_ino;
    uint64 st_nlink;
    mode_t st_mode;
    int16 st_padding0;
    uid_t st_uid;
    gid_t st_gid;
    int32 st_padding1;
    uint64 st_rdev;
#ifdef __i386__
    int32 st_atim_ext;
#endif
    timespec st_atim;
#ifdef __i386__
    int32 st_mtim_ext;
#endif
    timespec st_mtim;
#ifdef __i386__
    int32 st_ctim_ext;
#endif
    timespec st_ctim;
#ifdef __i386__
    int32 st_btim_ext;
#endif
    timespec st_birthtim;
    off_t st_size;
    int64 st_blocks;
    int32 st_blksize;
    uint32 st_flags;
    uint64 st_gen;
    uint64 st_spare[10];
#endif
};
#else
using ::stat;
using ::dirent;
#endif
int fstat(int fildes, struct ::stat* buf);
int lstat(const char* path, struct ::stat* buf);
int stat(const char* path, struct ::stat* buf);
DIR* opendir(const char* filename);
int closedir(DIR* dirp);
::dirent* readdir(DIR *dirp);
void rewinddir(DIR* dirp);
void seekdir(DIR* dirp, long loc);

void* mmap(void* addr, size_t len, int prot, int flags, int fd, off_t offset);
void* mremap(void* oldAddr, size_t oldLen, void* newAddr, size_t newsize, int flags);
int munmap(void* addr, size_t length);
int msync(void* addr, size_t len, int flags);
int mprotect(void* addr, size_t len, int prot);

char* getenv(const char* name);
int putenv(char* string);
int setenv(const char *name, const char* value, int overwrite);
int unsetenv(const char *name);

struct regex_t
{
    int re_magic;
    size_t re_nsub;	
    const char* re_endp;
    struct re_guts* re_g;
};
struct regmatch_t {index_t rm_so, rm_eo;};
int regcomp(::regex_t* preg, const char* pattern, int cflags);
size_t regerror(int errcode, const ::regex_t* preg, char* errbuf, size_t errbuf_size);
int regexec(const ::regex_t* preg, const char* string, size_t nmatch, ::regmatch_t* pmatch, int eflags);
void regfree(::regex_t* preg);

int kill(pid_t pid int sig);

#ifdef __linux__
// including Android
struct epoll_event
{
    uint32 events;
    uint64 data;
};
int epoll_create(int size);
int epoll_ctl(int epfd, int op, int fd, ::epoll_event* ev);
int epoll_wait(int epfd, ::epoll_event* events, int maxEvents, int timeoutMs);

off64_t lseek64(int fd, off64_t offset, int whence);
index_t pread64(int fd, void* buf, size_t n, off64_t offset);
index_t pwrite64(int fd, const void* buf, size_t n, off64_t offset);
index_t sendfile(int outFd, int inFd, off_t* offset, size_t count);
index_t readahead(int fd, off64_t offset, size_t count);

#if !defined(__ANDROID__) || __ANDROID_API__ >= 19 // in Android since 4.4+
struct itimerspec {timespec it_interval, it_value;};
int timerfd_create(int clockid, int flags);
int timerfd_settime(int fd, int flags, const ::itimerspec* newValue, ::itimerspec* oldValue);
int timerfd_gettime(int fd, ::itimerspec* curValue);
#endif

#if !defined(__ANDROID__) || __ANDROID_API__ >= 21 // in Android since 5.0+
index_t splice(int fdIn, off64_t* offIn, int fdOut, off64_t* offOut, size_t len, unsigned flags);
index_t tee(int fdIn, int fdOut, size_t len, unsigned flags);
#endif

#if !defined(__ANDROID__) || __ANDROID_API__ >= 23 // in Android since 6.0+
long telldir(DIR* dirp);
#endif

#elif INTRA_TARGET_IS_BSD
struct Kevent
{
    size_t ident;
#ifdef __NetBSD__
    uint32 filter;
    uint32 flags;
#else
    int16 filter;
    uint16 flags;
#endif
    uint32 fflags;
#ifdef __DragonFly__
    index_t data;
#else
    int64 data;
#endif
    void* udata;
#if defined(__FreeBSD__) || defined(__NetBSD__)
    uint64 ext[4];
#elif defined(__OpenBSD__)
#endif
};
// Platform-specific constants (see https://github.com/rust-lang/libc/tree/main/src/unix/bsd in mod.rs)
enum class EvFilt {
#ifdef __NetBSD__
    Read, Write, AIO, Vnode, Proc, Signal, Timer, FS
#else
    Read = -1, Write = -2, AIO = -3, Vnode = -4, Proc = -5, Signal = -6, Timer = -7,
#ifdef __APPLE__
    FS = -9, User = -10, Except = -15
#elif defined(__FreeBSD__)
    FS = -9, User = -11
#elif defined(__OpenBSD__)
    Except = -9
    // no FS, no User
#elif defined(__DragonFly__)
    Except = -8, User = -9, FS = -10
#endif
#endif
};
enum class EvNote {
#ifdef __APPLE__
    NSeconds = 4, AbsTime = 8,
#elif defined __FreeBSD__
    NSeconds = 8, AbsTime = 16,
#elif defined(__NetBSD__)
    NSeconds = 3, AbsTime = 16,
#else
    NSeconds = 0, AbsTime = 0, // not supported
#endif
    Trigger = 0x01000000
};
int kqueue();
int kevent(int kq, const struct ::kevent* changelist, int nchanges, struct ::kevent* eventList, int nevents, const ::timespec* timeout);
#ifdef __FreeBSD__
struct sf_hdtr
{
    iovec* headers;
    int hdr_cnt;
    iovec* trailers;
    int trl_cnt;
};
int sendfile(int srcFd, int dstSocketFd, off_t srcOffset, size_t numBytes, ::sf_hdtr* hdtr, off_t* sbytes, int flags);
#endif
#endif

#if !defined(__ANDROID__)
char* gcvt(double number, int precision, char* buf);
#else
int INTRA_CRTFUNC sprintf(char* buf, const char* format, ...); // not for Windows (not available in ucrt)
inline char* gcvt(double number, int precision, char* buf)
{
    char format[] = "%.0g";
    if(precision > 0) format[2] = '0' + Min(precision, 9);
    sprintf(buf, format, number);
    return buf;
}
#endif

#ifdef __APPLE__
size_t malloc_size(void* ptr);
size_t malloc_good_size(size_t size);
// The size may be a bit larger that the requested size.
// NOTE: quite slow on Mac OS (~50 CPU cycles) compared to Linux (~10 cycles).
inline size_t malloc_usable_size(void* ptr) {return ptr? malloc_size(ptr): 0;}
#else
// The size may be a bit larger that the requested size.
size_t malloc_usable_size(void* ptr); // available on Linux, Android and FreeBSD
#endif
#if defined(__FreeBSD__) || defined(__APPLE__)
#define INTRA_TOOLCHAIN_HAS_QSORT_WITH_CONTEXT
void qsort_r(void* base, size_t numElements, size_t bytesPerElement,
    void* context, int(*comparer)(void* context, const void* a, const void* b));
#elif !defined(__ANDROID__) && defined(__linux__) && !INTRA_SUPPORT_MUSL // available in musl since 1.2.3 (April 7, 2022)
#define INTRA_TOOLCHAIN_HAS_QSORT_WITH_CONTEXT
void qsort_r(void* base, size_t numElements, size_t bytesPerElement,
    int(*comparer)(const void* a, const void* b, void* context), void* context); // GLIBC-only
#endif
#endif

#if defined(__GNUC__) || defined(__clang__)
INTRA_FORCEINLINE void* memcpy(void* dst, const void* src, size_t size) noexcept {return __builtin_memcpy(dst, src, size);}
INTRA_FORCEINLINE void* memmove(void* dst, const void* src, size_t size) noexcept {return __builtin_memmove(dst, src, size);}
INTRA_FORCEINLINE void* memset(void* dst, int val, size_t size) noexcept {return __builtin_memset(dst, val, size);}
#elif defined(_MSC_VER)
INTRA_CRTFUNC(void*) memcpy(void* dst, const void* src, size_t size);
INTRA_CRTFUNC(void*) memmove(void* dst, const void* src, size_t size);
INTRA_CRTFUNC(void*) memset(void* dst, int val, size_t size);
INTRA_CRTFUNC(uint16) _byteswap_ushort(uint16);
INTRA_CRTFUNC(unsigned long) _byteswap_ulong(unsigned long);
INTRA_CRTFUNC(uint64) _byteswap_uint64(uint64);
uint8 _BitScanReverse(unsigned long* Index, unsigned long Mask);
uint8 _BitScanReverse64(unsigned long* Index, uint64 Mask);
uint8 _BitScanForward(unsigned long* Index, unsigned long Mask);
uint8 _BitScanForward64(unsigned long* Index, uint64 Mask);
uint16 __popcnt16(uint16 value);
uint32 __popcnt(uint32 value);
uint64 __popcnt64(uint64 value);

#if defined(__amd64__) || defined(__aarch64__)
uint64 __umulh(uint64, uint64);
int64 __mulh(int64, int64);
#endif

#ifdef __amd64__
uint64 _umul128(uint64 x, uint64 y, uint64* highProduct);
int64 _mul128(int64 x, int64 y, int64* highProduct);
uint64 __shiftright128(uint64 lowPart, uint64 highPart, uint8 shift);
uint64 __shiftleft128(uint64 lowPart, uint64 highPart, uint8 shift);
#endif

#pragma intrinsic(memcpy, memmove, memset, _byteswap_ushort, _byteswap_ulong, _byteswap_uint64)
#endif

#if defined(_WIN32) && INTRA_SUPPORT_MSVCRT && INTRA_BUILD_FOR_WINDOWS_XP
inline int strerror_s(char* dstBuf, size_t bufsz, int errnum)
{
	// not correct strerror_s, but good enough for our use case
	if(bufsz) strncpy(dstBuf, strerror(errnum), bufsz - 1);
	return 0;
}
#elif defined(_WIN32)
INTRA_CRTFUNC(int) strerror_s(char* dstBuf, size_t bufsz, int errnum);
#else
int strerror_r(int errnum, char* dstBuf, size_t bufsz);
INTRA_FORCEINLINE int strerror_s(char* dstBuf, size_t bufsz, int errnum) {return strerror_r(errnum, n, dstBuf);}
#endif

// Safe access to errno
#ifdef errno
INTRA_FORCEINLINE int GetErrno() {return errno;}
#else
#ifdef _WIN32
INTRA_CRTFUNC(int*) _errno();
INTRA_FORCEINLINE int GetErrno() {return *_errno();}
#elif defined(__linux__)
#ifdef __ANDROID__
int* __errno() __attribute__((__const__));
INTRA_FORCEINLINE int GetErrno() {return *__errno();}
#else
extern int errno;
INTRA_FORCEINLINE int GetErrno() {return errno;}
#endif
#elif defined(__APPLE__) || defined(__FreeBSD__) || INTRA_TARGET_IS_BSD // TODO: other BSDs may have another implementation
extern int* __error();
INTRA_FORCEINLINE int GetErrno() {return *__error();}
#else
static_assert(false, "errno must be fixed for this platform");
#endif
#endif
}}

#ifdef INTRA_VALIDATE_TOOLCHAIN
#if __has_include(<pthread.h>)
#include <pthread.h>
#define INTRAZ_D_CHECK(T) sizeof(::Intra::z_D::pthread_##T##_t) == sizeof(::pthread_##T##_t)
static_assert(
    INTRAZ_D_CHECK(attr) &&
    INTRAZ_D_CHECK(mutex) &&
    INTRAZ_D_CHECK(mutexattr) &&
    INTRAZ_D_CHECK(cond) &&
    INTRAZ_D_CHECK(condattr) &&
    INTRAZ_D_CHECK(rwlock) &&
    INTRAZ_D_CHECK(rwlockattr)
);
#undef INTRAZ_D_CHECK
#endif
#endif
