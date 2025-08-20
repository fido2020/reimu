#include "reimu/core/error.h"
#include "reimu/os/error.h"

#include <memory>
#include <reimu/core/result.h>

#include <string>

namespace reimu::config {

namespace detail {

struct ConfigValue;

}

class ConfigError : reimu::ErrorBase {
public:
    enum {
        NotFound,
        InvalidType
    };

    ConfigError(int type, std::string key)
        : type(type), key(std::move(key)) {}

    ConfigError() = default;
    ~ConfigError() override = default;

    std::string as_string() const override;

private:
    int type;

    std::string key;
};

class ConfigProvider {
public:
    virtual ~ConfigProvider() = default;

    virtual Result<detail::ConfigValue, ConfigError> get_value(const std::string &key, int type) = 0;

private:
};

class EnvConfigProvider : public ConfigProvider {
public:
    EnvConfigProvider(std::string prefix);

    void load_dotenv(std::string path);
};

class JSONConfigProvider : public ConfigProvider {
public:
    JSONConfigProvider();
    ~JSONConfigProvider() override;

    Result<detail::ConfigValue, ConfigError> get_value(const std::string &key, int type) override;

    Result<void, reimu::OSError> load_from_file(std::string path);
    Result<void, reimu::ErrorBox> load_from_string(std::string json);

private:
    struct JSONData;

    std::unique_ptr<JSONData> m_data;
};

}
