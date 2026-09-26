cmake_minimum_required(VERSION 2.8)

# Shared size-profile switches must be defined before add_subdirectory(Intra),
# otherwise the legacy Intra code-pruning level cannot affect libIntra.
option(INTRA_SIZE_OPT "Enable aggressive feature/code pruning (does not select -Oz)" OFF)
option(INTRA_SIZE_OZ "Use -Oz instead of canonical -Os (compiler choice only)" OFF)
set(INTRA_MINEXE_LEVEL "0" CACHE STRING "Legacy Intra code-pruning level (0..3); INTRA_SIZE_OPT defaults it to 3")
if(INTRA_SIZE_OPT AND INTRA_MINEXE_LEVEL STREQUAL "0")
    set(INTRA_MINEXE_LEVEL "3" CACHE STRING "Legacy Intra code-pruning level (0..3)" FORCE)
endif()
if(NOT INTRA_MINEXE_LEVEL STREQUAL "0")
    add_definitions(-DINTRA_MINEXE=${INTRA_MINEXE_LEVEL})
endif()

# The Emscripten synth uses only memory-backed APIs.  The legacy Intra IO
# module pulls global Std/FormattedWriter machinery into the unity-built static
# library, even though the WASM synth never opens files or writes formatted
# output.  Keep it out of web builds; native builds retain the original module.
if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    set(ENABLE_IO_MODULE OFF CACHE BOOL "Compile Intra IO module." FORCE)
    set(ENABLE_IMAGE_MODULE OFF CACHE BOOL "Compile Intra Image module." FORCE)
    set(ENABLE_SYSTEM_MODULE OFF CACHE BOOL "Compile Intra System module." FORCE)
    set(ENABLE_CONCURRENCY_MODULE OFF CACHE BOOL "Compile Intra Concurrency module." FORCE)
endif()

# Optional durable prebuilt Intra archive for ephemeral web builds.
# The bootstrap validates the archive fingerprint before passing this path.
set(INTRA_PREBUILT_LIBRARY "" CACHE FILEPATH "Prebuilt Emscripten libIntra.a")
if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten" AND NOT INTRA_PREBUILT_LIBRARY STREQUAL "")
    if(NOT EXISTS "${INTRA_PREBUILT_LIBRARY}")
        message(FATAL_ERROR "INTRA_PREBUILT_LIBRARY does not exist: ${INTRA_PREBUILT_LIBRARY}")
    endif()
    add_library(Intra STATIC IMPORTED GLOBAL)
    set_target_properties(Intra PROPERTIES IMPORTED_LOCATION "${INTRA_PREBUILT_LIBRARY}")
else()
    add_subdirectory(Intra)
endif()

# Demos are irrelevant to the browser synth and only add configure work.
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    add_subdirectory(Demos)
endif()
add_subdirectory(intrasynth)
