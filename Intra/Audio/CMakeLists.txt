cmake_minimum_required(VERSION 2.8)

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
target_link_libraries(Intra dsound dxguid)
elseif(LIBRARY_SOUND_SYSTEM_OpenAL)
add_definitions(-DINTRA_LIBRARY_SOUND_SYSTEM=INTRA_LIBRARY_SOUND_SYSTEM_OpenAL)
include_directories(${OPENAL_INCLUDE_DIR})
target_link_libraries(Intra ${OPENAL_LIBRARY})
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
target_link_libraries(Intra dsound)
elseif(LIBRARY_VORBIS_DECODER_libvorbis)
add_definitions(-DINTRA_LIBRARY_VORBIS_DECODER=INTRA_LIBRARY_VORBIS_DECODER_libvorbis)
include_directories(${VORBIS_INCLUDE_DIRS})
target_link_libraries(Intra ${VORBIS_LIBRARIES})
else()
add_definitions(-DINTRA_LIBRARY_VORBIS_DECODER=INTRA_LIBRARY_VORBIS_DECODER_None)
endif()

