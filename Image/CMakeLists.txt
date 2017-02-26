cmake_minimum_required(VERSION 2.8)

project(IntraImage)

include(../Configurations.cmake)

include_directories(../Core/include)
include_directories(include)

file(GLOB_RECURSE IMAGE_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" include/*.h)
file(GLOB_RECURSE IMAGE_SOURCES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" src/*.cpp)

foreach(_source IN ITEMS ${IMAGE_HEADERS})
    get_filename_component(_source_path "${_source}" PATH)
    string(REPLACE "/" "\\" _group_path "${_source_path}")
    source_group("${_group_path}" FILES "${_source}")
endforeach()

foreach(_source IN ITEMS ${IMAGE_SOURCES})
    get_filename_component(_source_path "${_source}" PATH)
    string(REPLACE "/" "\\" _group_path "${_source_path}")
    source_group("${_group_path}" FILES "${_source}")
endforeach()

option(UNITY_BUILD "Use fast unity build." ON)
if(UNITY_BUILD)
set_source_files_properties(${IMAGE_SOURCES} PROPERTIES HEADER_FILE_ONLY true)
add_definitions(-DINTRA_UNITY_BUILD)
else()
set_source_files_properties(src/IntraImage.cc PROPERTIES HEADER_FILE_ONLY true)
endif()
set(CORE_SOURCES ${IMAGE_SOURCES} src/IntraImage.cc)

option(BUILD_DYNAMIC_LIB "Build a dynamic library instead of static." OFF)



if(BUILD_DYNAMIC_LIB)
add_library(IntraImage SHARED ${IMAGE_SOURCES} ${IMAGE_HEADERS})
else()
add_library(IntraImage STATIC ${IMAGE_SOURCES} ${IMAGE_HEADERS})
endif()
target_link_libraries(IntraImage IntraCore)


if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
option(LIBRARY_FONT_LOADING_STB "Use STB_TrueType" ON)
else()
option(LIBRARY_FONT_LOADING_STB "Use STB_TrueType" OFF)
endif()

find_package(Freetype)
if(FREETYPE_FOUND)
if(UNIX)
option(LIBRARY_FONT_LOADING_FreeType "Use FreeType" ON)
else()
option(LIBRARY_FONT_LOADING_FreeType "Use FreeType" OFF)
endif()
endif()

if(LIBRARY_FONT_LOADING_FreeType)
add_definitions(-DINTRA_LIBRARY_FONT_LOADING=INTRA_LIBRARY_FONT_LOADING_FreeType)
include_directories(${FREETYPE_INCLUDE_DIRS})
target_link_libraries(IntraImage IntraCore ${FREETYPE_LIBRARIES})
elseif(LIBRARY_FONT_LOADING_STB)
add_definitions(-DINTRA_LIBRARY_FONT_LOADING=INTRA_LIBRARY_FONT_LOADING_STB)
else()
add_definitions(-DINTRA_LIBRARY_FONT_LOADING=INTRA_LIBRARY_FONT_LOADING_Dummy)
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
option(LIBRARY_IMAGE_LOADING_Gdiplus "Use Gdiplus for image loading" ON)
endif()

find_package(DevIL)
if(DEVIL_FOUND)
option(LIBRARY_IMAGE_LOADING_DevIL "Use DevIL for image loading" OFF)
endif()

find_package(Qt4)
if(QT4_FOUND)
option(LIBRARY_IMAGE_LOADING_Qt "Use Qt for image loading" OFF)
endif()


if(LIBRARY_IMAGE_LOADING_Gdiplus)
add_definitions(-DINTRA_LIBRARY_IMAGE_LOADING=INTRA_LIBRARY_IMAGE_LOADING_Gdiplus)
target_link_libraries(IntraImage gdiplus)
elseif(LIBRARY_IMAGE_LOADING_DevIL)
add_definitions(-DINTRA_LIBRARY_IMAGE_LOADING=INTRA_LIBRARY_IMAGE_LOADING_DevIL)
include_directories(${DEVIL_INCLUDE_DIRS})
target_link_libraries(IntraImage ${DEVIL_LIBRARIES})
elseif(LIBRARY_IMAGE_LOADING_Qt)
add_definitions(-DINTRA_LIBRARY_IMAGE_LOADING=INTRA_LIBRARY_IMAGE_LOADING_Qt)
include_directories(${QT4_INCLUDE_DIRS})
target_link_libraries(IntraImage ${QT4_LIBRARIES})
else()
add_definitions(-DINTRA_LIBRARY_IMAGE_LOADING=INTRA_LIBRARY_IMAGE_LOADING_Dummy)
endif()
