project(reimu)

enable_testing()
enable_language(CXX)

add_compile_options($<$<C_COMPILER_ID:Clang>:-fcolor-diagnostics>)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    set(WINDOWS 1)
else()
    set(WAYLAND 1)
    set(UNIX 1)
endif()

add_subdirectory(thirdparty/freetype)
add_subdirectory(thirdparty/miniaudio)
add_subdirectory(thirdparty/json)

# Make sure the MSVC ABI is used for WebGPU
set(MSVC TRUE CACHE BOOL "Pretend MSVC for webgpu")
add_subdirectory(thirdparty/webgpu)
unset(MSVC CACHE)

add_subdirectory(lib/reimu)
add_subdirectory(guitest)
add_subdirectory(terminal)
add_subdirectory(player)

if (UNIX)
    add_subdirectory(servers/wm)
endif()
