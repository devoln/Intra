cmake_minimum_required(VERSION 2.8)

project(IntraMeta)

include(../../Configurations.cmake)

include_directories(..)

init_project_sources(${CMAKE_CURRENT_SOURCE_DIR} META_HEADERS META_SOURCES)
add_library(IntraMeta INTERFACE ${META_SOURCES} ${META_HEADERS})
