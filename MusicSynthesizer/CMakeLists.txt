cmake_minimum_required(VERSION 2.8)

project(MusicSynthesizer)

include_directories(../IntraLib/include)

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


add_executable(MusicSynthesizer MusicSynthesizer.cpp)




if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
option(LIBRARY_SOUND_SYSTEM_DirectSound "Use DirectSound" ON)
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
option(LIBRARY_SOUND_SYSTEM_WebAudio "Use WebAudio" ON)
endif()

find_package(OpenAL)
if(OPENAL_FOUND)
if(UNIX)
option(LIBRARY_SOUND_SYSTEM_OpenAL "Use OpenAL" ON)
else()
option(LIBRARY_SOUND_SYSTEM_OpenAL "Use OpenAL" OFF)
endif()
endif()

if(LIBRARY_SOUND_SYSTEM_DirectSound)
add_definitions(-DINTRA_LIBRARY_SOUND_SYSTEM=INTRA_LIBRARY_SOUND_SYSTEM_DirectSound)
target_link_libraries(MusicSynthesizer dsound)
elseif(LIBRARY_SOUND_SYSTEM_OpenAL)
add_definitions(-DINTRA_LIBRARY_SOUND_SYSTEM=INTRA_LIBRARY_SOUND_SYSTEM_OpenAL)
include_directories(${OPENAL_INCLUDE_DIRS})
target_link_libraries(MusicSynthesizer ${OPENAL_LIBRARIES})
elseif(LIBRARY_SOUND_SYSTEM_WebAudio)
add_definitions(-DINTRA_LIBRARY_SOUND_SYSTEM=INTRA_LIBRARY_SOUND_SYSTEM_WebAudio)
else()
add_definitions(-DINTRA_LIBRARY_SOUND_SYSTEM=INTRA_LIBRARY_SOUND_SYSTEM_Dummy)
endif()


target_link_libraries(MusicSynthesizer IntraLib)
