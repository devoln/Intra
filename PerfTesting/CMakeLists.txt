cmake_minimum_required(VERSION 2.8)

project(PerfTesting)

if(${CMAKE_GENERATOR} MATCHES "Unix Makefiles"  OR ${CMAKE_GENERATOR} MATCHES "MinGW Makefiles")
    set(CMAKE_MAKE_PROGRAM      "${CMAKE_MAKE_PROGRAM} -j{PROCESSOR_COUNT}")
    set(CMAKE_CXX_FLAGS         "${CMAKE_CXX_FLAGS}         -std=c++1y -pthread"     )
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -std=c++1y -pthread -O3" )
    set(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG}   -std=c++1y -pthread -O0 -g3 -pedantic -Wall -Weffc++ -Wextra -Woverloaded-virtual -Wctor-dtor-privacy -Wnon-virtual-dtor -Wold-style-cast -Wconversion -Wsign-conversion -Winit-self -Wunreachable-code")
elseif(MSVC)

    foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES}       )
        string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG         )
        if(CMAKE_CXX_FLAGS_${OUTPUTCONFIG} MATCHES "/W[0-4]" )
            string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS_${OUTPUTCONFIG} ${CMAKE_CXX_FLAGS_${OUTPUTCONFIG}})
        else()
            set(CMAKE_CXX_FLAGS_${OUTPUTCONFIG} "${CMAKE_CXX_FLAGS_${OUTPUTCONFIG}} /W4")
        endif()

        set(CMAKE_CXX_FLAGS_${OUTPUTCONFIG} "${CMAKE_CXX_FLAGS_${OUTPUTCONFIG}} /MP")
    endforeach()
endif()

include_directories(../IntraLib/include)

set(PERF_TEST_HEADERS
src/PerfTestArray.h
src/PerfTestMap.h
src/PerfTestRandom.h
src/PerfTestRanges.h
src/PerfTestSerialization.h
src/PerfTestSort.h
src/PerfTestString.h
)

set(PERF_TEST_SOURCE
src/PerfTestArray.cpp
src/PerfTestMap.cpp
src/PerfTestRandom.cpp
src/PerfTestRanges.cpp
src/PerfTestSerialization.cpp
src/PerfTestSort.cpp
src/PerfTestString.cpp
src/PerfTesting.cpp
)


add_executable(PerfTesting ${PERF_TEST_SOURCE} ${PERF_TEST_HEADERS})
target_link_libraries(PerfTesting IntraLib)

