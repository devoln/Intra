cmake_minimum_required(VERSION 2.8)

project(IntraLib)

include_directories(include)

include(include/CMakeFileList.txt)
foreach(FILE ${INTRA_LIB_HEADERS})
	set(INTRA_LIB_HEADERS1 ${INTRA_LIB_HEADERS1} include/${FILE})
endforeach(FILE)

include(src/CMakeFileList.txt)
foreach(FILE ${INTRA_LIB_SOURCE})
	set(INTRA_LIB_SOURCE1 ${INTRA_LIB_SOURCE1} src/${FILE})
endforeach(FILE)

add_library(IntraLib STATIC ${INTRA_LIB_SOURCE1} ${INTRA_LIB_HEADERS1})

