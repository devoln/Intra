cmake_minimum_required(VERSION 2.8)

project(PerfTesting)

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

