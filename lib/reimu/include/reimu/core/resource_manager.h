#pragma once

#include <reimu/core/string_id.h>
#include <reimu/core/resource.h>

#include <unordered_map>

namespace reimu {

class ResourceManager {
public:
    template<typename T>
    Result<std::shared_ptr<T>, ReimuError> load_from_file(const std::string &filepath, StringID id) {
        auto file = TRY(open_resource_file(filepath));

        auto resource = std::shared_ptr<Resource>{ TRY(T::create(*file)) };
        m_resources[id] = resource;

        return OK(static_pointer_cast<T>(resource));
    }

    Optional<std::shared_ptr<Resource>> get(StringID id);

private:
    Result<std::unique_ptr<File>, ReimuError> open_resource_file(const std::string &filepath);

    std::unordered_map<StringID, std::shared_ptr<Resource>, StringIDHash> m_resources{};
};

}
