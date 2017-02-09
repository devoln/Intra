cmake_minimum_required(VERSION 2.8)

include(../../CMakeCommon.txt)

project(MusicSynthesizer)

include_directories(../../Core/include)
include_directories(../../Audio/include)

file(GLOB_RECURSE MUSIC_SYNTH_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" *.h)
file(GLOB_RECURSE MUSIC_SYNTH_SOURCES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" *.cpp)

add_executable(MusicSynthesizer ${MUSIC_SYNTH_HEADERS} ${MUSIC_SYNTH_SOURCES})

target_link_libraries(MusicSynthesizer IntraCore IntraAudio)
