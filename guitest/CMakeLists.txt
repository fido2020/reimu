find_package(PkgConfig)

add_executable(guitest
    main.cpp
)

target_link_libraries(guitest PUBLIC
    reimu
)
