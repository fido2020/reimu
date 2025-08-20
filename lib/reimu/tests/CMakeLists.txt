link_libraries(reimu)

add_executable(error
    error.cpp
)
add_test(error_test error)

add_executable(config
    config.cpp
)

add_test(config_test config)
