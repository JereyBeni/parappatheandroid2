cmake_minimum_required(VERSION 3.16)
project(TestForIssues LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(COMMON_SRC
    ../common/rom_check.cpp
    ../common/iso9660.cpp
    ../common/prlib_stub.cpp
    ../common/gs_translate.cpp
    ../common/vk_backend.cpp
    ../common/emu_session.cpp
)

if(WIN32)
    add_executable(TestForIssues WIN32 main_gui.cpp ${COMMON_SRC})
    target_include_directories(TestForIssues PRIVATE ../common)
    if(MSVC)
        target_compile_options(TestForIssues PRIVATE /W4 /utf-8)
    else()
        target_compile_options(TestForIssues PRIVATE -Wall -Wextra)
    endif()
    target_link_libraries(TestForIssues PRIVATE comdlg32 user32 gdi32)

    add_executable(TestForIssues_console main.cpp ${COMMON_SRC})
    target_include_directories(TestForIssues_console PRIVATE ../common)
    set_target_properties(TestForIssues_console PROPERTIES WIN32_EXECUTABLE FALSE)
    if(MSVC)
        target_link_options(TestForIssues_console PRIVATE "/SUBSYSTEM:CONSOLE")
        target_compile_options(TestForIssues_console PRIVATE /W4 /utf-8)
    endif()
else()
    add_executable(TestForIssues main.cpp ${COMMON_SRC})
    target_include_directories(TestForIssues PRIVATE ../common)
    target_compile_options(TestForIssues PRIVATE -Wall -Wextra)
endif()
