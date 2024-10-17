project(reimu)

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

add_subdirectory(lib/reimu)
add_subdirectory(guitest)
add_subdirectory(terminal)

if (UNIX)
    add_subdirectory(servers/wm)
endif()

add_subdirectory(thirdparty/webgpu)
