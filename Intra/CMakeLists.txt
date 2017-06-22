cmake_minimum_required(VERSION 2.8)

project(Intra)

include(../Configurations.cmake)

include_directories(.)

init_project_sources(${CMAKE_CURRENT_SOURCE_DIR} INTRA_HEADERS INTRA_SOURCES)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Audio/CMakeLists.txt")
option(ENABLE_AUDIO_MODULE "Compile Intra Audio module." ON)
if(ENABLE_AUDIO_MODULE)
else()
exclude_project_sources_directory(/Audio/, INTRA_HEADERS INTRA_SOURCES)
endif()
endif()

option(UNITY_BUILD "Use fast unity build." ON)
if(UNITY_BUILD)
enable_unity_build(Intra INTRA_SOURCES)
endif()

option(BUILD_DYNAMIC_LIB "Build a dynamic library instead of static." OFF)
if(BUILD_DYNAMIC_LIB)
add_library(Intra SHARED ${INTRA_SOURCES} ${INTRA_HEADERS})
else()
add_library(Intra STATIC ${INTRA_SOURCES} ${INTRA_HEADERS})
endif()

if(ENABLE_AUDIO_MODULE)
include(Audio/CMakeLists.txt)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Image/CMakeLists.txt")
include(Image/CMakeLists.txt)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Concurrency/CMakeLists.txt")
include(Concurrency/CMakeLists.txt)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/IO/CMakeLists.txt")
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
