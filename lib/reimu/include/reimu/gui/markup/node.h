#pragma once

#include <reimu/core/optional.h>

#include <string>
#include <map>
#include <vector>

namespace reimu::markup {

struct Node {
    enum class Type {
        Element,
        Content
    } type = Type::Element;

    std::map<std::string, std::string> props;
    std::vector<Node> children;
    std::string data;

    static Node make_content(std::string content) {
        Node t;
        t.data = std::move(content);
        t.type = Type::Content;
        return t;
    }

    static Node make_element(std::string name, std::map<std::string, std::string> props = {}) {
        Node t;
        t.data = std::move(name);
        t.props = std::move(props);
        t.type = Type::Element;
        return t;
    }
};

}
