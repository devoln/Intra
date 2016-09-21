cmake_minimum_required(VERSION 2.8)

include(../CMakeCommon.txt)

project(MusicSynthesizer)

include_directories(../IntraLib/include)


add_executable(MusicSynthesizer MusicSynthesizer.cpp)

target_link_libraries(MusicSynthesizer IntraLib)
