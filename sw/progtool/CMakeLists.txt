
cmake_minimum_required (VERSION 2.6)
project (progtool)

set(progtool_sources progtool.c ../common/hidprog.c ../common/hidprog_cmds.h)

if (WIN32)
    set(progtool_sources ${progtool_sources} hidapi/windows/hid.c)
    include_directories ("${PROJECT_SOURCE_DIR}/hidapi/")
	  include_directories ("${PROJECT_SOURCE_DIR}/hidapi/hidapi")
endif (WIN32)

include_directories ("${PROJECT_SOURCE_DIR}")
include_directories ("${PROJECT_SOURCE_DIR}/../bootloader")
include_directories ("${PROJECT_SOURCE_DIR}/../common/")

add_executable(progtool ${progtool_sources})

if (WIN32)
	target_link_libraries(progtool setupapi)
endif (WIN32)

if (UNIX)
    target_link_libraries(progtool hidapi-libusb)
endif (UNIX)

if(MSVC)
  target_compile_options(progtool PRIVATE /W4 )
else(MSVC)
  target_compile_options(progtool PRIVATE -Wall -Wextra -pedantic -Werror)
endif(MSVC)

