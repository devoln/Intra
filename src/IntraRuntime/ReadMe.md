**Warning: this thing is highly experimental, not tested and mostly broken. I'm going to move this to a separate repository soon**

Implements some minimal subset of CRT functions necessary to run Intra and Extra programs without need of full CRT.

It maybe useful for demoscene projects that require very small binary sizes (4K, 64K, etc) or to create extremely small emscripten libraries.

It includes:

1. An entry point function that calls `main(int argc, char* argv[])`

2. An implementation for math functions (currently only for 32-bit Windows)
3. thread_local support
4. Compiler-generated function calls like _ftol, _dtoui, etc
5. memcpy, memcmp, strlen, wcslen, ...
6. Import some functions from Windows system msvcrt.dll

This library has no public interface headers. Just link it with your project to replace default CRT with it.

