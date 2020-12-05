cmake_minimum_required(VERSION 3.8)

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
find_library(WSOCK32_LIBRARY wsock32)
find_library(WS2_32_LIBRARY ws2_32)
target_link_libraries(Intra wsock32 ws2_32)
endif()
