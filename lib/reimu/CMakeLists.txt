cmake_minimum_required(VERSION 3.29)

add_compile_options(-Wall -Wextra -fno-exceptions)

add_library(reimu SHARED
    lib.cpp
)

target_include_directories(reimu PUBLIC
    include
)

add_subdirectory(os/unix)

add_subdirectory(tests)
