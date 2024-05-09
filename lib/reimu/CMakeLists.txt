cmake_minimum_required(VERSION 3.29)

add_compile_options(-Wall -Wextra -fno-exceptions)

find_package(PkgConfig)

pkg_check_modules(wayland REQUIRED wayland-server wayland-client wayland-egl)
pkg_check_modules(egl REQUIRED egl)

add_library(reimu SHARED
    lib.cpp
)

target_include_directories(reimu PUBLIC
    include
)

target_link_libraries(reimu PUBLIC
    ${wayland_LIBRARIES}
    ${egl_LIBRARIES}
    webgpu
)

add_subdirectory(graphics)
add_subdirectory(video)

add_subdirectory(os/unix)

add_subdirectory(tests)
