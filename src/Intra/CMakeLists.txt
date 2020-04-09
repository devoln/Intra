cmake_minimum_required(VERSION 3.8)

if(POLICY CMP0076)
    cmake_policy(SET CMP0076 NEW)
endif()

init_project_sources(${CMAKE_CURRENT_SOURCE_DIR} INTRA_HEADERS INTRA_SOURCES INTRA_TEST_SOURCES)
add_library(Intra INTERFACE)
target_sources(Intra INTERFACE ${INTRA_CORE_HEADERS})
add_executable(IntraTest ${INTRA_TEST_SOURCES})
#add_custom_target(Intra_ SOURCES ${INTRA_CORE_HEADERS})
