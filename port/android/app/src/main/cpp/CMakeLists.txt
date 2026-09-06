cmake_minimum_required(VERSION 3.22.1)
project(parappa2)

add_library(parappa2 SHARED
    native-lib.cpp
    ${CMAKE_SOURCE_DIR}/../../../common/rom_check.cpp
    ${CMAKE_SOURCE_DIR}/../../../common/iso9660.cpp
    ${CMAKE_SOURCE_DIR}/../../../common/prlib_stub.cpp
)

target_include_directories(parappa2 PRIVATE
    ${CMAKE_SOURCE_DIR}/../../../common
)

find_library(log-lib log)
target_link_libraries(parappa2 ${log-lib})
