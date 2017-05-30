cmake_minimum_required(VERSION 3.0)


set(CMAKE_C_USE_RESPONSE_FILE_FOR_OBJECTS 0)
set(CMAKE_C_USE_RESPONSE_FILE_FOR_LIBRARIES 0)
set(CMAKE_CXX_USE_RESPONSE_FILE_FOR_OBJECTS 0)
set(CMAKE_CXX_USE_RESPONSE_FILE_FOR_LIBRARIES 0)

#link_directories("/this/is/a/test/path")

find_package(PkgConfig)
macro(get_lib_fullpath _prefix)
	foreach(dir ${${_prefix}_LIBRARY_DIRS})
		message(STATUS "library dir='${dir}'")
		foreach(lib ${${_prefix}_LIBRARIES})
			message(STATUS "library name='${lib}'")
			find_library(${_prefix}_LIB_TMP ${lib} PATHS ${dir} NO_DEFAULT_PATH)
			if(${_prefix}_LIB_TMP)
				message(STATUS "library='${${_prefix}_LIB_TMP}'")
				LIST(APPEND ${_prefix}_LIBRARIES_FULLPATH "${${_prefix}_LIB_TMP}")
			endif(${_prefix}_LIB_TMP)
			set(${_prefix}_LIB_TMP "")
		endforeach()
	endforeach()
endmacro()

pkg_check_modules(XML2 REQUIRED libxml-2.0)
message(STATUS "XML2 library dirs: ${XML2_LIBRARY_DIRS}")
message(STATUS "XML2 libraries: ${XML2_LIBRARIES}")
get_lib_fullpath(XML2)
message(STATUS "XML2 libraries (full paths): ${XML2_LIBRARIES_FULLPATH}")

include_directories(${XML2_INCLUDE_DIRS})
#link_directories(${XML2_LIBRARY_DIRS})

add_executable(main main.cc)

target_link_libraries(main
  ${XML2_LIBRARIES_FULLPATH}
  "/path/to/lib,ylib.dll.a"
)
