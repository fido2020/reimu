target_sources(reimu PRIVATE
    video.cpp
    window.cpp
)

if(WAYLAND)
    target_sources(reimu PRIVATE
        egl/egl.cpp

        wayland/wayland_client.cpp
        wayland/xdg-shell-protocol.c
    )
endif()

if(WINDOWS)
    target_sources(reimu PRIVATE
        win/win32.cpp
    )
endif()
