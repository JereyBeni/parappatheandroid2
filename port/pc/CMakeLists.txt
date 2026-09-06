cmake_minimum_required(VERSION 3.16)
project(TestForIssues LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- GUI version (default output: TestForIssues.exe) ----
add_executable(TestForIssues WIN32
    main_gui.cpp
    ../common/rom_check.cpp
)

target_include_directories(TestForIssues PRIVATE ../common)

if(MSVC)
    target_compile_options(TestForIssues PRIVATE /W4 /utf-8)
    target_link_libraries(TestForIssues PRIVATE comdlg32 user32 gdi32)
else()
    target_compile_options(TestForIssues PRIVATE -Wall -Wextra)
    # MinGW
    target_link_libraries(TestForIssues PRIVATE comdlg32 user32 gdi32)
endif()

# ---- Console version (optional, for debugging) ----
add_executable(TestForIssues_console
    main.cpp
    ../common/rom_check.cpp
)
target_include_directories(TestForIssues_console PRIVATE ../common)
if(WIN32)
    set_target_properties(TestForIssues_console PROPERTIES WIN32_EXECUTABLE FALSE)
    if(MSVC)
        target_link_options(TestForIssues_console PRIVATE "/SUBSYSTEM:CONSOLE")
        target_compile_options(TestForIssues_console PRIVATE /W4 /utf-8)
    endif()
endif()
