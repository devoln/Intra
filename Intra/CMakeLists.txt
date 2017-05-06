cmake_minimum_required(VERSION 2.8)

project(Intra)

include(../Configurations.cmake)

include_directories(include)

init_project_sources(${CMAKE_CURRENT_SOURCE_DIR} INTRA_HEADERS INTRA_SOURCES)

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

if(EXISTS "Audio/CMakeLists.txt")
add_subdirectory(Audio)
endif()

if(EXISTS "Image/CMakeLists.txt")
add_subdirectory(Image)
endif()

if(EXISTS "Concurrency/CMakeLists.txt")
add_subdirectory(Concurrency)
endif()

if(EXISTS "IO/CMakeLists.txt")
add_subdirectory(IO)
endif()


#if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
#target_link_libraries(Intra Shlwapi)
#endif()
