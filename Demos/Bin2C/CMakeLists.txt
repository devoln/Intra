cmake_minimum_required(VERSION 2.8)

project(Bin2C)

include(../../CMakeCommon.txt)

include_directories(../../Core/include)

file(GLOB_RECURSE BIN2C_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" *.h)
file(GLOB_RECURSE BIN2C_SOURCES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" *.cpp)

add_executable(Bin2C ${BIN2C_HEADERS} ${BIN2C_SOURCES})

target_link_libraries(Bin2C IntraCore)
