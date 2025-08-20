#include <reimu/config/config.h>

#include <cassert>

int main() {
    std::string json_config = R"({
        "number": 100,
        "string": "Hello, World!",
        "boolean": true
    })";

    reimu::config::Config config;

    config.add_key<int>("number")
        .add_key<std::string>("string")
        .add_key<bool>("boolean");

    reimu::config::JSONConfigProvider json_provider;
    json_provider.load_from_string(json_config)
        .ensure();

    config.load(json_provider);

    auto number = config.get_key<int>("number");
    assert(number.has_some());
    assert(number.ensure() == 100);

    auto str = config.get_key<std::string>("string");
    assert(str.has_some());
    assert(str.ensure().compare("Hello, World!") == 0);

    auto boolean = config.get_key<bool>("boolean");
    assert(boolean.has_some());
    assert(boolean.ensure() == true);
}