find_package(PkgConfig)

pkg_check_modules(wayland REQUIRED wayland-server wayland-client)
pkg_check_modules(libdrm REQUIRED libdrm)

add_executable(reimu-wm main.cpp)

target_include_directories(reimu-wm PRIVATE
    ${libdrm_INCLUDE_DIRS}
)

target_link_libraries(reimu-wm
    reimu
    ${wayland_LINK_LIBRARIES}
    ${libdrm_LINK_LIBRARIES}
)
