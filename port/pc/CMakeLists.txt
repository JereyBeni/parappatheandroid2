cmake_minimum_required(VERSION 3.16)
project(TestForIssues LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(TestForIssues
    main.cpp
    ../common/rom_check.cpp
)

target_include_directories(TestForIssues PRIVATE ../common)

# Output name exactly as requested
set_target_properties(TestForIssues PROPERTIES OUTPUT_NAME "TestForIssues")

if(MSVC)
    target_compile_options(TestForIssues PRIVATE /W4)
else()
    target_compile_options(TestForIssues PRIVATE -Wall -Wextra)
endif()
