
option(OUTPUT_MAP "Output .map file." OFF)

set(ADDITIONAL_COMPILER_PARAMETERS "" CACHE STRING "User-specified compiler flags")

option(ENABLE_NEON OFF)
option(ENABLE_SSE OFF)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_COMPILER_IS_GNUCXX)
    set(ALL_WARNINGS "-Wall -Wextra -Woverloaded-virtual -Wctor-dtor-privacy -Wnon-virtual-dtor")
    set(ALL_WARNINGS "${ALL_WARNINGS} -Wold-style-cast -Wconversion -Wsign-conversion -Winit-self -Wunreachable-code")
    set(COMMON_PARAMETERS "${ALL_WARNINGS} -std=c++11 ${ADDITIONAL_COMPILER_PARAMETERS}")

    if(${CMAKE_GENERATOR} MATCHES "Unix Makefiles")
        set(COMMON_PARAMETERS "${COMMON_PARAMETERS} -pthread")
    endif()

    option(COMPILE_32_BIT "Compile as 32 bit application." OFF)
    option(COMPILE_64_BIT "Compile as 64 bit application." OFF)
	
    option(NATIVE_TUNE "Optimize for current processor (-mtune=native)." OFF)

    if(COMPILE_64_BIT)
        set(COMMON_PARAMETERS "${COMMON_PARAMETERS} -m64")
    if(COMPILE_32_BIT)
        message("Selected both options 64 and 32 bit. 64 bit build is chosen.")
    endif()
    elseif(COMPILE_32_BIT)
        set(COMMON_PARAMETERS "${COMMON_PARAMETERS} -m32")
    endif()
	
    if(ENABLE_SSE)
        set(COMMON_PARAMETERS "${COMMON_PARAMETERS} -msse2")
    else()
        add_definitions(-D INTRA_MIN_SIMD_SUPPORT=0)
    endif()
	
    if(ENABLE_NEON AND NOT NATIVE_TUNE)
        set(COMMON_PARAMETERS "${COMMON_PARAMETERS} -march=armv7-a -mfloat-abi=softfp -mfpmath=neon")
    endif()
	
    if(NATIVE_TUNE)
        set(COMMON_PARAMETERS "${COMMON_PARAMETERS} -mtune=native")
    endif()

    if(OUTPUT_MAP)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-Map=output.map" CACHE INTERNAL "" FORCE)
    endif()

    if(USE_EXCEPTIONS)
        set(COMMON_PARAMETERS "${COMMON_PARAMETERS} -fexceptions")
    else()
        set(COMMON_PARAMETERS "${COMMON_PARAMETERS} -fno-exceptions")
    endif()
    
    set(COMMON_MINSIZE_OPTIMIZATIONS "-fno-math-errno -fmerge-all-constants -ffunction-sections -fdata-sections -fno-rtti")
    
    if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
      set(COMMON_MINSIZE_OPTIMIZATIONS "${COMMON_MINSIZE_OPTIMIZATIONS} ")
    else()
      set(COMMON_MINSIZE_OPTIMIZATIONS "${COMMON_MINSIZE_OPTIMIZATIONS} -Wl,--gc-sections")
    endif()
    
    set(ALL_MINSIZE_OPTIMIZATIONS "${COMMON_MINSIZE_OPTIMIZATIONS} -fno-stack-protector -g0 -fno-unroll-loops -fno-asynchronous-unwind-tables")
    
    if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
      set(ALL_MINSIZE_OPTIMIZATIONS "${ALL_MINSIZE_OPTIMIZATIONS} ")
    else()
      set(ALL_MINSIZE_OPTIMIZATIONS "${ALL_MINSIZE_OPTIMIZATIONS} -Wl,-s")
	  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wl,-s")
    endif()
    
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${COMMON_PARAMETERS} ${COMMON_MINSIZE_OPTIMIZATIONS} -Ofast -ffast-math")
    set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} ${COMMON_PARAMETERS} ${ALL_MINSIZE_OPTIMIZATIONS} -Os -ffast-math")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${COMMON_PARAMETERS} -O0 -g3 -D_DEBUG")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} ${COMMON_PARAMETERS} ${COMMON_MINSIZE_OPTIMIZATIONS} -Ofast -g3 -ffast-math -D_DEBUG")
    set(CMAKE_CXX_FLAGS "${COMMON_PARAMETERS}")
	
    if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
        set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL}")
        set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -g4")
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -s ASSERTIONS=2 -g4 -s SAFE_HEAP=1 -s DEMANGLE_SUPPORT=1")
    endif()
    
    if(CMAKE_BUILD_TYPE STREQUAL "")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_MINSIZEREL}")
    endif()
    
elseif(MSVC)

    set(COMMON_PARAMETERS "/Wall /MP ${ADDITIONAL_COMPILER_PARAMETERS}")
	
    if(ENABLE_SSE)
        set(COMMON_PARAMETERS "${COMMON_PARAMETERS} /arch:SSE2")
    elseif("${CMAKE_SIZEOF_VOID_P}" EQUAL "4")
        set(COMMON_PARAMETERS "${COMMON_PARAMETERS} /arch:IA32")
    endif()

    if(USE_EXCEPTIONS)
        set(COMMON_PARAMETERS "${COMMON_PARAMETERS} /EHsc")
    else()
        set(COMMON_PARAMETERS "${COMMON_PARAMETERS} ")
    endif()

    set(CMAKE_CXX_FLAGS_RELEASE "${COMMON_PARAMETERS} /Ox /Ot /Oi /GT /GL /GF /Gy /MT /fp:fast")
    set(CMAKE_CXX_FLAGS_MINSIZEREL "${COMMON_PARAMETERS} /O1 /Os /Oi /GT /GL /GF /GS- /Gy /MT /fp:fast")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${COMMON_PARAMETERS} /O3 /Ot /Oi /GT /GL /GF /Gy /MT /fp:fast /Zi /D_DEBUG")
    set(CMAKE_CXX_FLAGS_DEBUG "${COMMON_PARAMETERS} /Od /MDd /fp:fast /Zi /D_DEBUG /RTC1")
    set(CMAKE_CXX_FLAGS "${COMMON_PARAMETERS}")
	
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG")
	set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG /DYNAMICBASE:NO /FIXED")
	set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG")
	
    if(OUTPUT_MAP)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MAP")
    endif()
	
endif()