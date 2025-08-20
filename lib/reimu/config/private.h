#pragma once

#include <string>

namespace reimu::config::detail {
    enum ValueType {
        String,
        Integer,
        Double,
        Boolean,
        Null
    };

    struct ConfigValue {
        ValueType type;

        union {
            std::string _string;
            signed _signed;
            double _double;
        };

        ConfigValue() : type(Null) {}

        ConfigValue(const ConfigValue &v) : type(v.type) {
            switch (type) {
                case String:
                    new (&_string) std::string(v._string);
                    break;
                case Boolean:
                case Integer:
                    _signed = v._signed;
                    break;
                case Double:
                    _double = v._double;
                    break;
                default:
                    break;
            }
        }

        ~ConfigValue() {
            if (type == String) {
                _string.~basic_string();
            }
        }

        ConfigValue& operator=(const ConfigValue &v) {
            if (this != &v) {
                this->~ConfigValue();
                new (this) ConfigValue(v);
            }
            return *this;
        }

        static ConfigValue string(std::string s) {
            auto v = ConfigValue{};

            v.type = String;
            new (&v._string) std::string(std::move(s));

            return v;
        }

        static ConfigValue integer(signed i) {
            ConfigValue v;
            v.type = Integer;
            v._signed = i;
            return v;
        }

        static ConfigValue decimal(double d) {
            ConfigValue v;
            v.type = Double;
            v._double = d;
            return v;
        }

        static ConfigValue boolean(bool b) {
            ConfigValue v;
            v.type = Boolean;
            v._signed = b ? 1 : 0;
            return v;
        }
        
        static ConfigValue coerce(const std::string& value, int type) {
            switch (type) {
                case String:
                    return ConfigValue::string(value);
                case Integer: {
                    unsigned u = std::stoul(value);
                    return ConfigValue::integer(static_cast<signed>(u));
                }
                case Double: {
                    double d = std::stod(value);
                    return ConfigValue::decimal(d);
                }
                case Boolean: {
                    auto lowercase = [](const std::string& str) {
                        std::string result = str;
                        for (auto &c : result) c = tolower(c);
                        return result;
                    }(value);

                    bool b = (lowercase == "true" || lowercase == "on");
                    return ConfigValue::boolean(b);
                }
                default:
                    return ConfigValue{};
            }
        }
    };

}
