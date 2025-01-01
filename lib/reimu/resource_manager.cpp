#include <reimu/core/resource_manager.h>
#include <reimu/os/fs.h>

namespace reimu {

Optional<std::shared_ptr<Resource>> ResourceManager::get(StringID id) {
    auto it = m_resources.find(id);
    if (it == m_resources.end()) {
        return {};
    }

    return it->second;
}

Result<std::unique_ptr<File>, ReimuError> ResourceManager::open_resource_file(const std::string &filepath) {
    auto file = os::open(filepath, FileMode::ReadOnly);
    if (file.is_err()) {
        return ERR(ReimuError::FileNotFound);
    }

    return file.ensure();
}

}
