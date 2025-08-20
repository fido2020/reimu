find_package(PkgConfig)

add_executable(reimu-player
    main.cpp
)

target_link_libraries(reimu-player PUBLIC
    reimu
    miniaudio
)
