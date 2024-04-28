#pragma once

#include <memory>

#include <reimu/core/result.h>

namespace reimu {

class Server {
public:
    static Result<std::unique_ptr<Server>> create();
};

}
