link_directories("/this/is/a/test/path")

find_package(PkgConfig)
pkg_check_modules(FFTW3 REQUIRED fftw3)

include_directories(${XML2_INCLUDE_DIRS})
link_directories(${XML2_LIBRARY_DIRS})

add_executable(test main.cc)

target_link_libraries(test
  ${XML2_LIBRARIES}
)
