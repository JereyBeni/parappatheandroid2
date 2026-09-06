cmake_minimum_required(VERSION 3.22.1)
project(parappa2)

# CI copies port/common/* next to this CMakeLists.txt
set(SRC native-lib.cpp)

foreach(f rom_check iso9660 prlib_stub gs_translate vk_backend)
    if(EXISTS "${CMAKE_SOURCE_DIR}/${f}.cpp")
        list(APPEND SRC ${f}.cpp)
    endif()
endforeach()

add_library(parappa2 SHARED ${SRC})
target_include_directories(parappa2 PRIVATE ${CMAKE_SOURCE_DIR})

find_library(log-lib log)
target_link_libraries(parappa2 ${log-lib})
