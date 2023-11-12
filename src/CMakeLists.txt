cmake_minimum_required(VERSION 3.8)

include_directories(.)

option(OUTPUT_MAP "Output .map file." OFF)

set(ADDITIONAL_COMPILER_PARAMETERS "" CACHE STRING "User-specified compiler flags")

set(COMMON_PARAMETERS "${ADDITIONAL_COMPILER_PARAMETERS}")

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND NOT "x${CMAKE_CXX_SIMULATE_ID}" STREQUAL "xMSVC" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_COMPILER_IS_GNUCXX)
    set(COMMON_PARAMETERS "${COMMON_PARAMETERS} -I Intra -std=c++20 -ftemplate-backtrace-limit=0")

    if(${CMAKE_GENERATOR} MATCHES "Unix Makefiles")
        set(COMMON_PARAMETERS "${COMMON_PARAMETERS} -pthread")
    endif()
	
	if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	
	elseif(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
		set(COMMON_PARAMETERS "${COMMON_PARAMETERS} --memory-init-file 0 -s NO_EXIT_RUNTIME=1 -s NO_FILESYSTEM=1 -s EXPORTED_RUNTIME_METHODS=\"['UTF8ToString', 'HEAPF32', '_free', '_malloc', 'HEAPU8']\" -s EXPORTED_FUNCTIONS=\"['UTF8ToString', 'HEAPF32', '_free', '_malloc', 'HEAPU8']\" ")
	else()
		set(COMMON_PARAMETERS "${COMMON_PARAMETERS} -D_FILE_OFFSET_BITS=64")
	endif()

    if(OUTPUT_MAP)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-Map=output.map" CACHE INTERNAL "" FORCE)
    endif()

    if(USE_EXCEPTIONS)
        set(COMMON_PARAMETERS "${COMMON_PARAMETERS} -fexceptions")
    else()
        set(COMMON_PARAMETERS "${COMMON_PARAMETERS} -fno-exceptions")
    endif()
    
    set(COMMON_MINSIZE_OPTIMIZATIONS "-fno-math-errno -fmerge-all-constants -ffunction-sections -fdata-sections -fno-rtti")
    
    if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
      set(COMMON_MINSIZE_OPTIMIZATIONS "${COMMON_MINSIZE_OPTIMIZATIONS} ")
    else()
      set(COMMON_MINSIZE_OPTIMIZATIONS "${COMMON_MINSIZE_OPTIMIZATIONS} -Wl,--gc-sections")
    endif()
    
    set(ALL_MINSIZE_OPTIMIZATIONS "${COMMON_MINSIZE_OPTIMIZATIONS} -fno-stack-protector -g0 -fno-unroll-loops -fno-asynchronous-unwind-tables")
    
    if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
      set(ALL_MINSIZE_OPTIMIZATIONS "${ALL_MINSIZE_OPTIMIZATIONS} ")
    else()
      set(ALL_MINSIZE_OPTIMIZATIONS "${ALL_MINSIZE_OPTIMIZATIONS} -Wl,-s")
	  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wl,-s")
    endif()
    
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${COMMON_PARAMETERS} ${COMMON_MINSIZE_OPTIMIZATIONS} -ffast-math")
    set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} ${COMMON_PARAMETERS} ${ALL_MINSIZE_OPTIMIZATIONS} -ffast-math")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${COMMON_PARAMETERS} -O0 -D_DEBUG")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} ${COMMON_PARAMETERS} ${COMMON_MINSIZE_OPTIMIZATIONS} -ffast-math -D_DEBUG")
    set(CMAKE_CXX_FLAGS "${COMMON_PARAMETERS}")
	
    if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2 --llvm-lto 3 -s ELIMINATE_DUPLICATE_FUNCTIONS=1 --closure 1 --memory-init-file 0")
        set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -Os --llvm-lto 3
            -s ELIMINATE_DUPLICATE_FUNCTIONS=1 --closure 1 --memory-init-file 0 -s NO_EXIT_RUNTIME=1 -s NO_FILESYSTEM=1
            -s EXPORTED_RUNTIME_METHODS=\"['UTF8ToString', 'HEAPF32', '_free', '_malloc', 'HEAPU8']\"
            -s EXPORTED_FUNCTIONS=\"['UTF8ToString', 'HEAPF32', '_free', '_malloc', 'HEAPU8']\" ")
        set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -g4")
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -s ASSERTIONS=2 -g4 -s SAFE_HEAP=1 -s DEMANGLE_SUPPORT=1")
		set(CMAKE_CXX_FLAGS "${COMMON_PARAMETERS} -O2 --llvm-lto 1")
	else()
		set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Ofast")
		set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -Os")
		set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g3")
		set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -Ofast -g3")
		set(CMAKE_CXX_FLAGS "${COMMON_PARAMETERS}")
    endif()
    
    if(CMAKE_BUILD_TYPE STREQUAL "")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS_MINSIZEREL}")
    endif()
elseif(MSVC)
    set(COMMON_PARAMETERS "${COMMON_PARAMETERS} /MP /permissive- /std:c++latest")

    if(USE_EXCEPTIONS)
        set(COMMON_PARAMETERS "${COMMON_PARAMETERS} /EHsc")
    else()
        set(COMMON_PARAMETERS "${COMMON_PARAMETERS} ")
    endif()

    set(CMAKE_CXX_FLAGS_RELEASE "${COMMON_PARAMETERS} /Ox /Ot /Oi /GT /GL /GF /Gy /MT /fp:fast")
    set(CMAKE_CXX_FLAGS_MINSIZEREL "${COMMON_PARAMETERS} /O1 /Os /Oi /GT /GL /GF /GS- /Gy /MT /fp:fast")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${COMMON_PARAMETERS} /O3 /Ot /Oi /GT /GL /GF /Gy /MT /fp:fast /Zi /D_DEBUG")
    set(CMAKE_CXX_FLAGS_DEBUG "${COMMON_PARAMETERS} /Od /MDd /Zi /D_DEBUG /RTC1 /fp:fast")
    set(CMAKE_CXX_FLAGS "${COMMON_PARAMETERS}")
	
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG")
	set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG /DYNAMICBASE:NO /FIXED")
	set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG")
	
    if(OUTPUT_MAP)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MAP")
    endif()
endif()

function(init_project_sources DIR HEADER_VARIABLE_NAME SOURCE_VARIABLE_NAME TEST_SOURCE_VARIABLE_NAME)
  file(GLOB_RECURSE headers RELATIVE "${DIR}" *.h)
  #file(GLOB_RECURSE sources RELATIVE "${DIR}" *.cpp)
  file(GLOB_RECURSE sources RELATIVE "${DIR}" *.cc)
  file(GLOB_RECURSE testSources RELATIVE "${DIR}" *.test.cxx)

  foreach(_source IN ITEMS ${headers})
    get_filename_component(_source_path "${_source}" PATH)
    string(REPLACE "/" "\\" _group_path "${_source_path}")
    source_group("${_group_path}" FILES "${_source}")
  endforeach()

  foreach(_source IN ITEMS ${sources})
    get_filename_component(_source_path "${_source}" PATH)
    string(REPLACE "/" "\\" _group_path "${_source_path}")
    source_group("${_group_path}" FILES "${_source}")
  endforeach()

  foreach(_source IN ITEMS ${testSources})
    get_filename_component(_source_path "${_source}" PATH)
    string(REPLACE "/" "\\" _group_path "${_source_path}")
    source_group("${_group_path}" FILES "${_source}")
  endforeach()
  
  set(${HEADER_VARIABLE_NAME} ${${HEADER_VARIABLE_NAME}} ${headers} PARENT_SCOPE)
  set(${SOURCE_VARIABLE_NAME} ${${SOURCE_VARIABLE_NAME}} ${sources} PARENT_SCOPE)
  set(${TEST_SOURCE_VARIABLE_NAME} ${${TEST_SOURCE_VARIABLE_NAME}} ${testSources} PARENT_SCOPE)
endfunction(init_project_sources)


function(exclude_project_sources_directory DIR HEADER_VARIABLE_NAME SOURCE_VARIABLE_NAME)
	foreach(CURRENT_PATH ${${HEADER_VARIABLE_NAME}})
		string(FIND ${CURRENT_PATH} ${DIR} EXCLUDE_DIR_FOUND)
		if(${EXCLUDE_DIR_FOUND} EQUAL -1)
			list(APPEND NEW_HEADER_LIST ${CURRENT_PATH})
		endif()
	endforeach(CURRENT_PATH)
	set(${HEADER_VARIABLE_NAME} ${NEW_HEADER_LIST} PARENT_SCOPE)
	foreach(CURRENT_PATH ${${SOURCE_VARIABLE_NAME}})
		string(FIND ${CURRENT_PATH} ${DIR} EXCLUDE_DIR_FOUND)
		if(${EXCLUDE_DIR_FOUND} EQUAL -1)
			list(APPEND NEW_SOURCE_LIST ${CURRENT_PATH})
		endif()
	endforeach(CURRENT_PATH)
	set(${SOURCE_VARIABLE_NAME} ${NEW_SOURCE_LIST} PARENT_SCOPE)
endfunction(exclude_project_sources_directory)



function(enable_unity_build UB_NAME SOURCE_VARIABLE_NAME)
  set(files ${${SOURCE_VARIABLE_NAME}})
  # Generate a unique filename for the unity build translation unit
  set(unit_build_file ${CMAKE_CURRENT_BINARY_DIR}/${UB_NAME}.UB.cc)
  # Exclude all translation units from compilation
  set_source_files_properties(${files} PROPERTIES HEADER_FILE_ONLY true)
  # Open the ub file
  FILE(WRITE ${unit_build_file} "// Unity Build generated by CMake\n")
  # Add include statement for each translation unit
  foreach(source_file ${files} )
    FILE( APPEND ${unit_build_file} "#include <${CMAKE_CURRENT_SOURCE_DIR}/${source_file}>\n")
  endforeach(source_file)
  # Complement list of translation units with the name of ub
  set(${SOURCE_VARIABLE_NAME} ${${SOURCE_VARIABLE_NAME}} ${unit_build_file} PARENT_SCOPE)  
endfunction(enable_unity_build)

add_subdirectory(Intra)
#add_subdirectory(IntraX)
