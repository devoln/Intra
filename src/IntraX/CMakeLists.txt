cmake_minimum_required(VERSION 3.1)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Unstable/CMakeLists.txt")
option(ENABLE_UNSTABLE "Compile Extra/Unstable." ON)
endif()


if(ENABLE_UNSTABLE)
add_subdirectory(Unstable)
endif()

init_project_sources(${CMAKE_CURRENT_SOURCE_DIR} INTRA_HEADERS INTRA_SOURCES INTRA_TEST_SOURCES)
add_executable(ExtraTest ${INTRA_TEST_SOURCES})
target_link_libraries(ExtraTest Extra)
add_test(NAME ExtraTest COMMAND ExtraTest)

add_library(Extra STATIC ${INTRA_SOURCES} ${INTRA_HEADERS})
target_link_libraries(Extra Intra)


find_package(Threads)
if(THREADS_FOUND)
if(UNIX)
option(LIBRARY_THREADING_PThread "Use pthread" ON)
else()
option(LIBRARY_THREADING_PThread "Use pthread" OFF)
endif()
endif()

if(LIBRARY_THREADING_PThread)
add_definitions(-DINTRA_LIBRARY_THREADING=INTRA_LIBRARY_THREADING_PThread)
include_directories(${THREADS_INCLUDE_DIRS})
target_link_libraries(Extra ${CMAKE_THREAD_LIBS_INIT})
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
add_definitions(-DINTRA_LIBRARY_THREADING=INTRA_LIBRARY_THREADING_WinAPI)
else()
add_definitions(-DINTRA_LIBRARY_THREADING=INTRA_LIBRARY_THREADING_Dummy)
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
target_link_libraries(Extra Shlwapi)
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
target_link_libraries(Extra dl)
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
target_link_libraries(Extra execinfo)
endif()
