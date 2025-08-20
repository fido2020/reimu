#include <reimu/config/config.h>
#include <string>
#include <format>

#include "private.h"
#include "reimu/core/optional.h"

namespace reimu::config {

namespace detail {
    struct ConfigPrivateData {
        std::map<std::string, std::pair<ValueType, ConfigValue>> values;
    };
}

std::string ConfigError::as_string() const {
    switch (type) {
    case NotFound:
        return std::format("Config key '{}' not found", key);
    case InvalidType:
        return std::format("Invalid config value type for key '{}'", key);
    default:
        return std::format("Unknown config error");
    }
}

Config::Config() {
    m_data = new detail::ConfigPrivateData();
}

Config::~Config() {
    delete m_data;
}

void Config::load(ConfigProvider &provider) {
    for (const auto &v : m_data->values) {
        auto result = provider.get_value(v.first, v.second.first);
        if (result.is_err()) {
            continue;
        }

        m_data->values[v.first].second = result.ensure();
    }
}

template<>
Config &Config::add_key<int>(std::string key) {
    impl_add_key(std::move(key), detail::ValueType::Integer);

    return *this;
}

template<>
Config &Config::add_key<std::string>(std::string key) {
    impl_add_key(std::move(key), detail::ValueType::String);

    return *this;
}

template<>
Config &Config::add_key<bool>(std::string key) {
    impl_add_key(std::move(key), detail::ValueType::Boolean);

    return *this;
}

template<>
Config &Config::add_key<double>(std::string key) {
    impl_add_key(std::move(key), detail::ValueType::Double);

    return *this;
}

void Config::impl_add_key(std::string key, int type) {
    m_data->values[key] = {static_cast<detail::ValueType>(type), {}};
}

template<>
Optional<int> Config::get_key<int>(const std::string &key) const {
    auto it = m_data->values.find(key);
    if (it == m_data->values.end()) {
        return {};
    }

    if (it->second.second.type != detail::ValueType::Integer) {
        return {};
    }

    return it->second.second._signed;
}

template<>
Optional<std::string> Config::get_key<std::string>(const std::string &key) const {
    auto it = m_data->values.find(key);
    if (it == m_data->values.end()) {
        return {};
    }

    if (it->second.second.type != detail::ValueType::String) {
        return {};
    }

    return it->second.second._string;
}

template<>
Optional<bool> Config::get_key<bool>(const std::string &key) const {
    auto it = m_data->values.find(key);
    if (it == m_data->values.end()) {
        return {};
    }

    if (it->second.second.type != detail::ValueType::Boolean) {
        return {};
    }

    return it->second.second._signed != 0;
}

template<>
Optional<double> Config::get_key<double>(const std::string &key) const {
    auto it = m_data->values.find(key);
    if (it == m_data->values.end()) {
        return {};
    }

    if (it->second.second.type != detail::ValueType::Double) {
        return {};
    }

    return it->second.second._double;
}

}
