#include "Mutex.h"

#include "Cpp/Warnings.h"
#include "Cpp/PlatformDetect.h"
#include "Cpp/Runtime.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

#if(INTRA_LIBRARY_MUTEX == INTRA_LIBRARY_MUTEX_Dummy)

#include "detail/MutexDummy.hxx"

#elif(INTRA_LIBRARY_MUTEX == INTRA_LIBRARY_MUTEX_CPPLIB)

#include "detail/MutexCpp11.hxx"

#elif(INTRA_LIBRARY_MUTEX == INTRA_LIBRARY_MUTEX_WinAPI)

#include "detail/MutexWinAPI.hxx"

#elif(INTRA_LIBRARY_MUTEX == INTRA_LIBRARY_MUTEX_PThread)

#include "detail/MutexPThread.hxx"

#endif

INTRA_WARNING_POP
