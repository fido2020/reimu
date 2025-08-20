#pragma once

#include <reimu/config/provider.h>
#include <reimu/core/optional.h>

#include <string>
#include <map>

namespace reimu::config {

namespace detail {

struct ConfigValue;
struct ConfigPrivateData;

}

class Config {
public:
    Config();
    ~Config();

    void load(ConfigProvider &provider);

    template<typename T>
    Config &add_key(std::string key);

    template<typename T>
    Optional<T> get_key(const std::string &key) const;

private:
    void impl_add_key(std::string key, int type);

    detail::ConfigPrivateData *m_data;
};

}
