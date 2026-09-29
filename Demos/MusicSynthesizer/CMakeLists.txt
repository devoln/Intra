cmake_minimum_required(VERSION 2.8)

include(../../Configurations.cmake)

project(MusicSynthesizer)

include_directories(../../Intra)

file(GLOB_RECURSE MUSIC_SYNTH_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" *.h)
file(GLOB_RECURSE MUSIC_SYNTH_SOURCES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" *.cpp)

add_executable(MusicSynthesizer ${MUSIC_SYNTH_HEADERS} ${MUSIC_SYNTH_SOURCES})

target_link_libraries(MusicSynthesizer Intra)

if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    set_target_properties(MusicSynthesizer PROPERTIES
        LINK_FLAGS "-s MODULARIZE=1 -s EXPORT_NAME=IntraMidiSynth -s ALLOW_MEMORY_GROWTH=1 -s EXPORTED_RUNTIME_METHODS=['UTF8ToString','HEAPF32','HEAPU8'] -s EXPORTED_FUNCTIONS=['_SourceCreateFromMidiFileData','_SourceFree','_SourceSamplesLeft','_SourceGetUninterleavedSamples','_GetMidiInfoString','_malloc','_free']")
endif()
