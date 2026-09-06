cmake_minimum_required(VERSION 3.22.1)
project(parappa2)

# CI copies port/common/* next to this CMakeLists.txt
set(SRC native-lib.cpp)

if(EXISTS "${CMAKE_SOURCE_DIR}/rom_check.cpp")
    list(APPEND SRC rom_check.cpp)
endif()
if(EXISTS "${CMAKE_SOURCE_DIR}/iso9660.cpp")
    list(APPEND SRC iso9660.cpp)
endif()
if(EXISTS "${CMAKE_SOURCE_DIR}/prlib_stub.cpp")
    list(APPEND SRC prlib_stub.cpp)
endif()

add_library(parappa2 SHARED ${SRC})
target_include_directories(parappa2 PRIVATE ${CMAKE_SOURCE_DIR})

find_library(log-lib log)
target_link_libraries(parappa2 ${log-lib})
