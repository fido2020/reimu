#include "reimu/core/file.h"
#include <reimu/core/error.h>
#include <reimu/gui/markup/node.h>
#include <reimu/core/result.h>

#include <format>

namespace reimu::markup {

class ParseError : public reimu::ErrorBase {
public:
    ParseError(std::string file, int line, std::string msg) {
        message = std::format(
            "{}:{}: {}",
            file,
            line,
            msg
        );
    }

    std::string as_string() const override {
        return message;
    }

    std::string message;
};

Result<Node, ParseError> parse_markup(File *file);

}
