#pragma once

//TODO: чтобы не было побочных эффектов от этой библиотеки,
//перенести это только в те cpp файлы, которые используют стандартную библиотеку

#if(defined(_MSC_VER) && !defined(__GNUC__) && !defined(_HAS_EXCEPTIONS))
#define _HAS_EXCEPTIONS 0
#endif
