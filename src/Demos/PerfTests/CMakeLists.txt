cmake_minimum_required(VERSION 3.1)

project(Tests)

include(../../Configurations.cmake)

include_directories(../../Intra)

file(GLOB_RECURSE TEST_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" src/*.h)
file(GLOB_RECURSE TEST_SOURCES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" src/*.cpp)

add_executable(Tests ${TEST_SOURCES} ${TEST_HEADERS})
target_link_libraries(Tests Intra)

