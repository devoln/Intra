cmake_minimum_required(VERSION 2.8)

project(IntraLib)

include(../CMakeCommon.txt)

include_directories(include)

add_definitions(-DINTRA_LIBRARY_WINDOW_SYSTEM=INTRA_LIBRARY_WINDOW_SYSTEM_Console)

option(UNITY_BUILD "Use fast unity build." ON)

include(include/CMakeFileList.txt)
foreach(FILE ${INTRA_LIB_HEADERS})
	set(INTRA_LIB_HEADERS1 ${INTRA_LIB_HEADERS1} include/${FILE})
endforeach(FILE)

if(UNITY_BUILD)

set(INTRA_LIB_SOURCE1 ${INTRA_LIB_SOURCE1} src/Intra.cpp)
add_definitions(-DINTRA_UNITY_BUILD)

else()

include(src/CMakeFileList.txt)
foreach(FILE ${INTRA_LIB_SOURCE})
	set(INTRA_LIB_SOURCE1 ${INTRA_LIB_SOURCE1} src/${FILE})
endforeach(FILE)

endif()

option(BUILD_DYNAMIC_LIB "Build a dynamic library instead of static." OFF)

if(BUILD_DYNAMIC_LIB)
add_library(IntraLib SHARED ${INTRA_LIB_SOURCE1} ${INTRA_LIB_HEADERS1})
else()
add_library(IntraLib STATIC ${INTRA_LIB_SOURCE1} ${INTRA_LIB_HEADERS1})
endif()


if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
option(LIBRARY_SOUND_SYSTEM_DirectSound "Use DirectSound" ON)
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
option(LIBRARY_SOUND_SYSTEM_WebAudio "Use WebAudio" ON)
endif()

find_package(OpenAL)
if(OPENAL_FOUND)
	if(${CMAKE_SYSTEM_NAME} MATCHES "Windows" OR ${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
		option(LIBRARY_SOUND_SYSTEM_OpenAL "Use OpenAL" OFF)
	else()
		option(LIBRARY_SOUND_SYSTEM_OpenAL "Use OpenAL" ON)
	endif()
endif()

if(LIBRARY_SOUND_SYSTEM_DirectSound)
add_definitions(-DINTRA_LIBRARY_SOUND_SYSTEM=INTRA_LIBRARY_SOUND_SYSTEM_DirectSound)
target_link_libraries(IntraLib dsound dxguid)
elseif(LIBRARY_SOUND_SYSTEM_OpenAL)
add_definitions(-DINTRA_LIBRARY_SOUND_SYSTEM=INTRA_LIBRARY_SOUND_SYSTEM_OpenAL)
include_directories(${OPENAL_INCLUDE_DIR})
target_link_libraries(IntraLib ${OPENAL_LIBRARY})
elseif(LIBRARY_SOUND_SYSTEM_WebAudio)
add_definitions(-DINTRA_LIBRARY_SOUND_SYSTEM=INTRA_LIBRARY_SOUND_SYSTEM_WebAudio)
else()
add_definitions(-DINTRA_LIBRARY_SOUND_SYSTEM=INTRA_LIBRARY_SOUND_SYSTEM_Dummy)
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
option(LIBRARY_SOUND_SYSTEM_WebAudio "Use WebAudio" ON)
endif()




if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
option(LIBRARY_VORBIS_DECODER_STB "Use STB_Vorbis" OFF)
endif()


option(LIBRARY_VORBIS_DECODER_libvorbis "Use libvorbis" OFF)
if(UNIX)
set(VORBIS_LIBRARIES vorbis vorbisfile)
else()
set(VORBIS_INCLUDE_DIRS NOT_FOUND)
set(VORBIS_LIBRARIES NOT_FOUND)
endif()

if(LIBRARY_VORBIS_DECODER_STB)
add_definitions(-DINTRA_LIBRARY_VORBIS_DECODER=INTRA_LIBRARY_VORBIS_DECODER_STB)
target_link_libraries(IntraLib dsound)
elseif(LIBRARY_VORBIS_DECODER_libvorbis)
add_definitions(-DINTRA_LIBRARY_VORBIS_DECODER=INTRA_LIBRARY_VORBIS_DECODER_libvorbis)
include_directories(${VORBIS_INCLUDE_DIRS})
target_link_libraries(IntraLib ${VORBIS_LIBRARIES})
else()
add_definitions(-DINTRA_LIBRARY_VORBIS_DECODER=INTRA_LIBRARY_VORBIS_DECODER_None)
endif()





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
target_link_libraries(IntraLib ${FREETYPE_LIBRARIES})
elseif(LIBRARY_FONT_LOADING_STB)
add_definitions(-DINTRA_LIBRARY_FONT_LOADING=INTRA_LIBRARY_FONT_LOADING_STB)
else()
add_definitions(-DINTRA_LIBRARY_FONT_LOADING=INTRA_LIBRARY_FONT_LOADING_Dummy)
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
#target_link_libraries(IntraLib ${CMAKE_THREAD_LIBS_INIT})
#else()
#add_definitions(-DINTRA_LIBRARY_THREADING=INTRA_LIBRARY_THREADING_Dummy)
#endif()



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
target_link_libraries(IntraLib gdiplus)
elseif(LIBRARY_IMAGE_LOADING_DevIL)
add_definitions(-DINTRA_LIBRARY_IMAGE_LOADING=INTRA_LIBRARY_IMAGE_LOADING_DevIL)
include_directories(${DEVIL_INCLUDE_DIRS})
target_link_libraries(IntraLib ${DEVIL_LIBRARIES})
elseif(LIBRARY_IMAGE_LOADING_Qt)
add_definitions(-DINTRA_LIBRARY_IMAGE_LOADING=INTRA_LIBRARY_IMAGE_LOADING_Qt)
include_directories(${QT4_INCLUDE_DIRS})
target_link_libraries(IntraLib ${QT4_LIBRARIES})
else()
add_definitions(-DINTRA_LIBRARY_IMAGE_LOADING=INTRA_LIBRARY_IMAGE_LOADING_Dummy)
endif()


if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
target_link_libraries(IntraLib Shlwapi)
endif()
