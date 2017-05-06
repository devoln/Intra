cmake_minimum_required(VERSION 2.8)

project(IntraCpp)

include(../../Configurations.cmake)

init_project_sources(${CMAKE_CURRENT_SOURCE_DIR} CPP_HEADERS CPP_SOURCES)
add_library(IntraCpp STATIC ${CPP_SOURCES} ${CPP_HEADERS})
