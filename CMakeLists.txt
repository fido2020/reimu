project(reimu)

add_compile_options($<$<C_COMPILER_ID:Clang>:-fcolor-diagnostics>)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(lib/reimu)
add_subdirectory(servers/wm)
add_subdirectory(guitest)
