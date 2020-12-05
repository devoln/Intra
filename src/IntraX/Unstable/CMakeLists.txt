cmake_minimum_required(VERSION 3.1)

init_project_sources(${CMAKE_CURRENT_SOURCE_DIR} INTRA_UNSTABLE_HEADERS INTRA_UNSTABLE_SOURCES INTRA_UNSTABLE_TEST_SOURCES)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Audio/CMakeLists.txt")
option(ENABLE_AUDIO "Compile Intra/Unstable/Audio." ON)
if(ENABLE_AUDIO)
else()
exclude_project_sources_directory(Unstable/Audio/ INTRA_UNSTABLE_HEADERS INTRA_UNSTABLE_SOURCES)
endif()
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Image/CMakeLists.txt")
option(ENABLE_UNSTABLE "Compile Intra/Unstable/Image." ON)
if(ENABLE_IMAGE)
else()
exclude_project_sources_directory(Unstable/Image/ INTRA_UNSTABLE_HEADERS INTRA_UNSTABLE_SOURCES)
endif()
endif()

add_library(ExtraUnstable STATIC ${INTRA_UNSTABLE_SOURCES} ${INTRA_UNSTABLE_HEADERS})

if(ENABLE_AUDIO)
include(Audio/CMakeLists.txt)
endif()

if(ENABLE_IMAGE)
include(Image/CMakeLists.txt)
endif()
