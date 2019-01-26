
cmake_minimum_required (VERSION 3.4)
project (proggui)

set(proggui_sources proggui.c proggui.rc proggui.manifest)
set(proggui_sources  ${proggui_sources} ../common/hidprog.c ../common/hidprog_cmds.h)
set(proggui_sources  ${proggui_sources} ../ext/hidapi/windows/hid.c)
include_directories ("${PROJECT_SOURCE_DIR}/../ext/hidapi/")
include_directories ("${PROJECT_SOURCE_DIR}/../ext/hidapi/hidapi")
include_directories ("${PROJECT_SOURCE_DIR}")
include_directories ("${PROJECT_SOURCE_DIR}/../bootloader")
include_directories ("${PROJECT_SOURCE_DIR}/../common/")

add_definitions(-DUNICODE -D_UNICODE)

add_executable(proggui WIN32 ${proggui_sources})


target_link_libraries(proggui setupapi comctl32)


if(MINGW)
  set(CMAKE_RC_COMPILER_INIT windres)
  ENABLE_LANGUAGE(RC)
  SET(CMAKE_RC_COMPILE_OBJECT
    "<CMAKE_RC_COMPILER> <FLAGS> -O coff <DEFINES> -i <SOURCE> -o <OBJECT>")
endif(MINGW)

if(MSVC)
  target_compile_options(proggui PRIVATE /W3 )
  foreach(flag_var
    CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
    CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)
    if(${flag_var} MATCHES "/MD")
      string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
    endif(${flag_var} MATCHES "/MD")
  endforeach(flag_var)
else(MSVC)
  target_compile_options(proggui PRIVATE -Wall -Wextra)
endif(MSVC)

