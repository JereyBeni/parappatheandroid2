```cmake
cmake_minimum_required(VERSION 3.16)
project(TestForIssues LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ------------------------------------------------------------
# Common
# ------------------------------------------------------------

set(COMMON
    ../common/rom_check.cpp
    ../common/iso9660.cpp
    ../common/gs_translate.cpp
    ../common/vk_backend.cpp
    ../common/host_translate.cpp
)

# ------------------------------------------------------------
# prlib
# ------------------------------------------------------------

file(GLOB PRLIB
    CONFIGURE_DEPENDS
    ../prlib/*.cpp
    ../prlib/*.c
)

file(GLOB PRLIB_VU1
    CONFIGURE_DEPENDS
    ../prlib/vu1/*.cpp
    ../prlib/vu1/*.c
)

list(APPEND PRLIB ${PRLIB_VU1})

# ------------------------------------------------------------
# Include paths
# ------------------------------------------------------------

set(PRLIB_INCLUDE_DIRS
    ../prlib
    ../prlib/vu1
    ../common
)

# ------------------------------------------------------------
# Sources
# ------------------------------------------------------------

set(ALL_SOURCES
    ${COMMON}
    ${PRLIB}
)

# ------------------------------------------------------------
# Windows
# ------------------------------------------------------------

if(WIN32)

    add_executable(
        TestForIssues
        WIN32
        main_gui.cpp
        ${ALL_SOURCES}
    )

    target_include_directories(
        TestForIssues
        PRIVATE
        ${PRLIB_INCLUDE_DIRS}
    )

    target_link_libraries(
        TestForIssues
        PRIVATE
        comdlg32
        user32
        gdi32
    )

    if(MSVC)
        target_compile_options(
            TestForIssues
            PRIVATE
            /W4
            /utf-8
        )
    endif()


    # --------------------------------------------------------
    # Console
    # --------------------------------------------------------

    add_executable(
        TestForIssues_console
        main.cpp
        ${ALL_SOURCES}
    )

    target_include_directories(
        TestForIssues_console
        PRIVATE
        ${PRLIB_INCLUDE_DIRS}
    )

    set_target_properties(
        TestForIssues_console
        PROPERTIES
        WIN32_EXECUTABLE FALSE
    )

    if(MSVC)
        target_link_options(
            TestForIssues_console
            PRIVATE
            /SUBSYSTEM:CONSOLE
        )

        target_compile_options(
            TestForIssues_console
            PRIVATE
            /W4
            /utf-8
        )
    endif()

else()

    add_executable(
        TestForIssues
        main.cpp
        ${ALL_SOURCES}
    )

    target_include_directories(
        TestForIssues
        PRIVATE
        ${PRLIB_INCLUDE_DIRS}
    )

endif()
```
