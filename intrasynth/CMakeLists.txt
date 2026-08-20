cmake_minimum_required(VERSION 3.12)
project(IntraSynth CXX)

# The Intra checkout lives in the parent of this directory and is added as a
# sibling subdirectory by the root CMakeLists.txt, so the `Intra` target is
# available here.
set(INTRA_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Flags must match how the Intra library is built (see Configurations.cmake):
# no exceptions/RTTI, fast math. Native builds enable the SSE2-level SIMD
# kernels of Intra (Intra/Simd/Simd.h selects them via INTRA_SIMD_SUPPORT).
#
# SIMD/arch can be overridden for benchmarks:
#   -DINTRA_SYNTH_SIMD=INTRA_SIMD_NONE|INTRA_SIMD_SSE2|INTRA_SIMD_AVX2
#   -DINTRA_SYNTH_ARCH="-msse2" | "-mavx2" | "-msimd128" | "" | ...
# "DEFAULT" (or unset) keeps the original per-platform flags.
set(INTRA_SYNTH_SIMD "DEFAULT" CACHE STRING "INTRA_SIMD_SUPPORT override for the synth core")
set(INTRA_SYNTH_ARCH "DEFAULT" CACHE STRING "Arch flags override for the synth core")
option(INTRA_SIZE_OPT "Size-optimised build (-Oz on Emscripten, -Os native)" OFF)
option(INTRA_SIZE_LTO "Use LTO in the size build" OFF)
option(INTRA_SIZE_EMSMALLOC "Use emmalloc in the size build" OFF)
option(INTRA_SIZE_OS "Use -Os instead of -Oz in the size build" OFF)
option(INTRA_SIZE_HOT_FAST "Compile the render hot-loop files with -O3 in the size build" OFF)
set(INTRA_SIZE_HOT_LEVEL "-Os" CACHE STRING "Optimisation level for the hot-loop files in the size build (-O2/-O3/-Os)")

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    set(_synth_simd "INTRA_SIMD_SSE2")
    set(_synth_arch "")
    if(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        # Native release: полный набор инструкций машины (автовекторизация, -march=native).
        set(_synth_arch "-march=native")
    endif()
    if(NOT INTRA_SYNTH_SIMD STREQUAL "DEFAULT")
        set(_synth_simd "${INTRA_SYNTH_SIMD}")
    endif()
    if(NOT INTRA_SYNTH_ARCH STREQUAL "DEFAULT")
        set(_synth_arch "${INTRA_SYNTH_ARCH}")
    endif()
    set(INTRA_SYNTH_BASE_FLAGS "-pthread -D_FILE_OFFSET_BITS=64 -fno-exceptions -fno-rtti -ffast-math ${_synth_arch} -DINTRA_SIMD_SUPPORT=${_synth_simd}")
else()
    # Emscripten: Debug-сборки оставляют SAFE_HEAP/ASSERTIONS для отладки
    # памяти; Release — чистый быстрый билд (SAFE_HEAP+ASSERTIONS+отладочная
    # информация замедляли рендер в разы).
    set(_synth_simd "")
    set(_synth_arch "")
    if(NOT INTRA_SYNTH_SIMD STREQUAL "DEFAULT")
        set(_synth_simd "-DINTRA_SIMD_SUPPORT=${INTRA_SYNTH_SIMD}")
    endif()
    if(NOT INTRA_SYNTH_ARCH STREQUAL "DEFAULT")
        set(_synth_arch "${INTRA_SYNTH_ARCH}")
    endif()
    if(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        set(INTRA_SYNTH_BASE_FLAGS "-fno-exceptions -fno-rtti -ffast-math -msimd128 ${_synth_simd} ${_synth_arch}")
        set(INTRA_SYNTH_LINK_FLAGS "-msimd128 -s MODULARIZE=1 -s EXPORT_NAME=IntraMidiSynth -s ALLOW_MEMORY_GROWTH=1 -s EXPORTED_RUNTIME_METHODS=['UTF8ToString','HEAPF32','HEAPU8'] -s EXPORTED_FUNCTIONS=['_SourceCreateFromMidiFileData','_SourceCreateLive','_SourceFree','_SourceSamplesLeft','_SourceGetUninterleavedSamples','_SourceSendMidiEvent','_GetMidiInfoString','_malloc','_free']")
        if(INTRA_SIZE_OPT)
            set(_size_extra "")
            if(INTRA_SIZE_LTO)
                set(_size_extra "${_size_extra} -flto")
            endif()
            if(INTRA_SIZE_OS)
                set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Os ${_size_extra}")
            else()
                set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Oz ${_size_extra}")
            endif()
            if(INTRA_SIZE_LTO)
                set(INTRA_SYNTH_LINK_FLAGS "${INTRA_SYNTH_LINK_FLAGS} -flto")
            endif()
            if(INTRA_SIZE_EMSMALLOC)
                set(INTRA_SYNTH_LINK_FLAGS "${INTRA_SYNTH_LINK_FLAGS} -s MALLOC=emmalloc")
            endif()
        else()
            # -Os is the base level: it is not meaningfully slower than -O3 on
            # the synth kernels but produces a much smaller binary.
            set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Os")
        endif()
    else()
        set(INTRA_SYNTH_BASE_FLAGS "-g4 -fno-exceptions -fno-rtti -ffast-math ${_synth_simd} ${_synth_arch}")
        set(INTRA_SYNTH_LINK_FLAGS "-g4 -s SAFE_HEAP=1 -s ASSERTIONS=2 -s MODULARIZE=1 -s EXPORT_NAME=IntraMidiSynth -s ALLOW_MEMORY_GROWTH=1 -s EXPORTED_RUNTIME_METHODS=['UTF8ToString','HEAPF32','HEAPU8'] -s EXPORTED_FUNCTIONS=['_SourceCreateFromMidiFileData','_SourceCreateLive','_SourceFree','_SourceSamplesLeft','_SourceGetUninterleavedSamples','_SourceSendMidiEvent','_GetMidiInfoString','_malloc','_free']")
    endif()
endif()
# The synth target re-runs project(), so it does not inherit the Intra
# library's COMMON_PARAMETERS warning flags. Silence the same pre-existing
# library noise (legacy Cyrillic comments + deprecated builtins) here too.
# These are clang-only flags; GCC uses different spellings.
set(_synth_warn_noise "")
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(_synth_warn_noise "-Wno-invalid-utf8 -Wno-deprecated-builtins -Wno-deprecated-literal-operator -Wno-nan-infinity-disabled -Wno-deprecated-copy-with-user-provided-copy -Wno-nontrivial-memcall -Wno-deprecated-copy -Wno-unused-but-set-variable -Wno-enum-float-conversion -Wno-variadic-macro-arguments-omitted -Wno-implicit-int-float-conversion -Wno-implicit-const-int-float-conversion")
endif()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${_synth_warn_noise} ${INTRA_SYNTH_BASE_FLAGS} -include ${CMAKE_CURRENT_SOURCE_DIR}/shim/glue.h")

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Intra/Synth
    ${CMAKE_CURRENT_SOURCE_DIR}/shim
    ${INTRA_ROOT}/Intra
    ${INTRA_ROOT}
)

# Core synth used by the WASM export. The two CLI/IO files are excluded:
#   - MusicSynthesizer.cpp is the native command-line frontend (guarded out on
#     Emscripten anyway, but it drags in IO/Concurrency/Sound);
#   - MusicSynthesizerCommon.cpp is the native file/OpenAL helper that depends
#     on the old Sources::MidiSynth path.
file(GLOB SYNTH_CORE_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/Intra/Synth/*.cpp)
list(REMOVE_ITEM SYNTH_CORE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Intra/Synth/MusicSynthesizer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Intra/Synth/MusicSynthesizerCommon.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Intra/Synth/Octaves.cpp)

add_library(IntraSynthCore STATIC ${SYNTH_CORE_SOURCES})

# The render hot loops live in a handful of files. When size-matters builds are
# enabled, we can keep everything else at -Os/-Oz but still compile these files
# at -O3 so the audio path does not pay the size optimisation penalty.
if(INTRA_SIZE_HOT_FAST)
    set(_intra_synth_hot_sources		${CMAKE_CURRENT_SOURCE_DIR}/src/Intra/Synth/KarplusStrongSampler.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/Intra/Synth/GaussianStringSampler.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/Intra/Synth/SpectralStringSampler.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/Intra/Synth/AdditiveSampler.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/Intra/Synth/WaveTableSampler.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Intra/Synth/NoteSampler.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Intra/Synth/PostEffects.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Intra/Synth/DrumPhysicalModel.cpp
        # Master render driver and per-note effects: paid by every song, and the
        # -Oz codegen of these loops showed up in profiles (mix/compressor/ADSR).
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Intra/Synth/MidiSynth.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Intra/Synth/WaveTableGeneration.cpp
    )
    # Hot files opt out of -flto: with LTO the link step re-optimises every
    # TU with the link command's -Oz/-Os, silently destroying the per-file -O2
    # codegen. Compiling them as plain wasm objects keeps the -O2 codegen in
    # the final binary while the cold bulk still gets LTO dedup + size opts.
    set_source_files_properties(${_intra_synth_hot_sources} PROPERTIES
        COMPILE_OPTIONS "${INTRA_SIZE_HOT_LEVEL};-fno-lto")
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    add_executable(IntraSynth ${SYNTH_CORE_SOURCES})
    target_link_libraries(IntraSynth Intra)
    set_target_properties(IntraSynth PROPERTIES
        LINK_FLAGS "${INTRA_SYNTH_LINK_FLAGS}")
else()
    # Native CLI kept only for local sanity checks (requires libopenal-dev).
    add_executable(IntraSynth
        ${SYNTH_CORE_SOURCES}
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Intra/Synth/MusicSynthesizer.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Intra/Synth/MusicSynthesizerCommon.cpp)
    target_link_libraries(IntraSynth Intra openal dl pthread)
endif()
