cmake_minimum_required(VERSION 3.0)

#add_definitions (-D_GLIBCXX_USE_CXX11_ABI=0)


set(CMAKE_C_USE_RESPONSE_FILE_FOR_OBJECTS 0)
set(CMAKE_C_USE_RESPONSE_FILE_FOR_LIBRARIES 0)
set(CMAKE_CXX_USE_RESPONSE_FILE_FOR_OBJECTS 0)
set(CMAKE_CXX_USE_RESPONSE_FILE_FOR_LIBRARIES 0)

set(CMAKE_BUILD_TYPE RELEASE)

SET(CMAKE_CXX_FLAGS_RELEASE " -g -O3 -fpermissive") 
#SET(CMAKE_CXX_FLAGS_RELEASE " -O3 -ftree-vectorize -std=gnu++11 -fpermissive") 

#link_directories("/this/is/a/test/path")

find_package(PkgConfig)
macro(get_lib_fullpath _prefix)
	foreach(dir ${${_prefix}_LIBRARY_DIRS})
		message(STATUS "library dir='${dir}'")
		foreach(lib ${${_prefix}_LIBRARIES})
			message(STATUS "library name='${lib}'")
			find_library(${_prefix}_LIB_TMP ${lib} PATHS ${dir} NO_DEFAULT_PATH)
			message(STATUS "library='${${_prefix}_LIB_TMP}'")
			if(${_prefix}_LIB_TMP)
				if(${_prefix}_LIBRARIES_FULLPATH)
				#	set(${_prefix}_LIBRARIES_FULLPATH "${${_prefix}_LIBRARIES_FULLPATH} ${${_prefix}_LIB_TMP}")
					LIST(APPEND ${_prefix}_LIBRARIES_FULLPATH "${${_prefix}_LIB_TMP}")
				else(${_prefix}_LIBRARIES_FULLPATH)
				#	set(${_prefix}_LIBRARIES_FULLPATH "${${_prefix}_LIB_TMP}")
					LIST(APPEND ${_prefix}_LIBRARIES_FULLPATH "${${_prefix}_LIB_TMP}")
				endif(${_prefix}_LIBRARIES_FULLPATH)
				#LIST(APPEND ${_prefix}_LIBRARIES_FULLPATH "${${_prefix}_LIB_TMP}")
				#LIST(APPEND ${_prefix}_LIBRARIES_FULLPATH "-L${dir}")
				#LIST(APPEND ${_prefix}_LIBRARIES_FULLPATH "-l${lib}")
			endif(${_prefix}_LIB_TMP)
			unset(${_prefix}_LIB_TMP CACHE)
		endforeach()
	endforeach()
endmacro()

pkg_check_modules(XML2 REQUIRED libxml-2.0)
message(STATUS "XML2 library dirs: ${XML2_LIBRARY_DIRS}")
message(STATUS "XML2 libraries: ${XML2_LIBRARIES}")
get_lib_fullpath(XML2)
message(STATUS "XML2 libraries (full paths): ${XML2_LIBRARIES_FULLPATH}")

#pkg_check_modules(GLIBMM glibmm-2.4) 
#get_lib_fullpath(GLIBMM)

pkg_check_modules(VIPS REQUIRED vips>=8.3.3)
#pkg_check_modules(VIPSCC REQUIRED vipsCC)
MESSAGE( STATUS "VIPS_INCLUDE_DIRS:         " ${VIPS_INCLUDE_DIRS} )
MESSAGE( STATUS "VIPS_LIBRARY_DIRS:         " ${VIPS_LIBRARY_DIRS} )
MESSAGE( STATUS "VIPS_LIBRARIES:            " ${VIPS_LIBRARIES} )
get_lib_fullpath(VIPS)


include_directories(${XML2_INCLUDE_DIRS})
include_directories(${VIPS_INCLUDE_DIRS})
#link_directories(${XML2_LIBRARY_DIRS})

add_executable(vipstest main.cc)
#set_target_properties(vipstest LINK_FLAGS "-static-libgcc -static-libstdc++")
target_link_libraries(vipstest
  ${XML2_LIBRARIES}
  ${VIPS_LIBRARIES}
)

INSTALL(TARGETS vipstest RUNTIME DESTINATION bin)
