cmake_minimum_required(VERSION 2.8)

project(IntraCore)

include(../CMakeCommon.txt)

include_directories(include)

add_definitions(-DINTRA_LIBRARY_WINDOW_SYSTEM=INTRA_LIBRARY_WINDOW_SYSTEM_Console)

file(GLOB_RECURSE CORE_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" include/*.h)
file(GLOB_RECURSE CORE_SOURCES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" src/*.cpp)

foreach(_source IN ITEMS ${CORE_HEADERS})
    get_filename_component(_source_path "${_source}" PATH)
    string(REPLACE "/" "\\" _group_path "${_source_path}")
    source_group("${_group_path}" FILES "${_source}")
endforeach()

foreach(_source IN ITEMS ${CORE_SOURCES})
    get_filename_component(_source_path "${_source}" PATH)
    string(REPLACE "/" "\\" _group_path "${_source_path}")
    source_group("${_group_path}" FILES "${_source}")
endforeach()

option(UNITY_BUILD "Use fast unity build." ON)
if(UNITY_BUILD)
set_source_files_properties(${CORE_SOURCES} PROPERTIES HEADER_FILE_ONLY true)
add_definitions(-DINTRA_UNITY_BUILD)
else()
set_source_files_properties(src/IntraCore.cc PROPERTIES HEADER_FILE_ONLY true)
endif()
set(CORE_SOURCES ${CORE_SOURCES} src/IntraCore.cc)

option(BUILD_DYNAMIC_LIB "Build a dynamic library instead of static." OFF)

if(BUILD_DYNAMIC_LIB)
add_library(IntraCore SHARED ${CORE_SOURCES} ${CORE_HEADERS})
else()
add_library(IntraCore STATIC ${CORE_SOURCES} ${CORE_HEADERS})
endif()



#find_package(Threads)
#if(THREADS_FOUND)
#if(UNIX)
#option(LIBRARY_THREADING_PThread "Use pthread" ON)
#else()
#option(LIBRARY_THREADING_PThread "Use pthread" OFF)
#endif()
#endif()

#if(LIBRARY_THREADING_PThread)
#add_definitions(-DINTRA_LIBRARY_THREADING=INTRA_LIBRARY_THREADING_PThread)
#include_directories(${THREADS_INCLUDE_DIRS})
#target_link_libraries(IntraCore ${CMAKE_THREAD_LIBS_INIT})
#else()
#add_definitions(-DINTRA_LIBRARY_THREADING=INTRA_LIBRARY_THREADING_Dummy)
#endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
target_link_libraries(IntraCore Shlwapi)
endif()
