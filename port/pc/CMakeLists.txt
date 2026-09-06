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

# IMPORTANT: force console app on Windows (so stdout works in PowerShell/cmd)
if(WIN32)
    set_target_properties(TestForIssues PROPERTIES WIN32_EXECUTABLE FALSE)
    if(MSVC)
        target_link_options(TestForIssues PRIVATE "/SUBSYSTEM:CONSOLE")
    endif()
endif()

if(MSVC)
    target_compile_options(TestForIssues PRIVATE /W4 /utf-8)
else()
    target_compile_options(TestForIssues PRIVATE -Wall -Wextra)
endif()
