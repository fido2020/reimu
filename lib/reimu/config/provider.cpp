#include <reimu/config/config.h>

#include <nlohmann/json.hpp>
#include <print>

#include "private.h"

namespace reimu::config {

struct JSONError : public reimu::ErrorBase {
    JSONError(std::string message) : message(std::move(message)) {}

    std::string as_string() const override {
        return message;
    }
    
    std::string message;
};

struct JSONConfigProvider::JSONData {
    nlohmann::basic_json<> json;
};

JSONConfigProvider::JSONConfigProvider() {}
JSONConfigProvider::~JSONConfigProvider() {};

Result<detail::ConfigValue, ConfigError> JSONConfigProvider::get_value(const std::string &key, int type) {
    if (!m_data) {
        auto e = ConfigError(ConfigError::NotFound, key);

        return ERR(std::move(e));
    }

    auto it = m_data->json.find(key);
    if (it == m_data->json.end()) {
        auto e = ConfigError(ConfigError::NotFound, key);
        return ERR(std::move(e));
    }

    detail::ConfigValue value;
    switch (type) {
        case detail::ValueType::String:
            if (!it->is_string()) {
                return ERR(ConfigError(ConfigError::InvalidType, key));
            }
            value = detail::ConfigValue::string(it->get<std::string>());
            break;
        case detail::ValueType::Integer:
            if (!it->is_number_integer()) {
                return ERR(ConfigError(ConfigError::InvalidType, key));
            }
            value = detail::ConfigValue::integer(it->get<signed>());
            break;
        case detail::ValueType::Double:
            if (!it->is_number_float()) {
                return ERR(ConfigError(ConfigError::InvalidType, key));
            }
            value = detail::ConfigValue::decimal(it->get<double>());
            break;
        case detail::ValueType::Boolean:
            if (!it->is_boolean()) {
                return ERR(ConfigError(ConfigError::InvalidType, key));
            }
            value = detail::ConfigValue::boolean(it->get<bool>());
            break;
        default:
            return ERR(ConfigError(ConfigError::InvalidType, key));
    }

    return OK(value);
}

Result<void, reimu::ErrorBox> JSONConfigProvider::load_from_string(std::string json) {
    if (m_data) {
        return ERR(JSONError{"Configuration already loaded"});
    }

    m_data = std::make_unique<JSONData>();

    auto j = nlohmann::json::parse(json, nullptr, false);
    if (j.is_discarded()) {
        return ERR(JSONError{"Invalid JSON string"});
    }

    m_data->json = std::move(j);

    return OK();
}

}