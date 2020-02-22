#include "Mutex.h"

#include "System/Runtime.h"

#if(INTRA_LIBRARY_MUTEX != INTRA_LIBRARY_MUTEX_None)

#if(INTRA_LIBRARY_MUTEX == INTRA_LIBRARY_MUTEX_Cpp11)

#include "detail/MutexCpp11.hxx"

#elif(INTRA_LIBRARY_MUTEX == INTRA_LIBRARY_MUTEX_WinAPI)

#include "detail/MutexWinAPI.hxx"

#elif(INTRA_LIBRARY_MUTEX == INTRA_LIBRARY_MUTEX_PThread)

#include "detail/MutexPThread.hxx"

#endif

INTRA_BEGIN
const int Mutex::DataSize = Mutex::DATA_SIZE;
const int Mutex::ImplementationType = INTRA_LIBRARY_MUTEX;
INTRA_END

#endif
