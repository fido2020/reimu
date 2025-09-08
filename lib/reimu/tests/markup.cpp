#include "reimu/core/logger.h"
#include <reimu/core/file.h>
#include <reimu/gui/markup/parser.h>

#include <cassert>

int main() {
    reimu::TextStream file(R"(
        <box>
            <label text="Hello, World!" />
            this is some text lol
            <button text="Click Me!" />
        </box>
    )");

    auto res = reimu::markup::parse_markup(&file);
    auto root_node = res.ensure();

    assert(root_node.children.size() == 1);
    assert(root_node.children[0].children.size() == 3);
    assert(root_node.children[0].children[0].data == "label");
    assert(root_node.children[0].children[0].props["text"] == "Hello, World!");
    assert(root_node.children[0].children[2].data == "button");
    assert(root_node.children[0].children[2].props["text"] == "Click Me!");

    return 0;
}
