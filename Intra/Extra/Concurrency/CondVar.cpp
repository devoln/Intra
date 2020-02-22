#include "CondVar.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

#if(INTRA_LIBRARY_MUTEX == INTRA_LIBRARY_MUTEX_Cpp11)

#include "detail/CondVarCpp11.hxx"

#elif(INTRA_LIBRARY_MUTEX == INTRA_LIBRARY_MUTEX_WinAPI)

#include "detail/CondVarWinAPI.hxx"

#elif(INTRA_LIBRARY_MUTEX == INTRA_LIBRARY_MUTEX_PThread)

#include "detail/CondVarPThread.hxx"

#endif

INTRA_WARNING_POP
