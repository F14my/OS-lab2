cmake_minimum_required(VERSION 3.16)

project(lab2_variant22 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_executable(lab2_child
        src/child.c
        src/line_io.c
)
target_include_directories(lab2_child PRIVATE include)

add_executable(lab2_parent
        src/main.c
        src/line_io.c
)
target_include_directories(lab2_parent PRIVATE include)

if(WIN32)
    target_sources(lab2_parent PRIVATE src/process_win.c)
    if(MSVC)
        target_compile_options(lab2_parent PRIVATE /utf-8)
        target_compile_options(lab2_child PRIVATE /utf-8)
    endif()
elseif(UNIX)
    target_sources(lab2_parent PRIVATE src/process_posix.c)
else()
    message(FATAL_ERROR "Эта операционная система пока не поддерживается")
endif()

target_compile_definitions(lab2_parent PRIVATE
        "CHILD_EXECUTABLE_PATH=\"$<TARGET_FILE:lab2_child>\""
)
add_dependencies(lab2_parent lab2_child)
