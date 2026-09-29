cmake_minimum_required(VERSION 2.8)

project(Intra)

include(../Configurations.cmake)

include_directories(.)

init_project_sources(${CMAKE_CURRENT_SOURCE_DIR} INTRA_HEADERS INTRA_SOURCES)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Audio/CMakeLists.txt")
option(ENABLE_AUDIO_MODULE "Compile Intra Audio module." ON)
if(ENABLE_AUDIO_MODULE)
else()
exclude_project_sources_directory(Audio/ INTRA_HEADERS INTRA_SOURCES)
endif()
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Image/CMakeLists.txt")
option(ENABLE_IMAGE_MODULE "Compile Intra Image module." ON)
if(ENABLE_IMAGE_MODULE)
else()
exclude_project_sources_directory(Image/ INTRA_HEADERS INTRA_SOURCES)
endif()
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/IO/CMakeLists.txt")
option(ENABLE_IO_MODULE "Compile Intra IO module." ON)
if(ENABLE_IO_MODULE)
else()
exclude_project_sources_directory(IO/ INTRA_HEADERS INTRA_SOURCES)
endif()
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Range")
option(ENABLE_RANGE_MODULE "Compile Intra Range module." ON)
if(ENABLE_RANGE_MODULE)
else()
exclude_project_sources_directory(Range/ INTRA_HEADERS INTRA_SOURCES)
endif()
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Concurrency/CMakeLists.txt")
option(ENABLE_CONCURRENCY_MODULE "Compile Intra Concurrency module." ON)
if(ENABLE_CONCURRENCY_MODULE)
else()
exclude_project_sources_directory(Concurrency/ INTRA_HEADERS INTRA_SOURCES)
endif()
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/System")
option(ENABLE_SYSTEM_MODULE "Compile Intra System module." ON)
if(ENABLE_SYSTEM_MODULE)
else()
exclude_project_sources_directory(System/ INTRA_HEADERS INTRA_SOURCES)
endif()
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Container")
option(ENABLE_CONTAINER_MODULE "Compile Intra Container module." ON)
if(ENABLE_CONTAINER_MODULE)
else()
exclude_project_sources_directory(Container/ INTRA_HEADERS INTRA_SOURCES)
endif()
endif()

option(UNITY_BUILD "Use fast unity build." ON)

# The web synth links only this small closure of Intra implementation files.
# Keeping the source set explicit avoids compiling unrelated Image/Font/Data/
# legacy synth/test code on every ephemeral WASM rebuild. Headers remain
# available through include_directories and are not target source files.
if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    set(INTRA_SOURCES
        Audio/AudioProcessing.cpp
        Audio/AudioSource.cpp
        Audio/Midi/DeviceState.cpp
        Audio/Midi/MidiFileParser.cpp
        Audio/Midi/RawEvent.cpp
        Audio/Midi/TrackCombiner.cpp
        Audio/Midi/TrackParser.cpp
        Audio/MusicNote.cpp
        Audio/Resample.cpp
        Audio/SampleConversion.cpp
        Audio/Synth/ExponentialAttenuation.cpp
        Hash/Murmur.cpp
        Memory/Allocator/Global.cpp
        Memory/Allocator/System.cpp
        Range/Mutation/Transform.cpp
        Utils/Debug.cpp
    )
    set(INTRA_HEADERS "")
endif()

# CMake has had native unity-build support since 3.16. Prefer it whenever
# available; retain the historical generated Intra.UB.cc only as a fallback
# for genuinely old CMake installations.
set(_INTRA_LEGACY_UNITY OFF)
if(UNITY_BUILD AND CMAKE_VERSION VERSION_LESS "3.16")
    enable_unity_build(Intra INTRA_SOURCES)
    set(_INTRA_LEGACY_UNITY ON)
endif()

# In size-optimised builds the whole library compiles at -Oz/-Os, which badly
# hurts library code that runs during a render: the wave-table FFT
# (Audio/AudioProcessing.cpp) showed up as ~35% of render time at -Os native,
# and the MIDI parser lives here too. Allow overriding the library's own level
# (e.g. -Os) while the synth's cold code stays at -Oz.
option(BUILD_DYNAMIC_LIB "Build a dynamic library instead of static." OFF)
if(BUILD_DYNAMIC_LIB)
add_library(Intra SHARED ${INTRA_SOURCES} ${INTRA_HEADERS})
else()
add_library(Intra STATIC ${INTRA_SOURCES} ${INTRA_HEADERS})
endif()

# Four CPU-equivalents are available in the ChatGPT build container. With the
# web source set reduced to 15 files, one Intra unity TU plus three synth unity
# TUs keeps all CPUs busy without reparsing common headers.
if(UNITY_BUILD AND NOT _INTRA_LEGACY_UNITY)
    if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
        # Only 15 implementation files remain in the web closure, so one TU is
        # cheap and leaves the other CPU slots to the synth unity TUs.
        set_target_properties(Intra PROPERTIES UNITY_BUILD ON UNITY_BUILD_BATCH_SIZE 32)
    else()
        # Match the historical native all-in-one Intra.UB.cc behaviour while
        # letting CMake own generation/dependencies.
        set_target_properties(Intra PROPERTIES UNITY_BUILD ON UNITY_BUILD_BATCH_SIZE 10000)
    endif()
endif()

# In size-optimised builds the whole library compiles at -Oz/-Os, which badly
# hurts library code that runs during a render: the wave-table FFT
# (Audio/AudioProcessing.cpp) showed up as ~35% of render time at -Os native,
# and the MIDI parser lives here too. Allow overriding the library's own level
# (e.g. -Os) while the synth's cold code stays at -Oz.
set(INTRA_SIZE_LIB_LEVEL "" CACHE STRING "Optimisation level override for the Intra library in size builds (e.g. -Os)")
if(NOT INTRA_SIZE_LIB_LEVEL STREQUAL "")
    target_compile_options(Intra PRIVATE "${INTRA_SIZE_LIB_LEVEL}")
endif()

if(ENABLE_AUDIO_MODULE)
include(Audio/CMakeLists.txt)
endif()

if(ENABLE_IMAGE_MODULE)
include(Image/CMakeLists.txt)
endif()

if(ENABLE_CONCURRENCY_MODULE)
include(Concurrency/CMakeLists.txt)
endif()

if(ENABLE_IO_MODULE)
include(IO/CMakeLists.txt)
endif()


if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
target_link_libraries(Intra Shlwapi)
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
target_link_libraries(Intra dl)
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
target_link_libraries(Intra execinfo)
endif()
