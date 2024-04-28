find_package(PkgConfig)

pkg_check_modules(wayland REQUIRED wayland-server wayland-client)

add_executable(reimu-wm main.cpp)

target_link_libraries(reimu-wm
    reimu
    ${wayland_LINK_LIBRARIES}
)
