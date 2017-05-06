#pragma once

#ifdef INTRA_IMPORTS_FROM_DLL
#define INTRA_DLL_API __declspec(dllimport) 
#else
#define INTRA_DLL_API __declspec(dllexport) 
#endif
