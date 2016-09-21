cmake_minimum_required(VERSION 2.8)

include(../CMakeCommon.txt)

project(Bin2C)

include_directories(../IntraLib/include)
add_executable(Bin2C main.cpp)
target_link_libraries(Bin2C IntraLib)
