cmake_minimum_required(VERSION 3.22.1)
project(parappa2_android)

set(CMAKE_CXX_STANDARD 17)

add_library(parappa2 SHARED
    native-lib.cpp
    ../common/rom_check.cpp
)

target_include_directories(parappa2 PRIVATE ../common)

find_library(log-lib log)
target_link_libraries(parappa2 ${log-lib})
