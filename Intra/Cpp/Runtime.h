#pragma once

#if(defined(_MSC_VER) && defined(INTRA_MINIMIZE_CRT))

#define _NO_CRT_STDIO_INLINE

#define INTRA_NOT_LINK_CRT_LIB

#pragma comment(lib, "msvcrtOLD.lib")

#if _MSC_VER>=1900
#pragma comment(lib, "msvcrtOLD2015.lib")
#endif

#ifdef INTRA_NOT_LINK_CRT_LIB
#pragma comment(linker, "/NODEFAULTLIB:libcmt.lib")
#pragma comment(linker, "/NODEFAULTLIB:libcpmt.lib")
#pragma comment(linker, "/NODEFAULTLIB:msvcrt.lib")
#pragma comment(linker, "/NODEFAULTLIB:msvcrt100.lib")
#pragma comment(linker, "/NODEFAULTLIB:msvcrt110.lib")
#pragma comment(linker, "/NODEFAULTLIB:msvcrt120.lib")
#pragma comment(linker, "/NODEFAULTLIB:msvcrt140.lib")
#pragma comment(linker, "/NODEFAULTLIB:msvcp100.lib")
#pragma comment(linker, "/NODEFAULTLIB:msvcp110.lib")
#pragma comment(linker, "/NODEFAULTLIB:msvcp120.lib")
#pragma comment(linker, "/NODEFAULTLIB:msvcp140.lib")
#pragma comment(linker, "/NODEFAULTLIB:msvcr100.lib")
#pragma comment(linker, "/NODEFAULTLIB:msvcr110.lib")
#pragma comment(linker, "/NODEFAULTLIB:msvcr120.lib")
#pragma comment(linker, "/NODEFAULTLIB:msvcr140.lib")
//TODO: add libs for MSVC 2017
#endif


#endif
