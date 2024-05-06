target_sources(reimu PRIVATE
    video.cpp
    window.cpp

    egl/egl.cpp

    wayland/wayland_client.cpp
    wayland/xdg-shell-protocol.c
)
