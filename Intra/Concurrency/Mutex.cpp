#include "Mutex.h"

#include "Cpp/Warnings.h"
#include "Cpp/PlatformDetect.h"
#include "Cpp/Runtime.h"

#if(INTRA_LIBRARY_MUTEX != INTRA_LIBRARY_MUTEX_None)

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

#if(INTRA_LIBRARY_MUTEX == INTRA_LIBRARY_MUTEX_Cpp11)

#include "detail/MutexCpp11.hxx"

#elif(INTRA_LIBRARY_MUTEX == INTRA_LIBRARY_MUTEX_WinAPI)

#include "detail/MutexWinAPI.hxx"

#elif(INTRA_LIBRARY_MUTEX == INTRA_LIBRARY_MUTEX_PThread)

#include "detail/MutexPThread.hxx"

#endif

namespace Intra { namespace Concurrency {
const int Mutex::DataSize = Mutex::DATA_SIZE;
const int Mutex::ImplementationType = INTRA_LIBRARY_MUTEX;
}}

INTRA_WARNING_POP

#endif