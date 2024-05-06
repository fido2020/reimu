find_package(PkgConfig)

pkg_check_modules(wayland REQUIRED wayland-server wayland-client)

add_executable(guitest
    main.cpp
)

target_link_libraries(guitest PUBLIC
    reimu
    ${wayland_LINK_LIBRARIES}
)
