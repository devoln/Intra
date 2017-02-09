cmake_minimum_required(VERSION 2.8)

project(IntraAudio)

include(../CMakeCommon.txt)

include_directories(../Core/include)
include_directories(include)

file(GLOB_RECURSE AUDIO_HEADERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" include/*.h)
file(GLOB_RECURSE AUDIO_SOURCES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" src/*.cpp)

foreach(_source IN ITEMS ${AUDIO_HEADERS})
    get_filename_component(_source_path "${_source}" PATH)
    string(REPLACE "/" "\\" _group_path "${_source_path}")
    source_group("${_group_path}" FILES "${_source}")
endforeach()

foreach(_source IN ITEMS ${AUDIO_SOURCES})
    get_filename_component(_source_path "${_source}" PATH)
    string(REPLACE "/" "\\" _group_path "${_source_path}")
    source_group("${_group_path}" FILES "${_source}")
endforeach()

option(UNITY_BUILD "Use fast unity build." ON)
if(UNITY_BUILD)
set_source_files_properties(${AUDIO_SOURCES} PROPERTIES HEADER_FILE_ONLY true)
add_definitions(-DINTRA_UNITY_BUILD)
else()
set_source_files_properties(src/IntraAudio.cc PROPERTIES HEADER_FILE_ONLY true)
endif()
set(AUDIO_SOURCES ${AUDIO_SOURCES} src/IntraAudio.cc)

option(BUILD_DYNAMIC_LIB "Build a dynamic library instead of static." OFF)

if(BUILD_DYNAMIC_LIB)
add_library(IntraAudio SHARED ${AUDIO_SOURCES} ${AUDIO_HEADERS})
else()
add_library(IntraAudio STATIC ${AUDIO_SOURCES} ${AUDIO_HEADERS})
endif()
target_link_libraries(IntraAudio IntraCore)


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
target_link_libraries(IntraAudio dsound dxguid)
elseif(LIBRARY_SOUND_SYSTEM_OpenAL)
add_definitions(-DINTRA_LIBRARY_SOUND_SYSTEM=INTRA_LIBRARY_SOUND_SYSTEM_OpenAL)
include_directories(${OPENAL_INCLUDE_DIR})
target_link_libraries(IntraAudio ${OPENAL_LIBRARY})
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
target_link_libraries(IntraAudio dsound)
elseif(LIBRARY_VORBIS_DECODER_libvorbis)
add_definitions(-DINTRA_LIBRARY_VORBIS_DECODER=INTRA_LIBRARY_VORBIS_DECODER_libvorbis)
include_directories(${VORBIS_INCLUDE_DIRS})
target_link_libraries(IntraAudio ${VORBIS_LIBRARIES})
else()
add_definitions(-DINTRA_LIBRARY_VORBIS_DECODER=INTRA_LIBRARY_VORBIS_DECODER_None)
endif()

