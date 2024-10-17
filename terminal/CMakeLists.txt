find_package(PkgConfig)

add_executable(reimu-terminal
    main.cpp
)

target_link_libraries(reimu-terminal PUBLIC
    reimu
)
