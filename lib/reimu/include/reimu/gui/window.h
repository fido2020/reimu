#pragma once

#include <reimu/result.h>

#include <string>

class Window {
public:
    static Result<Window *> create(const std::string &title);

private:
};
