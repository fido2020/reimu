find_package(PkgConfig)

add_executable(reimu-player
    main.cpp
    device.cpp
    controls.cpp
)

find_package(FFMPEG REQUIRED)

# print warning
message(WARNING "FFMPEG_INCLUDE_DIRS: ${FFMPEG_INCLUDE_DIRS}")
message(WARNING "FFMPEG_LIBRARY_DIRS: ${FFMPEG_LIBRARY_DIRS}")
message(WARNING "FFMPEG_LIBRARIES: ${FFMPEG_LIBRARIES}")

target_include_directories(reimu-player PRIVATE ${FFMPEG_INCLUDE_DIRS})
target_link_directories(reimu-player PRIVATE ${FFMPEG_LIBRARY_DIRS})

target_link_libraries(reimu-player PRIVATE
    ${FFMPEG_LIBRARIES}
)

target_link_libraries(reimu-player PRIVATE
    reimu
    miniaudio
)
