
cmake_minimum_required (VERSION 2.6)
project (progtool)

include_directories ("${PROJECT_SOURCE_DIR}")
include_directories ("${PROJECT_SOURCE_DIR}/../bootloader")


add_executable(progtool progtool.c)
target_link_libraries(progtool hidapi-libusb)

