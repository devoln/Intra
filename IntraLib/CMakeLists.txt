cmake_minimum_required(VERSION 2.8)

project(IntraLib)

include(include/CMakeFileList.txt)
foreach(FILE ${INTRA_LIB_HEADERS})
	set(INTRA_LIB_HEADERS1, ${INTRA_LIB_HEADERS} include/${FILE})
endforeach

include(src/CMakeFileList.txt)
foreach(FILE ${INTRA_LIB_SOURCE})
	set(INTRA_LIB_SOURCE1, ${INTRA_LIB_SOURCE} src/${FILE})
endforeach

add_library(IntraLib STATIC ${INTRA_LIB_SOURCE1} ${INTRA_LIB_HEADERS1})

