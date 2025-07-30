cmake_minimum_required(VERSION 3.28)

add_compile_options(-Wall -Wextra -fno-exceptions)

find_package(PkgConfig)

add_library(reimu SHARED
    event.cpp
    lib.cpp
    unicode.cpp
    resource_manager.cpp
)

target_include_directories(reimu PUBLIC
    include
)

target_compile_options(freetype PUBLIC
    -fPIC
)

target_link_libraries(reimu
    freetype
    webgpu
)

target_include_directories(reimu PRIVATE
    ${CMAKE_SOURCE_DIR}/thirdparty/freetype/include
)

if (WAYLAND)
    pkg_check_modules(wayland REQUIRED wayland-server wayland-client wayland-egl wayland-cursor)
    pkg_check_modules(egl REQUIRED egl)

    pkg_check_modules(xkb REQUIRED xkbcommon)

    target_link_libraries(reimu
        ${wayland_LIBRARIES}
        ${xkb_LIBRARIES}
        ${egl_LIBRARIES}
    )

    target_compile_definitions(reimu PRIVATE
        -DREIMU_VIDEO_WAYLAND
    )
endif()

if (WINDOWS)
    target_compile_definitions(reimu PRIVATE
        -DREIMU_WIN32
    )

    add_subdirectory(os/win)
elseif(UNIX)
    target_compile_definitions(reimu PRIVATE
        -DREIMU_UNIX
    )

    add_subdirectory(os/unix)
endif()

add_subdirectory(graphics)
add_subdirectory(gui)
add_subdirectory(video)

add_subdirectory(tests)
