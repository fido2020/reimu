#include <reimu/core/logger.h>
#include <reimu/core/optional.h>
#include <reimu/core/result.h>
#include <reimu/gui/markup/node.h>
#include <reimu/gui/markup/parser.h>

#include <stack>

namespace reimu::markup {

struct Token {
    enum class Type {
        Data,
        Tag,
        EndTag,
        SelfClosingTag,
        ErrorTag
    } type;

    std::string value;
    std::map<std::string, std::string> props;

    std::string string_dump() const {
        switch(type) {
        case Type::Data:
            return "data: " +value;
        case Type::Tag: {
            auto s = "tag: <" + value;
            for (const auto &[key, val] : props) {
                s += " " + key + "=\"" + val + "\"";
            }
            s += ">";
            return s;
        } case Type::EndTag: {
            auto s = "end tag: </" + value + ">";
            return s;
        } case Type::SelfClosingTag: {
            auto s = "self-closing tag: <" + value;
            for (const auto &[key, val] : props) {
                s += " " + key + "=\"" + val + "\"";
            }
            s += " />";
            return s;
        } case Type::ErrorTag: {
            auto s = "<" + value + " (error)>";
            return s;
        }
        }
    }
};

std::vector<Token> tokenize_markup(File *file);

Result<Node, ParseError> parse_markup(File *file) {
    Node root = Node::make_element("root");
    
    // Ok to use pointers out of the vectors since a child never adds to
    // its parent's children vector
    std::stack<Node *> stack;
    stack.push(&root);

    auto tokens = tokenize_markup(file);
    for (auto &token : tokens) {
        switch (token.type) {
        case Token::Type::Data: {
            if (token.value.empty()) {
                continue;
            }

            auto content_node = Node::make_content(std::move(token.value));
            stack.top()->children.push_back(std::move(content_node));
            break;
        } case Token::Type::Tag: {
            Node element_node = Node::make_element(std::move(token.value), std::move(token.props));
            stack.top()->children.push_back(std::move(element_node));
            stack.push(&stack.top()->children.back());
            break;
        } case Token::Type::EndTag: {
            if (stack.top()->data != token.value) {
                reimu::logger::warn("End tag </{}> does not match start tag <{}>", token.value, stack.top()->data);
                return ERR(ParseError(
                    "<>", 
                    0,
                    std::format("End tag </{}> does not match start tag <{}>",
                        token.value,
                        stack.top()->data
                    )
                ));
            }

            stack.pop();
            break;
        } case Token::Type::SelfClosingTag: {
            Node element_node = Node::make_element(std::move(token.value), std::move(token.props));
            stack.top()->children.push_back(std::move(element_node));
            break;
        } case Token::Type::ErrorTag: {
            reimu::logger::warn("Error parsing tag <{}>", token.value);
            return ERR(ParseError(
                "<>",
                0,
                std::format("Error parsing tag <{}>", token.value)
            ));
            break;
        }
        }
    }

    return OK(std::move(root));
}

std::vector<Token> tokenize_markup(File *file){
    enum class State {
        Data,
        TagOpen,
        EndTagOpen,
        TagName,
        BeforePropName,
        PropName,
        AfterPropName,
        BeforePropValue,
        PropValueDoubleQuoted,
        PropValueSingleQuoted,
        PropValueUnquoted,
        AfterPropValue,
        SelfClosingStartTag,
        TagErrorState
    };

    Token current_token;
    std::string buffer;
    std::string prop_name;

    std::vector<Token> tokens;

    State state = State::Data;
    uint8_t encoded_char[5] = {0};

    const auto do_data = [&](uint32_t codepoint) {
        // If its a tag open don't consume it
        if (codepoint == '<') {
            if (!buffer.empty()) {
                tokens.push_back({
                    .type = Token::Type::Data,
                    .value = std::move(buffer)
                });
            }
            buffer.clear();

            state = State::TagOpen;
            return true;
        }

        if (isspace((int)codepoint) && buffer.empty()) {
            // Ignore leading whitespace
            return true;
        }

        buffer += (const char *)encoded_char;
        return true;
    };

    const auto do_tag_open = [&](uint32_t codepoint) {
        if (codepoint == '/') {
            state = State::EndTagOpen;
            return true;
        }

        if (std::isalpha((int)codepoint)) {
            current_token = {};
            current_token.type = Token::Type::Tag;

            state = State::TagName;
            return false;
        }

        current_token = {};
        current_token.type = Token::Type::ErrorTag;

        // Error
        state = State::TagErrorState;

        return false;
    };

    const auto do_end_tag_open = [&](uint32_t codepoint) {
        if (std::isalpha((int)codepoint)) {
            current_token = {};
            current_token.type = Token::Type::EndTag;

            state = State::TagName;
            return false;
        }

        current_token = {};
        current_token.type = Token::Type::ErrorTag;

        state = State::TagErrorState;

        return false;
    };

    const auto do_tag_name = [&](uint32_t codepoint) {
        if (std::isspace((int)codepoint)) {
            current_token.value = std::move(buffer);
            buffer.clear();

            state = State::BeforePropName;
            return true;
        }

        if (codepoint == '/') {
            current_token.value = std::move(buffer);
            buffer.clear();

            state = State::SelfClosingStartTag;
            return true;
        }

        if (codepoint == '>') {
            current_token.value = std::move(buffer);
            buffer.clear();

            tokens.push_back(std::move(current_token));
            state = State::Data;
            return true;
        }

        if (std::isupper((int)codepoint)) {
            buffer += (char)std::tolower((int)codepoint);
            return true;
        }

        buffer += (const char *)encoded_char;
        return true;
    };

    const auto do_before_prop_name = [&](uint32_t codepoint) {
        if (std::isspace((int)codepoint)) {
            return true;
        }

        if (codepoint == '/' || codepoint == '>') {
            state = State::AfterPropName;
            return false;
        }
        
        if (codepoint == '=') {
            // Error
            current_token.type = Token::Type::ErrorTag;
            state = State::TagErrorState;
            return false;
        }

        prop_name.clear();
        buffer.clear();

        state = State::PropName;
        return false;
    };

    const auto do_prop_name = [&](uint32_t codepoint) {
        if (std::isspace((int)codepoint) || codepoint == '/' || codepoint == '>') {
            prop_name = std::move(buffer);
            buffer.clear();
            
            // Insert into props with empty value
            current_token.props[prop_name] = "";

            state = State::AfterPropName;
            return false;
        }

        if (codepoint == '=') {
            prop_name = std::move(buffer);
            buffer.clear();

            state = State::BeforePropValue;
            return true;
        }

        if (std::isupper((int)codepoint)) {
            buffer += (char)std::tolower((int)codepoint);
            return true;
        }

        buffer += (const char *)encoded_char;
        return true;
    };

    const auto do_after_prop_name = [&](uint32_t codepoint) {
        if (std::isspace((int)codepoint)) {
            return true;
        }

        if (codepoint == '=') {
            state = State::BeforePropValue;
            return true;
        }

        if (codepoint == '/') {
            state = State::SelfClosingStartTag;
            return true;
        }

        if (codepoint == '>') {
            tokens.push_back(std::move(current_token));
            state = State::Data;

            buffer.clear();
            current_token = {};

            return true;
        }

        // Error
        current_token.type = Token::Type::ErrorTag;
        state = State::TagErrorState;
        return false;
    };

    const auto do_before_prop_value = [&](uint32_t codepoint) {
        if (std::isspace((int)codepoint)) {
            return true;
        }

        if (codepoint == '"') {
            buffer.clear();
            state = State::PropValueDoubleQuoted;
            return true;
        }

        if (codepoint == '\'') {
            buffer.clear();
            state = State::PropValueSingleQuoted;
            return true;
        }

        if (codepoint == '>') {
            // Error
            current_token.type = Token::Type::ErrorTag;
            state = State::TagErrorState;
            return false;
        }

        buffer.clear();
        state = State::PropValueUnquoted;
        return false;
    };

    const auto do_prop_value_double_quoted = [&](uint32_t codepoint) {
        if (codepoint == '"') {
            current_token.props[prop_name] = std::move(buffer);
            buffer.clear();
            prop_name.clear();

            state = State::AfterPropValue;
            return true;
        }

        buffer += (const char *)encoded_char;
        return true;
    };

    const auto do_prop_value_single_quoted = [&](uint32_t codepoint) {
        if (codepoint == '\'') {
            current_token.props[prop_name] = std::move(buffer);
            buffer.clear();
            prop_name.clear();

            state = State::AfterPropValue;
            return true;
        }

        buffer += (const char *)encoded_char;
        return true;
    };

    const auto do_prop_value_unquoted = [&](uint32_t codepoint) {
        if (std::isspace((int)codepoint)) {
            current_token.props[prop_name] = std::move(buffer);
            buffer.clear();
            prop_name.clear();

            state = State::BeforePropName;
            return true;
        }

        if (codepoint == '>') {
            current_token.props[prop_name] = std::move(buffer);
            buffer.clear();
            prop_name.clear();

            tokens.push_back(std::move(current_token));
            state = State::Data;

            current_token = {};
            return true;
        }

        buffer += (const char *)encoded_char;
        return true;
    };

    const auto do_after_prop_value = [&](uint32_t codepoint) {
        if (std::isspace((int)codepoint)) {
            state = State::BeforePropName;
            return true;
        }

        if (codepoint == '/') {
            state = State::SelfClosingStartTag;
            return true;
        }

        if (codepoint == '>') {
            tokens.push_back(std::move(current_token));
            state = State::Data;

            current_token = {};
            buffer.clear();
            return true;
        }

        state = State::BeforePropName;
        return false;
    };

    const auto do_self_closing_start_tag = [&](uint32_t codepoint) {
        if (codepoint == '>') {
            current_token.type = Token::Type::SelfClosingTag;
            tokens.push_back(std::move(current_token));
            state = State::Data;

            current_token = {};
            buffer.clear();
            return true;
        }

        // Unexpected '/' character in tag
        state = State::BeforePropName;
        return false;
    };

    auto res = file->get_utf8(encoded_char);
    while (!res.is_err()) {
        uint32_t codepoint = res.move_val();

        bool consume = false;

        auto old_state = state;

        switch (state) {
        case State::Data:
            consume = do_data(codepoint);
            break;
        case State::TagOpen:
            consume = do_tag_open(codepoint);
            break;
        case State::EndTagOpen:
            consume = do_end_tag_open(codepoint);
            break;
        case State::TagName:
            consume = do_tag_name(codepoint);
            break;
        case State::BeforePropName:
            consume = do_before_prop_name(codepoint);
            break;
        case State::PropName:
            consume = do_prop_name(codepoint);
            break;
        case State::AfterPropName:
            consume = do_after_prop_name(codepoint);
            break;
        case State::BeforePropValue:
            consume = do_before_prop_value(codepoint);
            break;
        case State::PropValueDoubleQuoted:  
            consume = do_prop_value_double_quoted(codepoint);
            break;
        case State::PropValueSingleQuoted:
            consume = do_prop_value_single_quoted(codepoint);
            break;
        case State::PropValueUnquoted:
            consume = do_prop_value_unquoted(codepoint);
            break;
        case State::AfterPropValue:
            consume = do_after_prop_value(codepoint);
            break;
        case State::SelfClosingStartTag:
            consume = do_self_closing_start_tag(codepoint);
            break;
        case State::TagErrorState:
            // Just consume until we hit a '>'
            if (codepoint == '>') {
                state = State::Data;
                buffer.clear();
                
            }
            consume = true;
            break;
        }

        if (consume) {
            res = file->get_utf8(encoded_char);
        }
    }

    return tokens;
}

}
