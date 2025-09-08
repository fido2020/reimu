link_libraries(reimu)

add_executable(error
    error.cpp
)
add_test(error_test error)

add_executable(config
    config.cpp
)

target_link_libraries(config PUBLIC
    reimu
)

add_test(config_test config)

add_executable(markup
    markup.cpp
)

target_link_libraries(markup PUBLIC
    reimu
)

add_test(markup_test markup)
