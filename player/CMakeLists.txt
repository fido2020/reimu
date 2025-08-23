find_package(PkgConfig)

add_executable(reimu-player
    main.cpp

    controls.cpp
)

find_package(FFMPEG REQUIRED)

target_include_directories(reimu-player PRIVATE ${FFMPEG_INCLUDE_DIRS})
target_link_directories(reimu-player PRIVATE ${FFMPEG_LIBRARY_DIRS})
target_link_libraries(reimu-player PUBLIC
    reimu
    ${FFMPEG_LIBRARIES}
)
